/*
  Copyright 2015 Google LLC All rights reserved.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at:

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/


/*
   american fuzzy lop - sample argv fuzzing wrapper
   ------------------------------------------------

   Written by Michal Zalewski <lcamtuf@google.com>

   This file shows a simple way to fuzz command-line parameters with stock
   afl-fuzz. To use, add:

   #include "/path/to/argv-fuzz-inl.h"

   ...to the file containing main(), ideally placing it after all the 
   standard includes. Next, put AFL_INIT_ARGV(); near the very beginning of
   main().

   This will cause the program to read NUL-delimited input from stdin and
   put it in argv[]. Two subsequent NULs terminate the array.

   If you would like to always preserve argv[0], use this instead:
   AFL_INIT_SET0("prog_name");
*/

#ifndef _HAVE_ARGV_FUZZ_INL
#define _HAVE_ARGV_FUZZ_INL

#include <unistd.h>
#include <ctype.h>

#define AFL_INIT_ARGV() do { argv = afl_init_argv(&argc); } while (0)

#define AFL_INIT_SET0(_p) do { \
    argv = afl_init_argv(&argc); \
    argv[0] = (_p); \
    if (!argc) argc = 1; \
  } while (0)

#define MAX_CMDLINE_LEN 100000
#define MAX_CMDLINE_PAR 1000

static char** afl_init_argv(int* argc) {

  static char  in_buf[MAX_CMDLINE_LEN];
  static char* ret[MAX_CMDLINE_PAR];

  char* ptr = in_buf;
  int   rc  = 1; /* start after argv[0] */

  if (read(0, in_buf, MAX_CMDLINE_LEN - 2) < 0);

  while (*ptr) {

    ret[rc] = ptr;

    /* insert '\0' at the end of ret[rc] on first space-sym */
    while (*ptr && !isspace(*ptr)) ptr++;
    *ptr = '\0';
    ptr++;

    /* skip more space-syms */
    while (*ptr && isspace(*ptr)) ptr++;

    rc++;
  }

  *argc = rc;

  return ret;

}

#undef MAX_CMDLINE_LEN
#undef MAX_CMDLINE_PAR

#endif /* !_HAVE_ARGV_FUZZ_INL */
