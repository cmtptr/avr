#include <mcs51/p89v51rd2.h>

#include "avr.h"
#include "stdio.h"
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

static char strncmp(const char *s1, const char *s2, unsigned char n)
{
	unsigned char i;
	for (i = 0; i < n; ++i) {
		char dc = s1[i] - s2[i];
		if (dc)
			return dc;
	}
	return 0;
}

static __bit ihex(const unsigned char *buf, unsigned char len)
{
	static unsigned short page = 0xffff;
	static unsigned char data[0x20];
	unsigned char i, checksum = buf[len + 4];
	unsigned short addr, newpage;
	if (!avr_is_programming_enabled())
		return 1;
	for (i = 0; i < len + 4; ++i)
		checksum += buf[i];
	if (checksum)  /* checksum  error */
		return 1;
	addr = buf[1] * 0x100 + buf[2];
	newpage = addr & ~0x1f;
	if (addr + len > newpage + sizeof data)  /* len extends beyond page boundary */
		return 1;
	if (!buf[3]) {
		/* data */
		unsigned char *ptr;
		if (newpage != page) {
			if (page != 0xffff) {
				for (i = 0; i < sizeof data; ++i)
					avr_flash_load(page + i, data[i]);
				avr_flash_write(page);
			}
			for (i = 0; i < sizeof data; ++i)
				data[i] = 0xff;
			page = newpage;
		}
		ptr = data + (addr & 0x10);
		for (i = 0; i < len; ++i)
			ptr[i] = buf[i + 4];
	} else if (buf[3] == 1) {
		/* end of file */
		if (page != 0xffff) {
			for (i = 0; i < sizeof data; ++i)
				avr_flash_load(page + i, data[i]);
			avr_flash_write(page);
		}
		for (i = 0; i < sizeof data; ++i)
			data[i] = 0xff;
		page = 0xffff;
	} else {
		/* unrecognized type */
		return 1;
	}
	return 0;
}

static __bit eval_eeprom(const char *args, unsigned char len)
{
	const char *end;
	unsigned char addr, value;
	if (!avr_is_programming_enabled()) {
		puts("eeprom: device is not in serial programming mode;"
				" use \"reset prog\"\n");
		return 0;
	}
	if (!len) {
		for (addr = 0; addr < 0x80; addr += 0x10) {
			unsigned char i, j, buf[0x10 + 1];
			puts("  ");
			print_hex(addr);
			puts("  ");
			for (i = addr, j = addr + 8; i < j; ++i) {
				unsigned char c = avr_eeprom_read(i);
				print_hex(c);
				putchar(' ');
				buf[i - addr] = !IS_WHITESPACE(c) || c == ' ' ?
					c : '.';
			}
			putchar(' ');
			for (j += 8; i < j; ++i) {
				unsigned char c = avr_eeprom_read(i);
				print_hex(c);
				putchar(' ');
				buf[i - addr] = !IS_WHITESPACE(c) || c == ' ' ?
					c : '.';
			}
			buf[sizeof buf - 1] = '\0';
			puts("  |");
			puts(buf);
			puts("|\n");
		}
		return 0;
	}
	addr = strtoh(args, &end);
	if (end == args || !IS_WHITESPACE(*end))
		return 1;
	for (args = end; IS_WHITESPACE(*args); ++args)
		if (!*args) {
			print_hex(avr_eeprom_read(addr));
			putchar('\n');
			return 0;
		}
	value = strtoh(args, &args);
	if (*args && !IS_WHITESPACE(*args))
		puts("eeprom: error parsing byte value\n");
	else
		avr_eeprom_write(addr, value);
	return 0;
}

