LIBPOPCORN = libpopcorn.a

#POPCORN=/usr/local/popcorn/x86_64
#CC = $(POPCORN)/bin/musl-gcc
CC = $(CROSS_COMPILE)gcc

AR = $(CROSS_COMPILE)ar
RANLIB = $(CROSS_COMPILE)ranlib

# Higher -O causes unexpected results
ifeq ($(ARCH),arm64)
OFLAGS = -O0
else ifeq ($(ARCH),powerpc)
OFLAGS = -O0
else
OFLAGS = -O2 -fno-schedule-insns -fno-schedule-insns2
endif

# Features
CFLAGS += $(OFLAGS)
CFLAGS += -I../include
CFLAGS += -DDEBUG -g -Wall -fPIC
CFLAGS += -D_OPENMP
#CFLAGS += -DWAIT_FOR_DEBUGGER

TARGETS = $(LIBPOPCORN)

all: $(TARGETS)

%.o: %.c
	$(CC) -c $(CFLAGS) $^ -o $@

libpopcorn.a: mapping.o migrate.o
	$(AR) -cr $@ $^
	$(RANLIB) $@

clean:
	rm -f $(TARGETS) *.o
