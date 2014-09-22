#define _XOPEN_SOURCE  /* for fileno() */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

#define HEX_RECORD_MAX (0x10 + 5)  /* len, address, cmd, 16 bytes, checksum */
#define HEX_LINE_MIN 12  /* ":00000001ff\n" */
#define SECTOR_SIZE 0x80

struct hex {
	unsigned int records_count;
	unsigned char records[][HEX_RECORD_MAX];
};

/* setup serial communication */
static int init_tty(const char *filename)
{
	int tty = open(filename, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (tty < 0) {
		perror("open()");
		return tty;
	}
	struct termios attr;
	int status = tcgetattr(tty, &attr);
	if (status < 0) {
		perror("tcgetattr()");
		close(tty);
		return status;
	}
	attr.c_iflag &= ~(IXON | IXANY | IXOFF);
	attr.c_oflag |= OPOST;
	attr.c_cflag &= ~(CSIZE | CSTOPB | PARENB);
	attr.c_cflag |= CS8 | CREAD | CLOCAL;
	cfsetospeed(&attr, B19200);
	cfsetispeed(&attr, 0);  /* use ospeed */
	status = tcsetattr(tty, TCSANOW, &attr);
	if (status < 0) {
		perror("tcsetattr()");
		close(tty);
		return status;
	}
	return tty;
}

/* parse intel hex records and return the count */
static unsigned int hex_parse(
		const char *filename,
		struct hex *rec,
		char *buffer)
{
	unsigned int count = 0;
	for (; *buffer; ++buffer, ++count) {
		if (*buffer != ':') {
			fprintf(stderr, "%s:%u: syntax error\n", filename,
					count + 1);
			return 0;
		}
		unsigned short j = 0;
		for (++buffer; j < HEX_RECORD_MAX && *buffer && *buffer != '\n';
				buffer += 2, ++j) {
			char tmp = buffer[2];
			buffer[2] = 0;
			rec->records[count][j] = strtoul(buffer, 0, 0x10);
			buffer[2] = tmp;
		}
		if (j < 5) {
			fprintf(stderr, "%s:%u: short record\n", filename,
					count + 1);
			return 0;
		}
		if (rec->records[count][3])
			continue;
		if (rec->records[count][0] != j - 5) {
			fprintf(stderr, "%s:%u: length error\n", filename,
					count + 1);
			return 0;
		}
	}
	rec->records_count = count;
	return count;
}

/* read and parse the intel hex records */
static struct hex *init_hex(const char *filename)
{
	FILE *f = fopen(filename, "r");
	if (!f) {
		perror("fopen()");
		return 0;
	}
	struct stat sbuf;
	if (fstat(fileno(f), &sbuf) < 0) {
		perror("fstat()");
		fclose(f);
		return 0;
	}
	unsigned int records_count =
		(sbuf.st_size + HEX_LINE_MIN) / HEX_LINE_MIN;
	struct hex *rec = malloc(sbuf.st_size + 1 + sizeof *rec
			+ records_count * sizeof *rec->records);
	char *buffer = (char *)(rec + records_count);
	if (fread(buffer, 1, sbuf.st_size, f) < (unsigned)sbuf.st_size) {
		fprintf(stderr, "%s: short read\n", filename);
		free(buffer);
		fclose(f);
		return 0;
	}
	fclose(f);
	buffer[sbuf.st_size] = 0;
	records_count = hex_parse(filename, rec, buffer);
	if (!records_count)
		return 0;
	rec = realloc(rec, rec->records_count * sizeof *rec
			+ rec->records_count * sizeof *rec->records);
	return rec;
}

/* put the mcu into in-system programming mode */
static void isp(int tty)
{
	char c = 0;
	ssize_t l;
	do {
		const char u = 'U';
		write(tty, &u, sizeof u);
		select(0, 0, 0, 0, &(struct timeval){0, 40000});
		l = read(tty, &c, sizeof c);
	} while (l < 0 || c != 'U');
}

/* send a record and return the status code received */
static char record_send(int tty, unsigned char *record, size_t len)
{
	char buffer[HEX_RECORD_MAX * 2 + 5];
	buffer[0] = ':';
	char *ptr = buffer + 1;
	for (size_t i = 0; i < len; ++i, ptr += 2)
		sprintf(ptr, "%02X", record[i]);
	len = ptr - buffer;
	ssize_t l = write(tty, buffer, len);
	if (l < 0) {
		perror("write()");
		return l;
	} else if (l < (ssize_t)len) {
		fprintf(stderr, "\"%.*s\": short write\n", (int)len, buffer);
		return -1;
	}
	ptr = buffer;
	do {
		fd_set fdr;
		FD_ZERO(&fdr);
		FD_SET(tty, &fdr);
		select(tty + 1, &fdr, 0, 0, 0);
		l = read(tty, ptr, 1);
		ptr += l;
	} while (l == 1 && (('0' <= ptr[-1] && ptr[-1] <= '9')
				|| ('A' <= ptr[-1] && ptr[-1] <= 'F')
				|| ('a' <= ptr[-1] && ptr[-1] <= 'f')
				|| ptr[-1] == ':' || ptr[-1] == '\r'
				|| ptr[-1] == '\n'));
	return ptr[-1];
}

int main(int argc, char **argv)
{
	if (argc != 3) {
		fprintf(stderr, "usage: %s <firmware.hex> <ttyS>\n", argv[0]);
		return 1;
	}
	int tty = init_tty(argv[2]);
	if (tty < 0)
		return tty;
	struct hex *rec = init_hex(argv[1]);
	if (!rec) {
		close(tty);
		return -1;
	}
	unsigned short addr_max = 0;
	for (unsigned int i = 0; i < rec->records_count; ++i) {
		if (rec->records[i][3])
			continue;
		unsigned short addr = (rec->records[i][1] << 8)
			| rec->records[i][2];
		addr += rec->records[i][0] - 1;
		if (addr > addr_max)
			addr_max = addr;
	}
	isp(tty);
	for (unsigned short addr = 0; addr < addr_max; addr += 0x80) {
		unsigned char al = addr & 0xff;
		unsigned char ah = (addr & 0xff00) >> 8;
		unsigned char checksum = -(0xe + ah + al);
		unsigned char record[] = {3, 0, 0, 3, 8, ah, al, checksum};
		char status = record_send(tty, record, sizeof record);
		if (status != '.') {
			fprintf(stderr, "erase sector 0x%04hx failed: 0x%02x\n",
					addr, (int)status);
			free(rec);
			close(tty);
			return -1;
		}
	}
	for (unsigned int i = 0; i < rec->records_count; ++i) {
		if (rec->records[i][3])
			continue;
		char status = record_send(tty, rec->records[i],
				rec->records[i][0] + 5);
		if (status != '.') {
			unsigned short addr = (rec->records[i][1] << 8)
				| rec->records[i][2];
			fprintf(stderr, "write %u bytes to address 0x%04hx failed: 0x%02x\n",
					(unsigned int)rec->records[i][0],
					addr, (int)status);
			free(rec);
			close(tty);
			return -1;
		}
	}
	free(rec);
	close(tty);
	return 0;
}
