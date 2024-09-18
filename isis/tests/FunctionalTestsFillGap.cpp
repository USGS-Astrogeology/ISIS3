#include <QTemporaryDir>

#include "fillgap.h"
#include "CubeFixtures.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"
#include "Histogram.h"
#include "LineManager.h"

#include "gtest/gtest.h"
#include "gmock/gmock.h"

using namespace Isis;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/fillgap.xml").expanded());


// First 9 test cases are checking for EXPECTed output depending on
// direction of gap in the cube and type of interpolation
TEST_F( SmallGapCube, FillGapTestBandAkima )
{
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/fillgap_out.cub";
  QVector<QString> args = {"from=" + bandCube->fileName(),
                          "to=" + cubeFileName,
                           "direction=band", "interp=akima" };
  UserInterface options(APP_XML, args);
  Pvl log;

  try {
    fillgap(options, &log);
  }
  catch (IException &e) {
    FAIL() <<  e.toString().c_str() << std::endl;
  }

  Cube outCube(cubeFileName.toStdString());
  Pvl *label = outCube.label();

  PvlGroup &dims = label->findGroup("Dimensions", Pvl::Traverse);
  EXPECT_EQ( (int)dims["Lines"], 9 );
  EXPECT_EQ( (int)dims["Samples"], 9 );
  EXPECT_EQ( (int)dims["Bands"], 9 );

  std::unique_ptr<Histogram> outHist (outCube.histogram());

  EXPECT_NEAR( outHist->Average(), 0.055633, 1e-6 );
  EXPECT_NEAR( outHist->Sum(), 4.506299, 1e-6);
  EXPECT_EQ( outHist->ValidPixels(), 81 );
  EXPECT_NEAR( outHist->StandardDeviation(), 1.003998, 1e-6 );
  EXPECT_NEAR( outHist->Variance(), 1.008013, 1e-6 );

  EXPECT_EQ( outHist->LisPixels(), 0 );
  EXPECT_EQ( outHist->LrsPixels(), 0 );
  EXPECT_EQ( outHist->HisPixels(), 0 );
  EXPECT_EQ( outHist->HrsPixels(), 0 );
}

TEST_F( SmallGapCube, FillGapTestBandCubic )
{
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/fillgap_out.cub";
  QVector<QString> args = {"from=" + bandCube->fileName(),
                           "to=" + cubeFileName,
                           "direction=band" };
  UserInterface options(APP_XML, args);
  Pvl log;

  try {
    fillgap(options, &log);
  }
  catch (IException &e) {
    FAIL() <<  e.toString().c_str() << std::endl;
  }

  Cube outCube(cubeFileName.toStdString());
  Pvl *label = outCube.label();

  PvlGroup &dims = label->findGroup("Dimensions", Pvl::Traverse);
  EXPECT_EQ( (int)dims["Lines"], 9 );
  EXPECT_EQ( (int)dims["Samples"], 9 );
  EXPECT_EQ( (int)dims["Bands"], 9 );

  std::unique_ptr<Histogram> outHist (outCube.histogram());

  EXPECT_NEAR( outHist->Average(), 0.055633, 1e-6 );
  EXPECT_NEAR( outHist->Sum(), 4.506299, 1e-6);
  EXPECT_EQ( outHist->ValidPixels(), 81 );
  EXPECT_NEAR( outHist->StandardDeviation(), 1.003998, 1e-6 );
  EXPECT_NEAR( outHist->Variance(), 1.008013, 1e-6 );

  EXPECT_EQ( outHist->LisPixels(), 0 );
  EXPECT_EQ( outHist->LrsPixels(), 0 );
  EXPECT_EQ( outHist->HisPixels(), 0 );
  EXPECT_EQ( outHist->HrsPixels(), 0 );
}

