#ifndef EX4OS_FBR_H
#define EX4OS_FBR_H

#include <iostream>
#include <list>
#include <unordered_map>
#include <unordered_set>


#define REF_COUNTER_INIT 1


using namespace std;
class Block;
class Cache;

typedef list<Block*> BlkList;
typedef list<int> IdxList;

/**
 * A file's block of data.
 */
class Block {
private:
    int fd;
    int index;
    int refCounter;
    string data;

public:
    Block(int fd, int index, const string &data) :
            fd(fd), index(index), refCounter(REF_COUNTER_INIT), data(data) {}

    inline int getFd() const {
        return fd;
    }

    inline int getIndex() const {
        return index;
    }

    inline int getRefCounter() const {
        return refCounter;
    }

    inline const string &getData() const {
        return data;
    }

    inline void incRefCounter() {
        ++refCounter;
    }

};


/**
 * A class represents a cache which uses the FBR algorithm.
 */
class Cache {
private:
    BlkList* blocksList;
    unordered_map<int, unordered_set<int>*>* blocksMap;
    int blkSize;
    int nOldBlks;
    int nNewBlks;
    int cacheSize;

    void divideBlocks(int fd, int lowerIdx, int upperIdx, IdxList &cacheHitList, IdxList &cacheMissList);
    void removeBlockBFR();

public:
    Cache(int blkSize, int nOldBlks, int nNewBlks, int cacheSize);

    virtual ~Cache();

    int readData(char *buf, int size,
                   off_t offset, int fd);
};




#endif
