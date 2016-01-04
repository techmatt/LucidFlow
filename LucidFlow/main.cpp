
#include "main.h"

void go()
{
    FCSFile file;
    //file.loadASCII(R"(C:\Users\mdfisher\Downloads\FCSDump.txt)", 12);
    //file.saveBinary("dump.dat");
    file.loadBinary("dump.dat");

    FCSProcessor processor;
    processor.makeTransforms(file);
    processor.makeClustering(file, 20);

    util::makeDirectory("viz");
    int axisA = 3;
    int axisB = 4;

    QuartileRemap params;
    for (int cluster = 0; cluster < 20; cluster++)
    {
        //params.quartiles = { 0.0f, 3.0f, 6.0f, 12.0f, 20.0f, 50.0f, 200.0f, 400.0f };

        auto image = FCSVisualizer::visualizeDensity(file, processor, axisA, axisB, 128, cluster, params);
        
        LodePNG::save(image, "viz/a" + to_string(axisA) + "_b" + to_string(axisB) + "_cluster" + to_string(cluster) + ".png");
    }
    auto imageAllA = FCSVisualizer::visualizePoint(file, processor, axisA, axisB, 512);
    auto imageAllB = FCSVisualizer::visualizeDensity(file, processor, axisA, axisB, 512, -1, params);
    LodePNG::save(imageAllA, "viz/a" + to_string(axisA) + "_b" + to_string(axisB) + "_allClustersA.png");
    LodePNG::save(imageAllB, "viz/a" + to_string(axisA) + "_b" + to_string(axisB) + "_allClustersB.png");
}

void main()
{
    go();

    cout << "done!" << endl;
    cin.get();
}
