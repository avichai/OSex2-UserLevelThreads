#include <unistd.h>
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

/**
 * todo
 */
//int Cache::readData(char *buf, size_t size, off_t offset, int fd) {
//    // checks if the call is valid.
//    off_t fileSize = lseek(fd, 0, SEEK_END);
//    if (fileSize == LSEEK_FALILURE)
//    {
//        return -errno;
//    }
//    if (fileSize < offset || size < 0 || offset < 0)
//    {
//        return SUCCESS;     //todo: -should ret SUCCESS? -is it possible to receive neg size and offset?
//    }
//
//    off_t end = min(offset + size, fileSize);
//    size_t lower = offset / this->blkSize;
//
//
//
//
//
//    return 0;
//}

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