TEST_F( SmallGapCube, FillGapTestBandLinear )
{
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/fillgap_out.cub";
  QVector<QString> args = {"from=" + bandCube->fileName(),
                           "to=" + cubeFileName,
                           "direction=band", "interp=linear" };
  UserInterface options(APP_XML, args);
  Pvl log;

  try {
    fillgap(options, &log);
  }
  catch (IException &e) {
    FAIL() <<  e.toString().c_str() << std::endl;
  }

  Cube outCube(cubeFileName.toStdString());
  Pvl *label = outCube.label();

  PvlGroup &dims = label->findGroup("Dimensions", Pvl::Traverse);
  EXPECT_EQ( (int)dims["Lines"], 9 );
  EXPECT_EQ( (int)dims["Samples"], 9 );
  EXPECT_EQ( (int)dims["Bands"], 9 );

  std::unique_ptr<Histogram> outHist (outCube.histogram());

  EXPECT_NEAR( outHist->Average(), 0.055633, 1e-6 );
  EXPECT_NEAR( outHist->Sum(), 4.506299, 1e-6);
  EXPECT_EQ( outHist->ValidPixels(), 81 );
  EXPECT_NEAR( outHist->StandardDeviation(), 1.003998, 1e-6 );
  EXPECT_NEAR( outHist->Variance(), 1.008013, 1e-6 );

  EXPECT_EQ( outHist->LisPixels(), 0 );
  EXPECT_EQ( outHist->LrsPixels(), 0 );
  EXPECT_EQ( outHist->HisPixels(), 0 );
  EXPECT_EQ( outHist->HrsPixels(), 0 );
}

TEST_F( SmallGapCube, FillGapTestHorzAkima )
{
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/fillgap_out.cub";
  QVector<QString> args = {"from=" + horzCube->fileName(),
                           "to=" + cubeFileName,
                           "direction=sample", "interp=akima" };
  UserInterface options(APP_XML, args);
  Pvl log;

  try {
    fillgap(options, &log);
  }
  catch (IException &e) {
    FAIL() <<  e.toString().c_str() << std::endl;
  }

  Cube outCube(cubeFileName.toStdString());
  Pvl *label = outCube.label();

  PvlGroup &dims = label->findGroup("Dimensions", Pvl::Traverse);
  EXPECT_EQ( (int)dims["Lines"], 9 );
  EXPECT_EQ( (int)dims["Samples"], 9 );
  EXPECT_EQ( (int)dims["Bands"], 9 );

  std::unique_ptr<Histogram> outHist (outCube.histogram());

  EXPECT_NEAR( outHist->Average(), 0.051918, 1e-6 );
  EXPECT_NEAR( outHist->Sum(), 4.205411, 1e-6);
  EXPECT_EQ( outHist->ValidPixels(), 81 );
  EXPECT_NEAR( outHist->StandardDeviation(), 1.003444, 1e-6 );
  EXPECT_NEAR( outHist->Variance(), 1.006901, 1e-6 );

  EXPECT_EQ( outHist->LisPixels(), 0 );
  EXPECT_EQ( outHist->LrsPixels(), 0 );
  EXPECT_EQ( outHist->HisPixels(), 0 );
  EXPECT_EQ( outHist->HrsPixels(), 0 );
}

TEST_F( SmallGapCube, FillGapTestHorzCubic )
{
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/fillgap_out.cub";
  QVector<QString> args = {"from=" + horzCube->fileName(),
                           "to=" + cubeFileName,
                           "direction=sample" };
  UserInterface options(APP_XML, args);
  Pvl log;

  try {
    fillgap(options, &log);
  }
  catch (IException &e) {
    FAIL() <<  e.toString().c_str() << std::endl;
  }

  Cube outCube(cubeFileName.toStdString());
  Pvl *label = outCube.label();

  PvlGroup &dims = label->findGroup("Dimensions", Pvl::Traverse);
  EXPECT_EQ( (int)dims["Lines"], 9 );
  EXPECT_EQ( (int)dims["Samples"], 9 );
  EXPECT_EQ( (int)dims["Bands"], 9 );

  std::unique_ptr<Histogram> outHist (outCube.histogram());

  EXPECT_NEAR( outHist->Average(), 0.055223, 1e-6 );
  EXPECT_NEAR( outHist->Sum(), 4.473087, 1e-6);
  EXPECT_EQ( outHist->ValidPixels(), 81 );
  EXPECT_NEAR( outHist->StandardDeviation(),  1.003932, 1e-6 );
  EXPECT_NEAR( outHist->Variance(), 1.007879, 1e-6 );

  EXPECT_EQ( outHist->LisPixels(), 0 );
  EXPECT_EQ( outHist->LrsPixels(), 0 );
  EXPECT_EQ( outHist->HisPixels(), 0 );
  EXPECT_EQ( outHist->HrsPixels(), 0 );
}

