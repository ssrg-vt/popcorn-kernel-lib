CC = $(CROSS_COMPILE)gcc
OBJDUMP = $(CROSS_COMPILE)objdump -d -S

LIBMIGRATE = libmigrate.a
POPCORN_LIB_PATH = ../lib
POPCORN_INC_PATH = ../include

# Features
CFLAGS += -Og
CFLAGS += -DDEBUG -g -Wall -I$(POPCORN_INC_PATH)
#CFLAGS += -DWAIT_FOR_DEBUGGER

LDFLAGS += -static -L$(POPCORN_LIB_PATH)
LIBS += -pthread -l:$(LIBMIGRATE)

TARGETS = basic ft2a ft2b ft2c pingpong demo mt stack rw status kmeans grep
OBJDUMPS =

all: $(TARGETS) $(OBJDUMPS)

%.o: %.c
	$(CC) -c $(CFLAGS) $^ -o $@

basic : basic.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

basic.asm: basic
	$(OBJDUMP) $< > $@

ft2a : ft2a.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

ft2a.asm: ft2a
	$(OBJDUMP) $< > $@

ft2b : ft2b.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

ft2b.asm: ft2b
	$(OBJDUMP) $< > $@

ft2c : ft2c.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

ft2c.asm: ft2c
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

kmeans: kmeans.o
	$(CC) $(LDFLAGS) $^ $(LIBS) -o $@

grep: grep.o
	$(CC) $(LDFLAGS) $^ $(LIBS) -o $@

clean:
	rm -f $(TARGETS) $(OBJDUMPS) *.o
