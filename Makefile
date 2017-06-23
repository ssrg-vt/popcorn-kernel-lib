LIBPOPCORN = libpopcorn.a

CC = gcc
CFLAGS += -DDEBUG -g -Wall

ARCH=$(shell uname -m)

ifeq ($(ARCH),x86_64)
CFLAGS += -DPOPCORN_X86
endif
ifeq ($(ARCH),aarch64)
CFLAGS += -DPOPCORN_ARM
endif

#CFLAGS += -DPOPCORN_PPC
#CFLAGS += -DPOPCORN_SPARC

LDFLAGS += -static -pthread -L.
LIBS += -l:$(LIBPOPCORN)

TARGETS = $(LIBPOPCORN) basic pingpong demo

all: $(TARGETS)

%.o: %.c
	$(CC) -c $(CFLAGS) $^ -o $@

libpopcorn.a: popcorn.o
	ar -cq $@ $^

basic : basic.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

pingpong: pingpong.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

demo: demo.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -f $(TARGETS) *.o
