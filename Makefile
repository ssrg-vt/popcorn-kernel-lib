CC = gcc
CFLAGS += -DDEBUG -g
#CFLAGS += POPCORN_X86
#CFLAGS += -DPOPCORN_ARM
LDFLAGS += -static -pthread
TARGETS = basic mt ping

all: $(TARGETS)

%.o: %.c
	$(CC) -c $(CFLAGS) $^ -o $@

basic : basic.o popcorn.o
	$(CC) $(LDFLAGS) -o $@ $^

mt: mt.o popcorn.o
	$(CC) $(LDFLAGS) -o $@ $^

ping: ping.o popcorn.o
	$(CC) $(LDFLAGS) -o $@ $^

clean:
	rm -f $(TARGETS) *.o [0-9]*
