#include "hist.h"

#include <QTemporaryFile>
#include <QTextStream>
#include <QStringList>

#include "CubeFixtures.h"
#include "PvlGroup.h"
#include "TestUtilities.h"
#include "CSVReader.h"
#include "LineManager.h"

#include "gmock/gmock.h"

using namespace Isis;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/hist.xml").expanded());

TEST_F(SmallCube, FunctionalTestHistDefault) {
  QString outputFile = tempDir.path()+"/output.txt";
  QVector<QString> args = {"to=" + outputFile};
  UserInterface ui(APP_XML, args);

  hist(testCube, ui);

  CSVReader::CSVAxis csvLine;

  CSVReader header = CSVReader(outputFile,
                               false, 0, ':', false, true);

  // Validate the header information is correct
  csvLine = header.getRow(0);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Cube");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[1].trimmed(),testCube->fileName());

  csvLine = header.getRow(1);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Band");
  EXPECT_EQ(csvLine[1].toInt(), 10);

  csvLine = header.getRow(2);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Average");
  EXPECT_DOUBLE_EQ(csvLine[1].toDouble(), 49.5);

  csvLine = header.getRow(3);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Std Deviation");
  EXPECT_DOUBLE_EQ(csvLine[1].toDouble(), 29.0115);

  csvLine = header.getRow(4);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Variance");
  EXPECT_DOUBLE_EQ(csvLine[1].toDouble(), 841.667);

  csvLine = header.getRow(5);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Median");
  EXPECT_DOUBLE_EQ(csvLine[1].toDouble(), 49.0007);

  csvLine = header.getRow(6);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Mode");
  EXPECT_EQ(csvLine[1].toInt(), 0);

  csvLine = header.getRow(7);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Skew");
  EXPECT_DOUBLE_EQ(csvLine[1].toDouble(), 0.0516279);

  csvLine = header.getRow(8);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Minimum");
  EXPECT_EQ(csvLine[1].toInt(), 0);

  csvLine = header.getRow(9);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Maximum");
  EXPECT_EQ(csvLine[1].toInt(), 99);

  csvLine = header.getRow(10);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Total Pixels");
  EXPECT_EQ(csvLine[1].toInt(), 100);

  csvLine = header.getRow(11);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Valid Pixels");
  EXPECT_EQ(csvLine[1].toInt(), 100);

  csvLine = header.getRow(12);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Pixels Below Min");
  EXPECT_EQ(csvLine[1].toInt(), 0);

  csvLine = header.getRow(13);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Pixels Above Max");
  EXPECT_EQ(csvLine[1].toInt(), 0);

  csvLine = header.getRow(14);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Null Pixels");
  EXPECT_EQ(csvLine[1].toInt(), 0);

  csvLine = header.getRow(15);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Lis Pixels");
  EXPECT_EQ(csvLine[1].toInt(), 0);

  csvLine = header.getRow(16);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Lrs Pixels");
  EXPECT_EQ(csvLine[1].toInt(), 0);

  csvLine = header.getRow(17);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "His Pixels");
  EXPECT_EQ(csvLine[1].toInt(), 0);

  csvLine = header.getRow(18);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Hrs Pixels");
  EXPECT_EQ(csvLine[1].toInt(), 0);
}

