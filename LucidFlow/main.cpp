
#include "main.h"

void main()
{
    FCSFile file;
    //file.loadASCII(R"(C:\Users\mdfisher\Downloads\FCSDump.txt)");
    //file.saveBinary("dump.dat");
    file.loadBinary("dump.dat");

    FCSProcessor processor;
    processor.makeTransforms(file);
    processor.makeClustering(file, 20);
    
    util::makeDirectory("viz");
    for (int random = 0; random < 10; random++)
    {
        int axisA = 3;
        int axisB = rand() % file.dim;
        auto image = FCSVisualizer::visualize(file, processor, axisA, axisB, 512);
        LodePNG::save(image, "viz/" + to_string(axisA) + "_" + to_string(axisB) + ".png");
    }
    

    cout << "done!" << endl;
    cin.get();
}