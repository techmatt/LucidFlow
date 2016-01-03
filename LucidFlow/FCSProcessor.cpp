
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

void FCSProcessor::makeClustering(FCSFile &file, int clusterCount)
{
    const int maxIterations = 1000;
    const double maxDelta = 1e-7;

    transform(file);
    clustering.cluster(file.transformedSamples, clusterCount, maxIterations, true, maxDelta);

    clusterColors.resize(clusterCount);

    for (int i = 0; i < clusterCount; i++)
    {
        vec3f randomColor;
        do {
            auto r = []() { return (float)util::randomUniform(); };
            randomColor = vec3f(r(), r(), r());
        } while (randomColor.length() <= 0.8f);
        randomColor *= 255.0f;
        clusterColors[i] = vec4uc(util::boundToByte(randomColor.r), util::boundToByte(randomColor.g), util::boundToByte(randomColor.b), 255);
    }
}

vec4uc FCSProcessor::getClusterColor(const MathVectorf &sample) const
{
    UINT clusterIndex = clustering.quantizeToNearestClusterIndex(sample);
    return clusterColors[clusterIndex];
}