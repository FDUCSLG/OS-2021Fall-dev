#include <common/string.h>

void *memset(void *s, int c, usize n) {
    for (usize i = 0; i < n; i++)
        ((u8 *)s)[i] = c & 0xff;

    return s;
}

void *memcpy(void *restrict dest, const void *restrict src, usize n) {
    for (usize i = 0; i < n; i++)
        ((u8 *)dest)[i] = ((u8 *)src)[i];

    return dest;
}

int memcmp(const void *s1, const void *s2, usize n) {
    for (usize i = 0; i < n; i++) {
        int c1 = ((u8 *)s1)[i];
        int c2 = ((u8 *)s2)[i];

        if (c1 != c2)
            return c1 - c2;
    }

    return 0;
}

char *strncpy(char *restrict dest, const char *restrict src, usize n) {
    usize i = 0;
    for (; i < n && src[i] != '\0'; i++)
        dest[i] = src[i];
    for (; i < n; i++)
        dest[i] = '\0';

    return dest;
}

char *strncpy_fast(char *restrict dest, const char *restrict src, usize n) {
    usize i = 0;
    for (; i < n && src[i] != '\0'; i++)
        dest[i] = src[i];
    if (i < n)
        dest[i] = '\0';

    return dest;
}

int strncmp(const char *s1, const char *s2, usize n) {
    for (usize i = 0; i < n; i++) {
        if (s1[i] != s2[i])
            return s1[i] - s2[i];
        if (s1[i] == '\0' || s2[i] == '\0')
            break;
    }

    return 0;
}

usize strlen(const char *s) {
    usize i = 0;
    while (s[i] != '\0')
        i++;

    return i;
}
