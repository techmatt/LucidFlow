
#include "main.h"

void goB()
{
    FCSDataset dataset;
    dataset.init(R"(D:\datasets\LucidFlow\HIV-FlowCAP4\)");
    dataset.loadLabels("MetaDataFull.csv");
    dataset.createBinaryFiles();

    dataset.makeResampledFile("sampleA.dat", 200, 5000);
    dataset.makeResampledFile("sampleB.dat", 2, 1000);

    dataset.initProcessor("sampleA.dat", constants::clusterCount);
    //dataset.initProcessor("sampleB.dat", 8);

    /*dataset.makeFeatures();

    dataset.evaluateFeatureSplits();

    dataset.chooseAndVizFeatures(128, 2, 6, dataset.baseDir + "selectedFeatures/");*/
}

void main()
{
    goB();
    //goA();
    
    cout << "done!" << endl;
    cin.get();
}
