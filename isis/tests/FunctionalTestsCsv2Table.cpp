#include "CameraFixtures.h"
#include "CubeFixtures.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"
#include "Table.h"
#include "TableRecord.h"

#include "csv2table.h"

#include "gtest/gtest.h"
#include "gmock/gmock.h"

using namespace Isis;

static QString APP_XML = FileName("$ISISROOT/bin/xml/csv2table.xml").expanded();

TEST_F(DefaultCube, FunctionalTestCsv2TableLabel) {
  QTemporaryDir tempDir;
  QString csvfile = "data/csv2table/test.csv";
  QString cubePath = testCube->fileName();
  testCube->close();
  QVector<QString> args = {"label="+cubePath,  "to="+cubePath,
    "csv="+csvfile, "tablename=TestTable"};

  UserInterface options(APP_XML, args);
  try {
    csv2table(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  testCube->open(cubePath);

  Table testTable("Temp");
  try {
    testTable = testCube->readTable("TestTable");
  } catch(IException &e) {
    std::string msg = "Failed to find/read TestTable";
    throw IException(e, IException::Unknown, msg, _FILEINFO_);
  }
  EXPECT_EQ(testTable.Records(), 3);
  EXPECT_EQ(testTable.RecordFields(), 6);
  EXPECT_EQ(testTable.RecordSize(), 48);
  std::vector<double> expected_values = {10, 0, 11, 0.2, 0.4, -5, 
                                         .45, 0.45, -12.58746324, 2, 7, -10,
                                         3, -1000000, 1000000, 100, 0.45678, 11};

  for (int i = 0; i < testTable.Records(); i++) {
    TableRecord record = testTable[i];
    for (int j = 0; j < record.Fields(); j++) {
      EXPECT_EQ(expected_values[(i * record.Fields()) + j], double(record[j]));
    }
  }
}

TEST_F(DefaultCube, FunctionalTestCsv2TableArrays) {
  QTemporaryDir tempDir;
  QString csvfile = "data/csv2table/test_arrays.csv";
  QString cubePath = testCube->fileName();
  testCube->close();
  QVector<QString> args = {"label="+cubePath,  "to="+cubePath,
    "csv="+csvfile, "tablename=TestTableArrays"};

  UserInterface options(APP_XML, args);
  try {
    csv2table(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  // Cube oCube(testCube->fileName(), "r");
  testCube->open(cubePath);

  Table testTable("Temp");
  try {
    testTable = testCube->readTable("TestTableArrays");
  } catch(IException &e) {
    std::string msg = "Failed to find/read TestTableArrays";
    throw IException(e, IException::Unknown, msg, _FILEINFO_);
  }
  EXPECT_EQ(testTable.Records(), 3);
  EXPECT_EQ(testTable.RecordFields(), 4);
  EXPECT_EQ(testTable.RecordSize(), 48);

  // Check array components
  std::vector<double> truth = {0.0, 11.0};
  EXPECT_TRUE(std::vector<double>(testTable[0][1]) == truth);
  truth = {0.45, -12.58746324};
  EXPECT_TRUE(std::vector<double>(testTable[1][1]) == truth);
  truth = {-1000000, 1000000};
  EXPECT_TRUE(std::vector<double>(testTable[2][1]) == truth);
  truth = {0.4,-5};
  EXPECT_TRUE(std::vector<double>(testTable[0][3]) == truth);
  truth = {7,-10};
  EXPECT_TRUE(std::vector<double>(testTable[1][3]) == truth);
  truth = {0.45678,11};
  EXPECT_TRUE(std::vector<double>(testTable[2][3]) == truth);

  // Check non-array components
  EXPECT_EQ(double(testTable[0][0]), 10.0);
  EXPECT_EQ(double(testTable[1][0]), 0.45);
  EXPECT_EQ(double(testTable[2][0]), 3.0);
  EXPECT_EQ(double(testTable[0][2]), 0.2);
  EXPECT_EQ(double(testTable[1][2]), 2.0);
  EXPECT_EQ(double(testTable[2][2]), 100.0);
}

TEST_F(DefaultCube, FunctionalTestCsv2TableOverwrite) {
  QTemporaryDir tempDir;
  QString csvfile = "data/csv2table/test.csv";
  QString cubePath = testCube->fileName();
  testCube->close();
  QVector<QString> args = {"label="+cubePath,  "to="+cubePath,
    "csv="+csvfile, "tablename=TestTable"};

  UserInterface options(APP_XML, args);
  try {
    csv2table(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  csvfile = "data/csv2table/test_2.csv";
  args = {"label="+cubePath,  "to="+cubePath,
          "csv="+csvfile, "tablename=TestTable"};
  options = UserInterface(APP_XML, args);
  try {
    csv2table(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  testCube->open(cubePath);

  Table testTable("Temp");
  try {
    testTable = testCube->readTable("TestTable");
  } catch(IException &e) {
    std::string msg = "Failed to find/read TestTable";
    throw IException(e, IException::Unknown, msg, _FILEINFO_);
  }
  EXPECT_EQ(testTable.Records(), 3);
  EXPECT_EQ(testTable.RecordFields(), 6);
  EXPECT_EQ(testTable.RecordSize(), 48);
  std::vector<double> expected_values = {10, 100, 11, 0.2, 0.4, -5, 
                                         .45, 0.45, -12.58746324, 2, 7, -10,
                                         3, -1000000, 1000000, 100, 0.45678, 11};

  for (int i = 0; i < testTable.Records(); i++) {
    TableRecord record = testTable[i];
    for (int j = 0; j < record.Fields(); j++) {
      EXPECT_EQ(expected_values[(i * record.Fields()) + j], double(record[j]));
    }
  }
}

TEST_F(DefaultCube, FunctionalTestCsv2TableErrors) {
  QTemporaryDir tempDir;
  QString cubePath = testCube->fileName();
  testCube->close();
  QVector<QString> args = {"label="+cubePath,  "to="+cubePath,
    "csv=not_a_file.csv", "tablename=TestTable"};

  UserInterface options(APP_XML, args);
  try {
    csv2table(options);
    FAIL() << "Csv2table should have failed" << std::endl;
  }
  catch (IException &e) {
    std::cout << e.what() << std::endl;
    EXPECT_THAT(e.what(), testing::HasSubstr("Failed to read CSV file"));
    EXPECT_THAT(e.what(), testing::HasSubstr("Unable to open file"));
  }

  args = {"label="+cubePath,  "to="+cubePath,
          "csv=data/csv2table/empty.csv", "tablename=TestTable"};
  options = UserInterface(APP_XML, args);
  try {
    csv2table(options);
    FAIL() << "Csv2table should have failed" << std::endl;
  }
  catch (IException &e) {
    EXPECT_THAT(e.what(), testing::HasSubstr("CSV file does not have data"));
    EXPECT_THAT(e.what(), testing::HasSubstr("File has [0] rows and [0] columns."));
  }

  args = {"label=not_a_file.pvl",  "to="+cubePath,
          "csv=data/csv2table/test.csv", "tablename=TestTable"};
  options = UserInterface(APP_XML, args);
  try {
    csv2table(options);
    FAIL() << "Csv2table should have failed" << std::endl;
  }
  catch (IException &e) {
    EXPECT_THAT(e.what(), testing::HasSubstr("Failed to read PVL label file"));
    EXPECT_THAT(e.what(), testing::HasSubstr("Unable to open"));
  }
}

TEST_F(DefaultCube, FunctionalTestCsv2TableTypes) {
  QTemporaryDir tempDir;
  QString csvfile = "data/csv2table/test.csv";
  QString cubePath = testCube->fileName();
  testCube->close();
  QVector<QString> args = {"label="+cubePath,  "to="+cubePath,
    "csv="+csvfile, "tablename=TestTable", "coltypes=(Double,Real,Double,Double,Double,Integer)"};

  UserInterface options(APP_XML, args);
  try {
    csv2table(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  testCube->open(cubePath);

  Table testTable("Temp");
  try {
    testTable = testCube->readTable("TestTable");
  } catch(IException &e) {
    std::string msg = "Failed to find/read TestTable";
    throw IException(e, IException::Unknown, msg, _FILEINFO_);
  }
  EXPECT_EQ(testTable.Records(), 3);
  EXPECT_EQ(testTable.RecordFields(), 6);
  EXPECT_EQ(testTable.RecordSize(), 40);
  std::vector<double> expected_values = {10, 0, 11, 0.2, 0.4, -5, 
                                         .45, 0.45, -12.58746324, 2, 7, -10,
                                         3, -1000000, 1000000, 100, 0.45678, 11};

  for (int i = 0; i < testTable.Records(); i++) {
    TableRecord record = testTable[i];
    for (int j = 0; j < record.Fields(); j++) {
      if (j == 0 || j == 2 || j == 3 || j == 4) {
        EXPECT_EQ(expected_values[(i * record.Fields()) + j], double(record[j]));
      }
      else if (j == 1) {
        EXPECT_EQ((float)expected_values[(i * record.Fields()) + j], float(record[j]));
      }
      else {
        EXPECT_EQ(expected_values[(i * record.Fields()) + j], int(record[j]));
      }
    }
  }
}

TEST_F(DefaultCube, FunctionalTestCsv2TableArrayTypes) {
  QTemporaryDir tempDir;
  QString csvfile = "data/csv2table/test_type_arrays.csv";
  QString cubePath = testCube->fileName();
  testCube->close();
  QVector<QString> args = {"label="+cubePath,  "to="+cubePath,
    "csv="+csvfile, "tablename=TestTable", "coltypes=(Integer,Integer,Text,Real,Real,Double,Double)"};

  UserInterface options(APP_XML, args);
  try {
    csv2table(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to open image: " << e.what() << std::endl;
  }

  testCube->open(cubePath);

  Table testTable("Temp");
  try {
    testTable = testCube->readTable("TestTable");
  } catch(IException &e) {
    std::string msg = "Failed to find/read TestTable";
    throw IException(e, IException::Unknown, msg, _FILEINFO_);
  }
  EXPECT_EQ(testTable.Records(), 3);
  EXPECT_EQ(testTable.RecordFields(), 4);
  EXPECT_EQ(testTable.RecordSize(), 39);
  std::vector<std::vector<int>> truth_ints = {{0, 1}, {2, 3}, {4, 5}};
  std::vector<std::string> truth_strings = {"Apple", "Banana", "Orange"};
  std::vector<std::vector<float>> truth_floats = {{0.0001, 0.0002}, {0.0003, 0.0004}, {0.0005, 0.0006}};
  std::vector<std::vector<double>> truth_doubles = {{0.0000000000001, 0.0000000000002}, {0.0000000000003, 0.0000000000004}, {0.0000000000005, 0.0000000000006}};


  for (int i = 0; i < testTable.Records(); i++) {
    TableRecord record = testTable[i];
    for (int j = 0; j < record.Fields(); j++) {
      if (j == 0) {
        EXPECT_TRUE(truth_ints[i] == std::vector<int>(record[j]));
      }
      else if (j == 1) {
        std::cout << truth_strings[i] << ", " << QString(record[j]).toStdString() << std::endl;
        EXPECT_TRUE(truth_strings[i] == QString(record[j]).toStdString());
      }
      else if (j == 2) {
        EXPECT_TRUE(truth_floats[i] == std::vector<float>(record[j]));
      }
      else {
        EXPECT_TRUE(truth_doubles[i] == std::vector<double>(record[j]));
      }
    }
  }
}