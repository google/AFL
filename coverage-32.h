#include "config.h"
#include "types.h"


static inline u32 classify_word(u32 word) {

  u16 mem16[2];
  memcpy(mem16, &word, sizeof(mem16));

  mem16[0] = count_class_lookup16[mem16[0]];
  mem16[1] = count_class_lookup16[mem16[1]];

  memcpy(&word, mem16, sizeof(mem16));
  return word;

}


static void simplify_trace(u8* bytes) {

  u32* mem = (u32*)bytes;
  u32 i = MAP_SIZE >> 2;

  while (i--) {
    /* Optimize for sparse bitmaps. */

    if (unlikely(*mem)) {
      u8* mem8 = (u8*)mem;

      mem8[0] = simplify_lookup[mem8[0]];
      mem8[1] = simplify_lookup[mem8[1]];
      mem8[2] = simplify_lookup[mem8[2]];
      mem8[3] = simplify_lookup[mem8[3]];

    } else
      *mem = 0x01010101;

    mem++;
  }

}


static inline void classify_counts(u8* bytes) {

  u64* mem = (u64*)bytes;
  u32 i = MAP_SIZE >> 2;

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
static inline void discover_word(u8* ret, u32* current, u32* virgin) {
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
          (cur[2] && vir[2] == 0xff) || (cur[3] && vir[3] == 0xff))
        *ret = 2;
      else
        *ret = 1;
    }

    *virgin &= ~*current;
  }
}
