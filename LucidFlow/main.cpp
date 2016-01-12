
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

    dataset.makeFeatures();

    dataset.evaluateFeatureSplits();

    dataset.chooseAndVizFeatures(128, 3, 4, dataset.baseDir + "selectedFeatures/");

    const int trainingSplit = math::round(dataset.patients.size() * 0.7);
    LevelDBExporter::exportDB(dataset, dataset.baseDir + "caffe/databaseTrain", 10, 10, 0, trainingSplit);
    LevelDBExporter::exportDB(dataset, dataset.baseDir + "caffe/databaseTest", 1, 10, trainingSplit, (int)dataset.patients.size());
}

void main()
{
    goB();
    //goA();
    
    cout << "done!" << endl;
    cin.get();
}
