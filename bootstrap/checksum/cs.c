#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
	if (argc < 2)
		return 1;
	unsigned char checksum = 0;
	for (char *c = argv[1]; *c; c += 2) {
		if (!c[1])
			return 1;
		char tmp = c[2];
		c[2] = 0;
		checksum += strtoul(c, 0, 0x10);
		c[2] = tmp;
	}
	for (char *c = argv[1]; *c; ++c)
		*c = toupper(*c);
	printf(":%s%02X\n", argv[1], -checksum & 0xff);
	return 0;
}
