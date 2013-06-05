CC := sdcc
CFLAGS += --stack-auto --std-c99 -mmcs51

sources := $(wildcard *.c)
objects := $(sources:.c=.rel)
hex := bootstrap.ihx

.PHONY: all
all: $(hex)

.PHONY: clean
clean:
	$(RM) $(wildcard *.asm) $(wildcard *.lk) $(wildcard *.lst)\
		$(wildcard *.map) $(wildcard *.mem) $(wildcard *.rst)\
		$(wildcard *.sym) $(objects) $(hex)

%.rel: %.c Makefile
	$(CC) $(CFLAGS) -c $<

$(hex): $(objects)
	$(CC) $(CFLAGS) -o $@ $^
