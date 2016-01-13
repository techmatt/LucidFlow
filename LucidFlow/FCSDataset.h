
struct FeatureDatabase
{
    struct Key
    {
        string ID;
        int axisA;
        int axisB;
    };

    const FCSFeatures* getFeatures(const string &ID, int axisA, int axisB) const
    {
        Key k;
        k.ID = ID;
        k.axisA = axisA;
        k.axisB = axisB;
        if (entries.count(k) == 0)
        {
            cout << "Entry not found: " << ID << "-" << axisA << "," << axisB << endl;
        }
        return entries.find(k)->second;
    }

    void addEntry(const string &ID, const FCSFeatures *features)
    {
        Key k;
        k.ID = ID;
        k.axisA = features->descriptions[0].axisA;
        k.axisB = features->descriptions[0].axisB;
        entries[k] = features;
    }

    map<Key, const FCSFeatures*> entries;
};

inline bool operator < (const FeatureDatabase::Key &a, const FeatureDatabase::Key &b)
{
    if (a.ID < b.ID) return true;
    if (a.ID > b.ID) return false;
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
        vector<vec2i> bestCoords;
    };

    void addEntry(const FeatureDescription &desc, const Grid2f &informationGain)
    {
        Entry *e = new Entry;
        e->desc = desc;
        
        vector< pair<vec2i, float> > values;
        for (auto &p : informationGain)
            values.push_back( make_pair(vec2i(p.x, p.y), p.value) );
        sort(values.begin(), values.end(), [](pair<vec2i, float> a, pair<vec2i, float> b) { return a.second > b.second; });

        double score = 0.0f;
        for (int i = 0; i < constants::topKInformationGainSum; i++)
            score += values[i].second;
        score /= (float)constants::topKInformationGainSum;
        e->score = score;

        e->informationGain = informationGain;

        for (int i = 0; i < constants::coordsPerFeature; i++)
            e->bestCoords.push_back(values[i].first);

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
    const Patient& sampleRandomPatient(int label) const;
    Bitmap makeFeatureViz(const Patient &patient, const FeatureDescription &desc) const;
    void saveFeatureDescription(const vector<const FeatureQualityDatabase::Entry*> &features, const string &filename) const;

    string baseDir;

    vector<Patient> patients;

    FeatureDatabase featureDatabase;

    FeatureQualityDatabase featureQualityDatabase;

    vector<const FeatureQualityDatabase::Entry*> selectedFeatures;

    FCSProcessor processor;
    FCSFile resampledFile;
};