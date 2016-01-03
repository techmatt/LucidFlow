
#include "main.h"

Bitmap FCSVisualizer::visualize(const FCSFile &file, const FCSProcessor &processor, int axisA, int axisB, int imageSize)
{
    Bitmap result(imageSize, imageSize);

    for (const MathVectorf &sample : file.transformedSamples)
    {
        vec2f v;
        v.x = sample[axisA];
        v.y = sample[axisB];
        vec2i coord = math::round(v * imageSize);
        if (result.isValidCoordinate(v))
        {
            result(v) = processor.getClusterColor(sample);
        }
    }

    return result;
}
