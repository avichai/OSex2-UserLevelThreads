#include <iostream>

using namespace std;

int main() {
    int fd = open("/tmp/f1", O_RDONLY | O_DIRECT | O_SYNC);




    return 0;
}