TEST_F(NullPixelCube, FunctionalTestHistNulls) {
  QString outputFile =  tempDir.path()+"/output.txt";
  QVector<QString> args = {"to=" + outputFile};
  UserInterface ui(APP_XML, args);

  hist(testCube, ui);

  CSVReader::CSVAxis csvLine;

  CSVReader header = CSVReader(outputFile,
                               false, 0, ':', false, true);

  // Validate the header information is correct
  csvLine = header.getRow(0);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Cube");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[1].trimmed(), testCube->fileName());

  csvLine = header.getRow(1);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Band");
  EXPECT_EQ(csvLine[1].toInt(), 10);

  csvLine = header.getRow(2);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Average");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[1].trimmed(), "N/A");

  csvLine = header.getRow(3);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Std Deviation");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[1].trimmed(), "N/A");

  csvLine = header.getRow(4);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Variance");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[1].trimmed(), "N/A");

  csvLine = header.getRow(5);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Median");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[1].trimmed(), "N/A");

  csvLine = header.getRow(6);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Mode");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[1].trimmed(), "N/A");

  csvLine = header.getRow(7);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Skew");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[1].trimmed(), "N/A");

  csvLine = header.getRow(8);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Minimum");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[1].trimmed(), "N/A");

  csvLine = header.getRow(9);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Maximum");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[1].trimmed(), "N/A");

  csvLine = header.getRow(10);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Total Pixels");
  EXPECT_EQ(csvLine[1].toInt(), 100);

  csvLine = header.getRow(11);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Valid Pixels");
  EXPECT_EQ(csvLine[1].toInt(), 0);

  csvLine = header.getRow(12);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Pixels Below Min");
  EXPECT_EQ(csvLine[1].toInt(), 0);

  csvLine = header.getRow(13);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Pixels Above Max");
  EXPECT_EQ(csvLine[1].toInt(), 0);

  csvLine = header.getRow(14);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Null Pixels");
  EXPECT_EQ(csvLine[1].toInt(), 100);

  csvLine = header.getRow(15);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Lis Pixels");
  EXPECT_EQ(csvLine[1].toInt(), 0);

  csvLine = header.getRow(16);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Lrs Pixels");
  EXPECT_EQ(csvLine[1].toInt(), 0);

  csvLine = header.getRow(17);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "His Pixels");
  EXPECT_EQ(csvLine[1].toInt(), 0);

  csvLine = header.getRow(18);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Hrs Pixels");
  EXPECT_EQ(csvLine[1].toInt(), 0);
}

TEST_F(SmallCube, FunctionalTestHistNbins) {
  QString outputFile = tempDir.path()+"/output.txt";
  QVector<QString> args = {"to=" + outputFile, "nbins=25"};
  UserInterface ui(APP_XML, args);

  hist(testCube, ui);

  CSVReader::CSVAxis csvLine;

  CSVReader header = CSVReader(outputFile,
                               false, 0, ':', false, true);

  // Validate the header information is correct
  csvLine = header.getRow(0);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Cube");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[1].trimmed(),testCube->fileName());

  csvLine = header.getRow(1);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Band");
  EXPECT_EQ(csvLine[1].toInt(), 10);

  csvLine = header.getRow(2);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Average");
  EXPECT_DOUBLE_EQ(csvLine[1].toDouble(), 49.5);

  csvLine = header.getRow(3);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Std Deviation");
  EXPECT_DOUBLE_EQ(csvLine[1].toDouble(), 29.0115);

  csvLine = header.getRow(4);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Variance");
  EXPECT_DOUBLE_EQ(csvLine[1].toDouble(), 841.667);

  csvLine = header.getRow(5);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Median");
  EXPECT_DOUBLE_EQ(csvLine[1].toDouble(), 49.5);

  csvLine = header.getRow(6);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Mode");
  EXPECT_EQ(csvLine[1].toInt(), 33);

  csvLine = header.getRow(7);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Skew");
  EXPECT_EQ(csvLine[1].toDouble(), 0);

  csvLine = header.getRow(8);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Minimum");
  EXPECT_EQ(csvLine[1].toInt(), 0);

  csvLine = header.getRow(9);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Maximum");
  EXPECT_EQ(csvLine[1].toInt(), 99);

  csvLine = header.getRow(10);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Total Pixels");
  EXPECT_EQ(csvLine[1].toInt(), 100);

  csvLine = header.getRow(11);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Valid Pixels");
  EXPECT_EQ(csvLine[1].toInt(), 100);

  csvLine = header.getRow(12);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Pixels Below Min");
  EXPECT_EQ(csvLine[1].toInt(), 0);

  csvLine = header.getRow(13);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Pixels Above Max");
  EXPECT_EQ(csvLine[1].toInt(), 0);

  csvLine = header.getRow(14);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Null Pixels");
  EXPECT_EQ(csvLine[1].toInt(), 0);

  csvLine = header.getRow(15);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Lis Pixels");
  EXPECT_EQ(csvLine[1].toInt(), 0);

  csvLine = header.getRow(16);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Lrs Pixels");
  EXPECT_EQ(csvLine[1].toInt(), 0);

  csvLine = header.getRow(17);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "His Pixels");
  EXPECT_EQ(csvLine[1].toInt(), 0);

  csvLine = header.getRow(18);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Hrs Pixels");
  EXPECT_EQ(csvLine[1].toInt(), 0);
}

