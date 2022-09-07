/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2022, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at https://curl.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 * SPDX-License-Identifier: curl
 *
 ***************************************************************************/
#include "tool_setup.h"

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#define ENABLE_CURLX_PRINTF
/* use our own printf() functions */
#include "curlx.h"

#include "tool_cfgable.h"
#include "tool_cb_prg.h"
#include "tool_util.h"
#include "tool_operate.h"

#include "memdebug.h" /* keep this as LAST include */

#ifdef HAVE_TERMIOS_H
#  include <termios.h>
#elif defined(HAVE_TERMIO_H)
#  include <termio.h>
#endif

/* 200 values generated by this perl code:

   my $pi = 3.1415;
   foreach my $i (1 .. 200) {
     printf "%d, ", sin($i/200 * 2 * $pi) * 500000 + 500000;
   }
*/
static const unsigned int sinus[] = {
  515704, 531394, 547052, 562664, 578214, 593687, 609068, 624341, 639491,
  654504, 669364, 684057, 698568, 712883, 726989, 740870, 754513, 767906,
  781034, 793885, 806445, 818704, 830647, 842265, 853545, 864476, 875047,
  885248, 895069, 904500, 913532, 922156, 930363, 938145, 945495, 952406,
  958870, 964881, 970434, 975522, 980141, 984286, 987954, 991139, 993840,
  996054, 997778, 999011, 999752, 999999, 999754, 999014, 997783, 996060,
  993848, 991148, 987964, 984298, 980154, 975536, 970449, 964898, 958888,
  952426, 945516, 938168, 930386, 922180, 913558, 904527, 895097, 885277,
  875077, 864507, 853577, 842299, 830682, 818739, 806482, 793922, 781072,
  767945, 754553, 740910, 727030, 712925, 698610, 684100, 669407, 654548,
  639536, 624386, 609113, 593733, 578260, 562710, 547098, 531440, 515751,
  500046, 484341, 468651, 452993, 437381, 421830, 406357, 390976, 375703,
  360552, 345539, 330679, 315985, 301474, 287158, 273052, 259170, 245525,
  232132, 219003, 206152, 193590, 181331, 169386, 157768, 146487, 135555,
  124983, 114781, 104959, 95526, 86493, 77868, 69660, 61876, 54525, 47613,
  41147, 35135, 29581, 24491, 19871, 15724, 12056, 8868, 6166, 3951, 2225,
  990, 248, 0, 244, 982, 2212, 3933, 6144, 8842, 12025, 15690, 19832, 24448,
  29534, 35084, 41092, 47554, 54462, 61809, 69589, 77794, 86415, 95445,
  104873, 114692, 124891, 135460, 146389, 157667, 169282, 181224, 193480,
  206039, 218888, 232015, 245406, 259048, 272928, 287032, 301346, 315856,
  330548, 345407, 360419, 375568, 390841, 406221, 421693, 437243, 452854,
  468513, 484202, 499907
};

static void fly(struct ProgressData *bar, bool moved)
{
  char buf[256];
  int pos;
  int check = bar->width - 2;

  msnprintf(buf, sizeof(buf), "%*s\r", bar->width-1, " ");
  memcpy(&buf[bar->bar], "-=O=-", 5);

  pos = sinus[bar->tick%200] / (1000000 / check);
  buf[pos] = '#';
  pos = sinus[(bar->tick + 5)%200] / (1000000 / check);
  buf[pos] = '#';
  pos = sinus[(bar->tick + 10)%200] / (1000000 / check);
  buf[pos] = '#';
  pos = sinus[(bar->tick + 15)%200] / (1000000 / check);
  buf[pos] = '#';

  fputs(buf, bar->out);
  bar->tick += 2;
  if(bar->tick >= 200)
    bar->tick -= 200;

  bar->bar += (moved?bar->barmove:0);
  if(bar->bar >= (bar->width - 6)) {
    bar->barmove = -1;
    bar->bar = bar->width - 6;
  }
  else if(bar->bar < 0) {
    bar->barmove = 1;
    bar->bar = 0;
  }
}

/*
** callback for CURLOPT_XFERINFOFUNCTION
*/

#define MAX_BARLENGTH 256

#if (SIZEOF_CURL_OFF_T == 4)
#  define CURL_OFF_T_MAX CURL_OFF_T_C(0x7FFFFFFF)
#else
   /* assume SIZEOF_CURL_OFF_T == 8 */
#  define CURL_OFF_T_MAX CURL_OFF_T_C(0x7FFFFFFFFFFFFFFF)
#endif

