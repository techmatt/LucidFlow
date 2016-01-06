
#include "main.h"

Bitmap FCSVisualizer::visualizePoint(const FCSFile &file, const FCSProcessor &processor, int axisA, int axisB, int imageSize)
{
    Bitmap result(imageSize, imageSize);

    for (const MathVectorf &sample : file.transformedSamples)
    {
        vec2f v;
        v.x = sample[axisA];
        v.y = 1.0f - sample[axisB];

        //if (v.x < 0.0f || v.y < 0.0f || v.x > 1.0f || v.y > 1.0f) cout << "bounds: " << v << endl;

        vec2i coord = math::round(v * (float)imageSize);
        if (result.isValidCoordinate(coord))
        {
            result(coord) = processor.clustering.getClusterColor(sample);
        }
    }

    return result;
}

Bitmap FCSVisualizer::visualizeDensity(const FCSFile &file, const FCSProcessor &processor, int axisA, int axisB, int imageSize, int clusterFilter, const QuartileRemap &params)
{
    Grid2f cells(imageSize, imageSize, 0.0f);
    const int borderSize = 3;

    for (const MathVectorf &sample : file.transformedSamples)
    {
        if (clusterFilter != -1 && processor.clustering.getClusterIndex(sample) != clusterFilter)
            continue;

        vec2f v;
        v.x = sample[axisA];
        v.y = 1.0f - sample[axisB];

        //if (v.x < 0.0f || v.y < 0.0f || v.x > 1.0f || v.y > 1.0f) cout << "bounds: " << v << endl;

        vec2i coord = math::round(v * (float)imageSize);
        if (cells.isValidCoordinate(coord) &&
            coord.x >= borderSize && coord.x < imageSize - borderSize &&
            coord.y >= borderSize && coord.y < imageSize - borderSize)
        {
            cells(coord)++;
        }
    }

    QuartileRemap paramsFinal = params;
    if (params.quartiles.size() == 0)
    {
        vector<float> nonzeroValues;
        for (auto &v : cells)
            if (v.value > 1.0f) nonzeroValues.push_back(v.value);
        std::sort(nonzeroValues.begin(), nonzeroValues.end());

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
    
    Bitmap result(imageSize, imageSize);
    for (auto &v : cells)
    {
        float intensity = paramsFinal.transform(v.value) * 255.0f;
        unsigned char c = util::boundToByte(intensity);
        result(v.x, v.y) = vec4uc(c, c, c, 255);
    }
    return result;
}

void FCSVisualizer::saveAllAxesViz(const FCSFile &file, const FCSProcessor &processor, int axisA, int imageSize, const string &outDir)
{
    QuartileRemap params;
    for (int axisB = 0; axisB < file.dim; axisB++)
    {
        //params.quartiles = { 0.0f, 3.0f, 6.0f, 12.0f, 20.0f, 50.0f, 200.0f, 400.0f };
        const auto image = FCSVisualizer::visualizeDensity(file, processor, axisA, axisB, 128, -1, params);

        const string imageFilename = file.fieldNames[axisA] + "_" + file.fieldNames[axisB] + ".png";
        LodePNG::save(image, outDir + imageFilename);
    }
}
