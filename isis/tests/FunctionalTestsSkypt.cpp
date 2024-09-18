#include <iostream>
#include <QTextStream>
#include <QStringList>
#include <QFile>

#include "skypt.h"

#include "CameraFixtures.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"

#include "gtest/gtest.h"

using namespace Isis;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/skypt.xml").expanded());

/**
   * FunctionalTestSkyptDefault
   * 
   * Skypt test of check valid functionality.
   *
   * Input ...
   *   1) Level 1 cube (Default test cube)
   *   2) Sample and line numbers (because the default option is "image")
   * 
   * Output ...
   *    1) Pvl log file (the default output option is PVL).
   *
   * Test that the output values in the pvl file are as expected to within a given tolerance.
   */
TEST_F(DefaultCube, FunctionalTestSkyptDefault) {  

  QVector<QString> args = {"from="+testCube->fileName(), 
                           "sample=10.0", 
                           "line=10.0"};                    
  UserInterface options(APP_XML, args); 
  Pvl appLog;
  skypt(testCube, options, &appLog);
  PvlGroup skyPoint = appLog.findGroup("SkyPoint");

  EXPECT_DOUBLE_EQ( (double) skyPoint.findKeyword("Sample"), 10.0);
  EXPECT_DOUBLE_EQ( (double) skyPoint.findKeyword("Line"), 10.0);
  EXPECT_NEAR( (double) skyPoint.findKeyword("RightAscension"), 311.67239851182, 1e-8);
  EXPECT_NEAR( (double) skyPoint.findKeyword("Declination"), -46.856497015346, 1e-8);
  EXPECT_NEAR( (double) skyPoint.findKeyword("EphemerisTime"), -709401200.26114, 1e-8);
  EXPECT_DOUBLE_EQ( (double)  skyPoint.findKeyword("PixelValue"), 136);
  EXPECT_NEAR( (double) skyPoint.findKeyword("CelestialNorthClockAngle"), 69.384799169319, 1e-8);
}

/**
   * FunctionalTestSkyptFlatSampleLine
   * 
   * Skypt test of the flat file output against the applog given the image option and 
   * sample and line inputs.
   *
   * Input ...
   *   1) Level 1 cube (Default test cube).
   *   2) Specify the output format is "flat."
   *   3) Provide the flat file name (a temporarily created text file).
   *   4) Set append = false.
   *   5) Set type = image; expects sample and line.
   *   6) Sample number.
   *   7) Line number. 
   * 
   * Output ...
   *    1) A comma-separated file with two lines, the first line being the field names,
   *       and the second line contains the values for each field.
   *
   * Check that the field names in the first line match those in the pvl applog.
   * Check that the values in the flat file also match those in the pvl applog.
   */
TEST_F(DefaultCube, FunctionalTestSkyptFlatSampleLine) {

  QFile flatFile(tempDir.path() + "/testOut.txt");

  QVector<QString> args = {"from="+testCube->fileName(), 
                           "format=flat",
                           "to="+flatFile.fileName(),
                           "append=false", 
                           "type=image",
                           "sample=10.0", 
                           "line=10.0"};
  UserInterface options(APP_XML, args);
  Pvl appLog;
  skypt(testCube, options, &appLog);
  PvlGroup skyPoint = appLog.findGroup("SkyPoint");

  int lineNumber = 0;
  QTextStream flatStream(&flatFile);

  if (flatFile.open(QIODevice::ReadOnly)) {
    while(!flatStream.atEnd()) {
      QString line = flatStream.readLine();
      QStringList fields = line.split(",");

      if(lineNumber == 0) {
           EXPECT_PRED_FORMAT2(AssertQStringsEqual, fields.value(0), "Filename");
           EXPECT_PRED_FORMAT2(AssertQStringsEqual, fields.value(1), "Sample");
           EXPECT_PRED_FORMAT2(AssertQStringsEqual, fields.value(2), "Line");
           EXPECT_PRED_FORMAT2(AssertQStringsEqual, fields.value(3), "RightAscension");
           EXPECT_PRED_FORMAT2(AssertQStringsEqual, fields.value(4), "Declination");
           EXPECT_PRED_FORMAT2(AssertQStringsEqual, fields.value(5), "EphemerisTime");
           EXPECT_PRED_FORMAT2(AssertQStringsEqual, fields.value(6), "PixelValue");
           EXPECT_PRED_FORMAT2(AssertQStringsEqual, fields.value(7), "CelestialNorthClockAngle");
       }
       else if(lineNumber == 1) {
           EXPECT_PRED_FORMAT2(AssertStringsEqual, fields.value(0).toStdString(), skyPoint.findKeyword("Filename"));
           EXPECT_DOUBLE_EQ(fields.value(1).toDouble(), skyPoint.findKeyword("Sample"));
           EXPECT_DOUBLE_EQ(fields.value(2).toDouble(), skyPoint.findKeyword("Line"));
           EXPECT_DOUBLE_EQ(fields.value(3).toDouble(), skyPoint.findKeyword("RightAscension"));
           EXPECT_DOUBLE_EQ(fields.value(4).toDouble(), skyPoint.findKeyword("Declination"));
           EXPECT_DOUBLE_EQ(fields.value(5).toDouble(), skyPoint.findKeyword("EphemerisTime"));
           EXPECT_DOUBLE_EQ(fields.value(6).toDouble(), skyPoint.findKeyword("PixelValue"));
           EXPECT_DOUBLE_EQ(fields.value(7).toDouble(), skyPoint.findKeyword("CelestialNorthClockAngle"));
      }
      lineNumber++;
    }
  }
  else {
    FAIL() << "FAILED TO OPEN FLATFILE";
  }
}

