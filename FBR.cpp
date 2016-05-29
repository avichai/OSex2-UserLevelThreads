#include<unistd.h>
#include <cmath>
#include <string.h>
#include "FBR.h"
#include <malloc.h>
#include <fstream>
#include <assert.h>

using namespace std;


#define SUCCESS 0
#define PATH_SUFF_IN_CACHE "/"

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



/*
 * Reads data using the cache and the BFR algorithm.
 */
int Cache::readData(char* buf, size_t start, size_t end, int fd, string path) {
    if (start >= end) {
        return 0;
    }

    path += PATH_SUFF_IN_CACHE;

    // the block indexes needed to perform the read task
    size_t lowerIdx = start / blkSize;
    size_t upperIdx = (size_t) (end / blkSize) + 1;
    if (end % blkSize == 0) {
        --upperIdx;
    }
    size_t diffIdx = upperIdx - lowerIdx;

    // assigns the set which corresponds to the blocks path
    bool pathInMap = blocksMap->find(path) != blocksMap->end();
    unordered_set<size_t>* set;
    if (pathInMap) {
        set = blocksMap->at(path);
    }
    else {
        try {
            set = new unordered_set<size_t>();
        } catch (bad_alloc) {
            return -errno;
        }
            blocksMap->insert(make_pair(path, set));
    }

    // blocks data
    char* data = (char*) malloc(blkSize * diffIdx);
    if (data == NULL) {
        return -errno;
    }

    // retrieving the required blocks
    for (size_t index = lowerIdx; index < upperIdx; ++index) {
        if (pathInMap && (set->find(index) != set->end())) {
            cacheHit(path, index, lowerIdx, data);
        }
        else {
            if (cacheMiss(fd, path,index, lowerIdx,data) != SUCCESS) {
                return -errno;
            }
        }
    }

    size_t diff = end - start;
    memcpy(buf, data + (start % blkSize), diff);
    free(data);

    return (int) diff;
}


/*
 * Cache hits
 */
void Cache::cacheHit(string path, size_t index, size_t lowerIdx,char* dataArr){
    unsigned int blkPosition = 0;   // the block's position in the blocks list.
    Block* block;
    for (auto it = blocksList->begin(); it != blocksList->end(); ++it) {
        block = *it;
        ++blkPosition;
        size_t blkIndex = block->getIndex();
        if ((block->getPath() == path) && (index == blkIndex)) {
            char* blkData = block->getData();
            memcpy(dataArr + ((blkIndex-lowerIdx) * blkSize),blkData,blkSize);
            if (blkPosition > nNewBlks) {
                block->incRefCounter();
            }
            blocksList->remove(block);
            blocksList->push_front(block);
            return;
        }
    }
}


/*
 * Cache misses
 */
int Cache::cacheMiss(int fd, string path, size_t index, size_t lowerIdx,
                     char* dataArr) {
    if (blocksList->size() == cacheSize) {
        removeBlockBFR(path);
    }
    char* buffer = (char*) aligned_alloc(blkSize, blkSize);
    if (buffer == NULL) {
        return -errno;
    }
    if (pread(fd, buffer, blkSize, index * blkSize) < SUCCESS) {
        return -errno;
    }
    Block* block;
    try {
        block = new Block(path, index, buffer);
    } catch(bad_alloc) {
        return -errno;
    }
    blocksMap->at(path)->insert(index);
    memcpy(dataArr + ((index - lowerIdx) * blkSize), buffer, blkSize);
    blocksList->push_front(block);

    return SUCCESS;
}

/*
 * Chooses a victim block to evict from the cache applying the BFR algorithm.
 * Shouldn't remove the set corresponds to the pathToKeep.
 */
void Cache::removeBlockBFR(string pathToKeep) {
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
    if (set->empty() && (pathToRem != pathToKeep)) {
        blocksMap->erase(pathToRem);
        delete set;
    }
}

/*
 * Renaming the paths in the cache's dasts.
 */
void Cache::rename(string fullPath, string fullNewPath) {

    fullPath += PATH_SUFF_IN_CACHE;
    fullNewPath += PATH_SUFF_IN_CACHE;

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
string Cache::getCacheData(string rootPath) {
    string buf = "";

    string fileName;
    for (auto it = (*blocksList).rbegin(); it != (*blocksList).rend(); ++it) {
        fileName = (*it)->getRelativePath(rootPath);
        buf += fileName + " " + to_string((*it)->getIndex()+1) + " " +
               to_string((*it)->getRefCounter()) + "\n";
    }
    return buf;
}

/*
 * Returns the relative path of the block.
 */
string Block::getRelativePath(string rootPath) {
    string relPath = path;
    relPath.pop_back();

    relPath.replace(relPath.find(rootPath), rootPath.length(), "");
    return relPath.substr(1, relPath.length()); // ignoring first '/'
}
