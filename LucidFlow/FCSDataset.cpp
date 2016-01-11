
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

    /*for (int i = 0; i < resampledFile.dim; i++)
    {
        cout << "data: " << resampledFile.data(100000, i) << endl;
        cout << "tran: " << resampledFile.transformedSamples[100000][i] << endl;
    }*/

    const string clusterFilename = baseDir + util::removeExtensions(resampledFilename) + "_" + to_string(clusterCount) + ".clusters";
    processor.makeClustering(resampledFile, clusterCount, clusterFilename);

    processor.assignClusters(resampledFile);

    const string quartileFilename = util::replace(clusterFilename, ".clusters", ".quartiles");
    processor.makeQuartiles(resampledFile, quartileFilename);
}

string FCSDataset::describeAxis(int axisIndex) const
{
    return resampledFile.fieldNames[axisIndex] + " (" + to_string(axisIndex) + ")";
}

void FCSDataset::makeFeatures()
{
    util::makeDirectory(baseDir + "features");
    
#pragma omp parallel for schedule(dynamic,1) num_threads(7)
    for (int entryIndex = 0; entryIndex < entries.size(); entryIndex++)
    {
        const auto &e = entries[entryIndex];
        const string featureFilename = baseDir + "features/" + e.patientID() + ".feat";

        if (util::fileExists(featureFilename))
        {
            cout << "Skipping " << featureFilename << endl;
            continue;
        }

        cout << "Creating " << featureFilename << endl;
        
        FCSFile fileUnstim, fileStim;
        fileUnstim.loadBinary(baseDir + "DAT/" + util::removeExtensions(e.fileUnstim) + ".dat");
        fileStim.loadBinary  (baseDir + "DAT/" + util::removeExtensions(e.fileStim  ) + ".dat");

        processor.saveFeatures(fileUnstim, fileStim, featureFilename);
    }

#pragma omp parallel for schedule(dynamic,1) num_threads(7)
    for (int entryIndex = 0; entryIndex < entries.size(); entryIndex++)
    {
        const auto &e = entries[entryIndex];
        const string featureFilename = baseDir + "features/" + e.patientID() + ".feat";

        vector<FCSFeatures> allFeatures;
        cout << "Loading " << featureFilename << endl;
        
        BinaryDataStreamZLibFile in(featureFilename, false);
        in >> allFeatures;
        in.closeStream();

        for (FCSFeatures &f : allFeatures)
        {
            FCSFeatures *newF = new FCSFeatures();
            newF->descriptions = std::move(f.descriptions);
            newF->features = std::move(f.features);
#pragma omp critical
            featureDatabase.addEntry(e.patientID(), newF);
        }
    }
}

void FCSDataset::evaluateFeatureSplits()
{
    /*for (int i = 0; i < (int)FeatureConditionCount; i++)
    {
        evaluateFeatureSplits((FeatureCondition)i);
    }*/
}

void FCSDataset::evaluateFeatureSplits(FeatureCondition condition)
{
    /*const string evalDir = baseDir + "featureEval" + to_string(condition) + "/";
    util::makeDirectory(evalDir);

    cout << "Evaluating all feature splits" << endl;
    const string fcsFile0 = entries[0].fileUnstim;

    int dim = resampledFile.dim;
    Grid2d bestSplitValue(dim, dim, 0.0f);

#pragma omp parallel for schedule(dynamic,1) num_threads(7)
    for (int axisA = 0; axisA < dim; axisA++)
        for (int axisB = 0; axisB < dim; axisB++)
        {
            if (processor.axesValid(resampledFile.fieldNames[axisA], resampledFile.fieldNames[axisB]))
            {
                const string axesString = to_string(axisA) + "_" + to_string(axisB);
                SplitResult bestSplit = evaluateFeatureSplits(axisA, axisB, evalDir + axesString + "/", condition);
                bestSplitValue(axisA, axisB) = bestSplit.informationGain;
            }
        }

    ofstream file(baseDir + "allFeaturePairs" + to_string(condition) + ".csv");
    for (int axisA = 0; axisA < dim; axisA++)
        file << "," << describeAxis(axisA);
    file << endl;

    for (int axisA = 0; axisA < dim; axisA++)
    {
        file << describeAxis(axisA);
        for (int axisB = 0; axisB < dim; axisB++)
        {
            file << "," << bestSplitValue(axisA, axisB);
        }
        file << endl;
    }*/
}

