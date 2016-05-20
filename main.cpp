//
// Created by roee28c on 5/15/16.
//

#include <iostream>
#include <linux/limits.h>
#include <tgmath.h>

using namespace std;

static bool isDouble(char* str, double &percentage)
{
    char* end  = 0;
    percentage = strtod(str, &end);
    return (*end == '\0') && (end != str);
}


int main()
{
    double d;
    cout << isDouble("213.2", &d) << endl;

    return 0;
}


