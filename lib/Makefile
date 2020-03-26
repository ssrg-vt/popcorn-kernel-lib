LIBMIGRATE = libmigrate.a

#POPCORN=/usr/local/popcorn/x86_64
#CC = $(POPCORN)/bin/musl-gcc
CC = $(CROSS_COMPILE)gcc

AR = $(CROSS_COMPILE)ar
RANLIB = $(CROSS_COMPILE)ranlib

# Higher -O causes unexpected results
ifeq ($(ARCH),arm64)
OFLAGS = -Og
else ifeq ($(ARCH),powerpc)
OFLAGS = -Og
else
OFLAGS = -Og -fno-schedule-insns -fno-schedule-insns2
endif

# Features
CFLAGS += $(OFLAGS)
CFLAGS += -I../include
CFLAGS += -DDEBUG -g -Wall -fPIC
CFLAGS += -D_OPENMP
#CFLAGS += -DWAIT_FOR_DEBUGGER

TARGETS = $(LIBMIGRATE)

all: $(TARGETS)

%.o: %.c
	$(CC) -c $(CFLAGS) $^ -o $@

libmigrate.a: mapping.o migrate.o
	$(AR) -cr $@ $^
	$(RANLIB) $@

clean:
	rm -f $(TARGETS) *.o
