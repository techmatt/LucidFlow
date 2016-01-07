
#include "main.h"

void goA()
{
    /*const int clusterCount = 1;

    FCSFile file;
    //file.loadASCII(R"(C:\Users\mdfisher\Downloads\FCSDump.txt)", 12);
    //file.saveBinary("dump.dat");
    file.loadBinary(R"(C:\Code\LucidFlow\datasets\HIV-FlowCAP4\DAT\001.dat)");

    FCSProcessor processor;
    processor.makeTransforms(file);
    processor.transform(file);
    processor.makeClustering(file, clusterCount);

    util::makeDirectory("viz");

    FCSVisualizer::saveAllAxesViz(file, processor, 2, 256, "viz/");*/

    /*int axisA = 2;
    int axisB = 10;

    cout << "Comparing " << file.fieldNames[axisA] << " vs " << file.fieldNames[axisB] << endl;

    QuartileRemap params;
    for (int cluster = 0; cluster < clusterCount; cluster++)
    {
        //params.quartiles = { 0.0f, 3.0f, 6.0f, 12.0f, 20.0f, 50.0f, 200.0f, 400.0f };

        auto image = FCSVisualizer::visualizeDensity(file, processor, axisA, axisB, 128, cluster, params);
        
        LodePNG::save(image, "viz/a" + to_string(axisA) + "_b" + to_string(axisB) + "_cluster" + to_string(cluster) + ".png");
    }
    auto imageAllA = FCSVisualizer::visualizePoint(file, processor, axisA, axisB, 512);
    auto imageAllB = FCSVisualizer::visualizeDensity(file, processor, axisA, axisB, 512, -1, params);
    LodePNG::save(imageAllA, "viz/a" + to_string(axisA) + "_b" + to_string(axisB) + "_allClustersA.png");
    LodePNG::save(imageAllB, "viz/a" + to_string(axisA) + "_b" + to_string(axisB) + "_allClustersB.png");*/
}

void goB()
{
    const int clusterCount = 30;

    FCSDataset dataset;
    dataset.init(R"(D:\datasets\LucidFlow\HIV-FlowCAP4\)");
    dataset.loadLabels("MetaDataFull.csv");
    dataset.createBinaryFiles();

    dataset.makeResampledFile("sampleA.dat", 200, 5000);
    dataset.makeResampledFile("sampleB.dat", 2, 1000);

    dataset.initProcessor("sampleA.dat", 50);
    //dataset.initProcessor("sampleB.dat", 8);

    dataset.makeFeatures();

    util::makeDirectory(dataset.baseDir + "featureEval");

    dataset.evaluateFeatureSplits();
}

void main()
{
    goB();
    //goA();
    
    cout << "done!" << endl;
    cin.get();
}
