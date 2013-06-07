CC := sdcc
CFLAGS += --stack-auto --std-c99 -mmcs51
LN := ln -f

sources := $(wildcard *.c)
objects := $(sources:.c=.rel)
cs := cs
hex := bootstrap.ihx

.PHONY: all
all: $(cs) $(hex)

.PHONY: clean
clean:
	@$(MAKE) -C checksum $(MAKECMDGOALS)
	$(RM) $(wildcard *.asm) $(wildcard *.lk) $(wildcard *.lst)\
		$(wildcard *.map) $(wildcard *.mem) $(wildcard *.rst)\
		$(wildcard *.sym) $(objects) $(cs) $(hex)

.PHONY: $(cs)
$(cs):
	@$(MAKE) -C checksum
	$(LN) checksum/cs $(cs)

%.rel: %.c Makefile
	$(CC) $(CFLAGS) -c $<

$(hex): $(objects)
	$(CC) $(CFLAGS) -o $@ $^