TEST_F( SmallGapCube, FillGapTestHorzLinear )
{
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/fillgap_out.cub";
  QVector<QString> args = {"from=" + horzCube->fileName(),
                           "to=" + cubeFileName,
                           "direction=sample", "interp=linear" };
  UserInterface options(APP_XML, args);
  Pvl log;

  try {
    fillgap(options, &log);
  }
  catch (IException &e) {
    FAIL() <<  e.toString().c_str() << std::endl;
  }

  Cube outCube(cubeFileName.toStdString());
  Pvl *label = outCube.label();

  PvlGroup &dims = label->findGroup("Dimensions", Pvl::Traverse);
  EXPECT_EQ( (int)dims["Lines"], 9 );
  EXPECT_EQ( (int)dims["Samples"], 9 );
  EXPECT_EQ( (int)dims["Bands"], 9 );

  std::unique_ptr<Histogram> outHist (outCube.histogram());

  EXPECT_NEAR( outHist->Average(), 0.051118, 1e-6 );
  EXPECT_NEAR( outHist->Sum(), 4.140625, 1e-6);
  EXPECT_EQ( outHist->ValidPixels(), 81 );
  EXPECT_NEAR( outHist->StandardDeviation(), 1.003339, 1e-6 );
  EXPECT_NEAR( outHist->Variance(), 1.006691, 1e-6 );

  EXPECT_EQ( outHist->LisPixels(), 0 );
  EXPECT_EQ( outHist->LrsPixels(), 0 );
  EXPECT_EQ( outHist->HisPixels(), 0 );
  EXPECT_EQ( outHist->HrsPixels(), 0 );
}

TEST_F( SmallGapCube, FillGapTestVertAkima )
{
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/fillgap_out.cub";
  QVector<QString> args = {"from=" + vertCube->fileName(),
                           "to=" + cubeFileName,
                           "direction=line", "interp=akima" };
  UserInterface options(APP_XML, args);
  Pvl log;

  try {
    fillgap(options, &log);
  }
  catch (IException &e) {
    FAIL() <<  e.toString().c_str() << std::endl;
  }

  Cube outCube(cubeFileName.toStdString());
  Pvl *label = outCube.label();

  PvlGroup &dims = label->findGroup("Dimensions", Pvl::Traverse);
  EXPECT_EQ( (int)dims["Lines"], 9 );
  EXPECT_EQ( (int)dims["Samples"], 9 );
  EXPECT_EQ( (int)dims["Bands"], 9 );

  std::unique_ptr<Histogram> outHist (outCube.histogram());

  EXPECT_NEAR( outHist->Average(), 0.053020, 1e-6 );
  EXPECT_NEAR( outHist->Sum(), 4.294623, 1e-6);
  EXPECT_EQ( outHist->ValidPixels(), 81 );
  EXPECT_NEAR( outHist->StandardDeviation(), 1.006800, 1e-6 );
  EXPECT_NEAR( outHist->Variance(),  1.013646, 1e-6 );

  EXPECT_EQ( outHist->LisPixels(), 0 );
  EXPECT_EQ( outHist->LrsPixels(), 0 );
  EXPECT_EQ( outHist->HisPixels(), 0 );
  EXPECT_EQ( outHist->HrsPixels(), 0 );
}

