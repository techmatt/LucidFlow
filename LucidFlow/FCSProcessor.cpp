
#include "main.h"

void FCSProcessor::makeQuartiles(FCSFile &file, const string &quartileCacheFile)
{
    fieldNames = file.fieldNames;

    if (!util::fileExists(quartileCacheFile))
    {
        cout << "Making quartiles..." << endl;
        
        //quartileSingleCluster
        vector<FeatureDescription> descriptionsSingle, descriptionsAll;

        const int randomGraphs = 64;
        int clusterIndex = 0;

        for (int graphIndex = 0; graphIndex < randomGraphs; graphIndex++)
        {
            int axisA = 0;
            int axisB = 0;
            while (!axesValid(fieldNames[axisA], fieldNames[axisB]))
            {
                axisA = rand() % file.fieldNames.size();
                axisB = rand() % file.fieldNames.size();
            }

            descriptionsSingle.push_back(FeatureDescription(FeatureCondition::Stim, axisA, axisB, clusterIndex));
            descriptionsAll.push_back(FeatureDescription(FeatureCondition::Stim, axisA, axisB, clusterCount));

            descriptionsSingle.push_back(FeatureDescription(FeatureCondition::Unstim, axisA, axisB, clusterIndex));
            descriptionsAll.push_back(FeatureDescription(FeatureCondition::Unstim, axisA, axisB, clusterCount));

            clusterIndex = (clusterIndex + 1) % clustering.clusters.size();
        }

        quartileSingleCluster = FCSFeatures::makeQuartiles(*this, file, file, constants::imageSize, descriptionsSingle);
        quartileAllClusters = FCSFeatures::makeQuartiles(*this, file, file, constants::imageSize, descriptionsAll);

        util::serializeToFile(quartileCacheFile, quartileSingleCluster, quartileAllClusters);
    }

    cout << "Loading quartiles from " << quartileCacheFile << endl;
    util::deserializeFromFile(quartileCacheFile, quartileSingleCluster, quartileAllClusters);

    cout << "Single cluster quartiles:" << endl;
    quartileSingleCluster.print();

    cout << "All clusters quartiles:" << endl;
    quartileAllClusters.print();
}

void FCSProcessor::makeClustering(FCSFile &file, int _clusterCount, const string &clusterCacheFile)
{
    clusterCount = _clusterCount;

    file.checkAndFixTransformedValues();
    if (!util::fileExists(clusterCacheFile))
    {
        cout << "Creating " << clusterCacheFile << "..." << endl;
        clustering.go(file, clusterCount);
        clustering.save(clusterCacheFile);
    }
    cout << "Loading " << clusterCacheFile << endl;
    clustering.load(clusterCacheFile);
}

void FCSProcessor::assignClusters(FCSFile &file) const
{
    if (file.sampleClusterIndices.size() != 0 || file.transformedSamples.size() == 0 || clusterCount == 0)
    {
        cout << "Skipping cluster assignment" << endl;
        return;
    }

    file.sampleClusterIndices.resize(file.sampleCount);
    for (int i = 0; i < file.sampleCount; i++)
    {
        file.sampleClusterIndices[i] = clustering.getClusterIndex(file.transformedSamples[i]);
    }
}

void FCSProcessor::transform(FCSFile &file) const
{
    MLIB_ASSERT_STR(transforms.size() == file.dim, "Mismatched dimensionality");
    if (file.transformedSamples.size() != 0)
    {
        cout << "Skipping transform" << endl;
        return;
    }
    file.transformedSamples.resize(file.sampleCount);
    for (int i = 0; i < file.sampleCount; i++)
    {
        MathVectorf &v = file.transformedSamples[i];
        v.resize(file.dim);
        for (int j = 0; j < file.dim; j++)
        {
            v[j] = transforms[j].transform(file.data(i, j));
        }
    }

    assignClusters(file);
}

void FCSProcessor::makeTransforms(const FCSFile &file)
{
    transforms.resize(file.dim);
    for (int i = 0; i < file.dim; i++)
    {
        vector<float> values(file.sampleCount);
        for (int j = 0; j < file.sampleCount; j++)
        {
            values[j] = file.data(j, i);
        }
        sort(values.begin(), values.end());
        
        cout << file.fieldNames[i] << ": (" << values.front() << ", " << values.back() << ")" << endl;

        //transforms[i] = FieldTransform::createLinear(file.fieldNames[i], values);
        //transforms[i] = FieldTransform::createLog(file.fieldNames[i], values);
        transforms[i] = FieldTransform::createLogQuartile(file.fieldNames[i], values);
    }
}

