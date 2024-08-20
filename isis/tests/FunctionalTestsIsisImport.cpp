#include <iostream>
#include <time.h>

#include <QString>
#include <nlohmann/json.hpp>

#include "TempFixtures.h"
#include "Histogram.h"
#include "md5wrapper.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "TestUtilities.h"

#include "isisimport.h"

#include "gmock/gmock.h"

using namespace Isis;
using namespace testing;
using json = nlohmann::json;

static QString APP_XML = FileName("$ISISROOT/bin/xml/isisimport.xml").expanded();

TEST_F(TempTestingFiles, FunctionalTestIsisImportLabelXmlInput) {
  QString labelFileName = "data/isisimport/pds4.xml";

  std::ofstream ofxml;
  ofxml.open(labelFileName.toStdString());
  ofxml << R"(<Dimensions> <Lines>2</Lines> <Samples>3</Samples> <Bands>1</Bands> </Dimensions>)";
  ofxml.close();

  QString templateFile = tempDir.path() + "/test_result.tpl";
  QString renderedCube = tempDir.path() + "/test_result.cub";

  std::ofstream of;
  of.open(templateFile.toStdString());
  of << R"(Object = IsisCube
  Object = Core
    Group = Dimensions
      Samples = {{Dimensions.Samples}}
      Lines   = {{Dimensions.Lines}}
      Bands   = {{Dimensions.Bands}}
    End_Group

    Group = Pixels
      Type       = Real
      ByteOrder  = Lsb
      Base       = 0.0
      Multiplier = 1.0
    End_Group
  End_Object
End_Object
Object = Translation
End_Object
End)";
  of.close();
  QVector<QString> args = {"from=" + labelFileName, "template=" + templateFile, "to=" + renderedCube};
  UserInterface options(APP_XML, args);

  isisimport(options);

  Cube cube;
  cube.open(renderedCube);
  Pvl *label = cube.label();
  PvlGroup dimensionsGroup = label->findObject("IsisCube").findObject("Core").findGroup("Dimensions");


  EXPECT_EQ(std::stoi(dimensionsGroup["Samples"][0]), 3);
  EXPECT_EQ(std::stoi(dimensionsGroup["Lines"][0]), 2);
  EXPECT_EQ(std::stoi(dimensionsGroup["Bands"][0]), 1);

  EXPECT_EQ(cube.sampleCount(), 3);
  EXPECT_EQ(cube.lineCount(), 2);
  EXPECT_EQ(cube.bandCount(), 1);

  EXPECT_EQ(cube.statistics()->Average(), 1);
  EXPECT_EQ(cube.statistics()->Minimum(), 1);
  EXPECT_EQ(cube.statistics()->Maximum(), 1);
  EXPECT_EQ(cube.statistics()->StandardDeviation(), 0);
}

TEST_F(TempTestingFiles, FunctionalTestIsisImportLabelPds4ErrorNoImage) {
  QString labelFileName = tempDir.path() + "/doesNotExist.xml";
  QString templateFile = tempDir.path() + "/test_result.tpl";
  QString renderedCube = tempDir.path() + "/test_result.cub";

  QVector<QString> args = {"from=" + labelFileName, "template=" + templateFile, "to=" + renderedCube};
  UserInterface options(APP_XML, args);
  EXPECT_ANY_THROW(isisimport(options));
}

TEST_F(TempTestingFiles, FunctionalTestIsisImportLabelPds4RemoveStartTimeZ) {
  QString labelFileName = tempDir.path() + "/pds4.xml";
  std::ofstream ofxml;
  ofxml.open(labelFileName.toStdString());
  ofxml << R"(<Cube><Dimensions> <Lines>2</Lines> <Samples>3</Samples> <Bands>1</Bands> </Dimensions><StartTime>2021-01-01T00:00:00Z</StartTime></Cube>)";
  ofxml.close();

  QString templateFile = tempDir.path() + "/test_result.tpl";
  QString renderedCube = tempDir.path() + "/test_result.cub";

  std::ofstream of;
  of.open(templateFile.toStdString());
  of << R"(Object = IsisCube
  Object = Core
    Group = Dimensions
      Samples = {{Cube.Dimensions.Samples}}
      Lines   = {{Cube.Dimensions.Lines}}
      Bands   = {{Cube.Dimensions.Bands}}
    End_Group

    Group = Pixels
      Type       = Real
      ByteOrder  = Lsb
      Base       = 0.0
      Multiplier = 1.0
    End_Group
  End_Object
    Group = Instrument
      StartTime = {{RemoveStartTimeZ(Cube.StartTime)}}
    End_Group
End_Object
Object = Translation
End_Object
End)";
  of.close();
  QVector<QString> args = {"from=" + labelFileName, "template=" + templateFile, "to=" + renderedCube};
  UserInterface options(APP_XML, args);

  isisimport(options);

  Cube cube;
  cube.open(renderedCube);
  Pvl *label = cube.label();
  PvlGroup instrumentGroup = label->findObject("IsisCube").findGroup("Instrument");

  EXPECT_EQ(instrumentGroup["StartTime"][0], "2021-01-01T00:00:00");
}

