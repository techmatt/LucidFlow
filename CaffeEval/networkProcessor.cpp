
#include "main.h"

void NetworkProcessor::init()
{
    const string baseDir = R"(D:\datasets\LucidFlow\HIV-FlowCAP4\caffe\)";
    const string meanFilename = baseDir + "databaseTrainA-mean.binaryproto";
    const string netFilename = baseDir + "FlowCAP4-net.prototxt";
    const string modelFilename = baseDir + "FlowCAP4.caffemodel";

    meanValues = CaffeUtil::gridFromBinaryProto(meanFilename);

    Netf net(new Net<float>(netFilename, caffe::TEST));
    net->CopyTrainedLayersFrom(modelFilename);
}