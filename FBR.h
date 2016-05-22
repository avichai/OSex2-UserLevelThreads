#ifndef EX4OS_FBR_H
#define EX4OS_FBR_H

#include <iostream>
#include <list>

using namespace std;

class Block {
private:
    int fd;
    size_t  blkInd;
    size_t refCounter;
    string data;


public:
    Block(int fd, size_t numBlk, size_t refCounter, const string &data) :
            fd(fd), blkInd(blkInd), refCounter(refCounter), data(data) {}

    inline int getFd() const {
        return fd;
    }

    inline size_t getNumBlk() const {
        return blkInd;
    }

    inline size_t getRefCounter() const {
        return refCounter;
    }

    inline const string &getData() const {
        return data;
    }

    inline void incRefCounter() {
        ++refCounter;
    }

};


class Cache {
private:
    list<Block*> *blockList;
    int blkSize;
    size_t nOldBlk;
    size_t nNewBlk;
    size_t cacheSize;


public:
    Cache(size_t nOldBlk, int blkSize, size_t nNewBlk, size_t cacheSize);

    virtual ~Cache();

    int readData(char *buf, size_t size,
                   off_t offset, struct fuse_file_info *fi);

public:
    inline int getBlkSize() const {
        return blkSize;
    }

};

#endif
