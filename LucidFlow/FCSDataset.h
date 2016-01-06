
struct FCSDataset
{
    struct Entry
    {
        string fileStim, fileUnstim;
        int status;
        int survivalTime;
    };

    void init(const string &_baseDir);
    void loadLabels(const string &filename);
    void createBinaryFiles();

    void makeResampledFile();

    void initProcessor(int clusterCount);

    void makeFeatures();

    string baseDir;

    vector<Entry> entries;

    FCSProcessor processor;
};