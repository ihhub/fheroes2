#ifndef _MEMCPY_NEON_H_
#define _MEMCPY_NEON_H_

#ifdef __cplusplus
extern "C" {
#endif

// NEON optimized memcpy
void *memcpy_neon(void *destination, const void *source, size_t num);

#ifdef __cplusplus
}
#endif

#endif
