
struct LevelDBExporter
{
    static void exportDB(const FCSDataset &dataset, const string &outDir, int epochs, int evalSamplesPerPatient, int startPatient, int endPatient);
};