SplitResult FCSDataset::evaluateFeatureSplits(int axisA, int axisB, const string &outDir, FeatureCondition condition)
{
    util::makeDirectory(outDir);
    cout << "Making feature split for " << axisA << "," << axisB << endl;

    const int entryCount = (int)entries.size();
    vector<const FCSFeatures*> featureListUnstim;
    vector<const FCSFeatures*> featureListStim;
    for (int entryIndex = 0; entryIndex < entryCount; entryIndex++)
    {
        auto &e = entries[entryIndex];
        
        const FCSFeatures *fUnstim = featureDatabase.getFeatures(e.fileUnstim, axisA, axisB);
        featureListUnstim.push_back(fUnstim);

        const FCSFeatures *fStim = featureDatabase.getFeatures(e.fileStim, axisA, axisB);
        featureListStim.push_back(fStim);
    }

    SplitResult bestOverallSplit;
    vector<SplitEntry> bestEntries;
    vec3i bestCoord;

    auto computeFeature = [&](int entryIndex, int x, int y, int clusterIndex)
    {
        if (condition == FeatureCondition::Stim)
            return (int)featureListUnstim[entryIndex]->features(x, y, clusterIndex);
        if (condition == FeatureCondition::Unstim)
            return (int)featureListStim[entryIndex]->features(x, y, clusterIndex);
        return 0;
    };

    const int imageSize = (int)featureListUnstim[0]->features.getDimX();
    const int clusterCount = (int)featureListUnstim[0]->features.getDimZ();
    for (int clusterIndex = 0; clusterIndex < clusterCount; clusterIndex++)
    {
        Grid2f informationGain(imageSize, imageSize);
        for (auto &p : informationGain)
        {
            vector<SplitEntry> sortedEntries(entryCount);
            for (int entryIndex = 0; entryIndex < entryCount; entryIndex++)
            {
                sortedEntries[entryIndex] = SplitEntry(computeFeature(entryIndex, (int)p.x, (int)p.y, clusterIndex), entries[entryIndex].label);
            }
            sort(sortedEntries.begin(), sortedEntries.end());

            const SplitResult bestSplit = FCSUtil::findBestSplit(sortedEntries);

            if (bestSplit.informationGain > bestOverallSplit.informationGain)
            {
                bestOverallSplit = bestSplit;
                bestEntries = sortedEntries;
                bestCoord = vec3i((int)p.x, (int)p.y, clusterIndex);
            }

            informationGain(p.x, p.y) = (float)bestSplit.informationGain;
            //cout << p.x << "," << p.y << "," << clusterIndex << " = " << bestSplit.informationGain << endl;
        }
        
        const Bitmap bmp = FCSVisualizer::gridToBmp(informationGain, 1.0f / 0.0004f);
        LodePNG::save(bmp, outDir + to_string(axisA) + "_" + to_string(axisB) + "_c" + to_string(clusterIndex) + ".png");

#pragma omp critical
        featureQualityDatabase.addEntry(condition, axisA, axisB, clusterIndex, informationGain);
    }

    ofstream file(outDir + "bestSplit.csv");
    file << "Information gain:," << bestOverallSplit.informationGain << endl;
    file << "Split:," << bestOverallSplit.splitValue << endl;
    file << "Coords:," << bestCoord.toString(",") << endl;
    file << "Axes:," << axisA << "," << axisB << endl;
    file << "Axes:," << resampledFile.fieldNames[axisA] << "," << resampledFile.fieldNames[axisB] << endl;
    for (const SplitEntry &entry : bestEntries)
    {
        file << entry.splitValue << "," << entry.state << endl;
    }
    return bestOverallSplit;
}

