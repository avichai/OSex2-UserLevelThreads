#include<unistd.h>
#include <cmath>
#include <string.h>
#include "FBR.h"
#include <assert.h>
#include <malloc.h>


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
    blocksMap = new unordered_map<string, unordered_set<size_t>*>();
}

/**
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
 * todo
 * todo check if no need to call read
 * todo: -should ret SUCCESS? -is it possible to receive neg size and offset?
 */
int Cache::readData(char *buf, size_t size, off_t offset, int fd, string path) {
    // checks if the call is valid.     // todo maybe checked autom by pread
    off_t fileSize = lseek(fd, 0, SEEK_END);
    if (fileSize == LSEEK_FALILURE) {
        return -errno;
    }

    // read bytes from start to end
    size_t start = (size_t) offset;
    size_t end = min(start + size, (size_t) fileSize); // todo maybe + 1
    cerr << "start: " << start << endl; //todo
    cerr << "end: " << end << endl; //todo

    // the block indexes needed to perform the read task
    size_t lowerIdx = start / blkSize;
    size_t upperIdx = (size_t) ceil(end / blkSize) + 1; // +1 to adjust upper bound
    size_t diffIdx = upperIdx - lowerIdx;
    cerr << "lowerIdx: " << lowerIdx << endl; //todo
    cerr << "upperIdx: " << upperIdx << endl; //todo

    IdxList cacheHitList, cacheMissList;
    divideBlocks(path, lowerIdx, upperIdx, cacheHitList, cacheMissList);

    cerr << "hit List size: " << cacheHitList.size() << endl; //todo
    for (size_t idx: cacheHitList) {
        cerr << "  " << idx;
    }
    cerr << endl;
    cerr << "miss List size: " << cacheMissList.size() << endl; //todo
    for (size_t idx: cacheMissList) {
        cerr << "  " << idx;
    }
    cerr << endl;


    string dataArr[diffIdx];
    bool isPathInMap = false;
    // cache hits
    if (!cacheHitList.empty()) {
        isPathInMap = true;
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

    // create set for path
    if (!isPathInMap) {
        cerr << "insert new path: " << path << endl; //todo
        try {
            blocksMap->insert(make_pair(path, new unordered_set<size_t>()));
        } catch (bad_alloc) {
            return -errno;
        }
    }
    // cache misses
    for (size_t index : cacheMissList) {
        if (blocksList->size() == cacheSize) {
            cerr << "removing block" << endl; //todo
            removeBlockBFR();
        }
        char* buffer = (char*) aligned_alloc(blkSize, blkSize); //todo!!!!!!
        if (buffer == NULL) {
            return -errno;
        }
        if (pread(fd, buffer, blkSize, index * blkSize) < SUCCESS) {
            return -errno;
        }
        cerr << "reads buffer: " << buffer << endl; // todo!!!!!!
        string bufferStr = buffer;
        free(buffer);
        Block* block;
        try {
            block = new Block(path, index, bufferStr);
            cerr << "new block - " << "path: " << path << " index: " << index << " buf: " << bufferStr <<endl; //todo
        } catch(bad_alloc) {
            return -errno;
        }
        blocksMap->at(path)->insert(index);
        dataArr[index - lowerIdx] = bufferStr;
        blocksList->push_front(block);
    }

    // copying the requested data to the buffer
    string data = "";
    for (string blkData : dataArr) {
        data += blkData;
    }
    data.substr(start % blkSize, end % blkSize); //todo check if correct
    strcpy(buf, data.c_str());
    return (int) strlen(buf);
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

void Cache::divideBlocks(string path, size_t lowerIdx, size_t upperIdx,
                         IdxList &cacheHitList, IdxList &cacheMissList)
{
    size_t size = upperIdx - lowerIdx;

    //no fd in table;
    if (blocksMap->find(path) == blocksMap->end()) {
        for (size_t i = lowerIdx; i < upperIdx; ++i) {
            cacheMissList.push_back(i);
        }
        return;
    }
    //fd found.
    else {
        for (size_t i = lowerIdx; i < upperIdx; ++i) {
            if(blocksMap->at(path)->find(i) == blocksMap->at(path)->end()) {
                cacheMissList.push_back(i);
            }
            else {
                cacheMissList.push_back(i);
            }
        }
    }
    assert((cacheHitList.size()+cacheMissList.size()) == size);                 //todo remove assert
}

void Cache::rename(std::string fullPath, std::string fullNewPath) {


}

std::string Cache::getCacheDatat() {
    string buf = "";

    for (Block* block : *blocksList) {
        buf += block->getPath() + " " + to_string(block->getIndex()) + " " +
                to_string(block->getRefCounter()) + "\n";
    }

    return buf;

}
