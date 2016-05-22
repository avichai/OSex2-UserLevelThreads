#ifndef EX4OS_FBR_H
#define EX4OS_FBR_H

#include <iostream>
#include <list>
#include <unordered_map>
#include <unordered_set>


#define REF_COUNTER_INIT 1


class Block;
class Cache;

typedef std::list<Block*> BlkList;
typedef std::list<size_t> IdxList;


/**
 * A file's block of data.
 */
class Block {
private:
    std::string path;
    size_t index;
    int refCounter;
    std::string data;

public:
    Block(std::string path, size_t index, const std::string &data) :
            path(path), index(index), refCounter(REF_COUNTER_INIT), data(data) {}

    inline std::string getPath() const {
        return path;
    }

    inline size_t getIndex() const {
        return index;
    }

    inline int getRefCounter() const {
        return refCounter;
    }

    inline const std::string &getData() const {
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
    std::unordered_map<std::string, std::unordered_set<size_t>*>* blocksMap;
    size_t blkSize;
    unsigned int nOldBlks;       // todo maybe unsigned int or size_t
    unsigned int nNewBlks;
    unsigned int cacheSize;

    void divideBlocks(std::string path, size_t lowerIdx, size_t upperIdx, IdxList &cacheHitList, IdxList &cacheMissList);
    void removeBlockBFR();

public:
    Cache(size_t blkSize, unsigned int nOldBlks, unsigned int nNewBlks, unsigned int cacheSize);

    virtual ~Cache();

    int readData(char *buf, size_t size, off_t offset, int fd, std::string path);
};




#endif
