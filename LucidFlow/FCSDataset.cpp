
#include "main.h"

void FCSDataset::init(const string &_baseDir)
{
    baseDir = _baseDir;
}

void FCSDataset::createBinaryFiles()
{
    const string csvDir = baseDir + "CSV/";

    auto files = Directory::enumerateFiles(csvDir);
#pragma omp parallel for schedule(dynamic,1) num_threads(7)
    for (int fileIndex = 0; fileIndex < files.size(); fileIndex++)
    {
        auto &csvFile = files[fileIndex];
        //if (csvFile != "001.csv")
        //    continue;
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
    // label is "does the patient die within 1000 days?"
    const int survivalCutoff = 1000;

    int rejectedCount = 0;
    auto lines = util::getFileLines(baseDir + filename, 3);
    for (int i = 1; i < lines.size(); i++)
    {
        const string &line = lines[i];
        auto parts = util::split(line, ",");
        if (parts.size() == 4)
        {
            Entry e;
            e.status = convert::toInt(parts[0]);
            e.survivalTime = convert::toInt(parts[1]);
            e.fileStim = util::remove(parts[2], ".fcs");
            e.fileUnstim = util::remove(parts[3], ".fcs");

            bool acceptable = true;

            if (e.status == 0 && e.survivalTime < survivalCutoff)
                acceptable = false;

            if (acceptable)
            {
                e.label = 0;

                if (e.status == 1 && e.survivalTime < survivalCutoff)
                    e.label = 1;

                entries.push_back(e);
            }
            else
                rejectedCount++;
        }
    }
    cout << entries.size() << " entries loaded from " << filename << " (" << rejectedCount << " rejected)" << endl;
}

void FCSDataset::makeResampledFile(const string &resampledFilename, const int maxFileCount, const int samplesPerFile)
{
    const string resampledPath = baseDir + resampledFilename;
    if (util::fileExists(resampledPath))
    {
        cout << "Resampled file already exists" << endl;
        return;
    }

    auto allFiles = Directory::enumerateFilesWithPath(baseDir + "DAT\\", ".dat");
    random_shuffle(allFiles.begin(), allFiles.end());
    if (allFiles.size() > maxFileCount)
        allFiles.resize(maxFileCount);
    FCSUtil::makeResampledFile(allFiles, samplesPerFile, resampledPath);
}

void FCSDataset::initProcessor(const string &resampledFilename, int clusterCount)
{
    resampledFile.loadBinary(baseDir + resampledFilename);

    processor.makeTransforms(resampledFile);
    processor.transform(resampledFile);

    processor.makeClustering(resampledFile, clusterCount, baseDir + util::replace(resampledFilename, ".dat", ".cls"));
}

void FCSDataset::makeFeatures()
{
    auto allFiles = Directory::enumerateFilesWithPath(baseDir + "DAT\\", ".dat");

    FCSFile sampleFile;
    sampleFile.loadBinary(allFiles.front());

#pragma omp parallel for schedule(dynamic,1) num_threads(7)
    for (int fileIndex = 0; fileIndex < allFiles.size(); fileIndex++)
    {
        const string &filename = allFiles[fileIndex];
        const string &outDir = baseDir + "features/" + util::removeExtensions(util::fileNameFromPath(filename)) + "/";

        if (processor.featureFilesMissing(sampleFile, outDir))
        {
            FCSFile file;
            file.loadBinary(filename);
            processor.saveFeatures(file, outDir);
        }
    }
}

void FCSDataset::evaluateFeatureSplits()
{
    const string fcsFile0 = entries[0].fileUnstim;

    int dim = resampledFile.dim;
    for (int axisA = 0; axisA < dim; axisA++)
        for (int axisB = 0; axisB < dim; axisB++)
        {
            const string axesString = to_string(axisA) + "_" + to_string(axisB);
            const string featureFilename = baseDir + "features/" + fcsFile0 + "/" + axesString + ".dat";
            if (util::fileExists(featureFilename))
            {
                evaluateFeatureSplits(axisA, axisB, baseDir + "featureEval/" + axesString + "/");
            }
        }
}

void FCSDataset::evaluateFeatureSplits(int axisA, int axisB, const string &outDir)
{
    util::makeDirectory(outDir);
    cout << "Making feature split for " << axisA << "," << axisB << endl;

    const int entryCount = (int)entries.size();
    vector<FCSFeatures> allFeatures(entryCount);
    for (int entryIndex = 0; entryIndex < entryCount; entryIndex++)
    {
        auto &e = entries[entryIndex];
        const string &featureFilename = baseDir + "features/" + e.fileUnstim + "/" + to_string(axisA) + "_" + to_string(axisB) + ".dat";
        allFeatures[entryIndex].load(featureFilename);
    }

    double bestGain = 0.0;
    vector<SplitEntry> bestEntries;
    vec3i bestCoord;

    const int imageSize = (int)allFeatures[0].features.getDimX();
    const int clusterCount = (int)allFeatures[0].features.getDimZ();
    for (int clusterIndex = 0; clusterIndex < clusterCount; clusterIndex++)
    {
        Bitmap bmp(imageSize, imageSize);
        for (auto &p : bmp)
        {
            vector<SplitEntry> sortedEntries(entryCount);
            for (int entryIndex = 0; entryIndex < entryCount; entryIndex++)
            {
                sortedEntries[entryIndex] = SplitEntry(allFeatures[entryIndex].features(p.x, p.y, clusterIndex), entries[entryIndex].label);
            }
            sort(sortedEntries.begin(), sortedEntries.end());

            const SplitResult bestSplit = FCSUtil::findBestSplit(sortedEntries);

            if (bestSplit.informationGain > bestGain)
            {
                bestGain = bestSplit.informationGain;
                bestEntries = sortedEntries;
                bestCoord = vec3i(p.x, p.y, clusterIndex);
            }

            BYTE value = util::boundToByte(bestSplit.informationGain / 0.0002f);
            p.value = vec4uc(value, value, value, 255);
            //cout << p.x << "," << p.y << "," << clusterIndex << " = " << bestSplit.informationGain << endl;
        }
        LodePNG::save(bmp, outDir + to_string(axisA) + "_" + to_string(axisB) + "_c" + to_string(clusterIndex) + ".png");
    }

    ofstream file(outDir + "bestSplit.csv");
    file << "Information gain:," << bestGain << endl;
    file << "Coords:," << bestCoord.toString(",") << endl;
    file << "Axes:" << axisA << "," << axisB << endl;
    file << "Axes:" << resampledFile.fieldNames[axisA] << "," << resampledFile.fieldNames[axisB] << endl;
    for (const SplitEntry &entry : bestEntries)
    {
        file << entry.splitValue << "," << entry.state << endl;
    }
}