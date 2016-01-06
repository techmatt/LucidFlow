
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

struct SpilloverMatrix
{
    int dim;
    DenseMatrixd m;
    vector<string> header;
};

struct FCSUtil
{
    static void makeResampledFile(const vector<string> &fileList, int samplesPerFile, const string &filenameOut);
    static string describeQuartiles(const vector<float> &sortedValues);
    static float getQuartile(const vector<float> &sortedValues, float quartile);
    static bool readSpilloverMatrix(const string &filename, SpilloverMatrix &result);
};