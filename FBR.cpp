#include<unistd.h>
#include <cmath>
#include <string.h>
#include "FBR.h"
#include <assert.h>


using namespace std;


#define SUCCESS 0
#define LSEEK_FALILURE -1



/*
 * Returns true iff the index is found in the list.
 */
static bool inList(IdxList &l, size_t index) {
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
Cache::Cache(size_t blkSize, unsigned int nOldBlks, unsigned int nNewBlks, unsigned int cacheSize) :
        blkSize(blkSize), nOldBlks(nOldBlks), nNewBlks(nNewBlks),
        cacheSize(cacheSize) {
    blocksList = new list<Block*>();
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
int Cache::readData(char *buf, size_t size, off_t offset, int fd, string path) {
    // checks if the call is valid.
    off_t fileSize = lseek(fd, 0, SEEK_END);
    if (fileSize == LSEEK_FALILURE)
    {
        return -errno;
    }

    // read bytes from start to end
    size_t start = (size_t) offset;
    size_t end = min(start + size, (size_t) fileSize); // todo maybe + 1

    // the block indexes needed to perform the read task
    size_t lowerIdx = start / blkSize;
    size_t upperIdx = (size_t) ceil(end / blkSize);
    size_t diffIdx = upperIdx - lowerIdx;

    IdxList cacheHitList, cacheMissList;
    divideBlocks(path, lowerIdx, upperIdx, cacheHitList, cacheMissList);

    string dataArr[diffIdx];
    // cache hits
    if (!cacheHitList.empty()) {
        unsigned int blkPosition = 0;     // the block's position in the blocks list.
        for (Block* block : *blocksList) {
            ++blkPosition;
            size_t blkIndex = block->getIndex();
            if (block->getPath() == path && inList(cacheHitList, blkIndex)) {
                dataArr[blkIndex - lowerIdx] = block->getData();
                if (blkPosition > nNewBlks) {
                    block->incRefCounter();
                }
                cacheHitList.remove(blkIndex);
            }
        }
    }
    // cache misses
    for (size_t index : cacheMissList) {
        if (blocksList->size() == cacheSize) {
            removeBlockBFR();
        }

        char buffer[blkSize];
        if (pread(fd, buffer, blkSize, start) < SUCCESS) { // todo set offset
            return -errno;
        }
        string bufferStr = buffer;
        Block* block;
        try {
            block = new Block(path, index, bufferStr);
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
    auto it = --(blocksList->end());
    Block* blkToRemove = (*it);
    for (unsigned int i = 0; i < nOldBlks; ++i)
    {
        if ((*it)->getRefCounter() < blkToRemove->getRefCounter())
        {
            blkToRemove = (*it);
        }
        --it;
    }
    blocksList->remove(blkToRemove);
    delete blkToRemove;

}

void Cache::divideBlocks(string path, size_t lowerIdx, size_t upperIdx,
                         IdxList &cacheHitList, IdxList &cacheMissList)
{
    size_t size = upperIdx - lowerIdx + 1;

    //no fd in table;
    if (blocksMap->find(path) == blocksMap->end())
    {
        for (size_t i = lowerIdx; i <= upperIdx; ++i)
        {
            cacheMissList.push_back(i);
        }
        return;
    }
    //fd found.
    else
    {
        for (size_t i = lowerIdx; i <= upperIdx; ++i)
        {
            if(blocksMap->at(path)->find(i) == blocksMap->at(path)->end())
            {
                cacheMissList.push_back(i);
            }
            else
            {
                cacheMissList.push_back(i);
            }
        }
    }
    assert((cacheHitList.size()+cacheMissList.size()) == size);                 //todo remove assert
}
