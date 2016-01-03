
//! maps an input value to [0, 1]
struct FieldTransform
{
    enum Type
    {
        Linear,
        Quartile,
    };

    static FieldTransform createLinear(const string &_name, const vector<float> &sortedValues);

    float transform(float inputValue) const;

    Type type;

    float minValue;
    float maxValue;

    string name;
};

struct FCSProcessor
{
    void transform(FCSFile &file);
    void makeResampledFile(const vector<string> &fileList, int samplesPerFile, const string &filenameOut);
    void makeTransforms(const FCSFile &file);
    void makeClustering(FCSFile &file, int clusterCount);

    vec4uc getClusterColor(const MathVectorf &v) const;

    vector<FieldTransform> transforms;
    FCSClustering clustering;
    vector<vec4uc> clusterColors;
};