
#include "main.h"

void FCSFile::loadASCII(const string &filename, int maxDim)
{
    auto lines = util::getFileLines(filename, 3);
    
    fieldNames = util::split(lines[0], '\t');
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
        cout << "  " << s << endl;

    data.allocate(sampleCount, dim);
    for (size_t i = 1; i < lines.size(); i++)
    {
        const string &line = lines[i];
        auto parts = util::split(line, '\t');
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