LIBPOPCORN = libpopcorn.a

CC = $(CROSS_COMPILE)gcc
AR = $(CROSS_COMPILE)ar

# Higher -O causes unexpected results
ifeq ($(ARCH),arm64)
OFLAG = -O0
else ifeq ($(ARCH),powerpc)
OFLAG = -O0
else
OFLAG = -O2 -fno-schedule-insns -fno-schedule-insns2
endif

# Features
CFLAGS += $(OFLAG) -DDEBUG -g -Wall -fPIC
CFLAGS += -D_OPENMP
#CFLAGS += -DWAIT_FOR_DEBUGGER

TARGETS = $(LIBPOPCORN)

all: $(TARGETS)

%.o: %.c
	$(CC) -c $(CFLAGS) $^ -o $@

libpopcorn.a: popcorn.o
	$(AR) -cq $@ $^

clean:
	rm -f $(TARGETS) *.o
