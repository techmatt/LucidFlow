
enum class FeatureCondition
{
    Stim,
    Unstim,
    Count
};

struct FeatureDescription : BinaryDataSerialize<FeatureDescription>
{
    FeatureDescription() {}
    FeatureDescription(FeatureCondition _condition, int _axisA, int _axisB, int _clusterIndex)
    {
        condition = _condition;
        axisA = _axisA;
        axisB = _axisB;
        clusterIndex = _clusterIndex;
    }
    string toString() const
    {
        return "cond" + to_string((int)condition) + "_" + to_string(axisA) + "_" + to_string(axisB) + "_c" + to_string(clusterIndex);
    }
    FeatureCondition condition;
    int axisA;
    int axisB;
    int clusterIndex;
};

struct QuartileRemap
{
    static QuartileRemap makeFromValues(const vector<float> &values, int quartileCount)
    {
        QuartileRemap result;
        result.quartiles.resize(quartileCount);
        vector<float> sortedValues = values;

        sort(sortedValues.begin(), sortedValues.end());
        result.quartiles[0] = sortedValues[0];
        result.quartiles[quartileCount - 1] = sortedValues.back();

        const int span = (int)values.size() / quartileCount;
        for (int i = 1; i < quartileCount - 1; i++)
        {
            result.quartiles[i] = values[i * span];
        }
        result.print();

        return result;
    }
    void print() const
    {
        cout << "Quartiles:";
        for (int i = 0; i < quartiles.size(); i++)
            cout << " " << quartiles[i];
        cout << endl;
    }
    float transform(float value) const
    {
        if (value <= quartiles[0]) return 0.0f;
        if (value >= quartiles.back()) return 1.0f;

        int highQuartile = 1;
        while (highQuartile != quartiles.size() - 1 && value > quartiles[highQuartile])
        {
            highQuartile++;
        }
        int lowQuartile = highQuartile - 1;

        const float span = 1.0f / (float)(quartiles.size() - 1);
        return math::linearMap(quartiles[lowQuartile], quartiles[highQuartile], lowQuartile * span, highQuartile * span, value);
    }

    vector<float> quartiles;
};

template<class BinaryDataBuffer, class BinaryDataCompressor>
inline BinaryDataStream<BinaryDataBuffer, BinaryDataCompressor>& operator<<(BinaryDataStream<BinaryDataBuffer, BinaryDataCompressor>& s, const QuartileRemap &c) {
    s << c.quartiles;
    return s;
}

template<class BinaryDataBuffer, class BinaryDataCompressor>
inline BinaryDataStream<BinaryDataBuffer, BinaryDataCompressor>& operator>>(BinaryDataStream<BinaryDataBuffer, BinaryDataCompressor>& s, QuartileRemap &c) {
    s >> c.quartiles;
    return s;
}


struct SpilloverMatrix
{
    int dim;
    DenseMatrixd m;
    vector<string> header;
};

struct SplitEntry
{
    SplitEntry() {}
    SplitEntry(int _splitValue, int _state)
    {
        splitValue = _splitValue;
        state = _state;
    }
    int splitValue;
    int state;
};

inline bool operator < (const SplitEntry &a, const SplitEntry &b)
{
    return a.splitValue < b.splitValue;
}

struct SplitResult
{
    SplitResult()
    {
        splitValue = -666;
        informationGain = 0.0;
    }
    int splitValue;
    double informationGain;
};

//! maps an input value to [0, 1]
struct FieldTransform
{
    enum class Type
    {
        Linear,
        Log,
        LogQuartile,
    };

    static FieldTransform createLinear(const string &_name, const vector<float> &sortedValues);
    static FieldTransform createLog(const string &_name, const vector<float> &sortedValues);
    static FieldTransform createLogQuartile(const string &_name, const vector<float> &sortedValues);

    float transform(float inputValue) const;

    Type type;

    float minValue;
    float maxValue;

    float logOffset;
    float logScale;

    QuartileRemap quartile;

    string name;

private:
    static float clampedLog(float x, float offset, float scale);
};

struct FCSUtil
{

    static void makeResampledFile(const vector<string> &fileList, int samplesPerFile, const string &filenameOut);
    static string describeQuartiles(const vector<float> &sortedValues);
    static float getQuartile(const vector<float> &sortedValues, float quartile);
    static bool readSpilloverMatrix(const string &filename, SpilloverMatrix &result);
    static SplitResult findBestSplit(const vector<SplitEntry> &sortedEntries);

    static double entropy(int aCount, int bCount);
};
