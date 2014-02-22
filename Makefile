CC =    cc
CCC =    c++
CFLAGS =  -pipe -O -W -Wall -Winline -Wpointer-arith -Wno-unused-parameter -Werror -g -std=c11 -funit-at-a-time
CCFLAGS =  -pipe -O -W -Wall -Winline -Wpointer-arith -Wno-unused-parameter -Werror -g -std=c++11 -funit-at-a-time
CPP =   cc -E

all: trace/trace.o src/post-process

trace/trace.o: trace/trace.c
	$(CC) -c $(CFLAGS) $(ALL_INCS) -finstrument-functions -o $@ $<

src/ntree.o: src/ntree.c src/ntree.h
	$(CC) -c $(CFLAGS) $(ALL_INCS) -o $@ $<

src/stack.o: src/stack.c src/stack.h
	$(CC) -c $(CFLAGS) $(ALL_INCS) -o $@ $<

src/post-process.o: src/post-process.c src/ntree.h src/stack.h
	$(CC) -c $(CFLAGS) $(ALL_INCS) -o $@ $<

src/post-process: src/post-process.o src/stack.o src/ntree.o
	$(CC) -o $@ $^ -g -luuid -lstdc++ -lrt

