CC =    cc
CCC =    c++
CFLAGS =  -pipe -O -W -Wall -Winline -Wpointer-arith -Wno-unused-parameter -Werror -g -std=c11 -funit-at-a-time
CCFLAGS =  -pipe -O -W -Wall -Winline -Wpointer-arith -Wno-unused-parameter -Werror -g -std=c++11 -funit-at-a-time
CPP =   cc -E

all: trace/trace.o

trace/trace.o: trace/trace.c
	$(CC) -c $(CFLAGS) $(ALL_INCS) -finstrument-functions -o $@ $<