/**
   * FunctionalTestSkyptFlatRaDec
   * 
   * Skypt test of the flat file output against the applog given the sky option and 
   * right ascension and declination inputs.
   *
   * Input ...
   *   1) Level 1 cube (Default test cube).
   *   2) Specify the output format is "flat."
   *   3) Provide the flat file name (a temporarily created text file).
   *   4) Set append = false.
   *   5) Set type = sky; expects ra and dec.
   *   6) Right ascension value.
   *   7) Declination value. 
   * 
   * Output ...
   *    1) A comma-separated file with two lines, the first line being the field names,
   *       and the second line contains the values for each field.
   *
   * Check that the field names in the first line match those in the pvl applog.
   * Check that the values in the flat file also match those in the pvl applog.
   */
TEST_F(DefaultCube, FunctionalTestSkyptFlatRaDec) {

  QFile flatFile(tempDir.path() + "/testOut2.txt");
 
  QVector<QString> args = {"from="+testCube->fileName(),
                           "format=flat",
                           "to="+flatFile.fileName(),
                           "append=false", 
                           "type=sky",
                           "ra=311.67239851182", 
                           "dec=-46.856497015346"};

  UserInterface options(APP_XML, args);
  Pvl appLog;
  skypt(testCube, options, &appLog);
  PvlGroup skyPoint = appLog.findGroup("SkyPoint");

  int lineNumber = 0;
  QTextStream flatStream(&flatFile);

  if (flatFile.open(QIODevice::ReadOnly)) {
    while(!flatStream.atEnd()) {
      QString line = flatStream.readLine();
      std::cout << std::endl << line.toStdString() << std::endl;
      QStringList fields = line.split(",");

      if(lineNumber == 0) {
           EXPECT_PRED_FORMAT2(AssertQStringsEqual, fields.value(0), "Filename");
           EXPECT_PRED_FORMAT2(AssertQStringsEqual, fields.value(1), "Sample");
           EXPECT_PRED_FORMAT2(AssertQStringsEqual, fields.value(2), "Line");
           EXPECT_PRED_FORMAT2(AssertQStringsEqual, fields.value(3), "RightAscension");
           EXPECT_PRED_FORMAT2(AssertQStringsEqual, fields.value(4), "Declination");
           EXPECT_PRED_FORMAT2(AssertQStringsEqual, fields.value(5), "EphemerisTime");
           EXPECT_PRED_FORMAT2(AssertQStringsEqual, fields.value(6), "PixelValue");
           EXPECT_PRED_FORMAT2(AssertQStringsEqual, fields.value(7), "CelestialNorthClockAngle");
       }
       else if(lineNumber == 1) {
           EXPECT_PRED_FORMAT2(AssertStringsEqual, fields.value(0).toStdString(), skyPoint.findKeyword("Filename"));
           EXPECT_DOUBLE_EQ(fields.value(1).toDouble(), skyPoint.findKeyword("Sample"));
           EXPECT_DOUBLE_EQ(fields.value(2).toDouble(), skyPoint.findKeyword("Line"));
           EXPECT_DOUBLE_EQ(fields.value(3).toDouble(), skyPoint.findKeyword("RightAscension"));
           EXPECT_DOUBLE_EQ(fields.value(4).toDouble(), skyPoint.findKeyword("Declination"));
           EXPECT_DOUBLE_EQ(fields.value(5).toDouble(), skyPoint.findKeyword("EphemerisTime"));
           EXPECT_DOUBLE_EQ(fields.value(6).toDouble(), skyPoint.findKeyword("PixelValue"));
           EXPECT_DOUBLE_EQ(fields.value(7).toDouble(), skyPoint.findKeyword("CelestialNorthClockAngle"));
      }
      lineNumber++;
    }
  }
  else {
    FAIL() << "FAILED TO OPEN FLATFILE";
  }
}


/**
   * FunctionalTestSkyptFlatFileError
   * 
   * Skypt test to fail if flatfile name is not provided.
   *
   * Input ...
   *   1) Level 1 cube (Default test cube).
   *   2) Specify the output format is "flat."
   *   3) Set append = false.
   *   4) Set type = image so sample and line are expected inputs.
   *   4) Sample value.
   *   5) Line value. 
   * 
   * Output ...
   *    1) Running without specifying a value for "TO" should throw the exception 
   *       "Flat file must have a name."
   *
   * Test that the expected error message is thrown.
   */
TEST_F(DefaultCube, FunctionalTestSkyptFlatFileError) {

  QVector<QString> args = {"from="+testCube->fileName(),
                           "format=flat"
                           "append=false", 
                           "type=image",
                           "sample=10.0", 
                           "line=10.0"};                       
  UserInterface options(APP_XML, args);
  Pvl appLog;

  try {
    skypt(testCube, options, &appLog);
    FAIL() << "Expected an exception to be thrown";
  }
  catch(Isis::IException &e) {
    EXPECT_TRUE(e.toString().find("Flat file must have a name.") != std::string::npos)
      <<  e.toString();
  }
  catch(...) {
    FAIL() << "Expected an IException with message: \"Flat file must have a name.\"";
  }
}


