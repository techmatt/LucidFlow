
#include "main.h"

void NetworkProcessor::init()
{
    const string baseDir = R"(D:\datasets\LucidFlow\HIV-FlowCAP4\caffe\)";
    const string meanFilename = baseDir + "databaseTrainA-mean.binaryproto";
    const string netFilename = baseDir + "FlowCAP4-net.prototxt";
    const string modelFilename = baseDir + "FlowCAP4.caffemodel";

    meanValues = CaffeUtil::gridFromBinaryProto(meanFilename);

    net = Netf(new Net<float>(netFilename, caffe::TEST));
    net->CopyTrainedLayersFrom(modelFilename);

    addPatients(baseDir + "databaseTrainSamples.dat", 0);
    addPatients(baseDir + "databaseTestSamples.dat", 1);
}

void NetworkProcessor::addPatients(const string &patientDataFilename, int testState)
{
    vector<PatientFeatureSample> allSamples;
    util::deserializeFromFileCompressed(patientDataFilename, allSamples);
    
    cout << allSamples.size() << " patient samples loaded from " << patientDataFilename << endl;

    for (auto &s : allSamples)
    {
        patients.push_back(PatientTableEntry());
        auto &newP = patients.back();
        newP.patient = s.patient;
        newP.features = s.features;
        newP.test = testState;
    }
}

void NetworkProcessor::evaluatePatient(PatientTableEntry &patient)
{
    Grid3f inputData(patient.features.getDimensions());
    for (auto &c : inputData)
    {
        const vec3i coord(c.x, c.y, c.z);
        const BYTE b = patient.features(coord);
        c.value = ((float)b - meanValues(coord)) / 255.0f;
    }

    CaffeUtil::runNetForward(net, "conv1", "dataA", inputData);
    auto grid = CaffeUtil::getBlobAsGrid(net, "ip5");
    
    patient.netOutcome = CaffeUtil::gridToVector(grid);
}

void NetworkProcessor::evaluateAllPatients()
{
    for (auto &p : patients)
    {
        cout << "Evaluating patient " << p.patient.index << endl;
        evaluatePatient(p);
    }
}