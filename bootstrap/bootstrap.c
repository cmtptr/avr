#include <mcs51/p89v51rd2.h>

#include "stdio.h"
void stdio_isr(void) __interrupt (SI0_VECTOR);

#define AVR_RESET P1_0

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

/* pretty accurate delay in milliseconds up to 16 seconds */
static void delay_ms(unsigned short ms)
{
	ms *= 4;  /* 1000 / 250 */
	TL0 = TH0;
	TR0 = 1;
	do {
		while (!TF0);
		TF0 = 0;
	} while (--ms != -1U);
	TR0 = 0;
}

static void spi_send(unsigned char c)
{
	SPSR &= ~SPIF;
	SPDAT = c;
	while (!(SPSR & SPIF));
}

static void avr_init(void)
{
	unsigned char c;
	do {
		/* as per the AVR datasheet:
		 * "In some systems, the programmer can not guarantee that SCK
		 * is held low during power-up. In this case, RESET must be
		 * given a positive pulse after SCK has been set to '0'. The
		 * duration of the pulse must be at least t(RST) plus two CPU
		 * clock cycles." */
		AVR_RESET = 1;
		delay_ms(1);
		AVR_RESET = 0;

		/* "Wait for at least 20 ms and enable serial programming by
		 * sending the Programming Enable serial instruction to pin
		 * MOSI." */
		delay_ms(20);
		spi_send(0xac);
		spi_send(0x53);

		/* "The serial programming instructions will not work if the
		 * communication is out of synchronization. When in sync. the
		 * second byte (0x53), will echo back when issuing the third
		 * byte of the Programming Enable instruction. Whether the echo
		 * is correct or not, all four bytes of the instruction must be
		 * transmitted. If the 0x53 did not echo back, give RESET a
		 * positive pulse and issue a new Programming Enable command." */
		spi_send(0);
		c = SPDAT;
		spi_send(0);
	} while (c != 0x53);
}

void main(void)
{
	/* hold the AVR in reset */
	AVR_RESET = 0;

	/* set up the delay timer */
	TMOD = T0_M1;  /* timer 0 in 8-bit auto-reload mode */
	TH0 = -250 & 0xff;

	/* set up the serial port */
	SCON = SCON_SM1 | SCON_REN;  /* 8-bit UART mode, receive enabled */
	RCAP2L = -20 & 0xff;  /* 19200 baud */
	RCAP2H = (-20 & 0xff00) >> 8;
	IE = IE_EA | IE_ES0;  /* enable the serial interrupt */
	T2CON = T2CON_TF2 | T2CON_RCLK | T2CON_TCLK | T2CON_TR2;  /* start T2 */

	/* set up the SPI */
	SPCR = SPE | MSTR | SPR1;  /* SPI on, master mode, SCK = f(OSC) / 64 */

	/* put the AVR into serial programming mode */
	avr_init();

	/* repl */
	while (1) {
		static char buffer[BUFSIZ];
		unsigned char ptr = 0;
		puts("> ");
		while (1) {
			char c = getchar();
			if (c == '\r')
				break;
			if (c == 0x7f || c == 8) {
				if (!ptr)
					continue;
				--ptr;
				puts("\x8 \x8");
			} else {
				if (ptr >= sizeof buffer / sizeof *buffer)
					continue;
				buffer[ptr++] = c;
				putchar(c);
			}
		}
		putchar('\n');
	}
}
