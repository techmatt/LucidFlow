
//! maps an input value to [0, 1]
struct FieldTransform
{
    enum Type
    {
        Linear,
        Quartile,
    };

    float transform(float inputValue);

    Type type;

    float minValue;
    float maxValue;

    string name;
};

struct FCSFile
{
    void loadASCII(const string &filename);

    void saveBinary(const string &filename);
    void loadBinary(const string &filename);

    int dim;
    int sampleCount;
    vector<string> fieldNames;
    Grid2f data;

    vector< MathVectorf > transformedData;
};

struct FCSProcessor
{
    void transform(FCSFile &file);

    vector<FieldTransform> transforms;
};
