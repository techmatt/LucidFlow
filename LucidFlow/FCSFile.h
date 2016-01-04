
struct FCSFile;
struct FCSClustering
{
    void go(const FCSFile &file, int clusterCount);

    int getClusterIndex(const MathVectorf &sample) const;
    vec4uc getClusterColor(const MathVectorf &sample) const;

    KMeansClustering < MathVectorf, MathVectorKMeansMetric<float> > c;
    vector<vec4uc> colors;
};

struct FCSFile
{
    void loadASCII(const string &filename, int maxDim);

    void saveBinary(const string &filename);
    void loadBinary(const string &filename);

    int dim;
    int sampleCount;
    vector<string> fieldNames;
    Grid2f data;

    //
    // These are computed as needed
    //
    vector< MathVectorf > transformedSamples;
    FCSClustering localClustering;
};
