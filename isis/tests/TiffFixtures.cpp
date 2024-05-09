#include "TiffFixtures.h"

#include "Brick.h"
#include "PixelType.h"
#include "SpecialPixel.h"

namespace Isis {

  void ReadWriteTiff::SetUp() {
    TempTestingFiles::SetUp();
    
    path = tempDir.path() + "/tiny.tiff";
  }

  void ReadWriteTiff::TearDown() {
    if (dataset) {
      delete dataset;
    }
    if (localBrick) {
      delete localBrick;
    }
    free(dbuf);
  }

  void ReadWriteTiff::createTiff(PixelType pixelType, bool write) {
    GDALAllRegister();
    GDALDriver* driver = GetGDALDriverManager()->GetDriverByName("GTiff");
    if (driver) {
      char **papszOptions = NULL;
      dataset = driver->Create(path.toStdString().c_str(), 6, 1, 1, IsisPixelToGdal(pixelType), papszOptions);
      for (int i = 1; i <= 1; i++) {
        GDALRasterBand *band = dataset->GetRasterBand(i);
        band->SetScale(1);
        band->SetOffset(0);
      }
    }
    if (write) {
      if (pixelType == Double) {
        dbuf = (void *)CPLMalloc(sizeof(double) * 6);

        ((double *)dbuf)[0] = HIGH_INSTR_SAT8;
        ((double *)dbuf)[1] = HIGH_REPR_SAT8;
        ((double *)dbuf)[2] = LOW_INSTR_SAT8;
        ((double *)dbuf)[3] = LOW_REPR_SAT8;
        ((double *)dbuf)[4] = NULL8;
        ((double *)dbuf)[5] = 1000;
      }
      else if (pixelType == Real) {
        dbuf = (void *)CPLMalloc(sizeof(float) * 6);
        
        ((float *)dbuf)[0] = HIGH_INSTR_SAT4;
        ((float *)dbuf)[1] = HIGH_REPR_SAT4;
        ((float *)dbuf)[2] = LOW_INSTR_SAT4;
        ((float *)dbuf)[3] = LOW_REPR_SAT4;
        ((float *)dbuf)[4] = NULL4;
        ((float *)dbuf)[5] = 1000;
      }
      else if (pixelType == SignedInteger) {
        dbuf = (void *)CPLMalloc(sizeof(int) * 6);

        ((int *)dbuf)[0] = HIGH_INSTR_SATI4;
        ((int *)dbuf)[1] = HIGH_REPR_SATI4;
        ((int *)dbuf)[2] = LOW_INSTR_SATI4;
        ((int *)dbuf)[3] = LOW_REPR_SATI4;
        ((int *)dbuf)[4] = NULLI4;
        ((int *)dbuf)[5] = 1000;
      }
      else if (pixelType == UnsignedInteger) {
        dbuf = (void *)CPLMalloc(sizeof(unsigned int) * 6);

        ((unsigned int *)dbuf)[0] = HIGH_INSTR_SATUI4;
        ((unsigned int *)dbuf)[1] = HIGH_REPR_SATUI4;
        ((unsigned int *)dbuf)[2] = LOW_INSTR_SATUI4;
        ((unsigned int *)dbuf)[3] = LOW_REPR_SATUI4;
        ((unsigned int *)dbuf)[4] = NULLUI4;
        ((unsigned int *)dbuf)[5] = 1000;
      }
      else if (pixelType == SignedWord) {
        dbuf = (void *)CPLMalloc(sizeof(short) * 6);

        ((short *)dbuf)[0] = HIGH_INSTR_SAT2;
        ((short *)dbuf)[1] = HIGH_REPR_SAT2;
        ((short *)dbuf)[2] = LOW_INSTR_SAT2;
        ((short *)dbuf)[3] = LOW_REPR_SAT2;
        ((short *)dbuf)[4] = NULL2;
        ((short *)dbuf)[5] = 1000;
      }
      else if (pixelType == UnsignedWord) {
        dbuf = (void *)CPLMalloc(sizeof(unsigned short) * 6);

        ((unsigned short *)dbuf)[0] = HIGH_INSTR_SATU2;
        ((unsigned short *)dbuf)[1] = HIGH_REPR_SATU2;
        ((unsigned short *)dbuf)[2] = LOW_INSTR_SATU2;
        ((unsigned short *)dbuf)[3] = LOW_REPR_SATU2;
        ((unsigned short *)dbuf)[4] = NULLU2;
        ((unsigned short *)dbuf)[5] = 1000;
      }
      else if (pixelType == SignedByte) {
        dbuf = (void *)CPLMalloc(sizeof(char) * 6);

        ((char *)dbuf)[0] = HIGH_INSTR_SATS1;
        ((char *)dbuf)[1] = HIGH_REPR_SATS1;
        ((char *)dbuf)[2] = LOW_INSTR_SATS1;
        ((char *)dbuf)[3] = LOW_REPR_SATS1;
        ((char *)dbuf)[4] = NULLS1;
        ((char *)dbuf)[5] = 50;
      }
      else if (pixelType == UnsignedByte) {
        dbuf = (void *)CPLMalloc(sizeof(unsigned char) * 6);

        ((unsigned char *)dbuf)[0] = HIGH_INSTR_SAT1;
        ((unsigned char *)dbuf)[1] = HIGH_REPR_SAT1;
        ((unsigned char *)dbuf)[2] = LOW_INSTR_SAT1;
        ((unsigned char *)dbuf)[3] = LOW_REPR_SAT1;
        ((unsigned char *)dbuf)[4] = NULL1;
        ((unsigned char *)dbuf)[5] = 50;
      }

      localBrick = new Brick(6, 1, 1, pixelType);
      localBrick->SetBasePosition(1, 1, 1);

      GDALRasterBand *poBand = dataset->GetRasterBand(1);
      CPLErr err = poBand->RasterIO(GF_Write, 0, 0,
                                              6, 1,
                                              dbuf,
                                              6, 1,
                                              IsisPixelToGdal(pixelType),
                                              0, 0);
    }
    dataset->Close();
  }
}