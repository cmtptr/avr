CC := sdcc
CFLAGS += --stack-auto --std-c99 --xram-size 768 -mmcs51
PACKIHX := packihx

sources := $(wildcard *.c)
objects := $(sources:.c=.rel)
prog := prog/prog
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

.PHONY: $(prog)
$(prog):
	@$(MAKE) -C prog

%.rel: %.c Makefile
	$(CC) $(CFLAGS) -c $<

$(ihx): $(objects)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDADD)

$(hex): $(ihx)
	$(PACKIHX) >$@ $^
