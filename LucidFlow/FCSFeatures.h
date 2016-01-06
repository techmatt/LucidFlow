
struct FCSFeatures
{
    void create(const FCSFile &file, const FCSProcessor &processor, int _axisA, int _axisB, int imageSize, const QuartileRemap &params);
    void setChannel(const Bitmap &bmp, int channel);
    
    Bitmap getChannel(int channel) const;
    
    void save(const string &filename) const;
    void saveDebugViz(const string &baseDir) const;
    void load(const string &filename);

    int axisA, axisB;
    Grid3uc features;
};