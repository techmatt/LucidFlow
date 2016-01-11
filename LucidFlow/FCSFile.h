
struct FCSFile;

struct FCSCluster
{
    MathVectorf center;
    vec4uc color;
};

template<class BinaryDataBuffer, class BinaryDataCompressor>
inline BinaryDataStream<BinaryDataBuffer, BinaryDataCompressor>& operator<<(BinaryDataStream<BinaryDataBuffer, BinaryDataCompressor>& s, const FCSCluster &c) {
    s << c.center << c.color;
    return s;
}

template<class BinaryDataBuffer, class BinaryDataCompressor>
inline BinaryDataStream<BinaryDataBuffer, BinaryDataCompressor>& operator>>(BinaryDataStream<BinaryDataBuffer, BinaryDataCompressor>& s, FCSCluster &c) {
    s >> c.center >> c.color;
    return s;
}

struct FCSClustering
{
    void go(const FCSFile &file, int clusterCount);
    void save(const string &filename) const;
    void load(const string &filename);

    int getClusterIndex(const MathVectorf &sample) const;
    const FCSCluster& getCluster(const MathVectorf &sample) const;

    vector<FCSCluster> clusters;
};

struct FCSFile
{
    void loadASCII(const string &filename, int maxDim, int maxSamples);

    void saveBinary(const string &filename);
    void loadBinary(const string &filename);

    void compensateSamples(const string &infoFilename);

    void printDataRanges();

    int getFieldIndex(const string &fieldName) const
    {
        for (int i = 0; i < dim; i++)
            if (fieldNames[i] == fieldName)
                return i;
        cout << "Field not found: " << fieldName << endl;
        return -1;
    }

    bool checkAndFixTransformedValues()
    {
        for (auto &i : transformedSamples)
            for (auto &j : i)
                if (j != j)
                {
                    cout << "Invalid transformed value: " << j << endl;
                    j = 0.0f;
                    //return false;
                }
        return true;
    }

    string id;
    int dim;
    int sampleCount;
    vector<string> fieldNames;
    Grid2f data;

    //
    // These are computed as needed
    //
    vector<MathVectorf> transformedSamples;
    vector<int> sampleClusterIndices;
};
