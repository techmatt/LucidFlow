
#include "main.h"

void FCSFeatures::create(const FCSFile &file, const FCSProcessor &processor, int _axisA, int _axisB, int imageSize, const QuartileRemap &params)
{
    axisA = _axisA;
    axisB = _axisB;

    const int clusterCount = (int)processor.clustering.clusters.size();
    features.allocate(imageSize, imageSize, clusterCount + 1);

    for (int clusterIndex = 0; clusterIndex < clusterCount; clusterIndex++)
    {
        const Bitmap bmp = FCSVisualizer::visualizeDensity(file, processor, axisA, axisB, imageSize, clusterIndex, params);
        setChannel(bmp, clusterIndex);
    }

    const Bitmap bmpAll = FCSVisualizer::visualizeDensity(file, processor, axisA, axisB, imageSize, -1, params);
    setChannel(bmpAll, clusterCount);
}

void FCSFeatures::saveDebugViz(const string &baseDir) const
{
    const Bitmap bmpA = getChannel(0);
    const Bitmap bmpB = getChannel((int)features.getDimZ() - 2);
    const Bitmap bmpC = getChannel((int)features.getDimZ() - 1);

    LodePNG::save(bmpA, baseDir + to_string(axisA) + "_" + to_string(axisB) + "_c0.png");
    LodePNG::save(bmpB, baseDir + to_string(axisA) + "_" + to_string(axisB) + "_c1.png");
    LodePNG::save(bmpC, baseDir + to_string(axisA) + "_" + to_string(axisB) + "_cA.png");
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
    out << axisA << axisB;
    out.writePrimitive(features);
    out.closeStream();
}

void FCSFeatures::load(const string &filename)
{
    BinaryDataStreamFile out(filename, false);
    out >> axisA >> axisB;
    out.readPrimitive(features);
    out.closeStream();
}
