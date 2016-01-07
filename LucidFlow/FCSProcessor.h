

//! maps an input value to [0, 1]
struct FieldTransform
{
    enum Type
    {
        Linear,
        Log,
        LogQuartile,
    };

    static FieldTransform createLinear(const string &_name, const vector<float> &sortedValues);
    static FieldTransform createLog(const string &_name, const vector<float> &sortedValues);
    static FieldTransform createLogQuartile(const string &_name, const vector<float> &sortedValues);

    float transform(float inputValue) const;

    Type type;

    float minValue;
    float maxValue;

    float logOffset;
    float logScale;

    QuartileRemap quartile;

    string name;

private:
    static float clampedLog(float x, float offset, float scale);
};

struct FCSProcessor
{
    void makeTransforms(const FCSFile &file);
    void makeClustering(FCSFile &file, int clusterCount, const string &clusterCacheFile);

    void transform(FCSFile &file) const;

    bool featureFilesMissing(const FCSFile &sampleFile, const string &outDir) const;
    void saveFeatures(FCSFile &file, const string &outDir) const;

    bool axesValid(const string &axisA, const string &axisB) const;


    vector<FieldTransform> transforms;
    FCSClustering clustering;
    vector<vec4uc> clusterColors;
};

struct FCSPerturbationGenerator
{
    static float avgMatchDist(const FCSClustering &clusteringA, const FCSClustering &clusteringB);
    static float avgMatchDistSymmetric(const FCSClustering &clusteringA, const FCSClustering &clusteringB);
    static float avgInterClusterDist(const FCSClustering &clustering);

    void init(const string &FCSDirectory, int maxFileCount);

    vector<FCSFile*> files;
};