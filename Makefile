MCU ?= attiny24a

CC := avr-gcc
CFLAGS += -mmcu=$(MCU) -pedantic -std=c99 -Wall -Werror -Wextra
OBJCOPY := avr-objcopy

sources := $(wildcard *.c)
depends := $(sources:.c=.d)
objects := $(sources:.c=.o)
bin := avr.elf
hex := avr.hex

.PHONY: all
all: $(hex)

.PHONY: clean
clean:
	$(RM) $(depends) $(objects) $(bin) $(hex)

%.d: %.c
	$(CC) $(CFLAGS) -MM -MP -MT "$(@:.d=.o) $@" -o $@ $^

ifneq ($(MAKECMDGOALS),clean)
-include $(depends)
endif

$(objects): Makefile

$(bin): $(objects)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDADD)

$(hex): $(bin)
	$(OBJCOPY) -O ihex $^ $@
