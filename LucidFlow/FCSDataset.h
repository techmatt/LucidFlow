
struct FCSFeatureDatabase
{
    struct Key
    {
        string fcsID;
        int axisA;
        int axisB;
    };

    const FCSFeatures& getFeatures(const string &fcsID, int axisA, int axisB) const
    {
        Key k;
        k.fcsID = fcsID;
        k.axisA = axisA;
        k.axisB = axisB;
        if (entries.count(k) == 0)
        {
            cout << "Entry not found: " << fcsID << "-" << axisA << "," << axisB << endl;
        }
        return *(entries.find(k)->second);
    }

    void addEntry(const FCSFeatures *features)
    {
        Key k;
        k.fcsID = features->fcsID;
        k.axisA = features->axisA;
        k.axisB = features->axisB;
        entries[k] = features;
    }

    map<Key, const FCSFeatures*> entries;
};

inline bool operator < (const FCSFeatureDatabase::Key &a, const FCSFeatureDatabase::Key &b)
{
    if (a.fcsID < b.fcsID) return true;
    if (a.fcsID > b.fcsID) return false;
    if (a.axisA < b.axisA) return true;
    if (a.axisA > b.axisA) return false;
    return a.axisB < b.axisB;
}

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
    SplitResult evaluateFeatureSplits(int axisA, int axisB, const string &outDir);

    string baseDir;

    vector<Entry> entries;

    FCSFeatureDatabase featureDatabase;

    FCSProcessor processor;
    FCSFile resampledFile;
};