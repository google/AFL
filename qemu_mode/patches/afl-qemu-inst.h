#include "../../config.h"

#define AFL_QEMU_INST_SNIPPET do { \
    afl_maybe_instrument(tb); \
  } while(0)

extern abi_ulong afl_start_code, afl_end_code;
extern unsigned int afl_inst_rms;

/* Decide whether it is appropriate to instrument a TranslationBlock
   and if so, generate a call to the helper tcg_log in the current
   tcg_ctx */

static void afl_maybe_instrument(TranslationBlock *tb) {
    if (tb->pc < afl_end_code && tb->pc >= afl_start_code) {
        /* Looks like QEMU always maps to fixed locations, so ASAN is not a
           concern. Phew. But instruction addresses may be aligned. Let's
           mangle the value to get something quasi-uniform. */
        abi_ulong loc_hash = (tb->pc >> 4) ^ (tb->pc << 8);
        loc_hash &= MAP_SIZE - 1;

        /* Implement probabilistic instrumentation by looking at loc_hash.
           This keeps the instrumented locations stable across runs.*/
        if (loc_hash < afl_inst_rms) {
            /* It would be possible to make the helper function discover its
               own virtual pc address at runtime but that would require us
               to mark the helper as "reads global state", causing tcg to
               spill its registers before each call. Instead we pre-calculate
               the hashed "location" value for each site and pass it as an
               argument through an immediate. */
            TCGv_i32 loc_imm = tcg_temp_new_i32();
            /* 32bit MAP_SIZE ought to be enough for anybody */
            tcg_gen_movi_i32(loc_imm, (uint32_t) loc_hash);
            gen_helper_afl_log(loc_imm);
        }
    }
}
