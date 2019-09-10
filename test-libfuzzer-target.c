/*
  Copyright 2019 Google LLC All rights reserved.

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
   american fuzzy lop - a trivial program to test libFuzzer target fuzzing.
   ------------------------------------------------------------------------

   Initially written and maintained by Michal Zalewski.
*/

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

// TODO(metzman): Create a test/ directory to store this and other similar
// files.
int LLVMFuzzerTestOneInput(uint8_t* buf, size_t size) {
  if (size < 2)
    return 0;

  if (buf[0] == '0')
    printf("Looks like a zero to me!\n");
  else
    printf("A non-zero value? How quaint!\n");

  return 0;
}
