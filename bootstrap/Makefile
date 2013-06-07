CC := sdcc
CFLAGS += --stack-auto --std-c99 --xram-size 768 -mmcs51
LN := ln -f
PACKIHX := packihx

sources := $(wildcard *.c)
objects := $(sources:.c=.rel)
cs := cs
ihx := bootstrap.ihx
hex := bootstrap.hex

.PHONY: all
all: $(hex)

.PHONY: clean
clean:
	@$(MAKE) -C checksum $(MAKECMDGOALS)
	$(RM) $(wildcard *.asm) $(wildcard *.lk) $(wildcard *.lst)\
		$(wildcard *.map) $(wildcard *.mem) $(wildcard *.rst)\
		$(wildcard *.sym) $(objects) $(cs) $(ihx) $(hex)

.PHONY: $(cs)
$(cs):
	@$(MAKE) -C checksum
	$(LN) checksum/cs $(cs)

%.rel: %.c Makefile
	$(CC) $(CFLAGS) -c $<

$(ihx): $(objects)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDADD)

$(hex): $(ihx)
	$(PACKIHX) >$@ $^
