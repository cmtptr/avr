#include <mcs51/p89v51rd2.h>

#include "stdio.h"

static __data unsigned char rx_rptr, rx_wptr;
static __idata char rx_buf[BUFSIZ * 2];
static __bit tx_idle = 1, tx_empty = 1;
static __data unsigned char tx_rptr, tx_wptr;
static __idata char tx_buf[BUFSIZ];
void stdio_isr(void) __interrupt (SI0_VECTOR)
{
	if (RI) {
		unsigned char next;
		RI = 0;
		next = rx_wptr + 1;
		next %= sizeof rx_buf;
		if (next != rx_rptr) {
			rx_buf[rx_wptr] = SBUF;
			rx_wptr = next;
		}
	}
	if (TI) {
		TI = 0;
		if (tx_empty) {
			tx_idle = 1;
		} else {
			SBUF = tx_buf[tx_rptr++];
			tx_rptr %= sizeof tx_buf;
			if (tx_rptr == tx_wptr)
				tx_empty = 1;
		}
	}
}

char getchar(void)
{
	char c;
	while (rx_rptr == rx_wptr);  /* block until something is available */
	ES = 0;
	c = rx_buf[rx_rptr++];
	rx_rptr %= sizeof rx_buf;
	ES = 1;
	return c;
}

void putchar(char c)
{
	if (c == '\n')
		putchar('\r');
	if (tx_idle) {
		tx_idle = 0;
		SBUF = c;
	} else {
		/* block until there is room */
		while (tx_rptr == tx_wptr && !tx_empty);
		if (tx_idle) {
			tx_idle = 0;
			SBUF = c;
		} else {
			ES = 0;
			tx_empty = 0;
			tx_buf[tx_wptr++] = c;
			tx_wptr %= sizeof tx_buf;
			ES = 1;
		}
	}
}

int puts(const char *s)
{
	for (; *s; ++s)
		putchar(*s);
	return 1;
}
