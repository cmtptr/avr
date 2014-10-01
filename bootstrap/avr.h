#ifndef AVR_H
#define AVR_H

#define AVR_RESET P1_0

void avr_reset(void);
__bit avr_is_programming_enabled(void);
__bit avr_programming_enable(void);
void avr_spi(const char *, char *);
unsigned char avr_signature(unsigned char);
void avr_erase(void);
unsigned char avr_flash_read(unsigned short);
void avr_flash_load(unsigned short, unsigned char);
void avr_flash_write(unsigned short);
unsigned char avr_eeprom_read(unsigned char);
void avr_eeprom_write(unsigned char, unsigned char);

#endif
