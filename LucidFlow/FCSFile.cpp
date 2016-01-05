
#include "main.h"

int FCSClustering::getClusterIndex(const MathVectorf &sample) const
{
    return c.quantizeToNearestClusterIndex(sample);
}

vec4uc FCSClustering::getClusterColor(const MathVectorf &sample) const
{
    UINT clusterIndex = c.quantizeToNearestClusterIndex(sample);
    return colors[clusterIndex];
}

void FCSClustering::go(const FCSFile &file, int clusterCount)
{
    const int maxIterations = 1000;
    const double maxDelta = 1e-7;

    MLIB_ASSERT_STR(file.transformedSamples.size() > 0, "Samples not transformed");
    c.cluster(file.transformedSamples, clusterCount, maxIterations, true, maxDelta);

    colors.resize(clusterCount);

    for (int i = 0; i < clusterCount; i++)
    {
        vec3f randomColor;
        do {
            auto r = []() { return (float)util::randomUniform(); };
            randomColor = vec3f(r(), r(), r());
        } while (randomColor.length() <= 0.8f);
        randomColor *= 255.0f;
        colors[i] = vec4uc(util::boundToByte(randomColor.r), util::boundToByte(randomColor.g), util::boundToByte(randomColor.b), 255);
    }
}

void FCSFile::loadASCII(const string &filename, int maxDim)
{
    auto lines = util::getFileLines(filename, 3);
    
    fieldNames = util::split(util::remove(lines[0], "\""), ',');
    //util::pop_front(fieldNames);
    const int expectedDim = (int)fieldNames.size();

    if (maxDim != -1 && fieldNames.size() > maxDim)
    {
        cout << "Truncating to " << maxDim << " dimensions" << endl;
        fieldNames.resize(maxDim);
    }

    dim = (int)fieldNames.size();
    sampleCount = (int)lines.size() - 1;

    cout << "Loading " << filename << " dim=" << dim << " samples=" << sampleCount << endl;
    for (const string &s : fieldNames)
        cout << s << endl;

    data.allocate(sampleCount, dim);
    for (size_t i = 1; i < lines.size(); i++)
    {
        const string &line = lines[i];
        auto parts = util::split(line, ',');
        util::pop_front(parts);
        if (parts.size() != expectedDim)
        {
            cout << "Invalid line: " << line << endl;
            return;
        }
        for (int j = 0; j < dim; j++)
        {
            data(i - 1, j) = convert::toFloat(parts[j]);
        }
    }
}

void FCSFile::saveBinary(const string &filename)
{
    BinaryDataStreamFile out(filename, true);
    out << dim << sampleCount << fieldNames;
    out.writePrimitiveGrid(data);
    out.closeStream();
}

void FCSFile::loadBinary(const string &filename)
{
    BinaryDataStreamFile in(filename, false);
    in >> dim >> sampleCount >> fieldNames;
    in.readPrimitiveGrid(data);
    in.closeStream();
}

void FCSFile::compensateSamples(const string &infoFilename)
{
    SpilloverMatrix spillover;
    if (!FCSUtil::readSpilloverMatrix(infoFilename, spillover))
    {
        cout << "No spillover matrix found" << endl;
        return;
    }
    DenseMatrixd compensationMatrix = spillover.m.inverse();
    
    map<string, int> fieldToSampleIndex;
    for (int i = 0; i < fieldNames.size(); i++)
        fieldToSampleIndex[fieldNames[i]] = i;

    for (int i = 0; i < spillover.dim; i++)
    {
        if (fieldToSampleIndex.count(spillover.header[i]) == 0)
        {
            cout << "Spillover field not found: " << spillover.header[i] << endl;
            return;
        }
    }

    for (int sampleIndex = 0; sampleIndex < sampleCount; sampleIndex++)
    {
        MathVectord v(spillover.dim);
        for (int i = 0; i < spillover.dim; i++)
        {
            const int sampleDim = fieldToSampleIndex[spillover.header[i]];
            v[i] = data(sampleIndex, sampleDim);
        }

        v = compensationMatrix * v;

        for (int i = 0; i < spillover.dim; i++)
        {
            const int sampleDim = fieldToSampleIndex[spillover.header[i]];
            data(sampleIndex, sampleDim) = v[i];
        }
    }
}
