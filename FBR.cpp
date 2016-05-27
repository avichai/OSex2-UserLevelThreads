#include<unistd.h>
#include <cmath>
#include <string.h>
#include "FBR.h"
#include <malloc.h>
#include <fstream>

using namespace std;


#define SUCCESS 0


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

/*
 * Block's destructor.
 */
Block::~Block() {
    free(data);
}

/*
 * Cache constructor.
 */
Cache::Cache(size_t blkSize, unsigned int nOldBlks, unsigned int nNewBlks,
             unsigned int cacheSize) :
        blkSize(blkSize), nOldBlks(nOldBlks), nNewBlks(nNewBlks),
        cacheSize(cacheSize) {
    blocksList = new BlkList();
    blocksMap = new unordered_map<string, unordered_set<size_t> *>();
}

/*
 * Cache destructor.
 */
Cache::~Cache() {
    for (Block* block : *blocksList) {
        delete block;
    }
    blocksList->clear();
    delete blocksList;

    for (pair<string,unordered_set<size_t>*> curPair : *blocksMap) {
        unordered_set<size_t>* set = curPair.second;
        set->clear();
        delete set;
    }
    blocksMap->clear();
    delete blocksMap;
}



/**
 * Reads data using the cache and the BFR algorithm.
 */
int Cache::readData(char* buf, size_t start, size_t end, int fd, string path) {
    if (start >= end) {
        return 0;
    }

    // the block indexes needed to perform the read task
    size_t lowerIdx = start / blkSize;
    size_t upperIdx = (size_t) (end / blkSize) + 1;
    if (end % blkSize == 0) {
        --upperIdx;
    }
    size_t diffIdx = upperIdx - lowerIdx;

    // assigns hit and miss block lists
    bool pathInMap = blocksMap->find(path) != blocksMap->end();
    IdxList hitList, missList;
    unordered_set<size_t>* set = NULL;
    if (pathInMap) {
        set = blocksMap->at(path);
    }
    else {
        try {
            blocksMap->insert(make_pair(path, new unordered_set<size_t>()));
        } catch (bad_alloc) {
            return -errno;
        }
    }
    // assigns the hit and miss lists
    for (size_t index = lowerIdx; index < upperIdx; ++index) {
        if (pathInMap && (set->find(index) != set->end())) {
            hitList.push_front(index);
        }
        else {
            missList.push_front(index);
        }
    }

    // blocks data
    char* data = (char*) malloc(blkSize * diffIdx);
    if (data == NULL) {
        return -errno;
    }

    // cache hits
    if (cacheHit(hitList, path, lowerIdx, data) != SUCCESS) {
        return -errno;
    }
    // cache misses
    if (cacheMiss(missList, fd, path, lowerIdx,data) != SUCCESS) {
        return -errno;
    }

    size_t diff = end - start;
    memcpy(buf, data + (start % blkSize), diff);
    free(data);

    return (int) diff;
}


/*
 * Cache hits
 */
int Cache::cacheHit(IdxList hitList, string path, size_t lowerIdx,
                    char* dataArr) {
    if (hitList.empty()) {
        return SUCCESS;
    }

    unsigned int blkPosition = 0;    // the block's position in the blocks list.
    Block* block;
    for (auto it = blocksList->begin(); it != blocksList->end();) {
        block = *it;
        ++blkPosition;
        size_t blkIndex = block->getIndex();
        if (block->getPath() == path && inList(hitList, blkIndex)) {
            cerr << "cache hit: index = " << blkIndex << endl; //todo
            char* blkData = block->getData();
            memcpy(dataArr + ((blkIndex - lowerIdx) * blkSize),blkData,blkSize);
            if (blkPosition > nNewBlks) {
                block->incRefCounter();
            }
            it = blocksList->erase(it);
            blocksList->push_front(block);
        }
        else {
            ++it;
        }
    }

    return SUCCESS;
}


/*
 * Cache misses
 */
int Cache::cacheMiss(IdxList missList, int fd, string path, size_t lowerIdx,
                     char* dataArr) {
    if (missList.empty()) {
        return SUCCESS;
    }

    for (size_t blkIndex : missList) {
        if (blocksList->size() == cacheSize) {
            removeBlockBFR();
        }
        char* buffer = (char*) aligned_alloc(blkSize, blkSize);
        if (buffer == NULL) {
            return -errno;
        }
        if (pread(fd, buffer, blkSize, blkIndex * blkSize) < SUCCESS) {
            return -errno;
        }
        Block* block;
        try {
            block = new Block(path, blkIndex, buffer);
        } catch(bad_alloc) {
            return -errno;
        }
        blocksMap->at(path)->insert(blkIndex);
        memcpy(dataArr + ((blkIndex - lowerIdx) * blkSize), buffer, blkSize);
        cerr << "cache miss: index = " << blkIndex << endl; //todo
        blocksList->push_front(block);
    }
    return SUCCESS;
}

/*
 * Chooses a victim block to evict from the cache applying the BFR algorithm.
 */
void Cache::removeBlockBFR() {
    auto it = --(blocksList->end());
    Block* blkToRemove = (*it);
    for (unsigned int i = 0; i < nOldBlks; ++i)
    {
        if ((*it)->getRefCounter() < blkToRemove->getRefCounter()) {
            blkToRemove = (*it);
        }
        --it;
    }
    size_t indexToRem = blkToRemove->getIndex();
    string pathToRem = blkToRemove->getPath();

    blocksList->remove(blkToRemove);
    delete blkToRemove;

    unordered_set<size_t>* set = blocksMap->at(pathToRem);
    set->erase(indexToRem);
    if(set->empty()) {
        blocksMap->erase(pathToRem);
        delete set;
    }
}

/*
 * Renaming the paths in the cache's dasts.
 */
void Cache::rename(string fullPath, string fullNewPath) {
    for (auto it = blocksList->begin(); it != blocksList->end(); ++it) {
        string oldPath = (*it)->getPath();
        size_t foundPath = oldPath.find(fullPath);

        if (foundPath != string::npos) {
            string newPath = oldPath.replace(oldPath.find(fullPath),
                                             fullPath.length(), fullNewPath);
            (*it)->setPath(newPath);
        }
    }

    for (auto it = blocksMap->begin(); it != blocksMap->end();) {
        string oldPath = (*it).first;
        if (oldPath.find(fullPath) != string::npos) {
            string newPath = oldPath.replace(oldPath.find(fullPath),
                                             fullPath.length(), fullNewPath);
            unordered_set<size_t>* set = (*it).second;
            it = blocksMap->erase(it);

            blocksMap->insert(make_pair(newPath, set));
        }
        else {
            ++it;
        }
    }
}

/*
 * Used by the ioctl function.
 */
std::string Cache::getCacheData() {
    string buf = "";

    for (Block* block : *blocksList) {
        buf += block->getPath() + " " + to_string(block->getIndex()) + " " +
                to_string(block->getRefCounter()) + "\n";
    }

    return buf;
}
