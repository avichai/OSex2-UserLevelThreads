#include <fuse.h>
#include <fstream>
#include <iostream>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include "FBR.h"

using namespace std;


void renameFile( BlkList* blocksList,  std::unordered_map<std::string, std::unordered_set<size_t>*>* blocksMap,
             std::string fullPath, std::string fullNewPath) {
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
            unordered_set<size_t> *set = (*it).second;
            it = blocksMap->erase(it);

            blocksMap->insert(make_pair(newPath, set));
        }
        else {
            ++it;
        }
    }
}


int main() {
    BlkList* blocksList = new BlkList();
    std::unordered_map<std::string, std::unordered_set<size_t>*>* blocksMap;


    blocksMap = new  std::unordered_map<std::string, std::unordered_set<size_t>*> ();

    blocksList->push_back(new Block("/tmp/root/f1", 1, "asdasd fvds"));
    blocksList->push_back(new Block("/tmp/root/f1", 2, "abmdfd"));
    blocksList->push_back(new Block("/tmp/root/f1", 3, "anfdhd fvddsfgdsfgs"));
    blocksList->push_back(new Block("/tmp/root/f1", 4, "assdfgsdfhdfhd fvds"));

    blocksList->push_back(new Block("/tmp/root/f2", 1, "b.,sm fvds"));
    blocksList->push_back(new Block("/tmp/root/f3", 2, "b.,sm fvds"));
    blocksList->push_back(new Block("/tmp/root/f4", 3, "b.,sm fvds"));


    std::unordered_set<size_t>* set1 = new std::unordered_set<size_t>();

    set1->insert(1);
    set1->insert(2);
    set1->insert(3);
    set1->insert(4);


    blocksMap->insert(make_pair("/tmp/root/f1",set1 ));
    blocksMap->insert(make_pair("/tmp/root/f1/aa",set1 ));
    blocksMap->insert(make_pair("/tmp/root/f3",set1 ));
    blocksMap->insert(make_pair("/tmp/root/f4",set1 ));

    for (auto it = blocksMap->begin(); it != blocksMap->end(); ++it)
    {
        cerr << (*it).first << endl;
    }

    renameFile(blocksList, blocksMap, "/tmp/root" , "/tmp/whattt/DID_IT");

    cerr << "\n\n" << endl;
    for (auto it = blocksMap->begin(); it != blocksMap->end(); ++it)
    {
        cerr << (*it).first << endl;
    }
    return 0;
}