const FCSDataset::Entry& FCSDataset::sampleRandomEntry(int label) const
{
    while (true)
    {
        auto &e = util::randomElement(entries);
        if (e.label == label)
            return e;
    }
}

Bitmap FCSDataset::makeFeatureViz(const FCSDataset::Entry &entry, FeatureCondition condition, int axisA, int axisB, int clusterIndex) const
{
    const FCSFeatures *fUnstim = featureDatabase.getFeatures(entry.fileUnstim, axisA, axisB);
    const FCSFeatures *fStim = featureDatabase.getFeatures(entry.fileStim, axisA, axisB);
    
    if (condition == FeatureCondition::Stim)
        return fStim->getChannel(clusterIndex);

    if (condition == FeatureCondition::Unstim)
        return fUnstim->getChannel(clusterIndex);

    const Bitmap bmpStim = fStim->getChannel(clusterIndex);
    const Bitmap bmpUnstim = fUnstim->getChannel(clusterIndex);
    Bitmap result = bmpStim;
    for (auto &p : result)
    {
        double valueF = (double)bmpStim(p.x, p.y).r - (double)bmpUnstim(p.x, p.y).r;
        BYTE valueC = util::boundToByte((valueF + 255.0) * 0.5);
        p.value = vec4uc(valueC, valueC, valueC, 255);
    }
    return result;
}

void FCSDataset::saveFeatureDescription(const vector<const FeatureQualityDatabase::Entry*> &features, const string &filename) const
{
    /*ofstream descFile(filename);
    descFile << "Total features:," << features.size() << endl;
    descFile << "index,condition,AxisA,AxisB,cluster,score" << endl;

    int featureIndex = 0;
    for (auto &e : features)
    {
        descFile << featureIndex << ",";
        descFile << e->condition << ",";
        descFile << describeAxis(e->axisA) << ",";
        descFile << describeAxis(e->axisB) << ",";
        descFile << e->clusterIndex << ",";
        descFile << e->score << endl;
        featureIndex++;
    }*/
}

void FCSDataset::chooseAndVizFeatures(int totalFeatures, int maxInstancesPerGraph, int samplesPerFeature, const string &outDir)
{
    /*util::makeDirectory(outDir);

    selectedFeatures = featureQualityDatabase.selectBestFeatures(totalFeatures, maxInstancesPerGraph);
    featureQualityDatabase.sortEntries();

    saveFeatureDescription(selectedFeatures, outDir + "selectedFeatures.csv");
    saveFeatureDescription(featureQualityDatabase.entries, outDir + "allFeatures.csv");

    int featureIndex = 0;
    for (auto &e : selectedFeatures)
    {
        const Bitmap bmp = FCSVisualizer::gridToBmp(e->informationGain, 1.0f / 0.0002f);
        LodePNG::save(bmp, outDir + "f" + to_string(featureIndex) + "_cond" + to_string(e->condition) + "_" +
                                    to_string(e->axisA) + "_" +
                                    to_string(e->axisB) + "_c" +
                                    to_string(e->clusterIndex) + ".png");

        for (int sample = 0; sample < samplesPerFeature; sample++)
        {
            const Entry &e0 = sampleRandomEntry(0);
            const Entry &e1 = sampleRandomEntry(1);
            const Bitmap bmp0 = makeFeatureViz(e0, e->condition, e->axisA, e->axisB, e->clusterIndex);
            const Bitmap bmp1 = makeFeatureViz(e1, e->condition, e->axisA, e->axisB, e->clusterIndex);

            LodePNG::save(bmp0, outDir + "f" + to_string(featureIndex) + "_status0_" + to_string(sample) + ".png");
            LodePNG::save(bmp1, outDir + "f" + to_string(featureIndex) + "_status1_" + to_string(sample) + ".png");
        }
        featureIndex++;
    }*/
}
