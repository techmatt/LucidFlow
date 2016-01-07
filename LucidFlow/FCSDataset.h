
struct FCSDataset
{
    struct Entry
    {
        string fileStim, fileUnstim;
        int status;
        int survivalTime;
        int label;
    };

    void init(const string &_baseDir);
    void loadLabels(const string &filename);
    void createBinaryFiles();

    void makeResampledFile(const string &resampledFileID, const int maxFileCount, const int samplesPerFile);

    void initProcessor(const string &resampledFileID, int clusterCount);

    void makeFeatures();

    void evaluateFeatureSplits();
    void evaluateFeatureSplits(int axisA, int axisB, const string &outDir);

    string baseDir;

    vector<Entry> entries;

    FCSProcessor processor;
    FCSFile resampledFile;
};