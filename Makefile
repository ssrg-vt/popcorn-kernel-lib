LIBPOPCORN = libpopcorn.a

CC = gcc
CFLAGS += -DDEBUG -g -Wall -O0 # Higher -O may cause unexpected results

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

TARGETS = $(LIBPOPCORN)
EXAMPLES = basic pingpong demo
OBJDUMPS = basic.asm demo.asm

all: $(TARGETS) $(EXAMPLES) $(OBJDUMPS)

%.o: %.c
	$(CC) -c $(CFLAGS) $^ -o $@

libpopcorn.a: popcorn.o
	ar -cq $@ $^

basic : basic.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

basic.asm: basic
	objdump -d $< > $@

pingpong: pingpong.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

demo: demo.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

demo.asm: demo
	objdump -d $< > $@

clean:
	rm -f $(TARGETS) *.o
