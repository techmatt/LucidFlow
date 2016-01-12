
struct Patient
{
    string ID() const
    {
        return util::removeExtensions(fileUnstim) + "_" + util::removeExtensions(fileStim);
    }
    vector<float> makeOutcomeVector() const
    {
        vector<float> result(constants::survivalIntervals, 0.0f);
        for (int i = 0; i < result.size(); i++)
        {
            const float s = (float)i / (result.size() - 1.0f);
            const float day = s * (float)constants::survivalCutoff;
            if (survivalTime <= day)
                result[i] = 1.0f;
        }
        return result;
    }

    int index;
    string fileStim, fileUnstim;
    int status;
    int survivalTime;
    int label;
};

template<class BinaryDataBuffer, class BinaryDataCompressor>
inline BinaryDataStream<BinaryDataBuffer, BinaryDataCompressor>& operator<<(BinaryDataStream<BinaryDataBuffer, BinaryDataCompressor>& s, const Patient &p) {
    s << p.index << p.fileStim << p.fileUnstim << p.status << p.survivalTime << p.label;
    return s;
}

template<class BinaryDataBuffer, class BinaryDataCompressor>
inline BinaryDataStream<BinaryDataBuffer, BinaryDataCompressor>& operator>>(BinaryDataStream<BinaryDataBuffer, BinaryDataCompressor>& s, Patient &p) {
    s >> p.index >> p.fileStim >> p.fileUnstim >> p.status >> p.survivalTime >> p.label;
    return s;
}

struct PatientFeatureSample
{
    Patient patient;
    Grid3uc features;
};

template<class BinaryDataBuffer, class BinaryDataCompressor>
inline BinaryDataStream<BinaryDataBuffer, BinaryDataCompressor>& operator<<(BinaryDataStream<BinaryDataBuffer, BinaryDataCompressor>& s, const PatientFeatureSample &p) {
    s << p.patient;
    s.writePrimitive(p.features);
    return s;
}

template<class BinaryDataBuffer, class BinaryDataCompressor>
inline BinaryDataStream<BinaryDataBuffer, BinaryDataCompressor>& operator>>(BinaryDataStream<BinaryDataBuffer, BinaryDataCompressor>& s, PatientFeatureSample &p) {
    s >> p.patient;
    s.readPrimitive(p.features);
    return s;
}