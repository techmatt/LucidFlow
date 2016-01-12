
struct FeatureDatabase
{
    struct Key
    {
        string patientID;
        int axisA;
        int axisB;
    };

    const FCSFeatures* getFeatures(const string &patientID, int axisA, int axisB) const
    {
        Key k;
        k.patientID = patientID;
        k.axisA = axisA;
        k.axisB = axisB;
        if (entries.count(k) == 0)
        {
            cout << "Entry not found: " << patientID << "-" << axisA << "," << axisB << endl;
        }
        return entries.find(k)->second;
    }

    void addEntry(const string &patientID, const FCSFeatures *features)
    {
        Key k;
        k.patientID = patientID;
        k.axisA = features->descriptions[0].axisA;
        k.axisB = features->descriptions[0].axisB;
        entries[k] = features;
    }

    map<Key, const FCSFeatures*> entries;
};

inline bool operator < (const FeatureDatabase::Key &a, const FeatureDatabase::Key &b)
{
    if (a.patientID < b.patientID) return true;
    if (a.patientID > b.patientID) return false;
    if (a.axisA < b.axisA) return true;
    if (a.axisA > b.axisA) return false;
    return a.axisB < b.axisB;
}

struct FeatureQualityDatabase
{
    struct Entry
    {
        FeatureDescription desc;
        double score;
        Grid2f informationGain;
    };

    void addEntry(const FeatureDescription &desc, const Grid2f &informationGain)
    {
        Entry *e = new Entry;
        e->desc = desc;
        
        vector<float> values;
        for (auto &p : informationGain)
            values.push_back(p.value);
        sort(values.rbegin(), values.rend());

        double score = 0.0f;
        for (int i = 0; i < constants::topKInformationGainSum; i++)
            score += values[i];
        score /= (float)constants::topKInformationGainSum;
        e->score = score;

        e->informationGain = informationGain;
        lock.lock();
        entries.push_back(e);
        lock.unlock();
    }

    void sortEntries()
    {
        auto compare = [](const Entry *a, const Entry *b) {
            return a->score > b->score;
        };
        sort(entries.begin(), entries.end(), compare);
    }

    vector<const Entry*> selectBestFeatures(int totalFeatures, int maxInstancesPerGraph) const
    {
        auto compare = [](const Entry *a, const Entry *b) {
            return a->score < b->score;
        };
        auto graphHash = [](const Entry *e) {
            return (UINT64)e->desc.axisA * 54729 + (UINT64)e->desc.axisB * 5468247 + (UINT64)e->desc.condition * 564781;
        };

        vector<const Entry*> entriesCopy = entries;
        sort(entriesCopy.begin(), entriesCopy.end(), compare);

        map<UINT64, int> graphCounts;

        vector<const Entry*> result;
        while (result.size() < totalFeatures)
        {
            const Entry *e = entriesCopy.back();
            entriesCopy.pop_back();

            const UINT64 hash = graphHash(e);
            if (graphCounts[hash] >= maxInstancesPerGraph)
            {
                cout << "Skipping " << e->desc.toString() << endl;
            }
            else
            {
                cout << "Adding " << e->desc.toString() << endl;
                result.push_back(e);
                graphCounts[hash]++;
            }
        }

        return result;
    }

    vector<const Entry*> entries;
    mutex lock;
};

struct FCSDataset
{
    struct Entry
    {
        string patientID() const
        {
            return util::removeExtensions(fileUnstim) + "_" + util::removeExtensions(fileStim);
        }
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

    void chooseAndVizFeatures(int totalFeatures, int maxInstancesPerGraph, int samplesPerFeature, const string &outDir);

    void evaluateFeatureSplits();
    SplitResult evaluateFeatureSplits(int axisA, int axisB, const string &outDir);

    string describeAxis(int axisIndex) const;
    const Entry& sampleRandomEntry(int label) const;
    Bitmap makeFeatureViz(const FCSDataset::Entry &entry, const FeatureDescription &desc) const;
    void saveFeatureDescription(const vector<const FeatureQualityDatabase::Entry*> &features, const string &filename) const;

    string baseDir;

    vector<Entry> entries;

    FeatureDatabase featureDatabase;

    FeatureQualityDatabase featureQualityDatabase;

    vector<const FeatureQualityDatabase::Entry*> selectedFeatures;

    FCSProcessor processor;
    FCSFile resampledFile;
};