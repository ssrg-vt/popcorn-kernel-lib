CC = $(CROSS_COMPILE)gcc
OBJDUMP = $(CROSS_COMPILE)objdump

LIBPOPCORN = libpopcorn.a
POPCORN_LIB_PATH = ../lib
POPCORN_INC_PATH = ../include

# Features
# Higher than O0 causes unexpected behavior
CFLAGS += -O0
CFLAGS += -DDEBUG -g -Wall -I$(POPCORN_INC_PATH)
#CFLAGS += -DWAIT_FOR_DEBUGGER

LDFLAGS += -static -pthread -L$(POPCORN_LIB_PATH)
LIBS += -l:$(LIBPOPCORN)

TARGETS = basic pingpong demo mt stack rw
OBJDUMPS = 

all: $(TARGETS) $(OBJDUMPS)

%.o: %.c
	$(CC) -c $(CFLAGS) $^ -o $@

basic : basic.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

basic.asm: basic
	$(OBJDUMP) -d $< > $@

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

demo.asm: demo
	$(OBJDUMP) -d $< > $@

clean:
	rm -f $(TARGETS) $(OBJDUMPS) *.o
