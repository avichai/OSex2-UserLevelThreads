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
int Cache::readData(char *buf, size_t size, off_t offset,
                    struct fuse_file_info *fi) {
    int fd = fi->fh;
    off_t fileSize = lseek(fd, 0, SEEK_END);
    if (fileSize == LSEEK_FALILURE)
    {
        return -errno;
    }
    if (fileSize < offset || size <= 0)
    {
        return SUCCESS;
    }
    // todo get the lower and upper bounds of block indexes (lower = offser, upper = min(off+size, fileSize)


    return 0;
}