int tool_progress_cb(void *clientp,
                     curl_off_t dltotal, curl_off_t dlnow,
                     curl_off_t ultotal, curl_off_t ulnow)
{
  struct timeval now = tvnow();
  struct per_transfer *per = clientp;
  struct OperationConfig *config = per->config;
  struct ProgressData *bar = &per->progressbar;
  curl_off_t total;
  curl_off_t point;

  /* Calculate expected transfer size. initial_size can be less than zero when
     indicating that we are expecting to get the filesize from the remote */
  if(bar->initial_size < 0) {
    if(dltotal || ultotal)
      total = dltotal + ultotal;
    else
      total = CURL_OFF_T_MAX;
  }
  else if((CURL_OFF_T_MAX - bar->initial_size) < (dltotal + ultotal))
    total = CURL_OFF_T_MAX;
  else
    total = dltotal + ultotal + bar->initial_size;

  /* Calculate the current progress. initial_size can be less than zero when
     indicating that we are expecting to get the filesize from the remote */
  if(bar->initial_size < 0) {
    if(dltotal || ultotal)
      point = dlnow + ulnow;
    else
      point = CURL_OFF_T_MAX;
  }
  else if((CURL_OFF_T_MAX - bar->initial_size) < (dlnow + ulnow))
    point = CURL_OFF_T_MAX;
  else
    point = dlnow + ulnow + bar->initial_size;

  if(bar->calls) {
    /* after first call... */
    if(total) {
      /* we know the total data to get... */
      if(bar->prev == point)
        /* progress didn't change since last invoke */
        return 0;
      else if((tvdiff(now, bar->prevtime) < 100L) && point < total)
        /* limit progress-bar updating to 10 Hz except when we're at 100% */
        return 0;
    }
    else {
      /* total is unknown */
      if(tvdiff(now, bar->prevtime) < 100L)
        /* limit progress-bar updating to 10 Hz */
        return 0;
      fly(bar, point != bar->prev);
    }
  }

  /* simply count invokes */
  bar->calls++;

  if((total > 0) && (point != bar->prev)) {
    char line[MAX_BARLENGTH + 1];
    char format[40];
    double frac;
    double percent;
    int barwidth;
    int num;
    if(point > total)
      /* we have got more than the expected total! */
      total = point;

    frac = (double)point / (double)total;
    percent = frac * 100.0;
    barwidth = bar->width - 7;
    num = (int) (((double)barwidth) * frac);
    if(num > MAX_BARLENGTH)
      num = MAX_BARLENGTH;
    memset(line, '#', num);
    line[num] = '\0';
    msnprintf(format, sizeof(format), "\r%%-%ds %%5.1f%%%%", barwidth);
    fprintf(bar->out, format, line, percent);
  }
  fflush(bar->out);
  bar->prev = point;
  bar->prevtime = now;

  if(config->readbusy) {
    config->readbusy = FALSE;
    curl_easy_pause(per->curl, CURLPAUSE_CONT);
  }

  return 0;
}

void progressbarinit(struct ProgressData *bar,
                     struct OperationConfig *config)
{
  char *colp;
  memset(bar, 0, sizeof(struct ProgressData));

  /* pass the resume from value through to the progress function so it can
   * display progress towards total file not just the part that's left. */
  if(config->use_resume)
    bar->initial_size = config->resume_from;

  colp = curlx_getenv("COLUMNS");
  if(colp) {
    char *endptr;
    long num = strtol(colp, &endptr, 10);
    if((endptr != colp) && (endptr == colp + strlen(colp)) && (num > 20) &&
       (num < 10000))
      bar->width = (int)num;
    curl_free(colp);
  }

  if(!bar->width) {
    int cols = 0;

#ifdef TIOCGSIZE
    struct ttysize ts;
    if(!ioctl(STDIN_FILENO, TIOCGSIZE, &ts))
      cols = ts.ts_cols;
#elif defined(TIOCGWINSZ)
    struct winsize ts;
    if(!ioctl(STDIN_FILENO, TIOCGWINSZ, &ts))
      cols = ts.ws_col;
#elif defined(WIN32)
    {
      HANDLE  stderr_hnd = GetStdHandle(STD_ERROR_HANDLE);
      CONSOLE_SCREEN_BUFFER_INFO console_info;

      if((stderr_hnd != INVALID_HANDLE_VALUE) &&
         GetConsoleScreenBufferInfo(stderr_hnd, &console_info)) {
        /*
         * Do not use +1 to get the true screen-width since writing a
         * character at the right edge will cause a line wrap.
         */
        cols = (int)
          (console_info.srWindow.Right - console_info.srWindow.Left);
      }
    }
#endif /* TIOCGSIZE */
    if(cols > 20)
      bar->width = cols;
  }

  if(!bar->width)
    bar->width = 79;
  else if(bar->width > MAX_BARLENGTH)
    bar->width = MAX_BARLENGTH;

  bar->out = config->global->errors;
  bar->tick = 150;
  bar->barmove = 1;
}
