/*
** string.c — implementation of the kernel library functions.
** All of them are self-contained: no external function is called.
*/

#include "string.h"

/* Returns the number of characters before the terminating '\0'. */
size_t k_strlen(const char *s)
{
	size_t len = 0;

	while (s[len] != '\0')
		len++;
	return (len);
}

/* Compares two strings: 0 when equal, otherwise the difference of the first
** differing byte. */
int k_strcmp(const char *a, const char *b)
{
	while (*a != '\0' && *a == *b)
	{
		a++;
		b++;
	}
	return ((int)(uint8_t)*a - (int)(uint8_t)*b);
}

/* Copies n bytes from src to dst (the blocks are assumed not to overlap). */
void *k_memcpy(void *dst, const void *src, size_t n)
{
	uint8_t       *d = (uint8_t *)dst;
	const uint8_t *s = (const uint8_t *)src;
	size_t         i = 0;

	while (i < n)
	{
		d[i] = s[i];
		i++;
	}
	return (dst);
}
