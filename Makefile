MCU ?= attiny25

CC := avr-gcc
CPPFLAGS := -DF_CPU=8000000UL -mmcu=$(MCU) -O2 -pedantic -std=c99 -Wall -Werror\
	-Wextra
OBJCOPY := avr-objcopy

sources := test.c
objects := $(sources:.c=.o)
bin := avr.elf
hex := avr.hex

.PHONY: all
all: $(hex)

.PHONY: clean
clean:
	$(RM) $(objects) $(bin) $(hex)

.SUFFIXES:
.SUFFIXES: .o .c
.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

$(bin): $(objects)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(objects) $(LDADD)

$(hex): $(bin)
	$(OBJCOPY) -O ihex $(bin) $@

$(objects): Makefile
