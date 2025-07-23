#include "base/base_core.h"

void AssertMessage(const char *message, const char *file, int line) {
    printf("Assert failed: %s, file %s, line %d\n", message, file, line);
}

Rng_U64 rng_u64(u64 min, u64 max) { if (min > max) Swap(u64, min, max); Rng_U64 result; result.min = min; result.max = max; return result; }
u64 rng_u64_len(Rng_U64 rng) { u64 result = rng.max - rng.min; return result; }

Rng_S64 rng_s64(s64 min, s64 max) { if (min > max) Swap(s64, min, max); Rng_S64 result; result.min = min; result.max = max; return result; }
s64 rng_s64_len(Rng_S64 rng) { s64 result = rng.max - rng.min; return result; }

