
#include "main.h"

void FCSFeatures::create(const FCSFile &file, const FCSProcessor &processor, int _axisA, int _axisB, int imageSize, const QuartileRemap &params)
{
    fcsID = file.id;
    axisA = _axisA;
    axisB = _axisB;

    const int clusterCount = (int)processor.clustering.clusters.size();
    //cout << "Cluster count: " << clusterCount << endl;
    features.allocate(imageSize, imageSize, clusterCount + 1, 0);

    /*for (int clusterIndex = 0; clusterIndex < clusterCount; clusterIndex++)
    {
        const Bitmap bmp = FCSVisualizer::visualizeDensity(file, processor, axisA, axisB, imageSize, clusterIndex, params);
        setChannel(bmp, clusterIndex);
    }

    const Bitmap bmpAll = FCSVisualizer::visualizeDensity(file, processor, axisA, axisB, imageSize, -1, params);
    setChannel(bmpAll, clusterCount);*/

    Grid3i cellCounts(imageSize, imageSize, clusterCount, 0);
    const int borderSize = 1;
    
    clusterHits = vector<int>(clusterCount, 0);

    for (int i = 0; i < constants::samplesPerFeatureSet; i++)
    {
        const int randomSampleIndex = util::randomInteger(0, (int)file.transformedSamples.size() - 1);
        const MathVectorf &sample = file.transformedSamples[randomSampleIndex];
        
        const int clusterIndex = processor.clustering.getClusterIndex(sample);
        clusterHits[clusterIndex]++;

        vec2f v;
        v.x = sample[axisA];
        v.y = 1.0f - sample[axisB];

        //if (v.x < 0.0f || v.y < 0.0f || v.x > 1.0f || v.y > 1.0f) cout << "bounds: " << v << endl;

        const vec3i coord(math::round(v * (float)imageSize), clusterIndex);
        if (cellCounts.isValidCoordinate(coord) &&
            coord.x >= borderSize && coord.x < imageSize - borderSize &&
            coord.y >= borderSize && coord.y < imageSize - borderSize)
        {
            cellCounts(coord)++;
        }
    }

    QuartileRemap paramsFinal = params;
    if (params.quartiles.size() == 0)
    {
        vector<float> nonzeroValues;
        for (auto &v : cellCounts)
            if (v.value > 1) nonzeroValues.push_back((float)v.value);
        std::sort(nonzeroValues.begin(), nonzeroValues.end());

        if (nonzeroValues.size() == 0)
            nonzeroValues.push_back(1.0f);

        paramsFinal.quartiles.resize(8);
        paramsFinal.quartiles[0] = 0.0f;
        paramsFinal.quartiles[1] = nonzeroValues[(int)(nonzeroValues.size() * 0.2f)];
        paramsFinal.quartiles[2] = nonzeroValues[(int)(nonzeroValues.size() * 0.4f)];
        paramsFinal.quartiles[3] = nonzeroValues[(int)(nonzeroValues.size() * 0.6f)];
        paramsFinal.quartiles[4] = nonzeroValues[(int)(nonzeroValues.size() * 0.7f)];
        paramsFinal.quartiles[5] = nonzeroValues[(int)(nonzeroValues.size() * 0.85f)];
        paramsFinal.quartiles[6] = nonzeroValues[(int)(nonzeroValues.size() * 0.98f)];
        paramsFinal.quartiles[7] = nonzeroValues.back();
        //params.quartiles = { 0.0f, 3.0f, 6.0f, 12.0f, 20.0f, 50.0f, 200.0f, 400.0f };
        //paramsFinal.print();
    }

    /*cout << "after" << endl;
    cin.get();
    for (auto &v : cellCounts)
        if (v.value != 0)
            cout << "value: " << v.x << "," << v.y << "," << v.z << ": " << v.value << endl;*/

    for (auto &c : cellCounts)
    {
        //if (c.value != 0)
        //    cout << "value: " << c.x << "," << c.y << "," << c.z << ": " << int(c.value) << endl;
        const float intensityF = paramsFinal.transform((float)c.value) * 255.0f;
        const unsigned char intensityC = util::boundToByte(intensityF);
        //if (intensityC != 0)
        //    cout << "value: " << c.x << "," << c.y << "," << c.z << ": " << int(intensityC) << endl;
        features(c.x, c.y, c.z) = intensityC;
    }

    const Bitmap bmpAll = FCSVisualizer::visualizeDensity(file, processor, axisA, axisB, imageSize, -1, params);
    setChannel(bmpAll, clusterCount);
}

void FCSFeatures::saveDebugViz(const string &baseDir) const
{
    const string prefix = baseDir + to_string(axisA) + "_" + to_string(axisB) + "_c";
    
    /*for (int clusterIndex = 0; clusterIndex < features.getDimZ() - 1; clusterIndex += 20)
    {
        const Bitmap bmp = getChannel(clusterIndex);
        LodePNG::save(bmp, prefix + to_string(clusterIndex) + ".png");
    }*/

    const Bitmap bmpAll = getChannel((int)features.getDimZ() - 1);
    LodePNG::save(bmpAll, prefix + "all.png");

    if (constants::outputClusterHits)
    {
        ofstream file(baseDir + "clusterHits" + to_string(axisA) + "_" + to_string(axisB) + ".csv");
        file << "cluster,hits" << endl;
        for (int clusterIndex = 0; clusterIndex < clusterHits.size(); clusterIndex++)
        {
            file << clusterIndex << "," << clusterHits[clusterIndex] << endl;
        }
    }
}

void FCSFeatures::setChannel(const Bitmap &bmp, int channel)
{
    for (auto &p : bmp)
    {
        features(p.x, p.y, channel) = p.value.r;
    }
}

Bitmap FCSFeatures::getChannel(int channel) const
{
    Bitmap result((int)features.getDimX(), (int)features.getDimY());
    for (auto &p : result)
    {
        BYTE v = features(p.x, p.y, channel);
        p.value = vec4uc(v, v, v, 255);
    }
    return result;
}

void FCSFeatures::save(const string &filename) const
{
    BinaryDataStreamFile out(filename, true);
    out << axisA << axisB << clusterHits;
    out.writePrimitive(features);
    out.closeStream();
}

void FCSFeatures::load(const string &filename)
{
    BinaryDataStreamFile out(filename, false);
    out >> axisA >> axisB >> clusterHits;
    out.readPrimitive(features);
    out.closeStream();
}
