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

TARGETS = ft2a ft2b ft2c ft2d kmeans
OBJDUMPS =

all: $(TARGETS) $(OBJDUMPS)

%.o: %.c
	$(CC) -c $(CFLAGS) $^ -o $@

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

ft2d : ft2d.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

ft2d.asm: ft2d
	$(OBJDUMP) $< > $@

kmeans: kmeans.o
	$(CC) $(LDFLAGS) $^ $(LIBS) -o $@

clean:
	rm -f $(TARGETS) $(OBJDUMPS) *.o
