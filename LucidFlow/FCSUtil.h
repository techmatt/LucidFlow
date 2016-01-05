
struct SpilloverMatrix
{
    int dim;
    DenseMatrixd m;
    vector<string> header;
};

struct FCSUtil
{
    static bool readSpilloverMatrix(const string &filename, SpilloverMatrix &result);
};