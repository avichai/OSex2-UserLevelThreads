CC = g++
CFLAGS = -Wall -std=c++11
EXTRA_FILES = Makefile README


all: CachingFileSystemApp

CachingFileSystemApp : CachingFileSystem.cpp
	${CC} ${CFLAGS} CachingFileSystem.cpp `pkg-config fuse --cflags --libs` -o caching
	caching /tmp/rootDir /tmp/mountDir 10 0.5 0.5
	
	
	
	
	
	
	
	
	
	
	

#caching /cs/stud/roee28c/ClionProjects/ex4OS/rootDir /cs/stud/roee28c/ClionProjects/ex4OS/mountDir 10 0.5 0.5
	
	
	
un:
	fusermount -u /tmp/mountDir 	
	

	
	
	
	
	
	
	
	
	
	
	
	
	
	
app: CachingFileSystem.o
	$(CC) $(CFLAGS) CachingFileSystem.o -o myApp
	

	
	

lib: CachingFileSystem.o
	ar rcs CachingFileSystem.a CachingFileSystem.o   

Search: Search.o lib
	$(CC) $(CFLAGS) Search.o -L. MapReduceFramework.a -o Search
	
	
	
	

tar:
	tar cvf ex4.tar CachingFileSystem.cpp $(EXTRA_FILES)

clean: 
	rm *.o CachingFileSystem.a ex4.tar myApp caching

.DEFAULT_GOAL := all

.PHONY: 
	clean all un

