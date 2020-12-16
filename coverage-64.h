#include "config.h"
#include "types.h"


static inline u64 classify_word(u64 word) {

  u16 mem16[4];
  memcpy(mem16, &word, sizeof(mem16));

  mem16[0] = count_class_lookup16[mem16[0]];
  mem16[1] = count_class_lookup16[mem16[1]];
  mem16[2] = count_class_lookup16[mem16[2]];
  mem16[3] = count_class_lookup16[mem16[3]];

  memcpy(&word, mem16, sizeof(mem16));
  return word;

}


static void simplify_trace(u8* bytes) {

  u64* mem = (u64*)bytes;
  u32 i = MAP_SIZE >> 3;

  while (i--) {
    /* Optimize for sparse bitmaps. */

    if (unlikely(*mem)) {
      u8* mem8 = (u8*)mem;

      mem8[0] = simplify_lookup[mem8[0]];
      mem8[1] = simplify_lookup[mem8[1]];
      mem8[2] = simplify_lookup[mem8[2]];
      mem8[3] = simplify_lookup[mem8[3]];
      mem8[4] = simplify_lookup[mem8[4]];
      mem8[5] = simplify_lookup[mem8[5]];
      mem8[6] = simplify_lookup[mem8[6]];
      mem8[7] = simplify_lookup[mem8[7]];

    } else
      *mem = 0x0101010101010101ULL;

    mem++;
  }

}


static inline void classify_counts(u8* bytes) {

  u64* mem = (u64*)bytes;
  u32 i = MAP_SIZE >> 3;

  while (i--) {
    /* Optimize for sparse bitmaps. */

    if (unlikely(*mem)) {
      *mem = classify_word(*mem);
    }

    mem++;
  }

}


/* Updates the virgin bits, then reflects whether a new count or a new tuple is
 * seen in ret. */
static inline void discover_word(u8* ret, u64* current, u64* virgin) {
  /* Optimize for (*current & *virgin) == 0 - i.e., no bits in current bitmap
     that have not been already cleared from the virgin map - since this will
     almost always be the case. */

  if (*current & *virgin) {
    if (likely(*ret < 2)) {
      u8* cur = (u8*)current;
      u8* vir = (u8*)virgin;

      /* Looks like we have not found any new bytes yet; see if any non-zero
         bytes in current[] are pristine in virgin[]. */

      if ((cur[0] && vir[0] == 0xff) || (cur[1] && vir[1] == 0xff) ||
          (cur[2] && vir[2] == 0xff) || (cur[3] && vir[3] == 0xff) ||
          (cur[4] && vir[4] == 0xff) || (cur[5] && vir[5] == 0xff) ||
          (cur[6] && vir[6] == 0xff) || (cur[7] && vir[7] == 0xff))
        *ret = 2;
      else
        *ret = 1;

    }

    *virgin &= ~*current;
  }

}
