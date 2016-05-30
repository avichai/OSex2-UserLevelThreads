CC = g++
CPP_FLAGS = -Wall -std=c++11
EXTRA_FILES = Makefile README

all: uthreads tar

uthreads.o: uthreads.cpp uthreads.h Thread.h
	$(CC) $(CPP_FLAGS) -c uthreads.cpp  Thread.cpp
    
uthreads: uthreads.o
	ar rcs libuthreads.a uthreads.o Thread.o    

tar: uthreads.cpp $(EXTRA_FILES)
	tar cvf ex2.tar uthreads.cpp Thread.cpp Thread.h $(EXTRA_FILES) 

clean: 
	rm *.o libuthreads.a ex2.tar

.DEFAULT_GOAL := uthreads 
    
.PHONY: 
	clean all 
