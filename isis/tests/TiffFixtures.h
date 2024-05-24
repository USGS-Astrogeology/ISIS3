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
      void *dbuf = NULL;
      GDALDataset *dataset = NULL;
      Brick *localBrick = NULL;
      QString path;

      void SetUp() override;
      void TearDown() override;
      void createSizedTiff(int samples, int lines, int bands, PixelType pixelType);
      void createTiff(PixelType pixelType, bool write=true);
  };
}

#endif