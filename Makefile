CC = g++
CFLAGS = -Wall -std=c++11 -D_FILE_OFFSET_BITS=64 -g
EXTRA_FILES = Makefile README


all: CachingFileSystem.o FBR.o
	${CC} ${CFLAGS} CachingFileSystem.o FBR.o \
	 `pkg-config fuse --cflags --libs` -o CachingFileSystem

CachingFileSystem.o:
	${CC} ${CFLAGS} CachingFileSystem.cpp -c	

FBR.o: FBR.h FBR.cpp
	${CC} ${CFLAGS} FBR.cpp -c

tar:
	tar cvf ex4.tar CachingFileSystem.cpp FBR.cpp FBR.h ${EXTRA_FILES}

clean: 
	rm CachingFileSystem *.o ex4.tar

.DEFAULT_GOAL := all

.PHONY: 
	clean all un
