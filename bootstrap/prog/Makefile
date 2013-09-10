CPPFLAGS += -pedantic -std=c99 -Wall -Werror -Wextra

sources := prog.c
objects := $(sources:.c=.o)
bin := prog

.PHONY: all
all: $(bin)

.PHONY: clean
clean:
	$(RM) $(depends) $(objects) $(bin)

.SUFFIXES:
.SUFFIXES: .c .o
.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

$(bin): $(objects)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(objects) $(LDADD)

$(objects): Makefile
