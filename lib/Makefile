LIBPOPCORN = libpopcorn.a

CC = $(CROSS_COMPILE)gcc
AR = $(CROSS_COMPILE)ar

# Higher than -O1 causes unexpected results
CFLAGS += -O0

# Features
CFLAGS += -DDEBUG -g -Wall
#CFLAGS += -DWAIT_FOR_DEBUGGER

TARGETS = $(LIBPOPCORN)

all: $(TARGETS)

%.o: %.c
	$(CC) -c $(CFLAGS) $^ -o $@

libpopcorn.a: popcorn.o
	$(AR) -cq $@ $^

clean:
	rm -f $(TARGETS) *.o