static __bit eval_erase(const char *args, unsigned char len)
{
	char c;
	(void)args;
	(void)len;
	if (!avr_is_programming_enabled()) {
		puts("erase: device is not in serial programming mode;"
				" use \"reset prog\"\n");
		return 0;
	}
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

static __bit eval_flash_read(unsigned short addr)
{
	unsigned short stop;
	print_hex(addr >> 8);
	print_hex(addr & 0xff);
	putchar(' ');
	for (stop = addr + 0x20; addr < stop; ++addr)
		print_hex(avr_flash_read(addr));
	putchar('\n');
	return 0;
}

static __bit eval_flash_write(const char *args, unsigned char len,
		unsigned short addr)
{
	unsigned char i, data[0x20];
	if (!len)
		return 1;
	for (len = 0; len < sizeof data; ++len) {
		unsigned char val;
		if (parse_hex(*args, &val))
			break;
		++args;
		if (parse_hex(*args, data + len))
			goto error_parse;
		++args;
		data[len] += val * 0x10;
	}
	if (!len || *args && !IS_WHITESPACE(*args))
		goto error_parse;
	for (i = 0; i < len; ++i)
		avr_flash_load(addr + i, data[i]);
	if (i % 2)  /* always write a complete word */
		avr_flash_load(addr + i, 0xff);
	avr_flash_write(addr);
	if (0) {
error_parse:
		puts("flash: error parsing page data\n");
	}
	return 0;
}

static __bit eval_flash(const char *args, unsigned char len)
{
	const char *end;
	unsigned short addr;
	if (!len)
		return 1;
	addr = strtoh(args, &end);
	if (end == args || !IS_WHITESPACE(*end))
		return 1;
	if (addr & 0x1f) {
		puts("flash: ");
		print_hex(addr >> 8);
		print_hex(addr & 0xff);
		puts(" is not on a page boundary\n");
		return 0;
	}
	if (!avr_is_programming_enabled()) {
		puts("flash: device is not in serial programming mode;"
				" use \"reset prog\"\n");
		return 0;
	}
	for (; IS_WHITESPACE(*end); ++end)
		if (!*end)
			return eval_flash_read(addr);
	return eval_flash_write(end, args + len - end, addr);
}

static __bit eval_hexdump(const char *args, unsigned char len)
{
	short start, count, addr, stop;
	if (!avr_is_programming_enabled()) {
		puts("hexdump: device is not in serial programming mode;"
				" use \"reset prog\"\n");
		return 0;
	}
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
				unsigned char c = avr_flash_read(i);
				print_hex(c);
				putchar(' ');
				buf[i - addr] = !IS_WHITESPACE(c) || c == ' ' ?
					c : '.';
			}
		}
		putchar(' ');
		for (j += 8; i < j; ++i) {
			if (i < start || stop <= i) {
				puts("   ");
				buf[i - addr] = ' ';
			} else {
				unsigned char c = avr_flash_read(i);
				print_hex(c);
				putchar(' ');
				buf[i - addr] = !IS_WHITESPACE(c) || c == ' ' ?
					c : '.';
			}
		}
		buf[sizeof buf - 1] = '\0';
		puts("  |");
		puts(buf);
		puts("|\n");
	}
	return 0;
}

static __bit eval_reset(const char *args, unsigned char len)
{
	if (!len)
		avr_reset();
	else if (strncmp(args, "prog", 4) || !IS_WHITESPACE(args[4]))
		return 1;
	else if (avr_programming_enable())
		puts("reset: failed to initialize AVR serial programming mode\n");
	return 0;
}

static __bit eval_signature(const char *args, unsigned char len)
{
	unsigned char i;
	(void)args;
	(void)len;
	if (!avr_is_programming_enabled()) {
		puts("signature: device is not in serial programming mode;"
				" use \"reset prog\"\n");
		return 0;
	}
	for (i = 0; i < 3; ++i)
		print_hex(avr_signature(i));
	putchar('\n');
	return 0;
}

static __bit eval_spi(const char *args, unsigned char len)
{
	unsigned char i, data[38];  /* (80 (input buffer) - 4 ("spi ")) / 2 */
	if (!len)
		return 1;
	for (len = 0; len < sizeof data; ++len) {
		unsigned char val;
		if (parse_hex(*args, &val))
			break;
		++args;
		if (parse_hex(*args, data + len))
			goto error_parse;
		++args;
		data[len] += val * 0x10;
	}
	if (!len || *args && !IS_WHITESPACE(*args))
		goto error_parse;
	avr_spi(data, data, len);
	for (i = 0; i < len; ++i)
		print_hex(data[i]);
	putchar('\n');
	if (0) {
error_parse:
		puts("spi: error parsing data\n");
	}
	return 0;
}

static void usage(const char *prefix, const char *cmd, const char *args)
{
	if (prefix)
		puts(prefix);
	puts(cmd);
	if (args) {
		putchar(' ');
		puts(args);
	}
	putchar('\n');
}

