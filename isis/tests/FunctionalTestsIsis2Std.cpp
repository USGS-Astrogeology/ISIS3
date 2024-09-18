#include "gmock/gmock.h"

#include <memory>

#include <QFile>
#include <QString>
#include <QVector>

#include "isis2std.h"

#include "Cube.h"
#include "std2isis.h"
#include "TempFixtures.h"
#include "Histogram.h"
#include "LineManager.h"
#include "Portal.h"
#include "SpecialPixel.h"
#include "Statistics.h"

using namespace Isis;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/isis2std.xml").expanded());
static QString STD2ISIS_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/std2isis.xml").expanded());

class IsisTruthCube : public TempTestingFiles {
    protected:
      Cube inputCube;
      QString inputCubeFilename;
      int chunkSize;

      void SetUp() override {
        inputCubeFilename = tempDir.path() + "/test_input.cub";
        inputCube.setDimensions(128, 128, 1);
        inputCube.create(inputCubeFilename);

        chunkSize = inputCube.lineCount() / 8.0;

        int pixVal = 0;
        LineManager lineWriter(inputCube);
        lineWriter.begin();
        // Write an integer gradient
        for (int line = 0; line < chunkSize; line++) {
          for (int sample = 0; sample < lineWriter.size(); sample++) {
            lineWriter[sample] = pixVal++;
          }
          inputCube.write(lineWriter);
          lineWriter++;
        }

        // Write VERY big negative and positive numbers
        double floatScale = ((double)VALID_MAX4 - (double)VALID_MIN4) / (chunkSize - 1);
        for (int line = 0; line < chunkSize; line++) {
          for (int sample = 0; sample < lineWriter.size(); sample++) {
            lineWriter[sample] = VALID_MIN4 + line * floatScale;
          }
          inputCube.write(lineWriter);
          lineWriter++;
        }

        // Write VERY small negative and positive numbers
        double tinyStart = -1e-20;
        double tinyStop = 1e-20;
        double tinyScale = (tinyStop - tinyStart) / (chunkSize - 1);
        for (int line = 0; line < chunkSize; line++) {
          for (int sample = 0; sample < lineWriter.size(); sample++) {
            lineWriter[sample] = tinyStart + line * tinyScale;
          }
          inputCube.write(lineWriter);
          lineWriter++;
        }

        // Write all of the special pixel values
        for (int line = 0; line < chunkSize; line++) {
          for (int sample = 0; sample < lineWriter.size(); sample++) {
            lineWriter[sample] = Lrs;
          }
          inputCube.write(lineWriter);
          lineWriter++;
        }
        for (int line = 0; line < chunkSize; line++) {
          for (int sample = 0; sample < lineWriter.size(); sample++) {
            lineWriter[sample] = Lis;
          }
          inputCube.write(lineWriter);
          lineWriter++;
        }
        for (int line = 0; line < chunkSize; line++) {
          for (int sample = 0; sample < lineWriter.size(); sample++) {
            lineWriter[sample] = His;
          }
          inputCube.write(lineWriter);
          lineWriter++;
        }
        for (int line = 0; line < chunkSize; line++) {
          for (int sample = 0; sample < lineWriter.size(); sample++) {
            lineWriter[sample] = Hrs;
          }
          inputCube.write(lineWriter);
          lineWriter++;
        }

        inputCube.close();
      }
  };


class SmallARGBCube : public TempTestingFiles {
    protected:
      Cube inputCube;
      QString inputCubeFilename;

      void SetUp() override {
        inputCubeFilename = tempDir.path() + "/test_input.cub";
        inputCube.setDimensions(10, 12, 4);
        inputCube.create(inputCubeFilename);

        LineManager lineWriter(inputCube);
        for (lineWriter.begin(); !lineWriter.end(); lineWriter++) {
          // Fancy integer stuff to write
          // 1 to band 1
          // 1 and 2 to band 2
          // 1, 2, and 3 to band 3
          // 1, 2, 3, and 4 to band 4
          double pixVal = 1 + (lineWriter.Line() - 1) / (inputCube.lineCount() / lineWriter.Band());
          for (int sample = 0; sample < lineWriter.size(); sample++) {
            lineWriter[sample] = pixVal;
          }
          inputCube.write(lineWriter);
        }
        inputCube.close();
      }
  };


