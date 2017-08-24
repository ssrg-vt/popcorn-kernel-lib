LIBPOPCORN = libpopcorn.a

CC = gcc
# Higher -O may cause unexpected results
CFLAGS += -O0

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
