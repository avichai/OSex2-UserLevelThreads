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


/**
 * A file's block of data.
 */
class Block {
private:
    std::string path;
    size_t index;
    int refCounter;
    char* data;

public:
    void setPath(const std::string &path) {
        Block::path = path;
    }

    Block(std::string path, size_t index, char* data) :
            path(path), index(index), refCounter(REF_COUNTER_INIT),data(data){}

    virtual  ~Block();

    inline std::string getPath() const {
        return path;
    }

    inline size_t getIndex() const {
        return index;
    }

    inline int getRefCounter() const {
        return refCounter;
    }

    inline char* getData() const {
        return data;
    }

    inline void incRefCounter() {
        ++refCounter;
    }

    std::string getRelativePath(std::string rootPath);
};


/**
 * A class represents a cache which uses the FBR algorithm.
 */
class Cache {
private:
    BlkList* blocksList;
    std::unordered_map<std::string, std::unordered_set<size_t>*>* blocksMap;

private:
    size_t blkSize;
    unsigned int nOldBlks;
    unsigned int nNewBlks;
    unsigned int cacheSize;

    void removeBlockBFR(std::string pathToKeep);
    void cacheHit(std::string path, size_t index, size_t lowerIdx,
                  char*  dataArr);
    int cacheMiss(int fd, std::string path, size_t index, size_t lowerIdx,
                  char* dataArr);

public:
    Cache(size_t blkSize, unsigned int nOldBlks, unsigned int nNewBlks,
          unsigned int cacheSize);
    virtual ~Cache();

    int readData(char *buf, size_t start, size_t end, int fd,std::string path);
    void rename(std::string fullPath, std::string fullNewPath);
    std::string getCacheData(std::string rootPath);
    inline bool isCacheEmpty() { return blocksList->empty(); }
};




#endif
