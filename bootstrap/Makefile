CC := sdcc
CFLAGS := --model-large
CPPFLAGS := --stack-auto --std-c99 -mmcs51
LDFLAGS := --code-size 65536 --iram-size 256 --xram-size 768
PACKIHX := packihx

sources := bootstrap.c stdio.c
objects := $(sources:.c=.rel)
ihx := bootstrap.ihx
hex := bootstrap.hex

.PHONY: all
all: $(hex)

.PHONY: clean
clean:
	@$(MAKE) -C prog $(MAKECMDGOALS)
	$(RM) $(wildcard *.asm) $(wildcard *.lk) $(wildcard *.lst)\
		$(wildcard *.map) $(wildcard *.mem) $(wildcard *.rst)\
		$(wildcard *.sym) $(objects) $(cs) $(ihx) $(hex)

.PHONY: prog
prog:
	@$(MAKE) -C prog

.SUFFIXES:
.SUFFIXES: .rel .c
.c.rel:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

$(ihx): $(objects)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(objects) $(LDADD)

$(hex): $(ihx)
	$(PACKIHX) >$@ $(ihx)

$(objects): Makefile
bootstrap.rel: bootstrap.c stdio.h
stdio.rel: stdio.c stdio.h
