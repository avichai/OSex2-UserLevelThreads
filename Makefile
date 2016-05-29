CC = g++
CFLAGS = -Wall -std=c++11 -D_FILE_OFFSET_BITS=64 -g
EXTRA_FILES = Makefile README
ARG1 = caching /tmp/root /tmp/mount 30 0.3 0.3
ARG2 = python ../test4/fuseTest.py caching


all: cache

cache: CachingFileSystem.cpp FBR.h
	${CC} ${CFLAGS} CachingFileSystem.cpp FBR.cpp  `pkg-config fuse --cflags --libs` -o caching	
	

run: cache
	caching /tmp/root /tmp/mount 30 0.3 0.3

un:
	fusermount -u /tmp/mount

fuseTest: cache
	python ../test4/fuseTest.py caching
	
	
	
dirs:
	mkdir /tmp/root
	mkdir /tmp/mount
	mkdir /tmp/root/d1
	mkdir /tmp/root/d2
	mkdir /tmp/root/d3	
	touch /tmp/root/f1
	touch /tmp/root/d1/f2
	touch /tmp/root/d1/f3
	echo dfjas adksjgdkjasg ldfkja klajgl kjasg > /tmp/root/f1
	echo dfjas adksjgdkjasg ldfkja klajgl kjasg > /tmp/root/d1/f2

py:
	python -c "import os, fcntl; fd = os.open('/tmp/mount/a', os.O_RDONLY); fcntl.ioctl(fd,0); os.close(fd)"
	#fcntl.ioctl(fd, 0);
	
renpy:
	python -c "import os, fcntl; fd = os.open('/tmp/mount/f1', os.O_RDONLY); fcntl.ioctl(fd,0); os.close(fd)"
	

lib: CachingFileSystem.o
	ar rcs CachingFileSystem.a CachingFileSystem.o   
	
	
valgrind: cache
	valgrind --leak-check=full --show-possibly-lost=yes --show-reachable=yes --undef-value-errors=yes  ${ARG1}


tar:
	tar cvf ex4.tar CachingFileSystem.cpp FBR.cpp FBR.h $(EXTRA_FILES)

clean: 

.DEFAULT_GOAL := all

.PHONY: 
	clean all un
