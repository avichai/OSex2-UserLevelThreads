#include "FBR.h"


Cache::Cache(size_t nOldBlk, int blkSize, size_t nNewBlk, size_t cacheSize) :
        nOldBlk(nOldBlk), blkSize(blkSize), nNewBlk(nNewBlk), cacheSize(cacheSize) {
    blockList = new list();
}

Cache::~Cache() {
    while (!blockList->empty()) {
        Block* blk = blockList->front();
        blockList->pop_front();
        delete blk;
    }
    delete blockList;
}

int Cache::readData(char *buf, size_t size, off_t offset,
                    struct fuse_file_info *fi) {
    return 0;
}
