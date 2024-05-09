#ifndef TiffFixtures_h
#define TiffFixtures_h

#include "gtest/gtest.h"

#include "Brick.h"
#include "PixelType.h"
#include "TempFixtures.h"

#include "gdal_priv.h"

namespace Isis {

  class ReadWriteTiff : public TempTestingFiles {
    protected:
      void *dbuf;
      GDALDataset *dataset;
      Brick *localBrick;
      QString path;

      void SetUp() override;
      void TearDown() override;
      void createTiff(PixelType pixelType, bool write=true);
  };
}

#endif