TEST_F(LargeCube, FunctionalTestHistMinMax) {
  QString outputFile = tempDir.path()+"/output.txt";
  QVector<QString> args = {"to=" + outputFile,
                           "nbins=255",
                           "minimum=0",
                           "maximum=255"};
  UserInterface ui(APP_XML, args);

  hist(testCube, ui);

  CSVReader::CSVAxis csvLine;

  CSVReader header = CSVReader(outputFile,
                               false, 0, ':', false, true);

  // Validate the header information is correct
  csvLine = header.getRow(0);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Cube");
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[1].trimmed(),testCube->fileName());

  csvLine = header.getRow(1);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Band");
  EXPECT_EQ(csvLine[1].toInt(), 10);

  csvLine = header.getRow(2);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Average");
  EXPECT_DOUBLE_EQ(csvLine[1].toDouble(), 127.5);

  csvLine = header.getRow(3);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Std Deviation");
  EXPECT_DOUBLE_EQ(csvLine[1].toDouble(), 73.9004);

  csvLine = header.getRow(4);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Variance");
  EXPECT_DOUBLE_EQ(csvLine[1].toDouble(), 5461.27);

  csvLine = header.getRow(5);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Median");
  EXPECT_DOUBLE_EQ(csvLine[1].toDouble(), 127.5);

  csvLine = header.getRow(6);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Mode");
  EXPECT_EQ(csvLine[1].toInt(), 0);

  csvLine = header.getRow(7);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Skew");
  EXPECT_EQ(csvLine[1].toDouble(), 0);

  csvLine = header.getRow(8);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Minimum");
  EXPECT_EQ(csvLine[1].toInt(), 0);

  csvLine = header.getRow(9);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Maximum");
  EXPECT_EQ(csvLine[1].toInt(), 255);

  csvLine = header.getRow(10);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Total Pixels");
  EXPECT_EQ(csvLine[1].toInt(), 1000000);

  csvLine = header.getRow(11);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Valid Pixels");
  EXPECT_EQ(csvLine[1].toInt(), 256000);

  csvLine = header.getRow(12);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Pixels Below Min");
  EXPECT_EQ(csvLine[1].toInt(), 0);

  csvLine = header.getRow(13);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Pixels Above Max");
  EXPECT_EQ(csvLine[1].toInt(), 744000);

  csvLine = header.getRow(14);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Null Pixels");
  EXPECT_EQ(csvLine[1].toInt(), 0);

  csvLine = header.getRow(15);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Lis Pixels");
  EXPECT_EQ(csvLine[1].toInt(), 0);

  csvLine = header.getRow(16);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Lrs Pixels");
  EXPECT_EQ(csvLine[1].toInt(), 0);

  csvLine = header.getRow(17);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "His Pixels");
  EXPECT_EQ(csvLine[1].toInt(), 0);

  csvLine = header.getRow(18);
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, csvLine[0], "Hrs Pixels");
  EXPECT_EQ(csvLine[1].toInt(), 0);
}
