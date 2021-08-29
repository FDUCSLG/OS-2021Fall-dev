#include <common/string.h>

void *memset(void *s, int c, size_t n) {
    for (size_t i = 0; i < n; i++)
        ((uint8_t *)s)[i] = c & 0xff;

    return s;
}

void *memcpy(void *restrict dest, const void *restrict src, size_t n) {
    for (size_t i = 0; i < n; i++)
        ((uint8_t *)dest)[i] = ((uint8_t *)src)[i];

    return dest;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    for (size_t i = 0; i < n; i++) {
        int c1 = ((uint8_t *)s1)[i];
        int c2 = ((uint8_t *)s2)[i];

        if (c1 != c2)
            return c1 - c2;
    }

    return 0;
}

char *strncpy(char *restrict dest, const char *restrict src, size_t n) {
    size_t i = 0;
    for (; i < n && src[i] != '\0'; i++)
        dest[i] = src[i];
    for (; i < n; i++)
        dest[i] = '\0';

    return dest;
}

char *strncpy_fast(char *restrict dest, const char *restrict src, size_t n) {
    size_t i = 0;
    for (; i < n && src[i] != '\0'; i++)
        dest[i] = src[i];
    if (i < n)
        dest[i] = '\0';

    return dest;
}

int strncmp(const char *s1, const char *s2, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (s1[i] != s2[i])
            return s1[i] - s2[i];
        if (s1[i] == '\0' || s2[i] == '\0')
            break;
    }

    return 0;
}

size_t strlen(const char *s) {
    size_t i = 0;
    while (s[i] != '\0')
        i++;

    return i;
}
