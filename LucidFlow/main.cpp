
#include "main.h"

void main()
{
    FCSFile file;
    file.loadASCII(R"(C:\Users\mdfisher\Downloads\FCSDump.txt)");
    file.saveBinary("dump.dat");

    cout << "done!" << endl;
    cin.get();
}