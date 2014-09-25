#include <mcs51/p89v51rd2.h>

#include "avr.h"
#include "stdio.h"
#include "string.h"
void stdio_isr(void) __interrupt (SI0_VECTOR);

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

#define IS_WHITESPACE(c) ((c) < '!' || '~' < (c))

/* pretty accurate delay in milliseconds up to 16 seconds */
void delay_ms(unsigned short ms)
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

/* parse a single ASCII hexadecimal character; return zero on success, non-zero
 * if the character is not valid hexadecimal */
static __bit parse_hex(char c, unsigned char *dest)
{
	if ('0' <= c && c <= '9')
		*dest = c - '0';
	else if ('A' <= c && c <= 'F')
		*dest = c - 'A' + 0xa;
	else if ('a' <= c && c <= 'f')
		*dest = c - 'a' + 0xa;
	else
		return 1;
	return 0;
}

/* convert a hexadecimal string to a short integer */
static short strtoh(const char *nptr, const char **endptr)
{
	__bit is_negative, is_valid = 0;
	const char *s = nptr;
	short val = 0;
	for (; *s && IS_WHITESPACE(*s); ++s);
	if (*s == '-') {
		++s;
		is_negative = 1;
	} else {
		is_negative = 0;
	}
	for (; *s; ++s) {
		unsigned char c;
		if (parse_hex(*s, &c))
			break;
		is_valid = 1;
		val *= 0x10;
		val += c;
	}
	if (is_negative)
		val = -val;
	if (endptr)
		*endptr = is_valid ? s : nptr;
	return val;
}

/* print a single byte in hexadecimal */
static void print_hex(unsigned char c)
{
	unsigned char h = c >> 4, l = c & 0xf;
	putchar(h + (h > 9 ? 'a' - 0xa : '0'));
	putchar(l + (l > 9 ? 'a' - 0xa : '0'));
}

static __bit eval_erase(const char *args, unsigned char len)
{
	char c;
	(void)args;
	(void)len;
	puts("This will erase all program memory and EEPROM!!"
			"  Are you sure? [y/N]: ");
	while (c = getchar(), IS_WHITESPACE(c) && c != '\r');
	putchar(c);
	putchar('\n');
	if (c == 'y' || c == 'Y')
		avr_erase();
	else
		puts("aborted\n");
	return 0;
}

static __bit eval_read(const char *args, unsigned char len)
{
	const char *end;
	unsigned short addr, stop;
	if (!len)
		return 1;
	addr = strtoh(args, &end);
	if (end == args || !IS_WHITESPACE(*end))
		return 1;
	if (addr & 0x1f) {
		puts("read: ");
		print_hex(addr >> 8);
		print_hex(addr & 0xff);
		puts(" is not on a page boundary\n");
		return 0;
	}
	print_hex(addr >> 8);
	print_hex(addr & 0xff);
	putchar(' ');
	for (stop = addr + 0x20; addr < stop; ++addr)
		print_hex(avr_read(addr));
	putchar('\n');
	return 0;
}

static __bit eval_hexdump(const char *args, unsigned char len)
{
	short start, count, addr, stop;
	if (!len) {
		goto start_default;
	} else {
		const char *end;
		start = strtoh(args, &end);
		if (end == args)
			goto start_default;
		if (!IS_WHITESPACE(*end))
			return 1;
		len -= end - args;
		args = end;
	}
	if (0) {
start_default:
		start = 0;
	} else if (start < 0) {
		start += 0x800;
	}
	if (!len) {
		goto count_default;
	} else {
		const char *end;
		count = strtoh(args, &end);
		if (end == args)
			goto count_default;
		if (!IS_WHITESPACE(*end))
			return 1;
	}
	if (0) {
count_default:
		count = 0x800 - start;
	} else if (count < 0) {
		start += count;
		count = -count;
	}
	stop = start + count;
	if (stop > 0x800)
		stop = 0x800;
	addr = start & ~0x1f;  /* floor to the nearest page boundary */
	count = (stop + 0x1f) & ~0x1f;  /* ceil to the nearest page boundary */
	for (; addr < count; addr += 0x10) {
		unsigned short i, j;
		char buf[0x10 + 1];
		print_hex(addr >> 8);
		print_hex(addr & 0xff);
		puts("  ");
		for (i = addr, j = addr + 8; i < j; ++i) {
			if (i < start || stop <= i) {
				puts("   ");
				buf[i - addr] = ' ';
			} else {
				unsigned char c = avr_read(i);
				print_hex(c);
				putchar(' ');
				buf[i - addr] = !IS_WHITESPACE(c) ? c : '.';
			}
		}
		putchar(' ');
		for (j += 8; i < j; ++i) {
			if (i < start || stop <= i) {
				puts("   ");
				buf[i - addr] = ' ';
			} else {
				unsigned char c = avr_read(i);
				print_hex(c);
				putchar(' ');
				buf[i - addr] = !IS_WHITESPACE(c) ? c : '.';
			}
		}
		buf[sizeof buf - 1] = '\0';
		puts("  |");
		puts(buf);
		puts("|\n");
	}
	return 0;
}

