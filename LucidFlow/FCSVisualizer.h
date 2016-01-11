
struct FCSVisualizer
{
    static Bitmap visualizePoint(const FCSFile &file, const FCSProcessor &processor, int axisA, int axisB, int imageSize);
    static Bitmap visualizeDensity(const FCSFile &file, const FCSProcessor &processor, int axisA, int axisB, int imageSize, int clusterFilter, const QuartileRemap &params);
    static void saveAllAxesViz(const FCSFile &file, const FCSProcessor &processor, int axisA, int imageSize, const string &outDir);

    static Bitmap gridToBmp(const Grid2f &g, float scale);
};