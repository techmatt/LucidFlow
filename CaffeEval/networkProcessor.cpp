
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

    //patients.resize(12);
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
        newP.imageFeatures = s.imageFeatures;
        newP.linearFeatures = s.linearFeatures;
        newP.test = testState;
    }

    /*ofstream file("dumpA.txt");
    for (auto &b : patients[0].features)
    {
        file << (int)b.value << " " << (int)patients[1].features(b.x, b.y, b.z) << " ";
        if (b.x == 0)
            file << endl;
    }*/
}

int globalPatientIndex = 0;
void NetworkProcessor::evaluatePatient(PatientTableEntry &patient)
{
    if (patient.patient.getOutcome() >= 1.0f)
        return;

    LOG(ERROR) << "mv0: " << meanValues(vec3i(0, 0, 0));
    LOG(ERROR) << "mv1: " << meanValues(vec3i(0, 0, 1));
    LOG(ERROR) << "mv3: " << meanValues(vec3i(0, 0, 2));

    Grid3f inputData;
    if (constants::linearFeatures)
    {
        inputData = Grid3f(1, 1, patient.linearFeatures.size());
        for (auto &c : inputData)
        {
            const vec3i coord(c.x, c.y, c.z);
            const BYTE b = patient.linearFeatures[c.z];

            const float scale = 1.0f;
            c.value = ((float)b) * scale;
        }
    }
    else
    {
        inputData = Grid3f(patient.imageFeatures.getDimensions());
        for (auto &c : inputData)
        {
            const vec3i coord(c.x, c.y, c.z);
            const BYTE b = patient.imageFeatures(coord);

            const float scale = 1.0f;
            c.value = ((float)b - meanValues(coord)) * scale;
        }
    }

    net->ForwardFrom(0);
    CaffeUtil::saveNetToDirectory(net, "netBefore" + to_string(globalPatientIndex) + "/");
    CaffeUtil::runNetForward(net, "dataA", "dataA", inputData);
    CaffeUtil::saveNetToDirectory(net, "netAfter" + to_string(globalPatientIndex) + "/");
    
    globalPatientIndex++;

    if (globalPatientIndex == 6)
        exit(0);
    

    /*Blobf inputBlob = net->blob_by_name(blobName);

    loadGrid3IntoBlob(inputLayerData, inputBlob, 0);

    const int inputLayerIndex = getLayerIndex(net, inputLayerName);
    net->ForwardFrom(inputLayerIndex + 1);*/

    auto grid = CaffeUtil::getBlobAsGrid(net, "ip4");
    
    patient.netOutcome = CaffeUtil::gridToVector(grid);

    //file.close();
    //exit(0);
}

void NetworkProcessor::evaluateAllPatients()
{
    for (auto &p : patients)
    {
        cout << "Evaluating patient " << p.patient.index << endl;
        evaluatePatient(p);
    }
}

void NetworkProcessor::outputPatients(const string &filename) const
{
    ofstream file(filename);

    file << "index,unstim,stim,status,label,survival time,type,truth,pred";
    for (int i = 0; i < constants::survivalIntervals; i++)
        file << ",i" << i;
    file << endl;

    for (auto &p : patients)
    {
        ostringstream s;
        s << p.patient.index;
        s << "," << p.patient.fileUnstim;
        s << "," << p.patient.fileStim;
        s << "," << p.patient.status;
        s << "," << p.patient.label;
        s << "," << p.patient.survivalTime;

        file << s.str();
        for (auto &v : p.patient.makeOutcomeVector())
            file << "," << v;
        for (auto &v : p.netOutcome)
            file << "," << v;

        /*file << s.str();
        file << ",truth";
        for (auto &v : p.patient.makeOutcomeVector())
            file << "," << v;
        file << endl;

        file << s.str();
        file << ",pred";
        for (auto &v : p.netOutcome)
            file << "," << v;*/
        file << endl;
    }
}
