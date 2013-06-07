#ifndef STDIO_H
#define STDIO_H

#include <stdio.h>

#define CHAR_BIT 8
#define BUFSIZ (1 << CHAR_BIT)

void stdio_isr(
		void
) __interrupt (SI0_VECTOR);

#endif
