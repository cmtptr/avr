#include <mcs51/p89v51rd2.h>

#include "stdio.h"

static unsigned char rx_rptr;
static volatile unsigned char rx_wptr;
static __xdata char rx_buf[BUFSIZ];

static __sbit tx_rdy = 1;
static unsigned char tx_rptr;
static unsigned char tx_wptr;
static __xdata char tx_buf[BUFSIZ];

void stdio_isr() __interrupt (SI0_VECTOR)
{
	if (RI) {
		unsigned char next;
		RI = 0;
		next = rx_wptr + 1;
		next %= BUFSIZ;
		if (next != rx_rptr) {
			rx_buf[rx_wptr] = SBUF;
			rx_wptr = next;
		}
	}
	if (TI) {
		TI = 0;
		if (tx_rptr == tx_wptr) {
			tx_rdy = 1;
		} else {
			SBUF = tx_buf[tx_rptr++];
			tx_rptr %= BUFSIZ;
		}
	}
}

char getchar()
{
	char c;
	while (rx_rptr == rx_wptr);  /* block until something is available */
	ES = 0;
	c = rx_buf[rx_rptr++];
	rx_rptr %= BUFSIZ;
	ES = 1;
	return c;
}

void putchar(char c)
{
	if (c == '\n')
		putchar('\r');
	ES = 0;
	if (tx_rdy) {
		tx_rdy = 0;
		SBUF = c;
	} else {
		unsigned char next = tx_wptr + 1;
		next %= BUFSIZ;
		while (next == tx_rptr);  /* block until there is room */
		tx_buf[tx_wptr] = c;
		tx_wptr = next;
	}
	ES = 1;
}

int puts(const char *s)
{
	for (; *s; ++s)
		putchar(*s);
	return 1;
}
