#include <mcs51/p89v51rd2.h>

#include "stdio.h"

static unsigned char rx_rptr;
static volatile unsigned char rx_wptr;
static __xdata char rx_buf[BUFSIZ];

static volatile unsigned char tx_rptr;
static unsigned char tx_wptr;
static volatile __bit tx_rdy = 1;
static __xdata char tx_buf[BUFSIZ];

void stdio_isr(void) __interrupt (SI0_VECTOR)
{
	if (RI) {
		rx_buf[rx_wptr++] = SBUF;
		RI = 0;
	} else if (TI) {
		if (tx_rptr == tx_wptr)
			tx_rdy = 1;
		else
			SBUF = tx_buf[tx_rptr++];
		TI = 0;
	}
}

char getchar(void)
{
	char c;
	while (rx_rptr == rx_wptr);  /* block until something is available */
	c = rx_buf[rx_rptr++];
	return c;
}

void putchar(char c)
{
	if (tx_rdy) {
		SBUF = c;
		tx_rdy = 0;
	} else {
		while (tx_wptr == tx_rptr);  /* block until flush */
		tx_buf[tx_wptr++] = c;
	}
}
