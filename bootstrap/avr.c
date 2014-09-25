#include <p89v51rd2.h>

#include "avr.h"

/* TODO delay_ms needs to be exposed by a header somewhere */
void delay_ms(unsigned short);

/* transmit/receive on the SPI */
#define spi_xcv(tx, rx) spi_xcv_l(tx, rx, sizeof tx)
static void spi_xcv_l(const unsigned char *tx, unsigned char *rx,
		unsigned char len)
{
	unsigned char i;
	for (i = 0; i < len; ++i) {
		SPSR &= ~SPIF;
		SPDAT = tx[i];
		while (!(SPSR & SPIF));
		rx[i] = SPDAT;
	}
}

/* put the AVR into serial programming mode; return zero on success, or non-zero
 * on failure */
__bit avr_init(void)
{
	static const unsigned char tx[] = {0xac, 0x53, 0, 0};
	unsigned char i, rx[sizeof tx];
	for (i = 0; i < 20; ++i) {
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

		/* "The serial programming instructions will not work if the
		 * communication is out of synchronization. When in sync. the
		 * second byte (0x53), will echo back when issuing the third
		 * byte of the Programming Enable instruction. Whether the echo
		 * is correct or not, all four bytes of the instruction must be
		 * transmitted. If the 0x53 did not echo back, give RESET a
		 * positive pulse and issue a new Programming Enable command." */
		spi_xcv(tx, rx);
		if (rx[2] == tx[1])
			return 0;
	}
	return 1;
}

/* poll the !RDY/BSY flag and non-zero if ready, or zero if not */
static __bit is_rdy(void)
{
	static const unsigned char tx[] = {0xf0, 0, 0, 0};
	unsigned char rx[sizeof tx];
	spi_xcv(tx, rx);
	return !(rx[3] & 1);
}

/* manually issue AVR serial programming instructions */
void avr_spi(const char *tx, char *rx)
{
	spi_xcv_l(tx, rx, 4);
}

/* send a chip-erase instruction */
void avr_erase(void)
{
	static const unsigned char tx[] = {0xac, 0x80, 0, 0};
	unsigned char rx[sizeof tx];
	spi_xcv(tx, rx);
	while (!is_rdy());
}

/* read an arbitrary byte address from program memory */
unsigned char avr_read(unsigned short addr)
{
	unsigned char buf[4] = {
		addr % 2 ? 0x28 : 0x20,
		addr / 2 / 0x100,
		addr / 2 & 0xff,
	};
	spi_xcv(buf, buf);
	return buf[3];
}

/* load a byte value to the temporary page buffer */
void avr_load(unsigned short addr, unsigned char value)
{
	unsigned char buf[4] = {
		addr % 2 ? 0x48 : 0x40,
		addr / 2 / 0x100,
		addr / 2 & 0xff,
		value,
	};
	spi_xcv(buf, buf);
}

/* write the temporary page buffer to program memory */
void avr_write(unsigned short addr)
{
	unsigned char buf[4] = {
		0x4c,
		addr / 2 / 0x100,
		addr / 2 & 0xff,
		0,
	};
	spi_xcv(buf, buf);
	while (!is_rdy());
}
