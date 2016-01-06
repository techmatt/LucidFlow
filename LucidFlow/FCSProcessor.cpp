
#include "main.h"

float FieldTransform::clampedLog(float x, float offset, float scale)
{
    x += offset;
    if (x <= 1.0f)
        return 0.0f;
    return log(x) * scale;
}

FieldTransform FieldTransform::createLinear(const string &_name, const vector<float> &sortedValues)
{
    FieldTransform result;
    result.name = _name;
    result.type = Linear;
    result.minValue = sortedValues.front();
    result.maxValue = sortedValues.back();
    return result;
}

FieldTransform FieldTransform::createLog(const string &_name, const vector<float> &sortedValues)
{
    FieldTransform result;
    result.name = _name;
    result.type = Log;
    result.logScale = 1.0f;
    result.logOffset = 500.0f;

    result.minValue = clampedLog(FCSUtil::getQuartile(sortedValues, 0.02f), result.logOffset, result.logScale);
    //result.minValue = 0.0f;
    result.maxValue = clampedLog(FCSUtil::getQuartile(sortedValues, 0.98f), result.logOffset, result.logScale);

    cout << "log range: (" << result.minValue << ", " << result.maxValue << ")" << endl;

    //result.maxValue = clampedLog(getQuartile(sortedValues, 0.99f), result.logOffset, result.logScale);

    return result;
}

FieldTransform FieldTransform::createLogQuartile(const string &_name, const vector<float> &sortedValues)
{
    FieldTransform result;
    result.name = _name;
    result.type = LogQuartile;
    result.logScale = 1.0f;
    result.logOffset = 1.0f;

    float smallValue = FCSUtil::getQuartile(sortedValues, 0.01f);
    if (smallValue < 0.0f)
        result.logOffset = -smallValue;

    vector<float> logValues;
    for (float f : sortedValues)
        logValues.push_back(clampedLog(f, result.logOffset, result.logScale));

    result.quartile = QuartileRemap::makeFromValues(logValues, 2);
    return result;
}

float FieldTransform::transform(float inputValue) const
{
    if (type == Linear)
    {
        return math::linearMap(minValue, maxValue, 0.0f, 1.0f, inputValue);
    }
    if (type == Log)
    {
        float value = clampedLog(inputValue, logOffset, logScale);
        return math::linearMap(minValue, maxValue, 0.0f, 1.0f, value);
    }
    if (type == LogQuartile)
    {
        float logValue = clampedLog(inputValue, logOffset, logScale);
        return quartile.transform(logValue);
    }
    return 0.0f;
}

void FCSProcessor::makeClustering(FCSFile &file, int clusterCount)
{
    clustering.go(file, clusterCount);
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
    if (axisA == axisB)
        return false;
    if (axisA == "Time" || axisB == "Time")
        return false;
    return true;
}

void FCSProcessor::saveFeatures(FCSFile &file, const string &outDir) const
{
    util::makeDirectory(outDir);

    const int imageSize = 32;
    transform(file);
    int axisA = file.getFieldIndex("SSC-A");
    for (int axisB = 0; axisB < file.dim; axisB++)
    {
        if (axesValid(file.fieldNames[axisA], file.fieldNames[axisB]))
        {
            const string featureFilename = outDir + to_string(axisA) + "_" + to_string(axisB) + ".dat";
            if (!util::fileExists(featureFilename))
            {
                cout << "Saving " << featureFilename << endl;
                FCSFeatures features;
                QuartileRemap quartile;
                features.create(file, *this, axisA, axisB, imageSize, quartile);
                features.save(featureFilename);
                features.saveDebugViz(outDir);
            }
        }
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
