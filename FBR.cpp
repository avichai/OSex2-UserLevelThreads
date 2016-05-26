#include<unistd.h>
#include <cmath>
#include <string.h>
#include "FBR.h"
#include <assert.h>
#include <malloc.h>
#include <sys/stat.h>
#include <fstream>

using namespace std;


#define SUCCESS 0

#define LOG_NAME ".filesystem.log" //todo delete
#define TIME_FAILURE -1 //todo delete



//static int writeFuncToLog(string funcName)
//{
//    // openning the log file
////	cFSdata.logFile.open(cFSdata.rootDirPath +  LOG_NAME, ios::app);        // todo: add ios::app flag
//    string t = "/tmp/";
//    ofstream logFile;
//    logFile.open(t +  LOG_NAME, ios::app);        // todo: add ios::app flag
//    if (logFile.fail()) {
//        return -errno; 		// todo how to handle this exception (not like this!).
//    }
//
//
//    time_t seconds = time(NULL);
//    if (seconds == (time_t) TIME_FAILURE) {
//        return -errno;
//    }
//    logFile << seconds << " " << funcName << endl;
//
//    // closing the log file
//    logFile.close();		// todo: check if close fails
//
//    return SUCCESS;
//}




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
 *
 */
Block::~Block() {
    free(data);
}

/**
 * Cache constructor.
 */
Cache::Cache(size_t blkSize, unsigned int nOldBlks, unsigned int nNewBlks, unsigned int cacheSize) :
        blkSize(blkSize), nOldBlks(nOldBlks), nNewBlks(nNewBlks),
        cacheSize(cacheSize) {
    blocksList = new BlkList();
    blocksMap = new unordered_map<string, unordered_set<size_t> *>();
    // todo check new (how to handle if failed?)
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
int Cache::readData(char* buf, size_t start, size_t end, int fd, string path) {
    // the block indexes needed to perform the read task
    size_t lowerIdx = start / blkSize;
    //todo check what happened when end = blkSize
    size_t upperIdx = (size_t) ceil(end / blkSize) + 1; // +1 to adjust upper bound
    size_t diffIdx = upperIdx - lowerIdx;

    // assigns hit and miss block lists
    bool pathInMap = blocksMap->find(path) != blocksMap->end();
    IdxList hitList, missList;
    unordered_set<size_t>* set;
    if (pathInMap) {
        set = blocksMap->at(path);
    }
    for (size_t index = lowerIdx; index < upperIdx; ++index) {
        if (pathInMap && (set->find(index) != set->end())) {
            hitList.push_back(index);
        }
        else {
            missList.push_back(index);
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
    memcpy(buf, data + (start % blkSize), diff); // todo +1?
    free(data);

    return (int) diff;
}


/*
 *
 */
int Cache::cacheHit(IdxList hitList, string path, size_t lowerIdx, char* dataArr) {
    bool isPathInMap = false;
    if (!hitList.empty()) {
        isPathInMap = true;
        unsigned int blkPosition = 0;     // the block's position in the blocks list.
        Block* block;
        for (auto it = blocksList->begin(); it != blocksList->end();) {
            block = *it;
            ++blkPosition;
            size_t blkIndex = block->getIndex();
            if (block->getPath() == path && inList(hitList, blkIndex)) {
                cerr << "cache hit: index = " << blkIndex << endl; //todo
                char* blkData = block->getData();
                memcpy(dataArr + ((blkIndex - lowerIdx) * blkSize), blkData, blkSize); //todo check if +1 needed
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
    }

    // create set for new path in the cache
    if (!isPathInMap) {
        try {
            blocksMap->insert(make_pair(path, new unordered_set<size_t>()));
        } catch (bad_alloc) {
            return -errno;
        }
    }
    return SUCCESS;
}


/*
 *
 */
int Cache::cacheMiss(IdxList missList, int fd, string path, size_t lowerIdx, char* dataArr) {
    // cache misses
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
        memcpy(dataArr + ((blkIndex - lowerIdx) * blkSize), buffer, blkSize); //todo check if +1 needed
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


void Cache::rename(string fullPath, string fullNewPath) {

    //todo check if it is a file and break
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


std::string Cache::getCacheData() {
    string buf = "";

    for (Block* block : *blocksList) {
        buf += block->getPath() + " " + to_string(block->getIndex()) + " " +
                to_string(block->getRefCounter()) + "\n";
    }

    return buf;
}



//void Cache::divideBlocks(string path, size_t lowerIdx, size_t upperIdx,
//                         IdxList &cacheHitList, IdxList &cacheMissList)
//{
//    size_t size = upperIdx - lowerIdx;
//
//    //no fd in table;
//    if (blocksMap->find(path) == blocksMap->end()) {
//        for (size_t i = lowerIdx; i < upperIdx; ++i) {
//            cacheMissList.push_back(i);
//        }
//        return;
//    }
//    //fd found.
//    else {
//        for (size_t i = lowerIdx; i < upperIdx; ++i) {
//            if(blocksMap->at(path)->find(i) == blocksMap->at(path)->end()) {
//                cacheMissList.push_back(i);
//            }
//            else {
//                cacheMissList.push_back(i);
//            }
//        }
//    }
//
//    assert((cacheHitList.size()+cacheMissList.size()) == size);                 //todo remove assert
//}

/*
 *    bool isPathInMap = false;
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
        try {
            blocksMap->insert(make_pair(path, new unordered_set<size_t>()));
        } catch (bad_alloc) {
            return -errno;
        }
    }
    // cache misses
    for (size_t index : cacheMissList) {
        if (blocksList->size() == cacheSize) {
            removeBlockBFR();
        }
        char* buffer = (char*) aligned_alloc(blkSize, blkSize); //todo!!!!!!
        if (buffer == NULL) {
            return -errno;
        }
        if (pread(fd, buffer, blkSize, index * blkSize) < SUCCESS) {
            return -errno;
        }
        string bufferStr = buffer;
        free(buffer);
        Block* block;
        try {
            block = new Block(path, index, bufferStr);
        } catch(bad_alloc) {
            return -errno;
        }
        blocksMap->at(path)->insert(index);
        dataArr[index - lowerIdx] = bufferStr;
        blocksList->push_front(block);
    }

 */
