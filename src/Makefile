CC = gcc
CFLAGS +=

LIBPOPCORN = libpopcorn.a
POPCORN_LIB_PATH = ../lib
POPCORN_INC_PATH = ../include

# Features
CFLAGS += -DDEBUG -g -Wall -I$(POPCORN_INC_PATH)
#CFLAGS += -DWAIT_FOR_DEBUGGER

LDFLAGS += -static -pthread -L$(POPCORN_LIB_PATH)
LIBS += -l:$(LIBPOPCORN)

TARGETS = basic pingpong demo mt
OBJDUMPS = basic.asm demo.asm

all: $(TARGETS) $(OBJDUMPS)

%.o: %.c
	$(CC) -c $(CFLAGS) $^ -o $@

basic : basic.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

basic.asm: basic
	objdump -d $< > $@

pingpong: pingpong.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

demo: demo.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mt: mt.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

demo.asm: demo
	objdump -d $< > $@

clean:
	rm -f $(TARGETS) $(OBJDUMPS) *.o