bool FCSProcessor::axesValid(const string &axisA, const string &axisB) const
{
    if (axisA >= axisB)
        return false;

    vector<string> unusedAxes;
    unusedAxes.push_back("B710-A");
    unusedAxes.push_back("V705-A");
    unusedAxes.push_back("V655-A");
    unusedAxes.push_back("V605-A");
    //unusedAxes.push_back("V565-A"); // test axis to see how strong the feature signal is
    unusedAxes.push_back("Time");

    for (auto &s : unusedAxes)
        if (axisA == s || axisB == s)
            return false;

    return true;

    /*if (axisA == "SSC-A" && axisB == "G660-A")
        return true;
    if (axisA == "V655-A" && axisB == "B710-A")
        return true;*/
    
    return false;
}

void FCSProcessor::saveFeatures(FCSFile &fileUnstim, FCSFile &fileStim, const string &outFilename) const
{
    if (util::fileExists(outFilename))
    {
        cout << "Skipping " << outFilename << endl;
        return;
    }

    const int dim = fileUnstim.dim;
    vector<FCSFeatures> allFeatures;
    allFeatures.reserve(dim * dim * 2);

    for (int axisA = 0; axisA < dim; axisA++)
        for (int axisB = 0; axisB < dim; axisB++)
        {
            if (axesValid(fieldNames[axisA], fieldNames[axisB]))
            {
                //cout << "Generating " << axisA << "-" << axisB << endl;
                    
                allFeatures.push_back(FCSFeatures());
                FCSFeatures &features = allFeatures.back();
                features.create(*this, fileUnstim, fileStim, constants::imageSize, axisA, axisB);
            }
        }

    BinaryDataStreamZLibFile out(outFilename, true);
    out << allFeatures;
    out.closeStream();

    int a = 10;
    util::serializeToFile("store.dat", allFeatures, a);
    util::deserializeFromFile("store.dat", allFeatures, a);
}

const QuartileRemap& FCSProcessor::getQuartileRemap(const FeatureDescription &desc) const
{
    if (desc.clusterIndex == clustering.clusters.size())
        return quartileAllClusters;
    else
        return quartileSingleCluster;
}

/*float FCSPerturbationGenerator::avgMatchDist(const FCSClustering &clusteringA, const FCSClustering &clusteringB)
{
    const int n = (int)clusteringA.clusters.size();
    float sum = 0.0f;
    for (int clusterBIndex = 0; clusterBIndex < n; clusterBIndex++)
    {
        int clusterAIndex = clusteringA.c.quantizeToNearestClusterIndex(clusteringB.clusters(clusterBIndex));
        sum += sqrtf(MathVectorKMeansMetric<float>::Dist(clusteringA.c.clusterCenter(clusterAIndex), clusteringB.c.clusterCenter(clusterBIndex)));
    }
    return sum / (float)n;
}

float FCSPerturbationGenerator::avgMatchDistSymmetric(const FCSClustering &clusteringA, const FCSClustering &clusteringB)
{
    return (avgMatchDist(clusteringA, clusteringB) + avgMatchDist(clusteringB, clusteringA)) * 0.5f;
}

float FCSPerturbationGenerator::avgInterClusterDist(const FCSClustering &clustering)
{
    const int n = (int)clustering.clusters.size();
    auto find2ndClusterIndex = [&](int sourceClusterIndex)
    {
        float bestDist = std::numeric_limits<float>::max();
        int bestClusterIndex = -1;
        for (int clusterIndex = 0; clusterIndex < n; clusterIndex++)
        {
            if (clusterIndex == sourceClusterIndex)
                continue;
            float dist = MathVectorKMeansMetric<float>::Dist(clustering.getClusterColor.clusterCenter(sourceClusterIndex), clustering.c.clusterCenter(clusterIndex));
            if (dist < bestDist)
            {
                bestDist = dist;
                bestClusterIndex = clusterIndex;
            }
        }
        return bestClusterIndex;
    };

    float sum = 0.0f;
    for (int clusterIndex = 0; clusterIndex < n; clusterIndex++)
    {
        sum += find2ndClusterIndex(clusterIndex);
    }
    return sum / (float)n;
}*/

void FCSPerturbationGenerator::init(const string &FCSDirectory, int maxFileCount)
{
    auto fileList = Directory::enumerateFilesWithPath(FCSDirectory, ".dat");
    set<string> chosenFilenames;
    if (fileList.size() <= maxFileCount)
        chosenFilenames = set<string>(fileList.begin(), fileList.end());
    else
    {
        while (chosenFilenames.size() < maxFileCount)
        {
            chosenFilenames.insert(util::randomElement(fileList));
        }
    }

    for (string s : chosenFilenames)
    {
        FCSFile *newFile = new FCSFile;
        newFile->loadBinary(s);
    }
}
