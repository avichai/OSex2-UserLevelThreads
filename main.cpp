#include <iostream>
#include <linux/limits.h>
#include <tgmath.h>
#include <list>
#include "FBR.h"
#include <assert.h>

using namespace std;

int nOldBlk = 3;
int nNewBlk = 3;


void divideBlocks(unordered_map<int, unordered_set<size_t>*>* blockMap, int fd, size_t lowerIdx, size_t upperIdx,
                         IdxList &cacheHitList, IdxList &cacheMissList)
{
    cerr << "here" << endl;
    size_t size = upperIdx - lowerIdx + 1;

    //no fd in table;
    if (blockMap->find(fd) == blockMap->end())
    {
        for (size_t i = lowerIdx; i <= upperIdx; ++i)
        {
            cacheMissList.push_back(i);
        }
    }
        //fd found.
    else
    {
        for (size_t i = lowerIdx; i <= upperIdx; ++i)
        {
            cerr << "what" << endl;
            blockMap->at(fd)->find(1);

            if(blockMap->at(fd)->find(i) == blockMap->at(fd)->end())
            {
                cacheMissList.push_back(i);
            }
            else
            {

                cacheMissList.push_back(i);
            }
        }
    }
    cout << "ccc" << endl;
    assert((cacheHitList.size()+cacheMissList.size()) == size);

}

void removeBlockBFR1(list<Block*> *blockList) {
    auto it = --(blockList->end());
    Block* blkToRemove = (*it);
    for (unsigned int i = 0; i < nOldBlk; ++i)
    {
        if ((*it)->getRefCounter() < blkToRemove->getRefCounter())
        {
            blkToRemove = (*it);
        }
        --it;
    }
    cout << "blkToRemove: " << blkToRemove->getFd() << " blkToRemove->getRefCounter(): " << blkToRemove->getRefCounter() << endl;
    blockList->remove(blkToRemove);
    delete blkToRemove;

}



static bool isDouble(char* str, double &percentage)
{
    char* end  = 0;
    percentage = strtod(str, &end);
    return (*end == '\0') && (end != str);
}


int main()
{
//    double d;
//    cout << isDouble("213.2", &d) << endl;


    list<Block*> *blockList = new list<Block*>();

    Block* blk1 = new Block(1,3,"aaa");
    Block* blk2 = new Block(2,2,"bbb");
    Block* blk3 = new Block(3,15,"ccc");
    Block* blk4 = new Block(4,3,"ddd");
    Block* blk5 = new Block(5,5,"eee");
    Block* blk6 = new Block(6,4,"fff");
    Block* blk7 = new Block(7,13,"ggg");
    Block* blk8 = new Block(8,13,"hhh");
    Block* blk9 = new Block(9,2,"iii");


    blockList->push_front(blk4);
    blockList->push_front(blk1);


    blockList->push_front(blk8);
    blockList->push_front(blk5);


    blockList->push_front(blk9);
    blockList->push_front(blk3);
    blockList->push_front(blk2);

    blockList->push_front(blk7);
    blockList->push_front(blk6);

//    cout << blockList->front()->getRefCounter() << endl;
//
//    cout << blockList->size() << endl;
//
//    removeBlockBFR1(blockList);
//
//    cout << blockList->size() << endl;



    IdxList *cacheHitList, *cacheMissList = new IdxList();


    unordered_map<int, unordered_set<size_t>*>* blockMap = new unordered_map<int, unordered_set<size_t>*>();

    unordered_set<size_t>* set1 = new unordered_set<size_t>();
    unordered_set<size_t>* set2 = new unordered_set<size_t>();
    unordered_set<size_t>* set3 = new unordered_set<size_t>();
    unordered_set<size_t>* set4 = new unordered_set<size_t>();

    set1->insert(1);
    set1->insert(2);
    set1->insert(3);
    set1->insert(4);


    set2->insert(5);
    set3->insert(6);
    set4->insert(7);


    set3->insert(8);
    set3->insert(9);


    set4->insert(1);
    set4->insert(5);
    set4->insert(8);
    set4->insert(14);




    blockMap->insert(make_pair(1, set1));
    blockMap->insert(make_pair(2, set2));
    blockMap->insert(make_pair(3, set3));
    blockMap->insert(make_pair(4, set4));

    cerr << "bbb" << endl;
    divideBlocks(blockMap, 1, 1, 4, *cacheHitList, *cacheMissList);

    cerr << "aaa" << endl;

//    cout << cacheHitList->front() << endl;




    return 0;
}


