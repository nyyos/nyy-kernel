#include <stddef.h>
#include <string.h>

void *memset(void *p, int c, size_t n) {
  char *str = (char *)p;
  for (size_t i = 0; i < n; i++) {
    str[i] = c;
  }
  return p;
}

void *memcpy(void *dest_, const void *src_, size_t n) {
  char *dest = (char *)dest_;
  const char *src = (const char *)src_;
  for (size_t i = 0; i < n; i++) {
    dest[i] = src[i];
  }
  return dest_;
}

int memcmp(const void *s1_, const void *s2_, size_t n) {
  const unsigned char *s1 = (const unsigned char *)(s1_);
  const unsigned char *s2 = (const unsigned char *)(s2_);

  for (size_t i = 0; i < n; i++) {
    if (s1[i] != s2[i])
      return s1[i] < s2[i] ? -1 : 1;
  }
  return 0;
}