// Helper function to re-ingest and check the truth cube
void checkReingestedCube(QString tempDirPath, QString exportedFile, int chunkSize,
                         double min, double mid, double max,
                         double lowSpecial, double hiSpecial, double nullSpecial,
                         double tolerance = 0) {

  QString reingestCubeFilename = tempDirPath + "/test_output.cub";
  QVector<QString> reingestArgs = {"from=" + exportedFile,
                                   "to=" + reingestCubeFilename};

  UserInterface reingestOptions(STD2ISIS_XML, reingestArgs);
  try {
    std2isis(reingestOptions);
  }
  catch (IException &e) {
    FAIL() << "Unable to reingest image: " << e.what() << std::endl;
  }
  Cube reingestCube(reingestCubeFilename.toStdString());

  Portal checkReader(reingestCube.sampleCount(), chunkSize, reingestCube.pixelType());

  // Check gradient
  checkReader.SetPosition(1, 1, 1);
  reingestCube.read(checkReader);

  Statistics gradientStats;
  gradientStats.AddData(checkReader.DoubleBuffer(), checkReader.size());
  EXPECT_EQ(gradientStats.Minimum(), mid);
  EXPECT_EQ(gradientStats.Maximum(), mid);

  // Check very large
  double newLargeSlope = (max - min) / (double)(chunkSize - 1);
  Portal largeReader(reingestCube.sampleCount(), 1, reingestCube.pixelType());
  for (int line = 0; line < chunkSize; line++) {
    largeReader.SetPosition(1, chunkSize+line+1, 1);
    reingestCube.read(largeReader);

    Statistics largeStats;
    largeStats.AddData(largeReader.DoubleBuffer(), largeReader.size());
    if (tolerance > 0) {
      EXPECT_NEAR(largeStats.Minimum(), min + (int)(line * newLargeSlope), tolerance);
      EXPECT_NEAR(largeStats.Maximum(), min + (int)(line * newLargeSlope), tolerance);
    }
    else {
      EXPECT_EQ(largeStats.Minimum(), min + (int)(line * newLargeSlope));
      EXPECT_EQ(largeStats.Maximum(), min + (int)(line * newLargeSlope));
    }
  }

  // Check very small
  checkReader.SetPosition(1, chunkSize*2+1, 1);
  reingestCube.read(checkReader);

  Statistics smallStats;
  smallStats.AddData(checkReader.DoubleBuffer(), checkReader.size());
  EXPECT_EQ(smallStats.Minimum(), mid);
  EXPECT_EQ(smallStats.Maximum(), mid);

  // Check special pixels
  checkReader.SetPosition(1, chunkSize*3+1, 1);
  reingestCube.read(checkReader);

  Statistics lrsStats;
  lrsStats.AddData(checkReader.DoubleBuffer(), checkReader.size());
  EXPECT_EQ(lrsStats.Minimum(), lowSpecial);
  EXPECT_EQ(lrsStats.Maximum(), lowSpecial);

  checkReader.SetPosition(1, chunkSize*4+1, 1);
  reingestCube.read(checkReader);

  Statistics lisStats;
  lisStats.AddData(checkReader.DoubleBuffer(), checkReader.size());
  EXPECT_EQ(lisStats.Minimum(), lowSpecial);
  EXPECT_EQ(lisStats.Maximum(), lowSpecial);

  checkReader.SetPosition(1, chunkSize*5+1, 1);
  reingestCube.read(checkReader);

  Statistics hisStats;
  hisStats.AddData(checkReader.DoubleBuffer(), checkReader.size());
  EXPECT_EQ(hisStats.Minimum(), hiSpecial);
  EXPECT_EQ(hisStats.Maximum(), hiSpecial);

  checkReader.SetPosition(1, chunkSize*6+1, 1);
  reingestCube.read(checkReader);

  Statistics hrsStats;
  hrsStats.AddData(checkReader.DoubleBuffer(), checkReader.size());
  EXPECT_EQ(hrsStats.Minimum(), hiSpecial);
  EXPECT_EQ(hrsStats.Maximum(), hiSpecial);

  checkReader.SetPosition(1, chunkSize*7+1, 1);
  reingestCube.read(checkReader);

  Statistics nullStats;
  nullStats.AddData(checkReader.DoubleBuffer(), checkReader.size());
  EXPECT_EQ(nullStats.Minimum(), nullSpecial);
  EXPECT_EQ(nullStats.Maximum(), nullSpecial);
}

//////////////////
// Bitmap Tests //
//////////////////

