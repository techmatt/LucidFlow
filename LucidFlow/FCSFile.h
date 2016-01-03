
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
