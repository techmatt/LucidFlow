
#include "main.h"

#include <leveldb/db.h>
#include <leveldb/write_batch.h>

#include <stdint.h>
#include <sys/stat.h>
#include <direct.h>

#include "caffe/proto/caffe.pb.h"

using namespace caffe;

void LevelDBExporter::exportDB(const FCSDataset &dataset, const string &outDir, int epochs, int evalSamplesPerPatient, int startPatient, int endPatient)
{
    const int pixelCount = constants::imageSize * constants::imageSize;
    const int featureCount = (int)dataset.selectedFeatures.size();

    vector<const Patient*> patients;
    for (int i = startPatient; i < endPatient; i++)
        patients.push_back(&dataset.patients[i]);
    const int patientCount = patients.size();

    // leveldb
    leveldb::DB* dbA;
    leveldb::DB* dbB;
    leveldb::Options options;
    options.error_if_exists = true;
    options.create_if_missing = true;
    options.write_buffer_size = 268435456;

    // Open dbs
    std::cout << "Opening leveldbs " << outDir << endl;
    leveldb::Status statusA = leveldb::DB::Open(options, outDir + "A", &dbA);
    leveldb::Status statusB = leveldb::DB::Open(options, outDir + "B", &dbB);
    if (!statusA.ok() || !statusB.ok())
    {
        std::cout << "Failed to open " << outDir << " or it already exists" << endl;
        return;
    }

    leveldb::WriteBatch* batchA = new leveldb::WriteBatch();
    leveldb::WriteBatch* batchB = new leveldb::WriteBatch();

    // Storing to db
    char* rawData = new char[pixelCount * featureCount];

    int count = 0;
    const int kMaxKeyLength = 10;
    char key_cstr[kMaxKeyLength];
    
    ColorImageR8G8B8A8 dummyImage(constants::imageSize, constants::imageSize);

    vector<FeatureDescription> selectedFeatures;
    for (auto &f : dataset.selectedFeatures)
        selectedFeatures.push_back(f->desc);

    cout << "A total of " << patientCount * epochs << " samples will be generated." << endl;
    int totalSampleIndex = 0;
    for (int epoch = 0; epoch < epochs; epoch++)
    {
        cout << "Start epoch " << epoch << endl;

        auto shuffledPatients = patients;
        random_shuffle(shuffledPatients.begin(), shuffledPatients.end());

        for (auto &p : shuffledPatients)
        {
            if (totalSampleIndex % 20 == 0)
                cout << "Sample " << totalSampleIndex << " / " << patientCount * epochs << endl;
            totalSampleIndex++;

            FCSFile fileUnstim, fileStim;
            fileUnstim.loadBinary(dataset.baseDir + "DAT/" + util::removeExtensions(p->fileUnstim) + ".dat");
            fileStim.loadBinary(dataset.baseDir + "DAT/" + util::removeExtensions(p->fileStim) + ".dat");
            dataset.processor.transform(fileUnstim);
            dataset.processor.transform(fileStim);

            FCSFeatures features;
            features.create(dataset.processor, fileUnstim, fileStim, constants::imageSize, selectedFeatures);

            int pIndex = 0;
            for (int feature = 0; feature < featureCount; feature++)
            {
                for (const auto &p : dummyImage)
                {
                    rawData[pIndex++] = features.features(p.x, p.y, feature);
                }
            }

            Datum datumA;
            datumA.set_channels(featureCount);
            datumA.set_height(constants::imageSize);
            datumA.set_width(constants::imageSize);
            datumA.set_data(rawData, pixelCount * featureCount);
            datumA.set_label(p->index);

            Datum datumB;
            datumB.set_channels(constants::survivalIntervals);
            datumB.set_width(1);
            datumB.set_height(1);
            datumB.set_label(p->index);
            vector<float> outcome = p->makeOutcomeVector();
            for (float f : outcome)
                datumB.add_float_data(f);

            sprintf_s(key_cstr, kMaxKeyLength, "%08d", totalSampleIndex);

            string valueA, valueB;
            datumA.SerializeToString(&valueA);
            datumB.SerializeToString(&valueB);

            string keystr(key_cstr);

            // Put in db
            batchA->Put(keystr, valueA);
            batchB->Put(keystr, valueB);

            if (++count % 1000 == 0) {
                // Commit txn
                dbA->Write(leveldb::WriteOptions(), batchA);
                delete batchA;
                batchA = new leveldb::WriteBatch();

                dbB->Write(leveldb::WriteOptions(), batchB);
                delete batchB;
                batchB = new leveldb::WriteBatch();
            }
        }
    }
    // write the last batch
    if (count % 1000 != 0) {
        dbA->Write(leveldb::WriteOptions(), batchA);
        dbB->Write(leveldb::WriteOptions(), batchB);
    }
    delete batchA;
    delete dbA;
    delete batchB;
    delete dbB;
    cout << "Processed " << count << " entries." << endl;

    vector<PatientFeatureSample> allSamples;
    allSamples.reserve(patients.size() * evalSamplesPerPatient);

    for (auto &p : patients)
    {
        cout << "Sampling patient " << p->index << endl;
        
        FCSFile fileUnstim, fileStim;
        fileUnstim.loadBinary(dataset.baseDir + "DAT/" + util::removeExtensions(p->fileUnstim) + ".dat");
        fileStim.loadBinary(dataset.baseDir + "DAT/" + util::removeExtensions(p->fileStim) + ".dat");
        dataset.processor.transform(fileUnstim);
        dataset.processor.transform(fileStim);

        for (int sample = 0; sample < evalSamplesPerPatient; sample++)
        {
            FCSFeatures features;
            features.create(dataset.processor, fileUnstim, fileStim, constants::imageSize, selectedFeatures);

            PatientFeatureSample newPatientSample;
            newPatientSample.patient = *p;
            newPatientSample.features = features.features;
            allSamples.push_back(newPatientSample);
        }
    }

    util::serializeToFileCompressed(outDir + "Samples.dat", allSamples);
}
