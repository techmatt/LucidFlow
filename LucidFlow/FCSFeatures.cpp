
#include "main.h"

Grid3i FCSFeatures::makeCounts(const FCSProcessor &processor, const FCSFile &fileUnstim, const FCSFile &fileStim, int imageSize, const vector<FeatureDescription> &descriptions)
{
    const int featureCount = (int)descriptions.size();
    
    Grid3i cellCounts(imageSize, imageSize, featureCount, 0);
    const int borderSize = 1;

    auto addSamples = [&](const FCSFile &file, FeatureCondition condition)
    {
        map< int, vector<int> > clusterIndexToFeatures;
        for (auto &d : iterate(descriptions))
            if (d.value.condition == condition)
            {
                if (d.value.clusterIndex == processor.clusterCount)
                {
                    for (int clusterIndex = 0; clusterIndex < processor.clusterCount; clusterIndex++)
                        clusterIndexToFeatures[clusterIndex].push_back((int)d.index);
                }
                else
                    clusterIndexToFeatures[d.value.clusterIndex].push_back((int)d.index);
            }

        for (int i = 0; i < constants::samplesPerFeatureSet; i++)
        {
            const int randomSampleIndex = util::randomInteger(0, (int)file.transformedSamples.size() - 1);
            const MathVectorf &sample = file.transformedSamples[randomSampleIndex];
            const int clusterIndex = file.sampleClusterIndices[randomSampleIndex];

            for (int featureIndex : clusterIndexToFeatures[clusterIndex])
            {
                const FeatureDescription &desc = descriptions[featureIndex];
                vec2f v;
                v.x = sample[desc.axisA];
                v.y = 1.0f - sample[desc.axisB];

                //if (v.x < 0.0f || v.y < 0.0f || v.x > 1.0f || v.y > 1.0f) cout << "bounds: " << v << endl;

                const vec3i coord(math::round(v * (float)imageSize), featureIndex);
                if (cellCounts.isValidCoordinate(coord) &&
                    coord.x >= borderSize && coord.x < imageSize - borderSize &&
                    coord.y >= borderSize && coord.y < imageSize - borderSize)
                {
                    cellCounts(coord)++;
                }
            }
        }
    };

    addSamples(fileUnstim, FeatureCondition::Unstim);
    addSamples(fileStim, FeatureCondition::Stim);

    return cellCounts;
}

void FCSFeatures::create(const FCSProcessor &processor, const FCSFile &fileUnstim, const FCSFile &fileStim, int imageSize, int axisA, int axisB)
{
    descriptions.clear();
    const int clusterCount = (int)processor.clustering.clusters.size();
    for (int clusterIndex = 0; clusterIndex <= clusterCount; clusterIndex++)
    {
        descriptions.push_back(FeatureDescription(FeatureCondition::Unstim, axisA, axisB, clusterIndex));
        descriptions.push_back(FeatureDescription(FeatureCondition::Stim, axisA, axisB, clusterIndex));
    }

    const Grid3i counts = makeCounts(processor, fileUnstim, fileStim, imageSize, descriptions);

    vector<const QuartileRemap*> quartiles;
    for (auto &desc : descriptions)
        quartiles.push_back(&processor.getQuartileRemap(desc));

    finalizeFromCounts(counts, quartiles);
}

void FCSFeatures::create(const FCSProcessor &processor, const FCSFile &fileUnstim, const FCSFile &fileStim, int imageSize, const vector<FeatureDescription> &_descriptions)
{
    descriptions = _descriptions;

    const Grid3i counts = makeCounts(processor, fileUnstim, fileStim, imageSize, descriptions);

    vector<const QuartileRemap*> quartiles;
    for (auto &desc : descriptions)
        quartiles.push_back(&processor.getQuartileRemap(desc));

    finalizeFromCounts(counts, quartiles);
}

void FCSFeatures::finalizeFromCounts(const Grid3i &counts, const vector<const QuartileRemap*> &params)
{
    features.allocate(counts.getDimensions());
    for (auto &c : counts)
    {
        const float intensityF = params[c.z]->transform((float)c.value) * 255.0f;
        const unsigned char intensityC = util::boundToByte(math::round(intensityF));
        features(c.x, c.y, c.z) = intensityC;
    }
}

void FCSFeatures::saveDebugViz(const string &baseDir) const
{
    for (auto &desc : iterate(descriptions))
    {
        const Bitmap bmp = getChannel((int)desc.index);
        LodePNG::save(bmp, baseDir + desc.value.toString() + ".png");
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

QuartileRemap FCSFeatures::makeQuartiles(const FCSProcessor &processor, const FCSFile &fileUnstim, const FCSFile &fileStim, int imageSize, const vector<FeatureDescription> &descriptions)
{
    const Grid3i counts = makeCounts(processor, fileUnstim, fileStim, imageSize, descriptions);
    
    vector<float> nonzeroValues;
    for (auto &v : counts)
        if (v.value > 2) nonzeroValues.push_back((float)v.value);
    std::sort(nonzeroValues.begin(), nonzeroValues.end());

    if (nonzeroValues.size() == 0)
        nonzeroValues.push_back(1.0f);

    QuartileRemap result;

    const float maxValue = (float)nonzeroValues.back();
    /*if (maxValue <= 255.0f)
    {
        cout << "Dynamic range is only " << maxValue << ". Defaulting to trivial scaling." << endl;
        result.quartiles.resize(2);
        result.quartiles[0] = 0;
        result.quartiles[1] = maxValue;
        return result;
    }*/

    result.quartiles.resize(8);
    result.quartiles[0] = 0.0f;
    result.quartiles[1] = nonzeroValues[(int)(nonzeroValues.size() * 0.2)];
    result.quartiles[2] = nonzeroValues[(int)(nonzeroValues.size() * 0.4)];
    result.quartiles[3] = nonzeroValues[(int)(nonzeroValues.size() * 0.6)];
    result.quartiles[4] = nonzeroValues[(int)(nonzeroValues.size() * 0.7)];
    result.quartiles[5] = nonzeroValues[(int)(nonzeroValues.size() * 0.85)];
    result.quartiles[6] = nonzeroValues[(int)(nonzeroValues.size() * 0.98)];
    result.quartiles[7] = nonzeroValues[(int)(nonzeroValues.size() * 0.9999)];

    //params.quartiles = { 0.0f, 3.0f, 6.0f, 12.0f, 20.0f, 50.0f, 200.0f, 400.0f };
    //result.print();

    return result;
}
