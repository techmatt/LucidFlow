
#ifdef _DEBUG
#define MLIB_ERROR_CHECK
#endif

#include "mLibCore.h"
#include "mLibLodePNG.h"

using namespace ml;
using namespace std;

typedef ColorImageR8G8B8A8 Bitmap;

#include "FCSUtil.h"
#include "FCSFile.h"
#include "FCSProcessor.h"
#include "FCSVisualizer.h"
#include "FCSDataset.h"
#include "FCSFeatures.h"