TEST_F(IsisTruthCube, FunctionalTestsIsis2StdBmp) {
  QString outputBmpFilename = tempDir.path() + "/test_output.bmp";
  QVector<QString> args = {"from=" + inputCubeFilename,
                           "to=" + outputBmpFilename,
                           "mode=grayscale",
                           "format=bmp",
                           "stretch=linear"};

  UserInterface options(APP_XML, args);
  try {
    isis2std(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to translate image: " << e.what() << std::endl;
  }

  checkReingestedCube(tempDir.path(), outputBmpFilename, chunkSize, 1, 127, 255, 1, 255, 0);
}

////////////////
// JPEG Tests //
////////////////

TEST_F(IsisTruthCube, FunctionalTestsIsis2StdJpeg) {
  QString outputJpgFilename = tempDir.path() + "/test_output.jpg";
  QVector<QString> args = {"from=" + inputCubeFilename,
                           "to=" + outputJpgFilename,
                           "mode=grayscale",
                           "format=jpeg",
                           "stretch=linear"};

  UserInterface options(APP_XML, args);
  try {
    isis2std(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to translate image: " << e.what() << std::endl;
  }

  checkReingestedCube(tempDir.path(), outputJpgFilename, chunkSize, 1, 127, 255, 1, 255, 0);
}

/////////////////////
// JPEG 2000 Tests //
/////////////////////

TEST_F(IsisTruthCube, FunctionalTestsIsis2StdJpeg2KGray) {
  QString outputJp2Filename = tempDir.path() + "/test_output.jp2";
  QVector<QString> args = {"from=" + inputCubeFilename,
                           "to=" + outputJp2Filename,
                           "mode=grayscale",
                           "format=jp2",
                           "stretch=linear"};

  UserInterface options(APP_XML, args);
  try {
    isis2std(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to translate image: " << e.what() << std::endl;
  }

  checkReingestedCube(tempDir.path(), outputJp2Filename, chunkSize, 1, 127, 255, 1, 255, 0);
}


TEST_F(SmallARGBCube, FunctionalTestsIsis2StdJpeg2KRGB) {
  QString outputJp2Filename = tempDir.path() + "/test_output.jp2";
  QVector<QString> args = {"red=" + inputCubeFilename + "+1",
                           "green=" + inputCubeFilename + "+2",
                           "blue=" + inputCubeFilename + "+3",
                           "to=" + outputJp2Filename,
                           "mode=rgb",
                           "format=jp2",
                           "stretch=linear"};

  UserInterface options(APP_XML, args);
  try {
    isis2std(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to translate image: " << e.what() << std::endl;
  }

  QString reingestCubeFilename = tempDir.path() + "/test_output.cub";
  QVector<QString> reingestArgs = {"from=" + outputJp2Filename,
                                   "to=" + reingestCubeFilename};

  UserInterface reingestOptions(STD2ISIS_XML, reingestArgs);
  try {
    std2isis(reingestOptions);
  }
  catch (IException &e) {
    FAIL() << "Unable to reingest image: " << e.what() << std::endl;
  }

  Cube reingestCube(reingestCubeFilename.toStdString());
  inputCube.open(inputCubeFilename);
  int pixelsPerBand = inputCube.lineCount() * inputCube.sampleCount();

  std::unique_ptr<Histogram> redHist(reingestCube.histogram(1));
  std::unique_ptr<Histogram> greenHist(reingestCube.histogram(2));
  std::unique_ptr<Histogram> blueHist(reingestCube.histogram(3));

  EXPECT_EQ(redHist->Maximum(), 1);
  EXPECT_EQ(redHist->Minimum(), 1);
  EXPECT_EQ(redHist->ValidPixels(), pixelsPerBand);
  EXPECT_EQ(redHist->TotalPixels(), pixelsPerBand);
  EXPECT_EQ(redHist->Average(), 1);
  EXPECT_EQ(redHist->StandardDeviation(), 0);

  EXPECT_EQ(greenHist->Maximum(), 255);
  EXPECT_EQ(greenHist->Minimum(), 1);
  EXPECT_EQ(greenHist->ValidPixels(), pixelsPerBand);
  EXPECT_EQ(greenHist->TotalPixels(), pixelsPerBand);
  EXPECT_EQ(greenHist->Average(), 128);
  EXPECT_NEAR(greenHist->StandardDeviation(), 127.532, 0.001);

  EXPECT_EQ(blueHist->Maximum(), 255);
  EXPECT_EQ(blueHist->Minimum(), 1);
  EXPECT_EQ(blueHist->ValidPixels(), pixelsPerBand);
  EXPECT_EQ(blueHist->TotalPixels(), pixelsPerBand);
  EXPECT_EQ(blueHist->Average(), 128);
  EXPECT_NEAR(blueHist->StandardDeviation(), 104.13, 0.001);
}


TEST_F(SmallARGBCube, FunctionalTestsIsis2StdJpeg2KARGB) {
  QString outputJp2Filename = tempDir.path() + "/test_output.jp2";
  QVector<QString> args = {"red=" + inputCubeFilename + "+1",
                           "green=" + inputCubeFilename + "+2",
                           "blue=" + inputCubeFilename + "+3",
                           "alpha=" + inputCubeFilename + "+4",
                           "to=" + outputJp2Filename,
                           "mode=argb",
                           "format=jp2",
                           "stretch=linear"};

  UserInterface options(APP_XML, args);
  try {
    isis2std(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to translate image: " << e.what() << std::endl;
  }

  QString reingestCubeFilename = tempDir.path() + "/test_output.cub";
  QVector<QString> reingestArgs = {"from=" + outputJp2Filename,
                                   "to=" + reingestCubeFilename};

  UserInterface reingestOptions(STD2ISIS_XML, reingestArgs);
  try {
    std2isis(reingestOptions);
  }
  catch (IException &e) {
    FAIL() << "Unable to reingest image: " << e.what() << std::endl;
  }

  Cube reingestCube(reingestCubeFilename.toStdString());
  inputCube.open(inputCubeFilename);
  int pixelsPerBand = inputCube.lineCount() * inputCube.sampleCount();

  std::unique_ptr<Histogram> redHist(reingestCube.histogram(1));
  std::unique_ptr<Histogram> greenHist(reingestCube.histogram(2));
  std::unique_ptr<Histogram> blueHist(reingestCube.histogram(3));
  std::unique_ptr<Histogram> alphaHist(reingestCube.histogram(4));

  EXPECT_EQ(redHist->Maximum(), 1);
  EXPECT_EQ(redHist->Minimum(), 1);
  EXPECT_EQ(redHist->ValidPixels(), pixelsPerBand);
  EXPECT_EQ(redHist->TotalPixels(), pixelsPerBand);
  EXPECT_EQ(redHist->Average(), 1);
  EXPECT_EQ(redHist->StandardDeviation(), 0);

  EXPECT_EQ(greenHist->Maximum(), 255);
  EXPECT_EQ(greenHist->Minimum(), 1);
  EXPECT_EQ(greenHist->ValidPixels(), pixelsPerBand);
  EXPECT_EQ(greenHist->TotalPixels(), pixelsPerBand);
  EXPECT_EQ(greenHist->Average(), 128);
  EXPECT_NEAR(greenHist->StandardDeviation(), 127.532, 0.001);

  EXPECT_EQ(blueHist->Maximum(), 255);
  EXPECT_EQ(blueHist->Minimum(), 1);
  EXPECT_EQ(blueHist->ValidPixels(), pixelsPerBand);
  EXPECT_EQ(blueHist->TotalPixels(), pixelsPerBand);
  EXPECT_EQ(blueHist->Average(), 128);
  EXPECT_NEAR(blueHist->StandardDeviation(), 104.13, 0.001);

  EXPECT_EQ(alphaHist->Maximum(), 255);
  EXPECT_EQ(alphaHist->Minimum(), 1);
  EXPECT_EQ(alphaHist->ValidPixels(), pixelsPerBand);
  EXPECT_EQ(alphaHist->TotalPixels(), pixelsPerBand);
  EXPECT_EQ(alphaHist->Average(), 127.75);
  EXPECT_NEAR(alphaHist->StandardDeviation(), 95.094, 0.001);
}


TEST_F(IsisTruthCube, FunctionalTestsIsis2StdJpeg2KU16) {
  QString outputJp2Filename = tempDir.path() + "/test_output.jp2";
  QVector<QString> args = {"from=" + inputCubeFilename,
                           "to=" + outputJp2Filename,
                           "mode=grayscale",
                           "format=jp2",
                           "bittype=u16bit",
                           "stretch=linear"};

  UserInterface options(APP_XML, args);
  try {
    isis2std(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to translate image: " << e.what() << std::endl;
  }

  checkReingestedCube(tempDir.path(), outputJp2Filename, chunkSize, 1, 32767, 65535, 1, 65535, 0);
}


TEST_F(IsisTruthCube, FunctionalTestsIsis2StdJpeg2KS16) {
  QString outputJp2Filename = tempDir.path() + "/test_output.jp2";
  QVector<QString> args = {"from=" + inputCubeFilename,
                           "to=" + outputJp2Filename,
                           "mode=grayscale",
                           "format=jp2",
                           "bittype=s16bit",
                           "stretch=linear"};

  UserInterface options(APP_XML, args);
  try {
    isis2std(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to translate image: " << e.what() << std::endl;
  }

  checkReingestedCube(tempDir.path(), outputJp2Filename, chunkSize, -32767, 0, 32767, -32767, 32767, -32768, 1);
}

///////////////
// PNG Tests //
///////////////

TEST_F(IsisTruthCube, FunctionalTestsIsis2StdPNG) {
  QString outputPNGFilename = tempDir.path() + "/test_output.png";
  QVector<QString> args = {"from=" + inputCubeFilename,
                           "to=" + outputPNGFilename,
                           "mode=grayscale",
                           "format=png",
                           "stretch=linear"};

  UserInterface options(APP_XML, args);
  try {
    isis2std(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to translate image: " << e.what() << std::endl;
  }

  checkReingestedCube(tempDir.path(), outputPNGFilename, chunkSize, 1, 127, 255, 1, 255, 0);
}


TEST_F(SmallARGBCube, FunctionalTestsIsis2StdPNGRGB) {
  QString outputPNGFilename = tempDir.path() + "/test_output.png";
  QVector<QString> args = {"red=" + inputCubeFilename + "+1",
                           "green=" + inputCubeFilename + "+2",
                           "blue=" + inputCubeFilename + "+3",
                           "to=" + outputPNGFilename,
                           "mode=rgb",
                           "format=png",
                           "stretch=linear"};

  UserInterface options(APP_XML, args);
  try {
    isis2std(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to translate image: " << e.what() << std::endl;
  }

  QString reingestCubeFilename = tempDir.path() + "/test_output.cub";
  QVector<QString> reingestArgs = {"from=" + outputPNGFilename,
                                   "to=" + reingestCubeFilename};

  UserInterface reingestOptions(STD2ISIS_XML, reingestArgs);
  try {
    std2isis(reingestOptions);
  }
  catch (IException &e) {
    FAIL() << "Unable to reingest image: " << e.what() << std::endl;
  }

  Cube reingestCube(reingestCubeFilename.toStdString());
  inputCube.open(inputCubeFilename);
  int pixelsPerBand = inputCube.lineCount() * inputCube.sampleCount();

  std::unique_ptr<Histogram> redHist(reingestCube.histogram(1));
  std::unique_ptr<Histogram> greenHist(reingestCube.histogram(2));
  std::unique_ptr<Histogram> blueHist(reingestCube.histogram(3));

  EXPECT_EQ(redHist->Maximum(), 1);
  EXPECT_EQ(redHist->Minimum(), 1);
  EXPECT_EQ(redHist->ValidPixels(), pixelsPerBand);
  EXPECT_EQ(redHist->TotalPixels(), pixelsPerBand);
  EXPECT_EQ(redHist->Average(), 1);
  EXPECT_EQ(redHist->StandardDeviation(), 0);

  EXPECT_EQ(greenHist->Maximum(), 255);
  EXPECT_EQ(greenHist->Minimum(), 1);
  EXPECT_EQ(greenHist->ValidPixels(), pixelsPerBand);
  EXPECT_EQ(greenHist->TotalPixels(), pixelsPerBand);
  EXPECT_EQ(greenHist->Average(), 128);
  EXPECT_NEAR(greenHist->StandardDeviation(), 127.532, 0.001);

  EXPECT_EQ(blueHist->Maximum(), 255);
  EXPECT_EQ(blueHist->Minimum(), 1);
  EXPECT_EQ(blueHist->ValidPixels(), pixelsPerBand);
  EXPECT_EQ(blueHist->TotalPixels(), pixelsPerBand);
  EXPECT_EQ(blueHist->Average(), 128);
  EXPECT_NEAR(blueHist->StandardDeviation(), 104.13, 0.001);
}

////////////////
// TIFF Tests //
////////////////

TEST_F(IsisTruthCube, FunctionalTestsIsis2StdTIFFGray) {
  QString outputTiffFilename = tempDir.path() + "/test_output.tif";
  QVector<QString> args = {"from=" + inputCubeFilename,
                           "to=" + outputTiffFilename,
                           "mode=grayscale",
                           "format=tiff",
                           "stretch=linear"};

  UserInterface options(APP_XML, args);
  try {
    isis2std(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to translate image: " << e.what() << std::endl;
  }

  checkReingestedCube(tempDir.path(), outputTiffFilename, chunkSize, 1, 127, 255, 1, 255, 0);
}

TEST_F(IsisTruthCube, FunctionalTestsIsis2StdTIFFPackBits) {
  QString outputTiffFilename = tempDir.path() + "/test_output.tif";
  QVector<QString> args = {"from=" + inputCubeFilename,
                           "to=" + outputTiffFilename,
                           "mode=grayscale",
                           "format=tiff",
                           "stretch=linear",
                           "compression=packbits"};

  UserInterface options(APP_XML, args);
  try {
    isis2std(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to translate image: " << e.what() << std::endl;
  }

  checkReingestedCube(tempDir.path(), outputTiffFilename, chunkSize, 1, 127, 255, 1, 255, 0);
}

TEST_F(IsisTruthCube, FunctionalTestsIsis2StdTIFFLZW) {
  QString outputTiffFilename = tempDir.path() + "/test_output.tif";
  QVector<QString> args = {"from=" + inputCubeFilename,
                           "to=" + outputTiffFilename,
                           "mode=grayscale",
                           "format=tiff",
                           "stretch=linear",
                           "compression=lzw"};

  UserInterface options(APP_XML, args);
  try {
    isis2std(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to translate image: " << e.what() << std::endl;
  }

  checkReingestedCube(tempDir.path(), outputTiffFilename, chunkSize, 1, 127, 255, 1, 255, 0);
}

TEST_F(IsisTruthCube, FunctionalTestsIsis2StdTIFFDeflate) {
  QString outputTiffFilename = tempDir.path() + "/test_output.tif";
  QVector<QString> args = {"from=" + inputCubeFilename,
                           "to=" + outputTiffFilename,
                           "mode=grayscale",
                           "format=tiff",
                           "stretch=linear",
                           "compression=deflate"};

  UserInterface options(APP_XML, args);
  try {
    isis2std(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to translate image: " << e.what() << std::endl;
  }

  checkReingestedCube(tempDir.path(), outputTiffFilename, chunkSize, 1, 127, 255, 1, 255, 0);
}


TEST_F(SmallARGBCube, FunctionalTestsIsis2StdTIFFRGB) {
  QString outputTiffFilename = tempDir.path() + "/test_output.tif";
  QVector<QString> args = {"red=" + inputCubeFilename + "+1",
                           "green=" + inputCubeFilename + "+2",
                           "blue=" + inputCubeFilename + "+3",
                           "to=" + outputTiffFilename,
                           "mode=rgb",
                           "format=tiff",
                           "stretch=linear"};

  UserInterface options(APP_XML, args);
  try {
    isis2std(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to translate image: " << e.what() << std::endl;
  }

  QString reingestCubeFilename = tempDir.path() + "/test_output.cub";
  QVector<QString> reingestArgs = {"from=" + outputTiffFilename,
                                   "to=" + reingestCubeFilename};

  UserInterface reingestOptions(STD2ISIS_XML, reingestArgs);
  try {
    std2isis(reingestOptions);
  }
  catch (IException &e) {
    FAIL() << "Unable to reingest image: " << e.what() << std::endl;
  }

  Cube reingestCube(reingestCubeFilename.toStdString());
  inputCube.open(inputCubeFilename);
  int pixelsPerBand = inputCube.lineCount() * inputCube.sampleCount();

  std::unique_ptr<Histogram> redHist(reingestCube.histogram(1));
  std::unique_ptr<Histogram> greenHist(reingestCube.histogram(2));
  std::unique_ptr<Histogram> blueHist(reingestCube.histogram(3));

  EXPECT_EQ(redHist->Maximum(), 1);
  EXPECT_EQ(redHist->Minimum(), 1);
  EXPECT_EQ(redHist->ValidPixels(), pixelsPerBand);
  EXPECT_EQ(redHist->TotalPixels(), pixelsPerBand);
  EXPECT_EQ(redHist->Average(), 1);
  EXPECT_EQ(redHist->StandardDeviation(), 0);

  EXPECT_EQ(greenHist->Maximum(), 255);
  EXPECT_EQ(greenHist->Minimum(), 1);
  EXPECT_EQ(greenHist->ValidPixels(), pixelsPerBand);
  EXPECT_EQ(greenHist->TotalPixels(), pixelsPerBand);
  EXPECT_EQ(greenHist->Average(), 128);
  EXPECT_NEAR(greenHist->StandardDeviation(), 127.532, 0.001);

  EXPECT_EQ(blueHist->Maximum(), 255);
  EXPECT_EQ(blueHist->Minimum(), 1);
  EXPECT_EQ(blueHist->ValidPixels(), pixelsPerBand);
  EXPECT_EQ(blueHist->TotalPixels(), pixelsPerBand);
  EXPECT_EQ(blueHist->Average(), 128);
  EXPECT_NEAR(blueHist->StandardDeviation(), 104.13, 0.001);
}


TEST_F(IsisTruthCube, FunctionalTestsIsis2StdTIFFU16) {
  QString outputTiffFilename = tempDir.path() + "/test_output.tif";
  QVector<QString> args = {"from=" + inputCubeFilename,
                           "to=" + outputTiffFilename,
                           "mode=grayscale",
                           "format=tiff",
                           "bittype=u16bit",
                           "stretch=linear"};

  UserInterface options(APP_XML, args);
  try {
    isis2std(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to translate image: " << e.what() << std::endl;
  }

  // std2isis smashes the 16-bit output back down to 8-bit on re-ingest
  checkReingestedCube(tempDir.path(), outputTiffFilename, chunkSize, 0, 127, 255, 0, 255, 0);
}


TEST_F(IsisTruthCube, FunctionalTestsIsis2StdTIFFS16) {
  QString outputTiffFilename = tempDir.path() + "/test_output.tif";
  QVector<QString> args = {"from=" + inputCubeFilename,
                           "to=" + outputTiffFilename,
                           "mode=grayscale",
                           "format=tiff",
                           "bittype=s16bit",
                           "stretch=linear"};

  UserInterface options(APP_XML, args);
  try {
    isis2std(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to translate image: " << e.what() << std::endl;
  }

  QString reingestCubeFilename = tempDir.path() + "/test_output.cub";
  QVector<QString> reingestArgs = {"from=" + outputTiffFilename,
                                   "to=" + reingestCubeFilename};

  UserInterface reingestOptions(STD2ISIS_XML, reingestArgs);
  try {
    std2isis(reingestOptions);
  }
  catch (IException &e) {
    FAIL() << "Unable to reingest image: " << e.what() << std::endl;
  }

  // std2isis smashes the 16-bit output back down to 8-bit on re-ingest
  // this results in negative values underflowing to large positive values
  Cube reingestCube(reingestCubeFilename.toStdString());

  Portal checkReader(reingestCube.sampleCount(), chunkSize, reingestCube.pixelType());

  // Check gradient
  checkReader.SetPosition(1, 1, 1);
  reingestCube.read(checkReader);

  Statistics gradientStats;
  gradientStats.AddData(checkReader.DoubleBuffer(), checkReader.size());
  EXPECT_EQ(gradientStats.Minimum(), 0);
  EXPECT_EQ(gradientStats.Maximum(), 0);

  // Check very large
  // This wraps around so the first 8 lines are 128-247
  // and the last 8 lines are 8-127
  double newLargeSlope = (255 - 1) / (double)(chunkSize - 1);
  Portal largeReader(reingestCube.sampleCount(), 1, reingestCube.pixelType());
  for (int line = 0; line < chunkSize; line++) {
    largeReader.SetPosition(1, chunkSize+line+1, 1);
    reingestCube.read(largeReader);

    Statistics largeStats;
    largeStats.AddData(largeReader.DoubleBuffer(), largeReader.size());
    if (line == 0) {
      EXPECT_EQ(largeStats.Minimum(), 128);
      EXPECT_EQ(largeStats.Maximum(), 128);
    }
    else if (line < 8) {
      EXPECT_EQ(largeStats.Minimum(), 129 + (int)(line * newLargeSlope));
      EXPECT_EQ(largeStats.Maximum(), 129 + (int)(line * newLargeSlope));
    }
    else {
      EXPECT_EQ(largeStats.Minimum(), -127 + (int)(line * newLargeSlope));
      EXPECT_EQ(largeStats.Maximum(), -127 + (int)(line * newLargeSlope));
    }
  }

  // Check very small
  checkReader.SetPosition(1, chunkSize*2+1, 1);
  reingestCube.read(checkReader);

  Statistics smallStats;
  smallStats.AddData(checkReader.DoubleBuffer(), checkReader.size());
  EXPECT_EQ(smallStats.Minimum(), 0);
  EXPECT_EQ(smallStats.Maximum(), 0);

  // Check special pixels
  checkReader.SetPosition(1, chunkSize*3+1, 1);
  reingestCube.read(checkReader);

  Statistics lrsStats;
  lrsStats.AddData(checkReader.DoubleBuffer(), checkReader.size());
  EXPECT_EQ(lrsStats.Minimum(), 128);
  EXPECT_EQ(lrsStats.Maximum(), 128);

  checkReader.SetPosition(1, chunkSize*4+1, 1);
  reingestCube.read(checkReader);

  Statistics lisStats;
  lisStats.AddData(checkReader.DoubleBuffer(), checkReader.size());
  EXPECT_EQ(lisStats.Minimum(), 128);
  EXPECT_EQ(lisStats.Maximum(), 128);

  checkReader.SetPosition(1, chunkSize*5+1, 1);
  reingestCube.read(checkReader);

  Statistics hisStats;
  hisStats.AddData(checkReader.DoubleBuffer(), checkReader.size());
  EXPECT_EQ(hisStats.Minimum(), 127);
  EXPECT_EQ(hisStats.Maximum(), 127);

  checkReader.SetPosition(1, chunkSize*6+1, 1);
  reingestCube.read(checkReader);

  Statistics hrsStats;
  hrsStats.AddData(checkReader.DoubleBuffer(), checkReader.size());
  EXPECT_EQ(hrsStats.Minimum(), 127);
  EXPECT_EQ(hrsStats.Maximum(), 127);

  checkReader.SetPosition(1, chunkSize*7+1, 1);
  reingestCube.read(checkReader);

  Statistics nullStats;
  nullStats.AddData(checkReader.DoubleBuffer(), checkReader.size());
  EXPECT_EQ(nullStats.Minimum(), 128);
  EXPECT_EQ(nullStats.Maximum(), 128);
}

TEST_F(IsisTruthCube, FunctionalTestsIsis2StdTIFFWorldFile) {
  std::istringstream labelStrm(R"(
  Group = Mapping
    ProjectionName     = Orthographic
    CenterLongitude    = 180.0
    TargetName         = Moon
    EquatorialRadius   = 1737400.0 <meters>
    PolarRadius        = 1737400.0 <meters>
    LatitudeType       = Planetocentric
    LongitudeDirection = PositiveEast
    LongitudeDomain    = 360
    MinimumLatitude    = -90.0
    MaximumLatitude    = 90.0
    MinimumLongitude   = 90.0
    MaximumLongitude   = 270.0
    UpperLeftCornerX   = -3866227.1790791 <meters>
    UpperLeftCornerY   = 2895879.9655063 <meters>
    PixelResolution    = 30323.350424149 <meters>
    Scale              = 1.0 <pixels/degree>
    CenterLatitude     = 0.0
  End_Group
  )");

  Pvl inMap;
  labelStrm >> inMap;
  PvlGroup &mapGrp = inMap.findGroup("Mapping", Pvl::Traverse);
  inputCube.open(inputCubeFilename, "rw");
  inputCube.putGroup(mapGrp);
  inputCube.close();

  QString outputTiffFilename = tempDir.path() + "/test_output.tif";
  QVector<QString> args = {"from=" + inputCubeFilename,
                           "to=" + outputTiffFilename,
                           "mode=grayscale",
                           "format=tiff",
                           "stretch=linear"};

  UserInterface options(APP_XML, args);
  try {
    isis2std(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to translate image: " << e.what() << std::endl;
  }

  QString worldFileName = tempDir.path() + "/test_output.tfw";
  QFile worldFile(worldFileName);
  worldFile.open(QIODevice::ReadOnly | QIODevice::Text);

  double pixResolution = (double)mapGrp.findKeyword("PixelResolution");
  double upperLeftX = (double)mapGrp.findKeyword("UpperLeftCornerX");
  double upperLeftY = (double)mapGrp.findKeyword("UpperLeftCornerY");

  QString worldFileLine = worldFile.readLine();
  EXPECT_NEAR(worldFileLine.toDouble(), pixResolution, 0.001);

  worldFileLine = worldFile.readLine();
  EXPECT_NEAR(worldFileLine.toDouble(), 0, 0.001);

  worldFileLine = worldFile.readLine();
  EXPECT_NEAR(worldFileLine.toDouble(), 0, 0.001);

  worldFileLine = worldFile.readLine();
  EXPECT_NEAR(worldFileLine.toDouble(), -1.0 * pixResolution, 0.001);

  worldFileLine = worldFile.readLine();
  EXPECT_NEAR(worldFileLine.toDouble(), upperLeftX + 0.5 * pixResolution, 0.001);

  worldFileLine = worldFile.readLine();
  EXPECT_NEAR(worldFileLine.toDouble(), upperLeftY - 0.5 * pixResolution, 0.001);
}

/////////////////////////
// Miscellaneous Tests //
/////////////////////////

TEST_F(SmallARGBCube, FunctionalTestsIsis2StdManualStretch) {
  QString outputPNGFilename = tempDir.path() + "/test_output.png";
  QVector<QString> args = {"from=" + inputCubeFilename + "+4",
                           "to=" + outputPNGFilename,
                           "mode=grayscale",
                           "format=png",
                           "stretch=manual",
                           "minimum=2",
                           "maximum=4"};

  UserInterface options(APP_XML, args);
  try {
    isis2std(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to translate image: " << e.what() << std::endl;
  }

  QString reingestCubeFilename = tempDir.path() + "/test_output.cub";
  QVector<QString> reingestArgs = {"from=" + outputPNGFilename,
                                   "to=" + reingestCubeFilename};

  UserInterface reingestOptions(STD2ISIS_XML, reingestArgs);
  try {
    std2isis(reingestOptions);
  }
  catch (IException &e) {
    FAIL() << "Unable to reingest image: " << e.what() << std::endl;
  }

  Cube reingestCube(reingestCubeFilename.toStdString());
  inputCube.open(inputCubeFilename);
  int pixelsPerBand = inputCube.lineCount() * inputCube.sampleCount();

  std::unique_ptr<Histogram> grayHist(reingestCube.histogram(1));

  EXPECT_EQ(grayHist->Maximum(), 255);
  EXPECT_EQ(grayHist->Minimum(), 1);
  EXPECT_EQ(grayHist->ValidPixels(), pixelsPerBand);
  EXPECT_EQ(grayHist->TotalPixels(), pixelsPerBand);
  EXPECT_EQ(grayHist->Average(), 96.25);
  EXPECT_NEAR(grayHist->StandardDeviation(), 105.744, 0.001);
}


TEST_F(SmallARGBCube, FunctionalTestsIsis2StdManualStretchRGB) {
  QString outputPNGFilename = tempDir.path() + "/test_output.png";
  QVector<QString> args = {"red=" + inputCubeFilename + "+4",
                           "green=" + inputCubeFilename + "+4",
                           "blue=" + inputCubeFilename + "+4",
                           "to=" + outputPNGFilename,
                           "mode=rgb",
                           "format=png",
                           "stretch=manual",
                           "rmin=0",
                           "rmax=2",
                           "gmin=1",
                           "gmax=3",
                           "bmin=2",
                           "bmax=6"};

  UserInterface options(APP_XML, args);
  try {
    isis2std(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to translate image: " << e.what() << std::endl;
  }

  QString reingestCubeFilename = tempDir.path() + "/test_output.cub";
  QVector<QString> reingestArgs = {"from=" + outputPNGFilename,
                                   "to=" + reingestCubeFilename};

  UserInterface reingestOptions(STD2ISIS_XML, reingestArgs);
  try {
    std2isis(reingestOptions);
  }
  catch (IException &e) {
    FAIL() << "Unable to reingest image: " << e.what() << std::endl;
  }

  Cube reingestCube(reingestCubeFilename.toStdString());
  inputCube.open(inputCubeFilename);
  int pixelsPerBand = inputCube.lineCount() * inputCube.sampleCount();

  std::unique_ptr<Histogram> redHist(reingestCube.histogram(1));
  std::unique_ptr<Histogram> greenHist(reingestCube.histogram(2));
  std::unique_ptr<Histogram> blueHist(reingestCube.histogram(3));

  EXPECT_EQ(redHist->Minimum(), 128);
  EXPECT_EQ(redHist->Maximum(), 255);
  EXPECT_EQ(redHist->ValidPixels(), pixelsPerBand);
  EXPECT_EQ(redHist->TotalPixels(), pixelsPerBand);
  EXPECT_EQ(redHist->Average(), 223.25);
  EXPECT_NEAR(redHist->StandardDeviation(), 55.223, 0.001);

  EXPECT_EQ(greenHist->Minimum(), 1);
  EXPECT_EQ(greenHist->Maximum(), 255);
  EXPECT_EQ(greenHist->ValidPixels(), pixelsPerBand);
  EXPECT_EQ(greenHist->TotalPixels(), pixelsPerBand);
  EXPECT_EQ(greenHist->Average(), 159.75);
  EXPECT_NEAR(greenHist->StandardDeviation(), 105.744, 0.001);

  EXPECT_EQ(blueHist->Minimum(), 1);
  EXPECT_EQ(blueHist->Maximum(), 128);
  EXPECT_EQ(blueHist->ValidPixels(), pixelsPerBand);
  EXPECT_EQ(blueHist->TotalPixels(), pixelsPerBand);
  EXPECT_EQ(blueHist->Average(), 48.5);
  EXPECT_NEAR(blueHist->StandardDeviation(), 52.835, 0.001);
}


TEST_F(IsisTruthCube, FunctionalTestsIsis2StdExtension) {
  QString outputBmpFilename = tempDir.path() + "/test_output";
  QVector<QString> args = {"from=" + inputCubeFilename,
                           "to=" + outputBmpFilename,
                           "mode=grayscale",
                           "format=bmp",
                           "stretch=linear"};

  UserInterface options(APP_XML, args);
  try {
    isis2std(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to translate image: " << e.what() << std::endl;
  }

  QFile outputFile(outputBmpFilename + ".bmp");
  EXPECT_TRUE(outputFile.exists());
}
