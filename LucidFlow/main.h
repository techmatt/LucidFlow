
#ifdef _DEBUG
#define MLIB_ERROR_CHECK
#endif

#include "mLibCore.h"
#include "mLibLodePNG.h"
#include "mLibZLib.h"

using namespace ml;
using namespace std;

typedef ColorImageR8G8B8A8 Bitmap;

#include "constants.h"

#include "../common/patient.h"
#include "FCSUtil.h"
#include "FCSFile.h"
#include "FCSProcessor.h"
#include "FCSVisualizer.h"
#include "FCSFeatures.h"
#include "FCSDataset.h"

#include "LevelDBExporter.h"