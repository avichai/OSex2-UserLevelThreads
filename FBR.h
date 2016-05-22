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
typedef list<size_t> IdxList;

/**
 * A file's block of data.
 */
class Block {
private:
    string path;
    size_t index;
    int refCounter;
    string data;

public:
    Block(string path, size_t index, const string &data) :
            path(path), index(index), refCounter(REF_COUNTER_INIT), data(data) {}

    inline string getPath() const {
        return path;
    }

    inline size_t getIndex() const {
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
    unordered_map<string, unordered_set<size_t>*>* blocksMap;
    size_t blkSize;
    int nOldBlks;
    int nNewBlks;
    int cacheSize;

    void divideBlocks(string path, size_t lowerIdx, size_t upperIdx, IdxList &cacheHitList, IdxList &cacheMissList);
    void removeBlockBFR();

public:
    Cache(size_t blkSize, int nOldBlks, int nNewBlks, int cacheSize);

    virtual ~Cache();

    int readData(char *buf, size_t size, off_t offset, int fd, string path);
};




#endif
