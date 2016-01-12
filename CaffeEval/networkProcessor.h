
struct PatientTableEntry
{
    Patient patient;
    Grid3uc features;
    int test;

    vector<float> netOutcome;
};

class NetworkProcessor
{
public:
    void init();

private:
    void addPatients(const string &patientDataFilename, int testState);
    void evaluatePatient(PatientTableEntry &patient);

    Netf net;
    Grid3f meanValues;

    vector<PatientTableEntry> patients;
};
