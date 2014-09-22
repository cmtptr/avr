CFLAGS ?= -O2 -pipe
CPPFLAGS += -pedantic -std=c99 -Wall -Werror -Wextra

prog_sources := prog.c
prog := prog

CC_TARG := sdcc
CFLAGS_TARG := --model-large
CPPFLAGS_TARG := --stack-auto --std-c99 -mmcs51
LDFLAGS_TARG := --code-size 65536 --iram-size 256 --xram-size 768
PACKIHX := packihx

sources := bootstrap.c stdio.c string.c
objects := $(sources:.c=.rel)
ihx := bootstrap.ihx
hex := bootstrap.hex

.PHONY: all
all: $(prog) $(hex)

.PHONY: clean
clean:
	$(RM) $(prog) $(objects:.rel=.asm) $(objects:.rel=.lst)\
		$(objects:.rel=.rst) $(objects:.rel=.sym) $(objects)\
		$(ihx:.ihx=.lk) $(ihx:.ihx=.map) $(ihx:.ihx=.mem) $(ihx) $(hex)

$(prog): Makefile $(prog_sources)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -o $@ $(prog_sources) $(LDADD)

.SUFFIXES:
.SUFFIXES: .rel .c
.c.rel:
	$(CC_TARG) $(CFLAGS_TARG) $(CPPFLAGS_TARG) -c -o $@ $<

$(ihx): $(objects)
	$(CC_TARG) $(CFLAGS_TARG) $(LDFLAGS_TARG) -o $@ $(objects) $(LDADD)

$(hex): $(ihx)
	$(PACKIHX) >$@ $(ihx)

$(objects): Makefile
bootstrap.rel: bootstrap.c stdio.h string.h
stdio.rel: stdio.c stdio.h
string.rel: string.c string.h
