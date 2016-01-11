
struct FCSProcessor
{
    void makeTransforms(const FCSFile &file);
    void makeClustering(FCSFile &file, int clusterCount, const string &clusterCacheFile);
    void makeQuartiles(FCSFile &file, const string &quartileCacheFile);

    void transform(FCSFile &file) const;
    void assignClusters(FCSFile &file) const;

    void saveFeatures(FCSFile &fileUnstim, FCSFile &fileStim, const string &outFilename) const;

    bool axesValid(const string &axisA, const string &axisB) const;

    const QuartileRemap& getQuartileRemap(const FeatureDescription &desc) const;

    vector<string> fieldNames;
    vector<FieldTransform> transforms;
    FCSClustering clustering;
    int clusterCount;

    QuartileRemap quartileSingleCluster;
    QuartileRemap quartileAllClusters;
};

struct FCSPerturbationGenerator
{
    static float avgMatchDist(const FCSClustering &clusteringA, const FCSClustering &clusteringB);
    static float avgMatchDistSymmetric(const FCSClustering &clusteringA, const FCSClustering &clusteringB);
    static float avgInterClusterDist(const FCSClustering &clustering);

    void init(const string &FCSDirectory, int maxFileCount);

    vector<FCSFile*> files;
};