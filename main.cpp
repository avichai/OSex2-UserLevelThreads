#include <fuse.h>
#include <fstream>
#include <iostream>
#include <dirent.h>
#include <string.h>
#include <unistd.h>

using namespace std;

int main() {
    int fd = open("/tmp/f1", O_RDONLY);
    if (fd < 0) {
        cerr << "fd failed" << endl;
    }


    return 0;
}