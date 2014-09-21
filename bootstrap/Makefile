CC := sdcc
CFLAGS := --model-large
CPPFLAGS := --stack-auto --std-c99 -mmcs51
LDFLAGS := --code-size 65536 --iram-size 256 --xram-size 768
PACKIHX := packihx

sources := bootstrap.c stdio.c string.c
objects := $(sources:.c=.rel)
ihx := bootstrap.ihx
hex := bootstrap.hex

.PHONY: all
all: $(hex)

.PHONY: clean
clean:
	@$(MAKE) -C prog $(MAKECMDGOALS)
	$(RM) $(objects:.rel=.asm) $(objects:.rel=.lst) $(objects:.rel=.rst)\
		$(objects:.rel=.sym) $(objects) $(ihx:.ihx=.lk) $(ihx:.ihx=.map)\
		$(ihx:.ihx=.mem) $(ihx) $(hex)

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
bootstrap.rel: bootstrap.c stdio.h string.h
stdio.rel: stdio.c stdio.h
string.rel: string.c string.h
