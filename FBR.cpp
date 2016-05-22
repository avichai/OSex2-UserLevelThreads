#include <unistd.h>
#include <cmath>
#include <string.h>
#include "FBR.h"

#define SUCCESS 0
#define LSEEK_FALILURE -1


/*
 * Returns true iff the index is found in the list.
 */
static bool inList(list<int> &l, int index) {
    for (auto it = l.begin(); it != l.end(); ++it) {
        if (*it == index) {
            return true;
        }
    }
    return false;
}

/**
 * Cache constructor.
 */
Cache::Cache(int blkSize, int nOldBlks, int nNewBlks, int cacheSize) :
        blkSize(blkSize), nOldBlks(nOldBlks), nNewBlks(nNewBlks),
        cacheSize(cacheSize) {
    blocksList = new list();
}

/**
 * Cache destructor.
 */
Cache::~Cache() {
    for (Block* block : *blocksList) {
        delete block;
    }
    delete blocksList;
}

/**
 * todo
 * todo check if no need to call read
 * todo: -should ret SUCCESS? -is it possible to receive neg size and offset?
 */
int Cache::readData(char *buf, int size, off_t offset, int fd) {
    // checks if the call is valid.
    off_t fileSize = lseek(fd, 0, SEEK_END);
    if (fileSize == LSEEK_FALILURE)
    {
        return -errno;
    }

    // read bytes from start to end
    int start = (int) offset;
    int end = min(start + size, (int) fileSize); // todo maybe + 1

    // the block indexes needed to perform the read task
    int lowerIdx = start / blkSize;
    int upperIdx = (int) ceil(end / blkSize);
    int diffIdx = upperIdx - lowerIdx;

    IdxList cacheHitList, cacheMissList;
    divideBlocks(fd, lowerIdx, upperIdx, cacheHitList, cacheMissList);

    string dataArr[diffIdx];
    // cache hits
    if (!cacheHitList.empty()) {
        int blkPosition = 0;     // the block's position in the blocks list.
        for (Block* block : *blocksList) {
            ++blkPosition;
            int blkIndex = block->getIndex();
            if (block->getFd() == fd && inList(cacheHitList, blkIndex)) {
                dataArr[blkIndex - lowerIdx] = block->getData();
                if (blkPosition > nNewBlks) {
                    block->incRefCounter();
                }
                cacheHitList.remove(blkIndex);
            }
        }
    }
    // cache misses
    for (int index : cacheMissList) {
        if (blocksList->size() == cacheSize) {
            removeBlockBFR();
        }

        char buffer[blkSize];
        if (pread(fd, buffer, blkSize, start) < SUCCESS) {
            return -errno;
        }
        string bufferStr = buffer;
        Block* block;
        try {
            block = new(fd, index, bufferStr);
        } catch(bad_alloc) {
            return -errno;
        }
        dataArr[index - lowerIdx] = bufferStr;
        blocksList->push_front(block);
    }

    // copying the requested data to the buffer
    string data = "";
    for (string blkData : dataArr) {
        data += blkData;
    }
    data.substr(start % blkSize, end % blkSize);
    strcpy(buf, data.c_str());      // todo \0 needed to be inserted manually?

    return 0;
}

/*
 * Chooses a victim block to evict from the cache applying the BFR algorithm.
 */
void Cache::removeBlockBFR() {

}






//    if (fileSize < offset || size < 0 || offset < 0)
//    {
//        return SUCCESS;
//    }
