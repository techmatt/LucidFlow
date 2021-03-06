
#include "main.h"

string FCSUtil::describeQuartiles(const vector<float> &sortedValues)
{
    ostringstream ss;
    ss << "(" << sortedValues.front() << ", ";
    ss << getQuartile(sortedValues, 0.02f) << ", ";
    ss << getQuartile(sortedValues, 0.5f) << ", ";
    ss << getQuartile(sortedValues, 0.98f) << ", ";
    ss << sortedValues.back() << ")";
    return ss.str();
}

float FCSUtil::getQuartile(const vector<float> &sortedValues, float quartile)
{
    const int i = math::clamp(math::round(quartile * (sortedValues.size() - 1.0f)), 0, (int)sortedValues.size() - 1);
    return sortedValues[i];
}

bool FCSUtil::readSpilloverMatrix(const string &filename, SpilloverMatrix &result)
{
    if (!util::fileExists(filename))
    {
        cout << "File not found: " << filename << endl;
        return false;
    }
    auto lines = util::getFileLines(filename);

    //"","SPILL.B515.A","SPILL.G560.A","SPILL.G610.A","SPILL.G660.A","SPILL.G710.A","SPILL.G780.A","SPILL.R660.A","SPILL.R710.A","SPILL.R780.A","SPILL.V450.A","SPILL.V545.A","SPILL.V585.A","SPILL.V800.A"
    //"1", 1, 0.003153, 0.001001, 0, 0, 0, 0, 0, 0, 0, 0.002917, 0.0005838, 0

    auto fixedHeader = util::remove(util::remove(lines[0], "\""), "SPILL.");
    fixedHeader = util::replace(fixedHeader, ".", "-");
    result.header = util::split(fixedHeader, ",");

    result.dim = (int)result.header.size();
    //cout << "spillover dim: " << result.dim << endl;

    result.m.resize(result.dim, result.dim);
    int valueIndex = 0;
    for (int i = 0; i < result.dim; i++)
        for (int j = 0; j < result.dim; j++)
        {
            const string sval = util::split(lines[i + 1], ",")[j + 1];
            double dval = convert::toDouble(sval);
            result.m(i, j) = dval;
        }
    return true;
}

void FCSUtil::makeResampledFile(const vector<string> &fileList, int samplesPerFile, const string &filenameOut)
{
    FCSFile result;
    result.sampleCount = samplesPerFile * (int)fileList.size();

    int totalSampleIndex = 0;
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
                result.data(totalSampleIndex, j) = file.data(sample, j);
            }
            totalSampleIndex++;
        }
    }

    cout << "Saved " << filenameOut << ", total samples: " << totalSampleIndex << endl;
    result.saveBinary(filenameOut);
}

double FCSUtil::entropy(int aCount, int bCount)
{
    if (aCount == 0 || bCount == 0) return 0.0;
    const double pA = (double)aCount / double(aCount + bCount);
    const double pB = 1.0 - pA;
    return -(pA * log(pA) + pB * log(pB));
}

SplitResult FCSUtil::findBestSplit(const vector<SplitEntry> &sortedEntries)
{
    SplitResult result;
    result.splitValue = -1;
    result.informationGain = 0;

    int aCount = 0;
    set<int> splitPoints;
    for (auto &e : sortedEntries)
    {
        splitPoints.insert(e.splitValue);
        if (e.state == 0) aCount++;
    }
    const int bCount = (int)sortedEntries.size() - aCount;

    const double entropyBefore = entropy(aCount, bCount);

    for (int splitValue : splitPoints)
    {
        int leftACount = 0, leftBCount = 0;
        int rightACount = 0, rightBCount = 0;
        for (auto &e : sortedEntries)
        {
            if (e.splitValue <= splitValue)
            {
                if (e.state == 0) leftACount++;
                else leftBCount++;
            }
            else
            {
                if (e.state == 0) rightACount++;
                else rightBCount++;
            }
        }

        const int leftCount = leftACount + leftBCount;
        const int rightCount = rightACount + rightBCount;
        const double leftEntropy = entropy(leftACount, leftBCount);
        const double rightEntropy = entropy(rightACount, rightBCount);

        const double entropyAfter = leftEntropy * (double)leftCount / (double)sortedEntries.size() +
                                    rightEntropy * (double)rightCount / (double)sortedEntries.size();
        const double informationGain = entropyBefore - entropyAfter;
        if (informationGain > result.informationGain)
        {
            result.informationGain = informationGain;
            result.splitValue = splitValue;
        }
    }
    return result;
}

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
    result.type = Type::Linear;
    result.minValue = sortedValues.front();
    result.maxValue = sortedValues.back();
    return result;
}

FieldTransform FieldTransform::createLog(const string &_name, const vector<float> &sortedValues)
{
    FieldTransform result;
    result.name = _name;
    result.type = Type::Log;
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
    result.type = Type::LogQuartile;
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
    if (type == Type::Linear)
    {
        return math::linearMap(minValue, maxValue, 0.0f, 1.0f, inputValue);
    }
    if (type == Type::Log)
    {
        float value = clampedLog(inputValue, logOffset, logScale);
        return math::linearMap(minValue, maxValue, 0.0f, 1.0f, value);
    }
    if (type == Type::LogQuartile)
    {
        float logValue = clampedLog(inputValue, logOffset, logScale);
        return quartile.transform(logValue);
    }
    return 0.0f;
}