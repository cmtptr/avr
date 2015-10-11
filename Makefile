MCU ?= attiny25

CC := avr-gcc
CFLAGS ?= -pipe
CFLAGS += -mmcu=$(MCU)
CPPFLAGS := -DF_CPU=1000000ul -O2 -pedantic -std=c99 -Wall -Werror -Wextra
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
	$(OBJCOPY) -j .text -j .data -O ihex $(bin) $@

$(objects): Makefile
