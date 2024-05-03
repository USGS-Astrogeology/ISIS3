#include <QTemporaryFile>
#include <QString>
#include <iostream>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "Brick.h"
#include "GdalIoHandler.h"
#include "CubeTileHandler.h"

#include "TestUtilities.h"

#include "gmock/gmock.h"

using namespace Isis;

TEST(GdalIoTests, DefaultRead) {
  QString filePath = "/Users/krodriguez/cubes/gdal/B10_013341_1010_XN_79S172W.tiff";
  const QList<int> *bandList = new QList<int>;
  Pvl label;

  GdalIoHandler handler(filePath, bandList, label);
  Brick localBrick(200, 200, 1, SignedWord);
  localBrick.SetBasePosition(1, 1, 1);
  handler.read(localBrick);

  long int sum = 0;
  for (int i = 0; i < localBrick.size(); i++) {
    sum += localBrick.at(i);
    localBrick[i] = 0.0;
  }
  EXPECT_EQ(sum, 19832794);

  // QString cubeFilePath = "/Users/acpaquette/repos/ISIS3/build/B10_013341_1010_XN_79S172W.cub";
  // label = Pvl(cubeFilePath);
  // QFile *dataFile = new QFile(cubeFilePath);
  // if (!dataFile->open(QIODevice::ReadWrite)) {
  //   QString msg = "Failed to open [" + dataFile->fileName() + "] for reading. ";
  //   msg += "Verify the output path exists and you have permission to read from the path.";
  //   throw IException(IException::Io, msg, _FILEINFO_);
  // }
  // PvlObject &core = label.findObject("Core", Isis::PvlObject::Traverse);
  // PvlGroup &pixelGroup = core.findGroup("Pixels");
  // PvlKeyword type = pixelGroup.findKeyword("Type");
  // QString strType = type[0];
  // Brick localBuf(200, 200, 1, PixelTypeEnumeration(strType));
  // localBuf.SetBasePosition(1, 1, 1);
  // CubeTileHandler cubeHandler(dataFile, bandList, label, true);
  // cubeHandler.read(localBuf);
  // sum = 0;
  // for (int i = 0; i < localBuf.size(); i++) {
  //   sum += localBuf.at(i);
  // }
  // std::cout << sum << std::endl;
  // delete dataFile;
}


TEST(GdalIoTests, DefaulWrite) {
  QString filePath = "/Users/krodriguez/cubes/gdal/B10_013341_1010_XN_79S172W.tiff";
  
  {
    const QList<int> *bandList = new QList<int>;
    Pvl label;

    GdalIoHandler handler(filePath, bandList, label);
    
    Brick localBrick(200, 200, 1, SignedWord);

    localBrick.SetBasePosition(1, 1, 1);
    // init everything to 1
    for (int i = 0; i < localBrick.size(); i++) {
      localBrick[i] = 100;
    }

    localBrick.SetBasePosition(1, 1, 1);
    handler.write(localBrick);
  } // file is closed  

  // read it back 
  GDALDatasetUniquePtr ds = GDALDatasetUniquePtr(GDALDataset::FromHandle(GDALOpen(filePath.toStdString().c_str(), GA_ReadOnly)));
  double* dbuf = (double *) CPLMalloc(sizeof(double)*200*200); 
  GDALRasterBand  *poBand = ds->GetRasterBand(1);
  CPLErr err = poBand->RasterIO(GF_Read, 0, 0, 
                  200, 200,
                  dbuf, 
                  200, 200, 
                  GDT_Float64,
                  0, 0);

  
  for (int i = 0; i<200*200; i++) { 
    EXPECT_EQ(dbuf[i], 1)00;
  }

}