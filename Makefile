CC = gcc
CFLAGS = -o

SRCS = oss.c uprocess.c
EXES = $(SRCS:.c=.out)
.SUFFIXES: .c .out

.c.out:
	$(CC) $(CFLAGS) $@ $<

all: $(EXES)

clean:
	rm -f $(EXES) 