TEST_F( SmallGapCube, FillGapTestVertCubic )
{
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/fillgap_out.cub";
  QVector<QString> args = {"from=" + vertCube->fileName(),
                           "to=" + cubeFileName,
                           "direction=line" };
  UserInterface options(APP_XML, args);
  Pvl log;

  try {
    fillgap(options, &log);
  }
  catch (IException &e) {
    FAIL() <<  e.toString().c_str() << std::endl;
  }

  Cube outCube(cubeFileName.toStdString());
  Pvl *label = outCube.label();

  PvlGroup &dims = label->findGroup("Dimensions", Pvl::Traverse);
  EXPECT_EQ( (int)dims["Lines"], 9 );
  EXPECT_EQ( (int)dims["Samples"], 9 );
  EXPECT_EQ( (int)dims["Bands"], 9 );

  std::unique_ptr<Histogram> outHist (outCube.histogram());

  EXPECT_NEAR( outHist->Average(), 0.058289, 1e-6 );
  EXPECT_NEAR( outHist->Sum(), 4.721464, 1e-6);
  EXPECT_EQ( outHist->ValidPixels(), 81 );
  EXPECT_NEAR( outHist->StandardDeviation(), 1.001199, 1e-6 );
  EXPECT_NEAR( outHist->Variance(), 1.002400, 1e-6 );

  EXPECT_EQ( outHist->LisPixels(), 0 );
  EXPECT_EQ( outHist->LrsPixels(), 0 );
  EXPECT_EQ( outHist->HisPixels(), 0 );
  EXPECT_EQ( outHist->HrsPixels(), 0 );
}

TEST_F( SmallGapCube, FillGapTestVertLinear )
{
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/fillgap_out.cub";
  QVector<QString> args = {"from=" + vertCube->fileName(),
                           "to=" + cubeFileName,
                           "direction=line", "interp=linear" };
  UserInterface options(APP_XML, args);
  Pvl log;

  try {
    fillgap(options, &log);
  }
  catch (IException &e) {
    FAIL() <<  e.toString().c_str() << std::endl;
  }

  Cube outCube(cubeFileName.toStdString());
  Pvl *label = outCube.label();

  PvlGroup &dims = label->findGroup("Dimensions", Pvl::Traverse);
  EXPECT_EQ( (int)dims["Lines"], 9 );
  EXPECT_EQ( (int)dims["Samples"], 9 );
  EXPECT_EQ( (int)dims["Bands"], 9 );

  std::unique_ptr<Histogram> outHist (outCube.histogram());

  EXPECT_NEAR( outHist->Average(), 0.084880, 1e-6 );
  EXPECT_NEAR( outHist->Sum(), 6.875352, 1e-6);
  EXPECT_EQ( outHist->ValidPixels(), 81 );
  EXPECT_NEAR( outHist->StandardDeviation(), 0.975967, 1e-6 );
  EXPECT_NEAR( outHist->Variance(), 0.952512, 1e-6 );

  EXPECT_EQ( outHist->LisPixels(), 0 );
  EXPECT_EQ( outHist->LrsPixels(), 0 );
  EXPECT_EQ( outHist->HisPixels(), 0 );
  EXPECT_EQ( outHist->HrsPixels(), 0 );
}

// Testing for logged warning when the app cannot interpolate over
// special pixels due to a gap taking place on the edge of the cube
TEST_F( SmallGapCube, FillGapTestGapsOnEdge )
{
  QTemporaryDir prefix;
  QString cubeFileName = prefix.path() + "/fillgap_out.cub";
  QVector<QString> args = {"from=" + vertCube->fileName(),
                           "to=" + cubeFileName,
                           "direction=line" };
  UserInterface options(APP_XML, args);
  Pvl log;


  // add gap line on edge
  LineManager line(*vertCube);
  int lineNum = 0;
  for(line.begin(); !line.end(); line++) {
    for(int i = 0; i < line.size(); i++) {
      if( lineNum == 0 ) {
        line[i] = NULL8;
      }
    }
    if( lineNum == 0 ) {
      vertCube->write(line);
    }
    lineNum++;
  }
  vertCube->reopen("rw");


  try {
    fillgap(options, &log);
  }
  catch (IException &e) {
    FAIL() <<  e.toString().c_str() << std::endl;
  }

  PvlGroup &mess = log.findGroup("Messages", Pvl::Traverse);
  EXPECT_EQ(mess["Warning"][0], "Unable to fill 9 special pixels." );
}
