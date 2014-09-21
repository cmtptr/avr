#include <mcs51/p89v51rd2.h>

#include "stdio.h"
#include "string.h"
void stdio_isr(void) __interrupt (SI0_VECTOR);

#define AVR_RESET P1_0
#define F_OSC 4915200
#define F_UART 19200

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
	ms = (ms << 2) + 1;  /* 1000us / 250us */
	TL0 = TH0;
	TR0 = 1;
	do {
		while (!TF0);
		TF0 = 0;
	} while (--ms);
	TR0 = 0;
}

/* transmit/receive on the SPI */
static void spi_xcv(unsigned char c)
{
	SPSR &= ~SPIF;
	SPDAT = c;
	while (!(SPSR & SPIF));
}

/* put the AVR into serial programming mode */
static void avr_init(void)
{
	unsigned char c;
	unsigned char count = 0;
	puts("initializing AVR serial programming mode... ");
	do {
		++count;
		if ((count & 0xf) > 9)
			count += 0x10 - 9;

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
		spi_xcv(0xac);
		spi_xcv(0x53);

		/* "The serial programming instructions will not work if the
		 * communication is out of synchronization. When in sync. the
		 * second byte (0x53), will echo back when issuing the third
		 * byte of the Programming Enable instruction. Whether the echo
		 * is correct or not, all four bytes of the instruction must be
		 * transmitted. If the 0x53 did not echo back, give RESET a
		 * positive pulse and issue a new Programming Enable command." */
		spi_xcv(0);
		c = SPDAT;
		spi_xcv(0);
	} while (c != 0x53);
	puts("success after ");
	c = (count >> 4) & 0xf;
	if (c)
		putchar(c + '0');
	putchar((count & 0xf) + '0');
	puts(" attempt");
	if (count > 1)
		puts("s!\n");
	else
		puts("!\n");
}

static void avr_read(const char *args, unsigned char len)
{
	unsigned char h = SP >> 4, l = SP & 0xf;
	if (!len) {
		puts("usage: read <page addr>\n");
		return;
	}
	puts("read stub!\naddr=\"");
	puts(args);
	puts("\" SP = 0x");
	putchar(h + (h > 9 ? 'a' - 0xa : '0'));
	putchar(l + (l > 9 ? 'a' - 0xa : '0'));
	putchar('\n');
}

static void test(char *buffer, unsigned char len)
{
	unsigned char i;
	(void)buffer;
	(void)len;
	for (i = 0; i < 24; ++i)
		puts("qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq\n");
}

static void eval(char *buffer, unsigned char len)
{
	static struct {
		unsigned char len;
		const char *key;
		void (*vector)(const char *, unsigned char);
	} vectors[] = {
		{4, "read", avr_read},
		{4, "test", test},
	};
	unsigned char i, key_len = strcspn(buffer, " ");
	for (i = 0; i < sizeof vectors / sizeof *vectors; ++i) {
		if (key_len == vectors[i].len
				&& !strncmp(buffer, vectors[i].key, key_len)) {
			for (; buffer[key_len] == ' '; ++key_len);
			vectors[i].vector(buffer + key_len, len - key_len);
			return;
		}
	}
	puts("invalid command \"");
	buffer[key_len] = '\0';
	puts(buffer);
	puts("\"\n");
}

void main(void)
{
	/* hold the AVR in reset */
	AVR_RESET = 0;

	/* delay timer setup */
	TMOD = T0_M1;  /* timer 0 in 8-bit auto-reload mode */
	TH0 = -F_OSC / 12 / 4000;  /* 250-us timer */

	/* serial port setup */
	SCON = SCON_SM1 | SCON_REN;  /* 8-bit UART mode, receive enabled */
	RCAP2L = -F_OSC / 32 / F_UART;
	RCAP2H = -F_OSC / 32 / F_UART >> 8;
	IE = IE_EA | IE_ES0;  /* enable the serial interrupt */
	T2CON = T2CON_TF2 | T2CON_RCLK | T2CON_TCLK | T2CON_TR2;  /* start T2 */

	/* SPI setup */
	SPCR = SPE | MSTR | SPR1;  /* SPI on, master mode, SCK = f(OSC) / 64 */

	/* AVR setup */
	avr_init();

	/* repl */
	while (1) {
		static char buffer[BUFSIZ];
		char ptr = 0;
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
				if (ptr >= BUFSIZ - 1)
					continue;
				buffer[ptr++] = c;
				putchar(c);
			}
		}
		putchar('\n');
		for (--ptr; ptr > 0 && buffer[ptr] < '!' || '~' < buffer[ptr];
				--ptr);
		if (++ptr) {
			char leading;
			buffer[ptr] = '\0';
			for (leading = 0; buffer[leading] == ' '; ++leading);
			if (leading < ptr)
				eval(buffer + leading, ptr - leading);
		}
	}
}