static void eval(char *buf, unsigned char len)
{
	static const struct {
		unsigned char len;
		const char *cmd;
		const char *args;
		__bit (*vector)(const char *, unsigned char);
	} vectors[] = {
#define VECTORS_ENTRY(cmd, args) {sizeof (#cmd) - 1, #cmd, args, eval_##cmd}
		VECTORS_ENTRY(eeprom, "[<addr> [<value>]]"),
		VECTORS_ENTRY(erase, 0),
		VECTORS_ENTRY(flash, "<addr> [<data>]"),
		VECTORS_ENTRY(hexdump, "[<addr> [<count>]]"),
		VECTORS_ENTRY(reset, "[prog]"),
		VECTORS_ENTRY(signature, 0),
		VECTORS_ENTRY(spi, "<data>"),
	};
	unsigned char i, key_len;
	for (key_len = 0; !IS_WHITESPACE(buf[key_len]); ++key_len);
	for (i = 0; i < sizeof vectors / sizeof *vectors; ++i) {
		if (key_len == vectors[i].len
				&& !strncmp(buf, vectors[i].cmd, key_len)) {
			for (; buf[key_len]
					&& IS_WHITESPACE(buf[key_len]);
					++key_len);
			if ((len > key_len
					&& !strncmp(buf + key_len, "help", 4)
					&& IS_WHITESPACE(buf[key_len + 4]))
					|| vectors[i].vector(buf + key_len,
						len - key_len)) {
				puts("usage: ");
				usage(0, vectors[i].cmd, vectors[i].args);
			}
			return;
		}
	}
	if (!strncmp(buf, "help", 4) && IS_WHITESPACE(buf[4])) {
		puts("usage: <:Intel HEX record> | <command> [<args>...]\n"
				"available commands:\n");
		for (i = 0; i < sizeof vectors / sizeof *vectors; ++i)
			usage(" ", vectors[i].cmd, vectors[i].args);
	} else {
		puts("invalid command \"");
		buf[key_len] = '\0';
		puts(buf);
		puts("\"; enter \"help\" for a list of recognized commands\n");
	}
}

void main(void)
{
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

	/* repl */
	while (1) {
		static char buf[81];
		char ptr = 0, ihex_len = 0;  /* in Intel HEX input mode? */
		unsigned char len = 0xff;  /* ihex record data length */
		puts("> ");
		while (1) {
			char c = getchar();
			if (!ihex_len && c == '\r')
				break;
			if (c == 0x7f || c == 8) {
				if (!ptr)
					continue;
				--ptr;
				if (ihex_len) {
					if (!ptr)
						ihex_len = 0;
					else if (ptr == 2)
						len = 0xff;
				}
				puts("\x8 \x8");
			} else if (ihex_len) {
				if (parse_hex(c, buf + ptr)) {
					putchar('X');
					break;
				}
				putchar(c);
				if (++ptr == 3) {
					len = buf[1] * 0x10 + buf[2];
					if (len > 0x10) {
						putchar('X');
						break;
					}
					/* track the expected buf[] length */
					ihex_len = 1 + 2 * (len + 5);
				} else if (ptr == ihex_len) {
					/* right now, buf[] is a string of hex
					 * digits (like BCD, but in hex);
					 * process it into raw values */
					for (ptr = 0; ptr < len + 5; ++ptr)
						buf[ptr] = buf[1 + ptr * 2] * 0x10
							+ buf[2 + ptr * 2];
					if (ihex(buf, len))
						putchar('X');
					else
						putchar('.');
					break;
				}
			} else if (ptr < sizeof buf - 1) {
				buf[ptr++] = c;
				putchar(c);
				if (c == ':' && ptr == 1) {
					ihex_len = 0xff;
					len = 0xff;
				}
			}
		}
		putchar('\n');
		if (!ihex_len) {
			for (--ptr; ptr > 0 && IS_WHITESPACE(buf[ptr]); --ptr);
			if (++ptr) {
				char leading;
				buf[ptr] = '\0';
				for (leading = 0; buf[leading]
						&& IS_WHITESPACE(buf[leading]);
						++leading);
				if (leading < ptr)
					eval(buf + leading, ptr - leading);
			}
		}
	}
}
