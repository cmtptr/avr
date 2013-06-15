#ifndef STDIO_H
#define STDIO_H

#define BUFSIZ 0x40

extern void stdio_isr(
		void
) __interrupt (SI0_VECTOR);

extern char getchar(
		void
);

extern void putchar(
		char c
);

extern int puts(
		const char *s
);

#endif
