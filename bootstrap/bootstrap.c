#include <mcs51/p89v51rd2.h>

#include "stdio.h"

#define SCON_RI 0x01
#define SCON_TI 0x02
#define SCON_RB8 0x04
#define SCON_TB8 0x08
#define SCON_REN 0x10
#define SCON_SM2 0x20
#define SCON_SM1 0x40
#define SCON_SM0 0x80

#define IE_EX0 0x01
#define IE_ET0 0x02
#define IE_EX1 0x04
#define IE_ET1 0x08
#define IE_ES0 0x10
#define IE_ET2 0x20
#define IE_EC 0x40
#define IE_EA 0x80

#define T2CON_CP_RL2 0x01
#define T2CON_C_T2 0x02
#define T2CON_TR2 0x04
#define T2CON_EXEN2 0x08
#define T2CON_TCLK 0x10
#define T2CON_RCLK 0x20
#define T2CON_EXF2 0x40
#define T2CON_TF2 0x80

void main(void)
{
	/* set up the serial port */
	SCON = SCON_SM1 | SCON_REN;  /* 8-bit UART mode, receive enabled */
	RCAP2L = -20 & 0xff;  /* 19200 baud */
	RCAP2H = (-20 & 0xff00) >> 8;
	IE = IE_EA | IE_ES0;  /* enable the serial interrupt */
	T2CON = T2CON_TF2 | T2CON_RCLK | T2CON_TCLK | T2CON_TR2;  /* start T2 */

	/* print something! */
	while (1) {
		char c = 0;
		puts("prompt> ");
		do {
			c = getchar();
			putchar(c);
		} while (c != '\r');
		putchar('\n');
	}
}
