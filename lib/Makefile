LIBPOPCORN = libpopcorn.a

CC = gcc
# Higher -O may cause unexpected results
CFLAGS += -O0

ARCH=$(shell uname -m)

ifeq ($(ARCH),x86_64)
CFLAGS += -DPOPCORN_X86
endif
ifeq ($(ARCH),aarch64)
CFLAGS += -DPOPCORN_ARM
endif

#CFLAGS += -DPOPCORN_PPC
#CFLAGS += -DPOPCORN_SPARC

# Features
CFLAGS += -DDEBUG -g -Wall
#CFLAGS += -DWAIT_FOR_DEBUGGER

TARGETS = $(LIBPOPCORN)

all: $(TARGETS)

%.o: %.c
	$(CC) -c $(CFLAGS) $^ -o $@

libpopcorn.a: popcorn.o
	ar -cq $@ $^

clean:
	rm -f $(TARGETS) *.o
