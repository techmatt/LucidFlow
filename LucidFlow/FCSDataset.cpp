
#include "main.h"

void FCSDataset::init(const string &_baseDir)
{
    baseDir = _baseDir;
}

void FCSDataset::createBinaryFiles()
{
    const string csvDir = baseDir + "CSV/";
    for (auto &csvFile : Directory::enumerateFiles(csvDir))
    {
        const string baseFilename = util::remove(csvFile, ".csv");

        const string binaryFilename = baseDir + "DAT/" + baseFilename + ".dat";
        if (!util::fileExists(binaryFilename))
        {
            cout << "Processing " << csvFile << endl;

            FCSFile file;
            file.loadASCII(csvDir + csvFile, -1, 150000);
            file.compensateSamples(baseDir + "info/" + baseFilename + ".txt");
            file.saveBinary(binaryFilename);
        }
    }
}

void FCSDataset::loadLabels(const string &filename)
{
    auto lines = util::getFileLines(baseDir + filename, 3);
    for (int i = 1; i < lines.size(); i++)
    {
        const string &line = lines[i];
        auto parts = util::split(line, " ");
        if (parts.size() == 4)
        {
            Entry e;
            e.status = convert::toInt(parts[0]);
            e.survivalTime = convert::toInt(parts[1]);
            e.fileStim = util::remove(parts[2], ".fcs");
            e.fileUnstim = util::remove(parts[3], ".fcs");
            entries.push_back(e);
        }
    }
}

void FCSDataset::makeResampledFile()
{
    const string resampledFilename = baseDir + "sampleA.dat";
    if (util::fileExists(resampledFilename))
    {
        cout << "Resampled file already exists" << endl;
        return;
    }

    const int maxFileCount = 200;
    const int samplesPerFile = 5000;

    auto allFiles = Directory::enumerateFilesWithPath(baseDir + "DAT\\", ".dat");
    random_shuffle(allFiles.begin(), allFiles.end());
    if (allFiles.size() > maxFileCount)
        allFiles.resize(maxFileCount);
    FCSUtil::makeResampledFile(allFiles, samplesPerFile, resampledFilename);
}

void FCSDataset::initProcessor(int clusterCount)
{
    FCSFile resampledFile;
    resampledFile.loadBinary(baseDir + "sampleA.dat");

    processor.makeTransforms(resampledFile);
    processor.transform(resampledFile);
    processor.makeClustering(resampledFile, clusterCount);
}

void FCSDataset::makeFeatures()
{
    //processor.saveFeatures(file, dataset.baseDir + "features/" + );
}