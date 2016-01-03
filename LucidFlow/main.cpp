
#include "main.h"

void go()
{
    FCSFile file;
    //file.loadASCII(R"(C:\Users\mdfisher\Downloads\FCSDump.txt)", 12);
    //file.saveBinary("dump.dat");
    file.loadBinary("dump.dat");

    FCSProcessor processor;
    processor.makeTransforms(file);
    processor.makeClustering(file, 1);

    util::makeDirectory("viz");
    for (int random = 0; random < 1; random++)
    {
        int axisA = 3;
        //int axisB = rand() % file.dim;
        int axisB = 4;
        FCSVisualizer viz;
        //auto image = viz.visualizePoint(file, processor, axisA, axisB, 512);

        QuartileRemap params;
        //params.quartiles = { 0.0f, 3.0f, 6.0f, 12.0f, 20.0f, 50.0f, 200.0f, 400.0f };

        auto image = viz.visualizeDensity(file, processor, axisA, axisB, 256, -1, params);
        
        LodePNG::save(image, "viz/" + to_string(axisA) + "_" + to_string(axisB) + ".png");
    }
}

void main()
{
    go();

    cout << "done!" << endl;
    cin.get();
}
