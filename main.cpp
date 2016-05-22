//
// Created by roee28c on 5/15/16.
//

#include <iostream>
#include <linux/limits.h>
#include <tgmath.h>
#include <list>
#include "FBR.h"


using namespace std;

int nOldBlk = 3;
int nNewBlk = 3;


void removeBlockBFR(list<Block*> *blockList) {
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


    list<Block*> *blockList = new list();

    Block* blk1 = new Block(1,3,1,"aaa");
    Block* blk2 = new Block(2,5,2,"bbb");
    Block* blk3 = new Block(3,12,15,"ccc");
    Block* blk4 = new Block(4,3,3,"ddd");
    Block* blk5 = new Block(5,5,5,"eee");
    Block* blk6 = new Block(6,8,4,"fff");
    Block* blk7 = new Block(7,13,13,"ggg");
    Block* blk8 = new Block(8,12,13,"hhh");
    Block* blk9 = new Block(9, 2,2,"iii");

    blockList->push_front(blk1);
    blockList->push_front(blk2);
    blockList->push_front(blk3);
    blockList->push_front(blk4);
    blockList->push_front(blk5);
    blockList->push_front(blk6);
    blockList->push_front(blk7);
    blockList->push_front(blk8);
    blockList->push_front(blk9);




    return 0;
}


