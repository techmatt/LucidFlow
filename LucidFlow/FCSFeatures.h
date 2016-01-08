
struct FCSFeatures
{
    void create(const FCSFile &file, const FCSProcessor &processor, int _axisA, int _axisB, int imageSize, const QuartileRemap &params);
    void setChannel(const Bitmap &bmp, int channel);
    
    Bitmap getChannel(int channel) const;
    
    void save(const string &filename) const;
    void saveDebugViz(const string &baseDir) const;
    void load(const string &filename);

    string fcsID;
    int axisA, axisB;
    Grid3uc features;
    vector<int> clusterHits;
};

template<class BinaryDataBuffer, class BinaryDataCompressor>
inline BinaryDataStream<BinaryDataBuffer, BinaryDataCompressor>& operator<<(BinaryDataStream<BinaryDataBuffer, BinaryDataCompressor>& s, const FCSFeatures &f) {
    s << f.fcsID << f.axisA << f.axisB << f.clusterHits;
    s.writePrimitive(f.features);
    return s;
}

template<class BinaryDataBuffer, class BinaryDataCompressor>
inline BinaryDataStream<BinaryDataBuffer, BinaryDataCompressor>& operator>>(BinaryDataStream<BinaryDataBuffer, BinaryDataCompressor>& s, FCSFeatures &f) {
    s >> f.fcsID >> f.axisA >> f.axisB >> f.clusterHits;
    s.readPrimitive(f.features);
    return s;
}