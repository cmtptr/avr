CC := sdcc
CFLAGS := --model-large
CPPFLAGS := --stack-auto --std-c99 -DF_OSC=4915200 -DF_UART=19200 -mmcs51
LDFLAGS := --code-size 65536 --iram-size 256 --xram-size 768
PACKIHX := packihx

sources := avr.c bootstrap.c stdio.c
objects := $(sources:.c=.rel)
ihx := bootstrap.ihx
hex := bootstrap.hex

.PHONY: all
all: $(hex)

.PHONY: clean
clean:
	$(RM) $(objects:.rel=.asm) $(objects:.rel=.lst) $(objects:.rel=.rst)\
		$(objects:.rel=.sym) $(objects) $(ihx:.ihx=.lk)\
		$(ihx:.ihx=.map) $(ihx:.ihx=.mem) $(ihx) $(hex)

.SUFFIXES:
.SUFFIXES: .rel .c
.c.rel:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

$(ihx): $(objects)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(objects) $(LDADD)

$(hex): $(ihx)
	$(PACKIHX) $(ihx) >$@

$(objects): Makefile
avr.rel: avr.c avr.h
bootstrap.rel: bootstrap.c avr.h stdio.h
stdio.rel: stdio.c stdio.h
