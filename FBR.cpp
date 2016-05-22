#include <unistd.h>
#include <cmath>
#include <string.h>
#include "FBR.h"

#define SUCCESS 0
#define LSEEK_FALILURE -1

/**
 * Cache constructor.
 */
Cache::Cache(size_t nOldBlk, int blkSize, size_t nNewBlk, size_t cacheSize) :
        nOldBlk(nOldBlk), blkSize(blkSize), nNewBlk(nNewBlk),
        cacheSize(cacheSize) {
    blockList = new list();
}

/**
 * Cache destructor.
 */
Cache::~Cache() {
    for (Block* block : *blockList) {
        delete block;
    }
    delete blockList;
}

//    if (fileSize < offset || size < 0 || offset < 0)
//    {
//        return SUCCESS;     //todo: -should ret SUCCESS? -is it possible to receive neg size and offset?
//    }
/**
 * todo
 * todo check if no need to call read
 */
int Cache::readData(char *buf, size_t size, off_t offset, int fd) {
    // checks if the call is valid.
    off_t fileSize = lseek(fd, 0, SEEK_END);
    if (fileSize == LSEEK_FALILURE)
    {
        return -errno;
    }

    // read bytes from start to end
    size_t start = (size_t) offset;
    size_t end = min(start + size, (size_t) fileSize);

    // the block indexes needed to perform the read task
    size_t lowerIdx = start / blkSize;
    size_t upperIdx = (size_t) ceil(end / blkSize);
    list<Block*> cacheHitList, cacheMissList;
    divideBlocks(fd, lowerIdx, upperIdx, cacheHitList, cacheMissList);
    size_t diffIdx = upperIdx - lowerIdx;

    // Retrieving the needed blocks to perform the read task
    string dataArr[diffIdx];
    for (Block* block : cacheHitList) {

    }
    for (Block* block : cacheMissList) {

    }
//    for (size_t blkIdx = lowerIdx; blkIdx < upperIdx; ++blkIdx) {
//        if (isBlockInCache(fd, blkIdx)) {       // cache hit
//
//        }
//        else {                                  // cache miss
//
//        }
//    }

    // copying the requested data to the buffer
    string data = "";
    for (string blkData : dataArr) {
        data += blkData;
    }
    data.substr(start % blkSize, end % blkSize);
    strcpy(buf, data.c_str());

    return 0;
}

/*
 * checks if the given block residents in the cache (in O(1)).
 */
bool Cache::isBlockInCache(int fd, size_t index) {
    if(blockMap[fd].find(index) == blockMap[fd].end())
    {
        return false;
    }
    return true;
}

/*
 * Chooses a victim block to evict from the cache applying the BFR algorithm.
 */
void Cache::removeBlockBFR() {
    auto it = --(blockList->end());
    Block* blkToRemove = (*it);
    for (unsigned int i = 0; i < nOldBlk; ++i)
    {
        if ((*it)->getRefCounter() > blkToRemove->getRefCounter())
        {
            blkToRemove = (*it);
        }
        --it;
    }
    blockList->remove(blkToRemove);
    delete blkToRemove;

}
