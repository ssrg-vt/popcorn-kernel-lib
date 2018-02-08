CC = $(CROSS_COMPILE)gcc
OBJDUMP = $(CROSS_COMPILE)objdump -d -S

LIBMIGRATE = libmigrate.a
POPCORN_LIB_PATH = ../lib
POPCORN_INC_PATH = ../include

# Features
CFLAGS += -O2
CFLAGS += -DDEBUG -g -Wall -I$(POPCORN_INC_PATH)
#CFLAGS += -DWAIT_FOR_DEBUGGER

LDFLAGS += -static -pthread -L$(POPCORN_LIB_PATH)
LIBS += -l:$(LIBMIGRATE)

TARGETS = basic pingpong demo mt stack rw status
OBJDUMPS =

all: $(TARGETS) $(OBJDUMPS)

%.o: %.c
	$(CC) -c $(CFLAGS) $^ -o $@

basic : basic.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

basic.asm: basic
	$(OBJDUMP) $< > $@

pingpong: pingpong.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

demo: demo.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

mt: mt.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

stack: stack.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

rw: rw.o
	$(CC) $(LDFLAGS) $^ $(LIBS) -o $@

status: status.o
	$(CC) $(LDFLAGS) $^ $(LIBS) -o $@

demo.asm: demo
	$(OBJDUMP) $< > $@

clean:
	rm -f $(TARGETS) $(OBJDUMPS) *.o
