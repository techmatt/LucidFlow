
struct FCSFeatures
{
    void create(const FCSProcessor &processor, const FCSFile &fileUnstim, const FCSFile &fileStim, int imageSize, int _axisA, int _axisB);
    void create(const FCSProcessor &processor, const FCSFile &fileUnstim, const FCSFile &fileStim, int imageSize, const vector<FeatureDescription> &_descriptions);
    void setChannel(const Bitmap &bmp, int channel);

    static QuartileRemap makeQuartiles(const FCSProcessor &processor, const FCSFile &fileUnstim, const FCSFile &fileStim, int imageSize, const vector<FeatureDescription> &descriptions);

    Bitmap getChannel(int channel) const;
    
    void saveDebugViz(const string &baseDir) const;

    vector<FeatureDescription> descriptions;
    Grid3uc features;

private:
    static Grid3i makeCounts(const FCSProcessor &processor, const FCSFile &fileUnstim, const FCSFile &fileStim, int imageSize, const vector<FeatureDescription> &descriptions);

    void finalizeFromCounts(const Grid3i &counts, const vector<const QuartileRemap*> &params);

};

template<class BinaryDataBuffer, class BinaryDataCompressor>
inline BinaryDataStream<BinaryDataBuffer, BinaryDataCompressor>& operator<<(BinaryDataStream<BinaryDataBuffer, BinaryDataCompressor>& s, const FCSFeatures &f) {
    s << f.descriptions;
    s.writePrimitive(f.features);
    return s;
}

template<class BinaryDataBuffer, class BinaryDataCompressor>
inline BinaryDataStream<BinaryDataBuffer, BinaryDataCompressor>& operator>>(BinaryDataStream<BinaryDataBuffer, BinaryDataCompressor>& s, FCSFeatures &f) {
    s >> f.descriptions;
    s.readPrimitive(f.features);
    return s;
}