//
// Created by roee28c on 5/15/16.
//

#include <iostream>
#include <linux/limits.h>
#include <tgmath.h>

using namespace std;

struct A {int a; double d;};

int main()
{

    A a;
    a = {.a = 3, .d = 34.3};

    cout << a.a <<" " <<a.d << endl;
    cout << "Usage: CachingFileSystem rootdir mountdir numberOf"
                    "Blocks fOld fNew" << endl;

    return 0;
}
