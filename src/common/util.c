#include <common/util.h>

void *memcpy(void *restrict dest, const void *restrict src, size_t n) {
    uint8_t *dest_ptr = (uint8_t *)dest;
    uint8_t *src_ptr = (uint8_t *)src;

    while (n > 0) {
        *dest_ptr++ = *src_ptr++;
        n--;
    }

    return dest;
}
