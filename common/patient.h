
struct Patient
{
    string ID() const
    {
        return util::removeExtensions(fileUnstim) + "_" + util::removeExtensions(fileStim);
    }
    float getOutcome() const
    {
        return math::clamp(survivalTime / 1000.0f, 0.0f, 1.0f);
    }
    vector<float> makeOutcomeVector() const
    {
        if (constants::directPrediction)
        {
            vector<float> result;
            result.push_back(getOutcome());
            return result;
        }

        vector<float> result(constants::survivalIntervals, -1.0f);
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
    vector<BYTE> linearFeatures;
    Grid3uc imageFeatures;
};

template<class BinaryDataBuffer, class BinaryDataCompressor>
inline BinaryDataStream<BinaryDataBuffer, BinaryDataCompressor>& operator<<(BinaryDataStream<BinaryDataBuffer, BinaryDataCompressor>& s, const PatientFeatureSample &p) {
    s << p.patient;
    s.writePrimitive(p.linearFeatures);
    s.writePrimitive(p.imageFeatures);
    return s;
}

template<class BinaryDataBuffer, class BinaryDataCompressor>
inline BinaryDataStream<BinaryDataBuffer, BinaryDataCompressor>& operator>>(BinaryDataStream<BinaryDataBuffer, BinaryDataCompressor>& s, PatientFeatureSample &p) {
    s >> p.patient;
    s.readPrimitive(p.linearFeatures);
    s.readPrimitive(p.imageFeatures);
    return s;
}