static __bit eval_write(const char *args, unsigned char len)
{
	const char *end;
	unsigned short addr;
	unsigned char i, data[0x20];
	if (!len)
		return 1;
	addr = strtoh(args, &end);
	if (end == args || !*end || !IS_WHITESPACE(*end))
		return 1;
	if (addr & 0x1f) {
		puts("write: ");
		print_hex(addr >> 8);
		print_hex(addr & 0xff);
		puts(" is not on a page boundary\n");
		return 0;
	}
	for (args = end; *args && IS_WHITESPACE(*args); ++args);
	for (i = 0; i < sizeof data; ++i) {
		unsigned char val;
		if (parse_hex(*args++, &val) || parse_hex(*args++, data + i))
			goto error_parse;
		data[i] += val * 0x10;
	}
	if (*args)
		goto error_parse;
	for (i = 0; i < sizeof data; ++i)
		avr_load(addr + i, data[i]);
	avr_write(addr);
	if (0) {
error_parse:
		puts("write: error parsing page data\n");
	}
	return 0;
}

static __bit eval_spi(const char *args, unsigned char len)
{
	unsigned char i, buf[4];
	(void)len;
	for (i = 0; i < sizeof buf; ++i) {
		const char *end;
		buf[i] = strtoh(args, &end);
		if (end == args || !IS_WHITESPACE(*end))
			return 1;
		args = end;
	}
	avr_spi(buf, buf);
	for (i = 0; i < sizeof buf; ++i) {
		print_hex(buf[i]);
		putchar(' ');
	}
	putchar('\n');
	return 0;
}

static void usage(const char *prefix, const char *cmd, const char *args)
{
	puts(prefix);
	puts(cmd);
	putchar(' ');
	puts(args);
	putchar('\n');
}

static void eval(char *buffer, unsigned char len)
{
	static struct {
		unsigned char len;
		const char *cmd;
		const char *args;
		__bit (*vector)(const char *, unsigned char);
	} vectors[] = {
#define VECTORS_ENTRY(cmd, args) {sizeof (#cmd) - 1, #cmd, args, eval_##cmd}
		VECTORS_ENTRY(erase, ""),
		VECTORS_ENTRY(read, "<page addr>"),
		VECTORS_ENTRY(hexdump, "[<addr> [<count>]]"),
		VECTORS_ENTRY(write, "<page addr> <page data>"),
		VECTORS_ENTRY(spi, "<byte1> <byte2> <byte3> <byte4>"),
	};
	unsigned char i, key_len = strcspn(buffer, " ");
	for (i = 0; i < sizeof vectors / sizeof *vectors; ++i) {
		if (key_len == vectors[i].len
				&& !strncmp(buffer, vectors[i].cmd, key_len)) {
			for (; buffer[key_len]
					&& IS_WHITESPACE(buffer[key_len]);
					++key_len);
			if ((len > key_len
					&& !strncmp(buffer + key_len, "help", 4)
					&& IS_WHITESPACE(buffer[key_len + 4]))
					|| vectors[i].vector(buffer + key_len,
						len - key_len)) {
				puts("usage: ");
				usage("", vectors[i].cmd, vectors[i].args);
			}
			return;
		}
	}
	if (!strncmp(buffer, "help", 4) && IS_WHITESPACE(buffer[4])) {
		puts("available commands:\n");
		for (i = 0; i < sizeof vectors / sizeof *vectors; ++i)
			usage(" ", vectors[i].cmd, vectors[i].args);
	} else {
		puts("invalid command \"");
		buffer[key_len] = '\0';
		puts(buffer);
		puts("\"\n");
	}
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
	if (avr_init())
		puts("Failed to initialize AVR serial programming mode.\n");

	/* repl */
	while (1) {
		static char buffer[81];
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
				if (ptr >= sizeof buffer - 1)
					continue;
				buffer[ptr++] = c;
				putchar(c);
			}
		}
		putchar('\n');
		for (--ptr; ptr > 0 && IS_WHITESPACE(buffer[ptr]); --ptr);
		if (++ptr) {
			char leading;
			buffer[ptr] = '\0';
			for (leading = 0; buffer[leading]
					&& IS_WHITESPACE(buffer[leading]);
					++leading);
			if (leading < ptr)
				eval(buffer + leading, ptr - leading);
		}
	}
}
