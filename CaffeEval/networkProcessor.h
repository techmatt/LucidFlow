
struct PatientTableEntry
{
    Patient patient;
    Grid3uc imageFeatures;
    vector<BYTE> linearFeatures;
    int test;

    vector<float> netOutcome;
};

class NetworkProcessor
{
public:
    void init();
    void evaluateAllPatients();
    void outputPatients(const string &filename) const;

private:
    void addPatients(const string &patientDataFilename, int testState);
    void evaluatePatient(PatientTableEntry &patient);

    Netf net;
    Grid3f meanValues;

    vector<PatientTableEntry> patients;
};
