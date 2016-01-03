
#include "main.h"

float FieldTransform::transform(float inputValue)
{
    if (type == Linear)
    {
        return math::linearMap(minValue, maxValue, 0.0f, 1.0f, inputValue);
    }
}

void FCSFile::loadASCII(const string &filename)
{
    auto lines = util::getFileLines(filename, 3);
    
    fieldNames = util::split(lines[0], '\t');
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
        if (parts.size() != dim)
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