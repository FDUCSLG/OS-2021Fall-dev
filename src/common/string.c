#include <common/string.h>

void *memcpy(void *restrict dest, const void *restrict src, size_t n) {
    uint8_t *dest_ptr = (uint8_t *)dest;
    uint8_t *src_ptr = (uint8_t *)src;

    while (n > 0) {
        *dest_ptr++ = *src_ptr++;
        n--;
    }

    return dest;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    uint8_t *s1_ptr = (uint8_t *)s1;
    uint8_t *s2_ptr = (uint8_t *)s2;

    while (n > 0) {
        int c1 = *s1_ptr++;
        int c2 = *s2_ptr++;
        n--;

        if (c1 != c2)
            return c1 - c2;
    }

    return 0;
}

int strncmp(const char *s1, const char *s2, size_t n) {
    while (n > 0) {
        int c1 = *s1++;
        int c2 = *s2++;
        n--;

        if (c1 != c2)
            return c1 - c2;
        if (c1 == 0 || c2 == 0)
            break;
    }

    return 0;
}
