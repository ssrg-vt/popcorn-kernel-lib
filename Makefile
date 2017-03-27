CC = gcc
CFLAGS += -DDEBUG -g

ARCH=$(shell uname -m)

ifeq ($(ARCH),x86_64)
CFLAGS += -DPOPCORN_X86
endif

ifeq ($(ARCH),aarch64)
CFLAGS += -DPOPCORN_ARM
endif

#CFLAGS += -DPOPCORN_PPC
#CFLAGS += -DPOPCORN_SPARC

LDFLAGS += -static -pthread

TARGETS = basic mt ping demo

all: $(TARGETS)

%.o: %.c
	$(CC) -c $(CFLAGS) $^ -o $@

basic : basic.o popcorn.o
	$(CC) $(LDFLAGS) -o $@ $^

mt: mt.o popcorn.o
	$(CC) $(LDFLAGS) -o $@ $^

ping: ping.o popcorn.o
	$(CC) $(LDFLAGS) -o $@ $^

demo: demo.o popcorn.o
	$(CC) $(LDFLAGS) -o $@ $^

clean:
	rm -f $(TARGETS) *.o [0-9]*
