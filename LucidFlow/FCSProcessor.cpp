
#include "main.h"

FieldTransform FieldTransform::createLinear(const string &_name, const vector<float> &sortedValues)
{
    FieldTransform result;
    result.name = _name;
    result.type = Linear;
    result.minValue = sortedValues.front();
    result.maxValue = sortedValues.back();
    return result;
}

float FieldTransform::transform(float inputValue) const
{
    if (type == Linear)
    {
        return math::linearMap(minValue, maxValue, 0.0f, 1.0f, inputValue);
    }
    return 0.0f;
}

void FCSProcessor::transform(FCSFile &file)
{
    MLIB_ASSERT_STR(transforms.size() == file.dim, "Mismatched dimensionality");
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
}

void FCSProcessor::makeResampledFile(const vector<string> &fileList, int samplesPerFile, const string &filenameOut)
{
    FCSFile result;
    result.sampleCount = samplesPerFile * (int)fileList.size();

    for (auto &filenameIn : fileList)
    {
        FCSFile file;
        file.loadBinary(filenameIn);
        if (result.fieldNames.size() == 0)
        {
            result.dim = file.dim;
            result.fieldNames = file.fieldNames;
            result.data.allocate(result.sampleCount, result.dim);
        }
        for (int i = 0; i < samplesPerFile; i++)
        {
            const int sample = util::randomInteger(0, file.sampleCount - 1);
            for (int j = 0; j < result.dim; j++)
            {
                result.data(i, j) = file.data(sample, j);
            }
        }
    }
    result.saveBinary(filenameOut);
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
        transforms[i] = FieldTransform::createLinear(file.fieldNames[i], values);
    }
}

float FCSPerturbationGenerator::avgMatchDist(const FCSClustering &clusteringA, const FCSClustering &clusteringB)
{
    const int n = clusteringA.c.clusterCount();
    float sum = 0.0f;
    for (int clusterBIndex = 0; clusterBIndex < n; clusterBIndex++)
    {
        int clusterAIndex = clusteringA.c.quantizeToNearestClusterIndex(clusteringB.c.clusterCenter(clusterBIndex));
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
    const int n = clustering.c.clusterCount();
    auto find2ndClusterIndex = [&](int sourceClusterIndex)
    {
        float bestDist = std::numeric_limits<float>::max();
        int bestClusterIndex = -1;
        for (int clusterIndex = 0; clusterIndex < n; clusterIndex++)
        {
            if (clusterIndex == sourceClusterIndex)
                continue;
            float dist = MathVectorKMeansMetric<float>::Dist(clustering.c.clusterCenter(sourceClusterIndex), clustering.c.clusterCenter(clusterIndex));
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
}

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
