#ifndef AVR_H
#define AVR_H

#define AVR_RESET P1_0

__bit avr_init(void);
void avr_spi(const char *, char *);
void avr_erase(void);
void avr_hexdump(unsigned short, unsigned short);
unsigned char avr_read(unsigned short);

#endif
