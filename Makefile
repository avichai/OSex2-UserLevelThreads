CC = g++
CFLAGS = -Wall -std=c++11 -D_FILE_OFFSET_BITS=64
EXTRA_FILES = Makefile README


all: CachingFileSystemApp
	


CachingFileSystemApp : CachingFileSystem.cpp FBR.h FBR.cpp
#	${CC} ${CFLAGS} CashTemp.cpp `pkg-config fuse --cflags --libs` -o caching
	${CC} ${CFLAGS} CachingFileSystem.cpp FBR.cpp  `pkg-config fuse --cflags --libs` -o caching	
	caching /tmp/root /tmp/mount 10 0.5 0.5
	
	
	
	
#	${CC} ${CFLAGS} CashTemp.cpp `pkg-config fuse --cflags --libs` -o caching
	
	
	
	

#caching /cs/stud/roee28c/ClionProjects/ex4OS/rootDir /cs/stud/roee28c/ClionProjects/ex4OS/mountDir 10 0.5 0.5
	
	
	
un:
	fusermount -u /tmp/mount
	
dirs:
	mkdir /tmp/root
	mkdir /tmp/mount
	mkdir /tmp/root/d1
	touch /tmp/root/f1
	touch /tmp/root/d1/f2
	echo dfjas adksjgdkjasg ldfkja klajgl kjasg > /tmp/root/f1

py:
	python -c "import os, fcntl; fd = os.open('/tmp/mount/f1', os.O_RDONLY); os.ioctl(fd,0); os.close(fd)"
	#fcntl.ioctl(fd, 0);
	
	
	
	
	
	
	
	
	
	
	
	
	
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

