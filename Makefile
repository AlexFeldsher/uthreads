CC=g++
CFLAGS=-std=c++11
OBJECTS=uthreads.o blackbox.o scheduler.o
LIB=libuthreads.a
AR=ar
ARFLAGS=rcs

lib: uthreads.o blackbox.o scheduler.o
	$(AR) $(ARFLAGS) $(LIB) uthreads.o blackbox.o scheduler.o
	rm -f $(OBJECTS)
uthreads.o: uthreads.cpp uthreads.h scheduler.h thread.h blackbox.h messages.h
	$(CC) $(CFLAGS) -c uthreads.cpp
blackbox.o: blackbox.h blackbox.cpp
	$(CC) $(CFLAGS) -c blackbox.cpp
scheduler.o: thread.h uthreads.h scheduler.cpp scheduler.h messages.h
	$(CC) $(CFLAGS) -c scheduler.cpp
tar: thread.h uthreads.cpp blackbox.cpp blackbox.h scheduler.h scheduler.cpp Makefile README messages.h
	tar -cvf ex2.tar thread.h uthreads.cpp blackbox.cpp blackbox.h scheduler.h scheduler.cpp Makefile README messages.h
clean:
	rm -f $(OBJECTS) $(LIB)
.PHONE: clean lib tar
