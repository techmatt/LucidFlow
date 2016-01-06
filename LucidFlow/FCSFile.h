
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
    void loadASCII(const string &filename, int maxDim, int maxSamples);

    void saveBinary(const string &filename);
    void loadBinary(const string &filename);

    void compensateSamples(const string &infoFilename);

    void printDataRanges();

    int getFieldIndex(const string &fieldName)
    {
        for (int i = 0; i < dim; i++)
            if (fieldNames[dim] == fieldName)
                return i;
        cout << "Field not found: " << fieldName << endl;
        return -1;
    }

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
