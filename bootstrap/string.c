#include "string.h"

char strncmp(const char *s1, const char *s2, unsigned char n)
{
	unsigned char i;
	for (i = 0; i < n; ++i) {
		char dc = s1[i] - s2[i];
		if (dc)
			return dc;
	}
	return 0;
}

unsigned char strcspn(const char *s, const char *reject)
{
	unsigned char i;
	for (i = 0;; ++i) {
		const char *r = reject;
		do {
			if (*r == s[i])
				return i;
		} while (*r++);
	}
}
