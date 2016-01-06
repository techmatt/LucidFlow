
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
    cout << "spillover dim: " << result.dim << endl;

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