TEST_F(TempTestingFiles, FunctionalTestIsisImportLabelPds4YearDoy) {
  QString labelFileName = tempDir.path() + "/pds4.xml";
  std::ofstream ofxml;
  ofxml.open(labelFileName.toStdString());
  ofxml << R"(<Cube><Dimensions> <Lines>2</Lines> <Samples>3</Samples> <Bands>1</Bands> </Dimensions><StartTime>2021-02-01T00:00:00Z
</StartTime></Cube>)";
  ofxml.close();

  QString templateFile = tempDir.path() + "/test_result.tpl";
  QString renderedCube = tempDir.path() + "/test_result.cub";
  std::ofstream of;
  of.open(templateFile.toStdString());
  of << R"(Object = IsisCube
  Object = Core
    Group = Dimensions
      Samples = {{Cube.Dimensions.Samples}}
      Lines   = {{Cube.Dimensions.Lines}}
      Bands   = {{Cube.Dimensions.Bands}}
    End_Group

    Group = Pixels
      Type       = Real
      ByteOrder  = Lsb
      Base       = 0.0
      Multiplier = 1.0
    End_Group
  End_Object
    Group = Archive
      YearDoy = {{YearDoy(Cube.StartTime)}}
    End_Group
End_Object
Object = Translation
End_Object
End)";
  of.close();
  QVector<QString> args = {"from=" + labelFileName, "template=" + templateFile, "to=" + renderedCube};
  UserInterface options(APP_XML, args);

  isisimport(options);

  Cube cube;
  cube.open(renderedCube);
  Pvl *label = cube.label();
  PvlGroup archiveGroup = label->findObject("IsisCube").findGroup("Archive");

  EXPECT_EQ(archiveGroup["YearDoy"][0], "202132");
}

TEST_F(TempTestingFiles, FunctionalTestIsisImportLabelObservationId) {
  QString labelFileName = tempDir.path() + "/pds4.xml";
  std::ofstream ofxml;
  ofxml.open(labelFileName.toStdString());
  ofxml << R"(<Cube><Dimensions> <Lines>2</Lines> <Samples>3</Samples> <Bands>1</Bands> </Dimensions><UniqueIdentifier>2021
</UniqueIdentifier><Target>Mars</Target></Cube>)";
  ofxml.close();

  QString templateFile = tempDir.path() + "/test_result.tpl";
  QString renderedCube = tempDir.path() + "/test_result.cub";
  std::ofstream of;
  of.open(templateFile.toStdString());
  of << R"(Object = IsisCube
  Object = Core
    Group = Dimensions
      Samples = {{Cube.Dimensions.Samples}}
      Lines   = {{Cube.Dimensions.Lines}}
      Bands   = {{Cube.Dimensions.Bands}}
    End_Group

    Group = Pixels
      Type       = Real
      ByteOrder  = Lsb
      Base       = 0.0
      Multiplier = 1.0
    End_Group
  End_Object
    Group = Archive
      ObservationId = {{UniqueIdtoObservId(Cube.UniqueIdentifier, Cube.Target)}}
    End_Group
End_Object
Object = Translation
End_Object
End)";
  of.close();
  QVector<QString> args = {"from=" + labelFileName, "template=" + templateFile, "to=" + renderedCube};
  UserInterface options(APP_XML, args);

  isisimport(options);

  Cube cube;
  cube.open(renderedCube);
  Pvl *label = cube.label();
  PvlGroup archiveGroup = label->findObject("IsisCube").findGroup("Archive");

  EXPECT_EQ(archiveGroup["ObservationId"][0], "CRUS_000000_505_1");
}
