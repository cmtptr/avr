CFLAGS += -O3 -pedantic -std=c99 -Wall -Werror -Wextra

sources := $(wildcard *.c)
objects := $(sources:.c=.o)
bin := cs

.PHONY: all
all: $(bin)

.PHONY: clean
clean:
	$(RM) $(depends) $(objects) $(bin)

%.d: %.c
	$(CC) $(CFLAGS) -MM -MP -MT "$(@:.d=.o) $@" -o $@ $^

ifneq ($(MAKECMDGOALS),clean)
-include $(depends)
endif

$(bin): $(objects)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDADD)
