
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

void FCSProcessor::makeClustering(FCSFile &file, int clusterCount, const string &clusterCacheFile)
{
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

void FCSProcessor::saveFeatures(FCSFile &file, const string &outFilename) const
{
    if (util::fileExists(outFilename))
    {
        cout << "Skipping " << outFilename << endl;
        return;
    }

    vector<FCSFeatures> allFeatures;
    allFeatures.reserve(file.dim * file.dim);

    const int imageSize = 32;
    transform(file);
    for (int axisA = 0; axisA < file.dim; axisA++)
        for (int axisB = 0; axisB < file.dim; axisB++)
        {
            if (axesValid(file.fieldNames[axisA], file.fieldNames[axisB]))
            {
                //cout << "Generating " << axisA << "-" << axisB << endl;
                    
                allFeatures.push_back(FCSFeatures());
                FCSFeatures &features = allFeatures.back();
                QuartileRemap quartile;
                features.create(file, *this, axisA, axisB, imageSize, quartile);
                //features.save(featureFilename);
                //features.saveDebugViz(outDir);

                /*vector<int> clusterHits(clustering.clusters.size(), 0);
                for (auto &sample : file.transformedSamples)
                    clusterHits[clustering.getClusterIndex(sample)]++;
                ofstream clusterHitsFile(outDir + "allClusterHits.csv");
                clusterHitsFile << "cluster,hits" << endl;
                for (int clusterIndex = 0; clusterIndex < clusterHits.size(); clusterIndex++)
                    clusterHitsFile << clusterIndex << "," << clusterHits[clusterIndex] << endl;*/
            }
        }

    //BinaryDataStreamFile out(outFilename, true);
    BinaryDataStreamZLibFile out(outFilename, true);
    out << allFeatures;
    out.closeStream();
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
