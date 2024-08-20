#include <QTemporaryDir>

#include "Fixtures.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"
#include "Histogram.h"

#include "tgocassis2isis.h"
#include "tgocassisrdrgen.h"
#include "tgocassismos.h"
#include "spiceinit.h"
#include "tgocassisstitch.h"
#include "tgocassisunstitch.h"
#include "mosrange.h"
#include "cam2map.h"
#include "cubeit.h"

#include "gtest/gtest.h"

using namespace Isis;

static QString TGOCASSIS2ISIS_XML = FileName("$ISISROOT/bin/xml/tgocassis2isis.xml").expanded();
static QString RDRGEN_XML = FileName("$ISISROOT/bin/xml/tgocassisrdrgen.xml").expanded();
static QString MOS_XML = FileName("$ISISROOT/bin/xml/tgocassismos.xml").expanded();
static QString SPICEINIT_XML = FileName("$ISISROOT/bin/xml/spiceinit.xml").expanded();
static QString STITCH_XML = FileName("$ISISROOT/bin/xml/tgocassisstitch.xml").expanded();
static QString UNSTITCH_XML = FileName("$ISISROOT/bin/xml/tgocassisunstitch.xml").expanded();
static QString MOSRANGE_XML = FileName("$ISISROOT/bin/xml/mosrange.xml").expanded();
static QString CAM2MAP_XML = FileName("$ISISROOT/bin/xml/cam2map.xml").expanded();
static QString CUBEIT_XML = FileName("$ISISROOT/bin/xml/cubeit.xml").expanded();


TEST_F(TgoCassisModuleKernels, TgoCassisStitchUnstitch) {
  QTemporaryDir prefix;

  // run tgocassis2isis and spiceinit on pan framelet.
  QString panFileName = prefix.path() + "/panframelet.cub";
  QVector<QString> tgocassis2isisArgs = {"from=data/tgoCassis/CAS-MCO-2016-11-26T22.50.27.381-PAN-00005-B1.xml",
                                         "to=" + panFileName};

  UserInterface tgocassis2isisPan(TGOCASSIS2ISIS_XML, tgocassis2isisArgs);
  try {
    tgocassis2isis(tgocassis2isisPan);
  }
  catch (IException &e) {
    FAIL() << "Unable to run tgocassis2isis on pan image: " << e.what() << std::endl;
  }

  QVector<QString> spiceinitArgs = {"from=" + panFileName,
                                    "ck=" + binaryCkKernelsAsString,
                                    "spk=" + binarySpkKernelsAsString};
  UserInterface spiceinitPan(SPICEINIT_XML, spiceinitArgs);
  try {
    spiceinit(spiceinitPan);
  }
  catch (IException &e) {
    FAIL() << "Unable to run spiceinit on pan image: " << e.what() << std::endl;
  }

  // run tgocassis2isis and spiceinit on red framelet.
  QString redFileName = prefix.path() + "/redframelet.cub";
  tgocassis2isisArgs = {"from=data/tgoCassis/CAS-MCO-2016-11-26T22.50.27.381-RED-01005-B1.xml",
                        "to=" + redFileName};
  UserInterface tgocassis2isisRed(TGOCASSIS2ISIS_XML, tgocassis2isisArgs);
  try {
    tgocassis2isis(tgocassis2isisRed);
  }
  catch (IException &e) {
    FAIL() << "Unable to run tgocassis2isis on pan image: " << e.what() << std::endl;
  }

  spiceinitArgs = {"from=" + redFileName,
                   "ck=" + binaryCkKernelsAsString,
                   "spk=" + binarySpkKernelsAsString};
  UserInterface spiceinitRed(SPICEINIT_XML, spiceinitArgs);
  try {
    spiceinit(spiceinitRed);
  }
  catch (IException &e) {
    FAIL() << "Unable to run spiceinit on pan image: " << e.what() << std::endl;
  }

  // run tgocassis2isis and spiceinit on blu framelet.
  QString bluFileName = prefix.path() + "/bluframelet.cub";
  tgocassis2isisArgs = {"from=data/tgoCassis/CAS-MCO-2016-11-26T22.50.27.381-BLU-03005-B1.xml",
                        "to=" + bluFileName};
  UserInterface tgocassis2isisBlu(TGOCASSIS2ISIS_XML, tgocassis2isisArgs);
  try {
    tgocassis2isis(tgocassis2isisBlu);
  }
  catch (IException &e) {
    FAIL() << "Unable to run tgocassis2isis on blu image: " << e.what() << std::endl;
  }

  spiceinitArgs = {"from=" + bluFileName,
                   "ck=" + binaryCkKernelsAsString,
                   "spk=" + binarySpkKernelsAsString};
  UserInterface spiceinitBlu(SPICEINIT_XML, spiceinitArgs);
  try {
    spiceinit(spiceinitBlu);
  }
  catch (IException &e) {
    FAIL() << "Unable to run spiceinit on blu image: " << e.what() << std::endl;
  }

  // run tgocassis2isis and spiceinit on nir framelet.
  QString nirFileName = prefix.path() + "/nirframelet.cub";
  tgocassis2isisArgs = {"from=data/tgoCassis/CAS-MCO-2016-11-26T22.50.27.381-NIR-02005-B1.xml",
                        "to=" + nirFileName};
  UserInterface tgocassis2isisNir(TGOCASSIS2ISIS_XML, tgocassis2isisArgs);
  try {
    tgocassis2isis(tgocassis2isisNir);
  }
  catch (IException &e) {
    FAIL() << "Unable to run tgocassis2isis on nir image: " << e.what() << std::endl;
  }

  spiceinitArgs = {"from=" + nirFileName,
                   "ck=" + binaryCkKernelsAsString,
                   "spk=" + binarySpkKernelsAsString};
  UserInterface spiceinitNir(SPICEINIT_XML, spiceinitArgs);
  try {
    spiceinit(spiceinitNir);
  }
  catch (IException &e) {
    FAIL() << "Unable to run spiceinit  on nir image: " << e.what() << std::endl;
  }

  // run stitch and unstitch on cube list
  FileList *cubeList = new FileList();
  cubeList->append(panFileName);
  cubeList->append(redFileName);
  cubeList->append(bluFileName);
  cubeList->append(nirFileName);

  QString cubeListFile = prefix.path() + "/cubelist.lis";
  cubeList->write(cubeListFile);

  QVector<QString> stitchArgs = {"fromlist=" + cubeListFile,
                "outputprefix=" + prefix.path() + "/stitched"};
  UserInterface stitchOptions(STITCH_XML, stitchArgs);

  try {
    tgocassisstitch(stitchOptions);
  }
  catch (IException &e) {
    FAIL() << "Unable to run tgocassisstitch with cube list: " << e.what() << std::endl;
  }

  QVector<QString> unstitchArgs = {"from=" + prefix.path() + "/stitched-2016-11-26T22:50:27.381.cub",
                  "outputprefix=" + prefix.path() + "/unstitched"};
  UserInterface unstitchOptions(UNSTITCH_XML, unstitchArgs);

  try {
    tgocassisunstitch(unstitchOptions);
  }
  catch (IException &e) {
    FAIL() << "Unable to run tgocassisunstitch with stitched cube: " << e.what() << std::endl;
  }

  // Compare Pan unstitched cube to original cube
  QString panUnstitchFile = prefix.path() + "/unstitched_PAN.cub";
  Cube panUnstitchCube(panUnstitchFile);
  Pvl *panUnstitchLabel = panUnstitchCube.label();

  Cube panOrigCube(panFileName);
  Pvl *panOrigLabel = panOrigCube.label();

  // Dimensions group
  EXPECT_EQ(panUnstitchCube.sampleCount(), panOrigCube.sampleCount());
  EXPECT_EQ(panUnstitchCube.lineCount(), panOrigCube.lineCount());
  EXPECT_EQ(panUnstitchCube.bandCount(), panOrigCube.bandCount());

  // Instrument Group
  PvlGroup &panUnstitchInst = panUnstitchLabel->findGroup("Instrument", Pvl::Traverse);
  PvlGroup &panOrigInst = panOrigLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ(panUnstitchInst["SpacecraftName"], panOrigInst["SpacecraftName"]);
  EXPECT_EQ(panUnstitchInst["InstrumentId"], panOrigInst["InstrumentId"]);
  EXPECT_EQ(panUnstitchInst["TargetName"], panOrigInst["TargetName"]);
  EXPECT_EQ(panUnstitchInst["StartTime"], panOrigInst["StartTime"]);
  EXPECT_EQ(panUnstitchInst["SpaceCraftClockStartCount"], panOrigInst["SpaceCraftClockStartCount"]);
  EXPECT_EQ(panUnstitchInst["ExposureDuration"], panOrigInst["ExposureDuration"]);
  EXPECT_EQ(panUnstitchInst["SummingMode"], panOrigInst["SummingMode"]);
  EXPECT_EQ(panUnstitchInst["Filter"], panOrigInst["Filter"]);

  // Archive Group
  PvlGroup &panUnstitchArch = panUnstitchLabel->findGroup("Archive", Pvl::Traverse);
  PvlGroup &panOrigArch = panOrigLabel->findGroup("Archive", Pvl::Traverse);
  EXPECT_EQ(panUnstitchArch["DataSetId"], panOrigArch["DataSetId"]);
  EXPECT_EQ(panUnstitchArch["ProductVersionId"], panOrigArch["ProductVersionId"]);
  EXPECT_EQ(panUnstitchArch["ProductCreationTime"], panOrigArch["ProductCreationTime"]);
  EXPECT_EQ(panUnstitchArch["ScalingFactor"], panOrigArch["ScalingFactor"]);
  EXPECT_EQ(panUnstitchArch["Offset"], panOrigArch["Offset"]);
  EXPECT_EQ(panUnstitchArch["PredictMaximumExposureTime"], panOrigArch["PredictMaximumExposureTime"]);
  EXPECT_EQ(panUnstitchArch["CassisOffNadirAngle"], panOrigArch["CassisOffNadirAngle"]);
  EXPECT_EQ(panUnstitchArch["PredictedRepetitionFrequency"], panOrigArch["PredictedRepetitionFrequency"]);
  EXPECT_EQ(panUnstitchArch["GroundTrackVelocity"], panOrigArch["GroundTrackVelocity"]);
  EXPECT_EQ(panUnstitchArch["ForwardRotationAngle"], panOrigArch["ForwardRotationAngle"]);
  EXPECT_EQ(panUnstitchArch["SpiceMisalignment"], panOrigArch["SpiceMisalignment"]);
  EXPECT_EQ(panUnstitchArch["FocalLength"], panOrigArch["FocalLength"]);
  EXPECT_EQ(panUnstitchArch["FNumber"], panOrigArch["FNumber"]);
  EXPECT_EQ(panUnstitchArch["ExposureTimeCommand"], panOrigArch["ExposureTimeCommand"]);
  EXPECT_EQ(panUnstitchArch["FrameletNumber"], panOrigArch["FrameletNumber"]);
  EXPECT_EQ(panUnstitchArch["NumberOfFramelets"], panOrigArch["NumberOfFramelets"]);
  EXPECT_EQ(panUnstitchArch["ImageFrequency"], panOrigArch["ImageFrequency"]);
  EXPECT_EQ(panUnstitchArch["NumberOfWindows"], panOrigArch["NumberOfWindows"]);
  EXPECT_EQ(panUnstitchArch["UniqueIdentifier"], panOrigArch["UniqueIdentifier"]);
  EXPECT_EQ(panUnstitchArch["ExposureTimestamp"], panOrigArch["ExposureTimestamp"]);
  EXPECT_EQ(panUnstitchArch["ExposureTimePEHK"], panOrigArch["ExposureTimePEHK"]);
  EXPECT_EQ(panUnstitchArch["PixelsPossiblySaturated"], panOrigArch["PixelsPossiblySaturated"]);
  EXPECT_EQ(panUnstitchArch["WindowCount"], panOrigArch["WindowCount"]);
  EXPECT_EQ(panUnstitchArch["Window1Binning"], panOrigArch["Window1Binning"]);
  EXPECT_EQ(panUnstitchArch["Window1StartSample"], panOrigArch["Window1StartSample"]);
  EXPECT_EQ(panUnstitchArch["Window1EndSample"], panOrigArch["Window1EndSample"]);
  EXPECT_EQ(panUnstitchArch["Window1StartLine"], panOrigArch["Window1StartLine"]);
  EXPECT_EQ(panUnstitchArch["Window1EndLine"], panOrigArch["Window1EndLine"]);
  EXPECT_EQ(panUnstitchArch["Window2Binning"], panOrigArch["Window2Binning"]);
  EXPECT_EQ(panUnstitchArch["Window2StartSample"], panOrigArch["Window2StartSample"]);
  EXPECT_EQ(panUnstitchArch["Window2EndSample"], panOrigArch["Window2EndSample"]);
  EXPECT_EQ(panUnstitchArch["Window2StartLine"], panOrigArch["Window2StartLine"]);
  EXPECT_EQ(panUnstitchArch["Window2EndLine"], panOrigArch["Window2EndLine"]);
  EXPECT_EQ(panUnstitchArch["Window3Binning"], panOrigArch["Window3Binning"]);
  EXPECT_EQ(panUnstitchArch["Window3StartSample"], panOrigArch["Window3StartSample"]);
  EXPECT_EQ(panUnstitchArch["Window3EndSample"], panOrigArch["Window3EndSample"]);
  EXPECT_EQ(panUnstitchArch["Window3StartLine"], panOrigArch["Window3StartLine"]);
  EXPECT_EQ(panUnstitchArch["Window3EndLine"], panOrigArch["Window3EndLine"]);
  EXPECT_EQ(panUnstitchArch["Window4Binning"], panOrigArch["Window4Binning"]);
  EXPECT_EQ(panUnstitchArch["Window4StartSample"], panOrigArch["Window4StartSample"]);
  EXPECT_EQ(panUnstitchArch["Window4EndSample"], panOrigArch["Window4EndSample"]);
  EXPECT_EQ(panUnstitchArch["Window4StartLine"], panOrigArch["Window4StartLine"]);
  EXPECT_EQ(panUnstitchArch["Window4EndLine"], panOrigArch["Window4EndLine"]);
  EXPECT_EQ(panUnstitchArch["Window5Binning"], panOrigArch["Window5Binning"]);
  EXPECT_EQ(panUnstitchArch["Window5StartSample"], panOrigArch["Window5StartSample"]);
  EXPECT_EQ(panUnstitchArch["Window5EndSample"], panOrigArch["Window5EndSample"]);
  EXPECT_EQ(panUnstitchArch["Window5StartLine"], panOrigArch["Window5StartLine"]);
  EXPECT_EQ(panUnstitchArch["Window5EndLine"], panOrigArch["Window5EndLine"]);
  EXPECT_EQ(panUnstitchArch["Window6Binning"], panOrigArch["Window6Binning"]);
  EXPECT_EQ(panUnstitchArch["Window6StartSample"], panOrigArch["Window6StartSample"]);
  EXPECT_EQ(panUnstitchArch["Window6EndSample"], panOrigArch["Window6EndSample"]);
  EXPECT_EQ(panUnstitchArch["Window6StartLine"], panOrigArch["Window6StartLine"]);
  EXPECT_EQ(panUnstitchArch["Window6EndLine"], panOrigArch["Window6EndLine"]);
  EXPECT_EQ(panUnstitchArch["YearDoy"], panOrigArch["YearDoy"]);

  // Bandbin Group
  PvlGroup &panUnstitchBand = panUnstitchLabel->findGroup("BandBin", Pvl::Traverse);
  PvlGroup &panOrigBand = panOrigLabel->findGroup("BandBin", Pvl::Traverse);
  EXPECT_EQ(panUnstitchBand["FilterName"], panOrigBand["FilterName"]);
  EXPECT_EQ(panUnstitchBand["Center"], panOrigBand["Center"]);
  EXPECT_EQ(panUnstitchBand["Width"], panOrigBand["Width"]);


  std::unique_ptr<Histogram> panUnstitchHist (panUnstitchCube.histogram());
  std::unique_ptr<Histogram> panOrigHist (panOrigCube.histogram());

  EXPECT_NEAR(panUnstitchHist->Average(), panOrigHist->Average(), 0.0001);
  EXPECT_EQ(panUnstitchHist->Sum(), panOrigHist->Sum());
  EXPECT_EQ(panUnstitchHist->ValidPixels(), panOrigHist->ValidPixels());
  EXPECT_EQ(panUnstitchHist->StandardDeviation(), panOrigHist->StandardDeviation());

  // Compare Red unstitched cube to original cube
  QString redUnstitchFile = prefix.path() + "/unstitched_RED.cub";
  Cube redUnstitchCube(redUnstitchFile);
  Pvl *redUnstitchLabel = redUnstitchCube.label();

  Cube redOrigCube(redFileName);
  Pvl *redOrigLabel = redOrigCube.label();

  // Dimensions group
  EXPECT_EQ(redUnstitchCube.sampleCount(), redOrigCube.sampleCount());
  EXPECT_EQ(redUnstitchCube.lineCount(), redOrigCube.lineCount());
  EXPECT_EQ(redUnstitchCube.bandCount(), redOrigCube.bandCount());

  // Instrument Group
  PvlGroup &redUnstitchInst = redUnstitchLabel->findGroup("Instrument", Pvl::Traverse);
  PvlGroup &redOrigInst = redOrigLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ(redUnstitchInst["SpacecraftName"], redOrigInst["SpacecraftName"]);
  EXPECT_EQ(redUnstitchInst["InstrumentId"], redOrigInst["InstrumentId"]);
  EXPECT_EQ(redUnstitchInst["TargetName"], redOrigInst["TargetName"]);
  EXPECT_EQ(redUnstitchInst["StartTime"], redOrigInst["StartTime"]);
  EXPECT_EQ(redUnstitchInst["SpaceCraftClockStartCount"], redOrigInst["SpaceCraftClockStartCount"]);
  EXPECT_EQ(redUnstitchInst["ExposureDuration"], redOrigInst["ExposureDuration"]);
  EXPECT_EQ(redUnstitchInst["SummingMode"], redOrigInst["SummingMode"]);
  EXPECT_EQ(redUnstitchInst["Filter"], redOrigInst["Filter"]);

  // Archive Group
  PvlGroup &redUnstitchArch = redUnstitchLabel->findGroup("Archive", Pvl::Traverse);
  PvlGroup &redOrigArch = redOrigLabel->findGroup("Archive", Pvl::Traverse);
  EXPECT_EQ(redUnstitchArch["DataSetId"], redOrigArch["DataSetId"]);
  EXPECT_EQ(redUnstitchArch["ProductVersionId"], redOrigArch["ProductVersionId"]);
  EXPECT_EQ(redUnstitchArch["ProductCreationTime"], redOrigArch["ProductCreationTime"]);
  EXPECT_EQ(redUnstitchArch["ScalingFactor"], redOrigArch["ScalingFactor"]);
  EXPECT_EQ(redUnstitchArch["Offset"], redOrigArch["Offset"]);
  EXPECT_EQ(redUnstitchArch["PredictMaximumExposureTime"], redOrigArch["PredictMaximumExposureTime"]);
  EXPECT_EQ(redUnstitchArch["CassisOffNadirAngle"], redOrigArch["CassisOffNadirAngle"]);
  EXPECT_EQ(redUnstitchArch["PredictedRepetitionFrequency"], redOrigArch["PredictedRepetitionFrequency"]);
  EXPECT_EQ(redUnstitchArch["GroundTrackVelocity"], redOrigArch["GroundTrackVelocity"]);
  EXPECT_EQ(redUnstitchArch["ForwardRotationAngle"], redOrigArch["ForwardRotationAngle"]);
  EXPECT_EQ(redUnstitchArch["SpiceMisalignment"], redOrigArch["SpiceMisalignment"]);
  EXPECT_EQ(redUnstitchArch["FocalLength"], redOrigArch["FocalLength"]);
  EXPECT_EQ(redUnstitchArch["FNumber"], redOrigArch["FNumber"]);
  EXPECT_EQ(redUnstitchArch["ExposureTimeCommand"], redOrigArch["ExposureTimeCommand"]);
  EXPECT_EQ(redUnstitchArch["FrameletNumber"], redOrigArch["FrameletNumber"]);
  EXPECT_EQ(redUnstitchArch["NumberOfFramelets"], redOrigArch["NumberOfFramelets"]);
  EXPECT_EQ(redUnstitchArch["ImageFrequency"], redOrigArch["ImageFrequency"]);
  EXPECT_EQ(redUnstitchArch["NumberOfWindows"], redOrigArch["NumberOfWindows"]);
  EXPECT_EQ(redUnstitchArch["UniqueIdentifier"], redOrigArch["UniqueIdentifier"]);
  EXPECT_EQ(redUnstitchArch["ExposureTimestamp"], redOrigArch["ExposureTimestamp"]);
  EXPECT_EQ(redUnstitchArch["ExposureTimePEHK"], redOrigArch["ExposureTimePEHK"]);
  EXPECT_EQ(redUnstitchArch["PixelsPossiblySaturated"], redOrigArch["PixelsPossiblySaturated"]);
  EXPECT_EQ(redUnstitchArch["WindowCount"], redOrigArch["WindowCount"]);
  EXPECT_EQ(redUnstitchArch["Window1Binning"], redOrigArch["Window1Binning"]);
  EXPECT_EQ(redUnstitchArch["Window1StartSample"], redOrigArch["Window1StartSample"]);
  EXPECT_EQ(redUnstitchArch["Window1EndSample"], redOrigArch["Window1EndSample"]);
  EXPECT_EQ(redUnstitchArch["Window1StartLine"], redOrigArch["Window1StartLine"]);
  EXPECT_EQ(redUnstitchArch["Window1EndLine"], redOrigArch["Window1EndLine"]);
  EXPECT_EQ(redUnstitchArch["Window2Binning"], redOrigArch["Window2Binning"]);
  EXPECT_EQ(redUnstitchArch["Window2StartSample"], redOrigArch["Window2StartSample"]);
  EXPECT_EQ(redUnstitchArch["Window2EndSample"], redOrigArch["Window2EndSample"]);
  EXPECT_EQ(redUnstitchArch["Window2StartLine"], redOrigArch["Window2StartLine"]);
  EXPECT_EQ(redUnstitchArch["Window2EndLine"], redOrigArch["Window2EndLine"]);
  EXPECT_EQ(redUnstitchArch["Window3Binning"], redOrigArch["Window3Binning"]);
  EXPECT_EQ(redUnstitchArch["Window3StartSample"], redOrigArch["Window3StartSample"]);
  EXPECT_EQ(redUnstitchArch["Window3EndSample"], redOrigArch["Window3EndSample"]);
  EXPECT_EQ(redUnstitchArch["Window3StartLine"], redOrigArch["Window3StartLine"]);
  EXPECT_EQ(redUnstitchArch["Window3EndLine"], redOrigArch["Window3EndLine"]);
  EXPECT_EQ(redUnstitchArch["Window4Binning"], redOrigArch["Window4Binning"]);
  EXPECT_EQ(redUnstitchArch["Window4StartSample"], redOrigArch["Window4StartSample"]);
  EXPECT_EQ(redUnstitchArch["Window4EndSample"], redOrigArch["Window4EndSample"]);
  EXPECT_EQ(redUnstitchArch["Window4StartLine"], redOrigArch["Window4StartLine"]);
  EXPECT_EQ(redUnstitchArch["Window4EndLine"], redOrigArch["Window4EndLine"]);
  EXPECT_EQ(redUnstitchArch["Window5Binning"], redOrigArch["Window5Binning"]);
  EXPECT_EQ(redUnstitchArch["Window5StartSample"], redOrigArch["Window5StartSample"]);
  EXPECT_EQ(redUnstitchArch["Window5EndSample"], redOrigArch["Window5EndSample"]);
  EXPECT_EQ(redUnstitchArch["Window5StartLine"], redOrigArch["Window5StartLine"]);
  EXPECT_EQ(redUnstitchArch["Window5EndLine"], redOrigArch["Window5EndLine"]);
  EXPECT_EQ(redUnstitchArch["Window6Binning"], redOrigArch["Window6Binning"]);
  EXPECT_EQ(redUnstitchArch["Window6StartSample"], redOrigArch["Window6StartSample"]);
  EXPECT_EQ(redUnstitchArch["Window6EndSample"], redOrigArch["Window6EndSample"]);
  EXPECT_EQ(redUnstitchArch["Window6StartLine"], redOrigArch["Window6StartLine"]);
  EXPECT_EQ(redUnstitchArch["Window6EndLine"], redOrigArch["Window6EndLine"]);
  EXPECT_EQ(redUnstitchArch["YearDoy"], redOrigArch["YearDoy"]);

  // Bandbin Group
  PvlGroup &redUnstitchBand = redUnstitchLabel->findGroup("BandBin", Pvl::Traverse);
  PvlGroup &redOrigBand = redOrigLabel->findGroup("BandBin", Pvl::Traverse);
  EXPECT_EQ(redUnstitchBand["FilterName"], redOrigBand["FilterName"]);
  EXPECT_EQ(redUnstitchBand["Center"], redOrigBand["Center"]);
  EXPECT_EQ(redUnstitchBand["Width"], redOrigBand["Width"]);


  std::unique_ptr<Histogram> redUnstitchHist (redUnstitchCube.histogram());
  std::unique_ptr<Histogram> redOrigHist (redOrigCube.histogram());

  EXPECT_NEAR(redUnstitchHist->Average(), redOrigHist->Average(), 0.0001);
  EXPECT_EQ(redUnstitchHist->Sum(), redOrigHist->Sum());
  EXPECT_EQ(redUnstitchHist->ValidPixels(), redOrigHist->ValidPixels());
  EXPECT_EQ(redUnstitchHist->StandardDeviation(), redOrigHist->StandardDeviation());


  // compare Blu unstitched cube to original cube
  QString bluUnstitchFile = prefix.path() + "/unstitched_BLU.cub";
  Cube bluUnstitchCube(bluUnstitchFile);
  Pvl *bluUnstitchLabel = bluUnstitchCube.label();

  Cube bluOrigCube(bluFileName);
  Pvl *bluOrigLabel = bluOrigCube.label();

  // Dimensions group
  EXPECT_EQ(bluUnstitchCube.sampleCount(), bluOrigCube.sampleCount());
  EXPECT_EQ(bluUnstitchCube.lineCount(), bluOrigCube.lineCount());
  EXPECT_EQ(bluUnstitchCube.bandCount(), bluOrigCube.bandCount());

  // Instrument Group
  PvlGroup &bluUnstitchInst = bluUnstitchLabel->findGroup("Instrument", Pvl::Traverse);
  PvlGroup &bluOrigInst = bluOrigLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ(bluUnstitchInst["SpacecraftName"], bluOrigInst["SpacecraftName"]);
  EXPECT_EQ(bluUnstitchInst["InstrumentId"], bluOrigInst["InstrumentId"]);
  EXPECT_EQ(bluUnstitchInst["TargetName"], bluOrigInst["TargetName"]);
  EXPECT_EQ(bluUnstitchInst["StartTime"], bluOrigInst["StartTime"]);
  EXPECT_EQ(bluUnstitchInst["SpaceCraftClockStartCount"], bluOrigInst["SpaceCraftClockStartCount"]);
  EXPECT_EQ(bluUnstitchInst["ExposureDuration"], bluOrigInst["ExposureDuration"]);
  EXPECT_EQ(bluUnstitchInst["SummingMode"], bluOrigInst["SummingMode"]);
  EXPECT_EQ(bluUnstitchInst["Filter"], bluOrigInst["Filter"]);

  // Archive Group
  PvlGroup &bluUnstitchArch = bluUnstitchLabel->findGroup("Archive", Pvl::Traverse);
  PvlGroup &bluOrigArch = bluOrigLabel->findGroup("Archive", Pvl::Traverse);
  EXPECT_EQ(bluUnstitchArch["DataSetId"], bluOrigArch["DataSetId"]);
  EXPECT_EQ(bluUnstitchArch["ProductVersionId"], bluOrigArch["ProductVersionId"]);
  EXPECT_EQ(bluUnstitchArch["ProductCreationTime"], bluOrigArch["ProductCreationTime"]);
  EXPECT_EQ(bluUnstitchArch["ScalingFactor"], bluOrigArch["ScalingFactor"]);
  EXPECT_EQ(bluUnstitchArch["Offset"], bluOrigArch["Offset"]);
  EXPECT_EQ(bluUnstitchArch["PredictMaximumExposureTime"], bluOrigArch["PredictMaximumExposureTime"]);
  EXPECT_EQ(bluUnstitchArch["CassisOffNadirAngle"], bluOrigArch["CassisOffNadirAngle"]);
  EXPECT_EQ(bluUnstitchArch["PredictedRepetitionFrequency"], bluOrigArch["PredictedRepetitionFrequency"]);
  EXPECT_EQ(bluUnstitchArch["GroundTrackVelocity"], bluOrigArch["GroundTrackVelocity"]);
  EXPECT_EQ(bluUnstitchArch["ForwardRotationAngle"], bluOrigArch["ForwardRotationAngle"]);
  EXPECT_EQ(bluUnstitchArch["SpiceMisalignment"], bluOrigArch["SpiceMisalignment"]);
  EXPECT_EQ(bluUnstitchArch["FocalLength"], bluOrigArch["FocalLength"]);
  EXPECT_EQ(bluUnstitchArch["FNumber"], bluOrigArch["FNumber"]);
  EXPECT_EQ(bluUnstitchArch["ExposureTimeCommand"], bluOrigArch["ExposureTimeCommand"]);
  EXPECT_EQ(bluUnstitchArch["FrameletNumber"], bluOrigArch["FrameletNumber"]);
  EXPECT_EQ(bluUnstitchArch["NumberOfFramelets"], bluOrigArch["NumberOfFramelets"]);
  EXPECT_EQ(bluUnstitchArch["ImageFrequency"], bluOrigArch["ImageFrequency"]);
  EXPECT_EQ(bluUnstitchArch["NumberOfWindows"], bluOrigArch["NumberOfWindows"]);
  EXPECT_EQ(bluUnstitchArch["UniqueIdentifier"], bluOrigArch["UniqueIdentifier"]);
  EXPECT_EQ(bluUnstitchArch["ExposureTimestamp"], bluOrigArch["ExposureTimestamp"]);
  EXPECT_EQ(bluUnstitchArch["ExposureTimePEHK"], bluOrigArch["ExposureTimePEHK"]);
  EXPECT_EQ(bluUnstitchArch["PixelsPossiblySaturated"], bluOrigArch["PixelsPossiblySaturated"]);
  EXPECT_EQ(bluUnstitchArch["WindowCount"], bluOrigArch["WindowCount"]);
  EXPECT_EQ(bluUnstitchArch["Window1Binning"], bluOrigArch["Window1Binning"]);
  EXPECT_EQ(bluUnstitchArch["Window1StartSample"], bluOrigArch["Window1StartSample"]);
  EXPECT_EQ(bluUnstitchArch["Window1EndSample"], bluOrigArch["Window1EndSample"]);
  EXPECT_EQ(bluUnstitchArch["Window1StartLine"], bluOrigArch["Window1StartLine"]);
  EXPECT_EQ(bluUnstitchArch["Window1EndLine"], bluOrigArch["Window1EndLine"]);
  EXPECT_EQ(bluUnstitchArch["Window2Binning"], bluOrigArch["Window2Binning"]);
  EXPECT_EQ(bluUnstitchArch["Window2StartSample"], bluOrigArch["Window2StartSample"]);
  EXPECT_EQ(bluUnstitchArch["Window2EndSample"], bluOrigArch["Window2EndSample"]);
  EXPECT_EQ(bluUnstitchArch["Window2StartLine"], bluOrigArch["Window2StartLine"]);
  EXPECT_EQ(bluUnstitchArch["Window2EndLine"], bluOrigArch["Window2EndLine"]);
  EXPECT_EQ(bluUnstitchArch["Window3Binning"], bluOrigArch["Window3Binning"]);
  EXPECT_EQ(bluUnstitchArch["Window3StartSample"], bluOrigArch["Window3StartSample"]);
  EXPECT_EQ(bluUnstitchArch["Window3EndSample"], bluOrigArch["Window3EndSample"]);
  EXPECT_EQ(bluUnstitchArch["Window3StartLine"], bluOrigArch["Window3StartLine"]);
  EXPECT_EQ(bluUnstitchArch["Window3EndLine"], bluOrigArch["Window3EndLine"]);
  EXPECT_EQ(bluUnstitchArch["Window4Binning"], bluOrigArch["Window4Binning"]);
  EXPECT_EQ(bluUnstitchArch["Window4StartSample"], bluOrigArch["Window4StartSample"]);
  EXPECT_EQ(bluUnstitchArch["Window4EndSample"], bluOrigArch["Window4EndSample"]);
  EXPECT_EQ(bluUnstitchArch["Window4StartLine"], bluOrigArch["Window4StartLine"]);
  EXPECT_EQ(bluUnstitchArch["Window4EndLine"], bluOrigArch["Window4EndLine"]);
  EXPECT_EQ(bluUnstitchArch["Window5Binning"], bluOrigArch["Window5Binning"]);
  EXPECT_EQ(bluUnstitchArch["Window5StartSample"], bluOrigArch["Window5StartSample"]);
  EXPECT_EQ(bluUnstitchArch["Window5EndSample"], bluOrigArch["Window5EndSample"]);
  EXPECT_EQ(bluUnstitchArch["Window5StartLine"], bluOrigArch["Window5StartLine"]);
  EXPECT_EQ(bluUnstitchArch["Window5EndLine"], bluOrigArch["Window5EndLine"]);
  EXPECT_EQ(bluUnstitchArch["Window6Binning"], bluOrigArch["Window6Binning"]);
  EXPECT_EQ(bluUnstitchArch["Window6StartSample"], bluOrigArch["Window6StartSample"]);
  EXPECT_EQ(bluUnstitchArch["Window6EndSample"], bluOrigArch["Window6EndSample"]);
  EXPECT_EQ(bluUnstitchArch["Window6StartLine"], bluOrigArch["Window6StartLine"]);
  EXPECT_EQ(bluUnstitchArch["Window6EndLine"], bluOrigArch["Window6EndLine"]);
  EXPECT_EQ(bluUnstitchArch["YearDoy"], bluOrigArch["YearDoy"]);

  // Bandbin Group
  PvlGroup &bluUnstitchBand = bluUnstitchLabel->findGroup("BandBin", Pvl::Traverse);
  PvlGroup &bluOrigBand = bluOrigLabel->findGroup("BandBin", Pvl::Traverse);
  EXPECT_EQ(bluUnstitchBand["FilterName"], bluOrigBand["FilterName"]);
  EXPECT_EQ(bluUnstitchBand["Center"], bluOrigBand["Center"]);
  EXPECT_EQ(bluUnstitchBand["Width"], bluOrigBand["Width"]);


  std::unique_ptr<Histogram> bluUnstitchHist (bluUnstitchCube.histogram());
  std::unique_ptr<Histogram> bluOrigHist (bluOrigCube.histogram());

  EXPECT_NEAR(bluUnstitchHist->Average(), bluOrigHist->Average(), 0.0001);
  EXPECT_EQ(bluUnstitchHist->Sum(), bluOrigHist->Sum());
  EXPECT_EQ(bluUnstitchHist->ValidPixels(), bluOrigHist->ValidPixels());
  EXPECT_EQ(bluUnstitchHist->StandardDeviation(), bluOrigHist->StandardDeviation());


  // compare Nir unstitched cube to original cube
  QString nirUnstitchFile = prefix.path() + "/unstitched_NIR.cub";
  Cube nirUnstitchCube(nirUnstitchFile);
  Pvl *nirUnstitchLabel = nirUnstitchCube.label();

  Cube nirOrigCube(nirFileName);
  Pvl *nirOrigLabel = nirOrigCube.label();

  // Dimensions group
  EXPECT_EQ(nirUnstitchCube.sampleCount(), nirOrigCube.sampleCount());
  EXPECT_EQ(nirUnstitchCube.lineCount(), nirOrigCube.lineCount());
  EXPECT_EQ(nirUnstitchCube.bandCount(), nirOrigCube.bandCount());

  // Instrument Group
  PvlGroup &nirUnstitchInst = nirUnstitchLabel->findGroup("Instrument", Pvl::Traverse);
  PvlGroup &nirOrigInst = nirOrigLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ(nirUnstitchInst["SpacecraftName"], nirOrigInst["SpacecraftName"]);
  EXPECT_EQ(nirUnstitchInst["InstrumentId"], nirOrigInst["InstrumentId"]);
  EXPECT_EQ(nirUnstitchInst["TargetName"], nirOrigInst["TargetName"]);
  EXPECT_EQ(nirUnstitchInst["StartTime"], nirOrigInst["StartTime"]);
  EXPECT_EQ(nirUnstitchInst["SpaceCraftClockStartCount"], nirOrigInst["SpaceCraftClockStartCount"]);
  EXPECT_EQ(nirUnstitchInst["ExposureDuration"], nirOrigInst["ExposureDuration"]);
  EXPECT_EQ(nirUnstitchInst["SummingMode"], nirOrigInst["SummingMode"]);
  EXPECT_EQ(nirUnstitchInst["Filter"], nirOrigInst["Filter"]);

  // Archive Group
  PvlGroup &nirUnstitchArch = nirUnstitchLabel->findGroup("Archive", Pvl::Traverse);
  PvlGroup &nirOrigArch = nirOrigLabel->findGroup("Archive", Pvl::Traverse);
  EXPECT_EQ(nirUnstitchArch["DataSetId"], nirOrigArch["DataSetId"]);
  EXPECT_EQ(nirUnstitchArch["ProductVersionId"], nirOrigArch["ProductVersionId"]);
  EXPECT_EQ(nirUnstitchArch["ProductCreationTime"], nirOrigArch["ProductCreationTime"]);
  EXPECT_EQ(nirUnstitchArch["ScalingFactor"], nirOrigArch["ScalingFactor"]);
  EXPECT_EQ(nirUnstitchArch["Offset"], nirOrigArch["Offset"]);
  EXPECT_EQ(nirUnstitchArch["PredictMaximumExposureTime"], nirOrigArch["PredictMaximumExposureTime"]);
  EXPECT_EQ(nirUnstitchArch["CassisOffNadirAngle"], nirOrigArch["CassisOffNadirAngle"]);
  EXPECT_EQ(nirUnstitchArch["PredictedRepetitionFrequency"], nirOrigArch["PredictedRepetitionFrequency"]);
  EXPECT_EQ(nirUnstitchArch["GroundTrackVelocity"], nirOrigArch["GroundTrackVelocity"]);
  EXPECT_EQ(nirUnstitchArch["ForwardRotationAngle"], nirOrigArch["ForwardRotationAngle"]);
  EXPECT_EQ(nirUnstitchArch["SpiceMisalignment"], nirOrigArch["SpiceMisalignment"]);
  EXPECT_EQ(nirUnstitchArch["FocalLength"], nirOrigArch["FocalLength"]);
  EXPECT_EQ(nirUnstitchArch["FNumber"], nirOrigArch["FNumber"]);
  EXPECT_EQ(nirUnstitchArch["ExposureTimeCommand"], nirOrigArch["ExposureTimeCommand"]);
  EXPECT_EQ(nirUnstitchArch["FrameletNumber"], nirOrigArch["FrameletNumber"]);
  EXPECT_EQ(nirUnstitchArch["NumberOfFramelets"], nirOrigArch["NumberOfFramelets"]);
  EXPECT_EQ(nirUnstitchArch["ImageFrequency"], nirOrigArch["ImageFrequency"]);
  EXPECT_EQ(nirUnstitchArch["NumberOfWindows"], nirOrigArch["NumberOfWindows"]);
  EXPECT_EQ(nirUnstitchArch["UniqueIdentifier"], nirOrigArch["UniqueIdentifier"]);
  EXPECT_EQ(nirUnstitchArch["ExposureTimestamp"], nirOrigArch["ExposureTimestamp"]);
  EXPECT_EQ(nirUnstitchArch["ExposureTimePEHK"], nirOrigArch["ExposureTimePEHK"]);
  EXPECT_EQ(nirUnstitchArch["PixelsPossiblySaturated"], nirOrigArch["PixelsPossiblySaturated"]);
  EXPECT_EQ(nirUnstitchArch["WindowCount"], nirOrigArch["WindowCount"]);
  EXPECT_EQ(nirUnstitchArch["Window1Binning"], nirOrigArch["Window1Binning"]);
  EXPECT_EQ(nirUnstitchArch["Window1StartSample"], nirOrigArch["Window1StartSample"]);
  EXPECT_EQ(nirUnstitchArch["Window1EndSample"], nirOrigArch["Window1EndSample"]);
  EXPECT_EQ(nirUnstitchArch["Window1StartLine"], nirOrigArch["Window1StartLine"]);
  EXPECT_EQ(nirUnstitchArch["Window1EndLine"], nirOrigArch["Window1EndLine"]);
  EXPECT_EQ(nirUnstitchArch["Window2Binning"], nirOrigArch["Window2Binning"]);
  EXPECT_EQ(nirUnstitchArch["Window2StartSample"], nirOrigArch["Window2StartSample"]);
  EXPECT_EQ(nirUnstitchArch["Window2EndSample"], nirOrigArch["Window2EndSample"]);
  EXPECT_EQ(nirUnstitchArch["Window2StartLine"], nirOrigArch["Window2StartLine"]);
  EXPECT_EQ(nirUnstitchArch["Window2EndLine"], nirOrigArch["Window2EndLine"]);
  EXPECT_EQ(nirUnstitchArch["Window3Binning"], nirOrigArch["Window3Binning"]);
  EXPECT_EQ(nirUnstitchArch["Window3StartSample"], nirOrigArch["Window3StartSample"]);
  EXPECT_EQ(nirUnstitchArch["Window3EndSample"], nirOrigArch["Window3EndSample"]);
  EXPECT_EQ(nirUnstitchArch["Window3StartLine"], nirOrigArch["Window3StartLine"]);
  EXPECT_EQ(nirUnstitchArch["Window3EndLine"], nirOrigArch["Window3EndLine"]);
  EXPECT_EQ(nirUnstitchArch["Window4Binning"], nirOrigArch["Window4Binning"]);
  EXPECT_EQ(nirUnstitchArch["Window4StartSample"], nirOrigArch["Window4StartSample"]);
  EXPECT_EQ(nirUnstitchArch["Window4EndSample"], nirOrigArch["Window4EndSample"]);
  EXPECT_EQ(nirUnstitchArch["Window4StartLine"], nirOrigArch["Window4StartLine"]);
  EXPECT_EQ(nirUnstitchArch["Window4EndLine"], nirOrigArch["Window4EndLine"]);
  EXPECT_EQ(nirUnstitchArch["Window5Binning"], nirOrigArch["Window5Binning"]);
  EXPECT_EQ(nirUnstitchArch["Window5StartSample"], nirOrigArch["Window5StartSample"]);
  EXPECT_EQ(nirUnstitchArch["Window5EndSample"], nirOrigArch["Window5EndSample"]);
  EXPECT_EQ(nirUnstitchArch["Window5StartLine"], nirOrigArch["Window5StartLine"]);
  EXPECT_EQ(nirUnstitchArch["Window5EndLine"], nirOrigArch["Window5EndLine"]);
  EXPECT_EQ(nirUnstitchArch["Window6Binning"], nirOrigArch["Window6Binning"]);
  EXPECT_EQ(nirUnstitchArch["Window6StartSample"], nirOrigArch["Window6StartSample"]);
  EXPECT_EQ(nirUnstitchArch["Window6EndSample"], nirOrigArch["Window6EndSample"]);
  EXPECT_EQ(nirUnstitchArch["Window6StartLine"], nirOrigArch["Window6StartLine"]);
  EXPECT_EQ(nirUnstitchArch["Window6EndLine"], nirOrigArch["Window6EndLine"]);
  EXPECT_EQ(nirUnstitchArch["YearDoy"], nirOrigArch["YearDoy"]);

  // Bandbin Group
  PvlGroup &nirUnstitchBand = nirUnstitchLabel->findGroup("BandBin", Pvl::Traverse);
  PvlGroup &nirOrigBand = nirOrigLabel->findGroup("BandBin", Pvl::Traverse);
  EXPECT_EQ(nirUnstitchBand["FilterName"], nirOrigBand["FilterName"]);
  EXPECT_EQ(nirUnstitchBand["Center"], nirOrigBand["Center"]);
  EXPECT_EQ(nirUnstitchBand["Width"], nirOrigBand["Width"]);


  std::unique_ptr<Histogram> nirUnstitchHist (nirUnstitchCube.histogram());
  std::unique_ptr<Histogram> nirOrigHist (nirOrigCube.histogram());

  EXPECT_NEAR(nirUnstitchHist->Average(), nirOrigHist->Average(), 0.0001);
  EXPECT_EQ(nirUnstitchHist->Sum(), nirOrigHist->Sum());
  EXPECT_EQ(nirUnstitchHist->ValidPixels(), nirOrigHist->ValidPixels());
  EXPECT_EQ(nirUnstitchHist->StandardDeviation(), nirOrigHist->StandardDeviation());
}


TEST_F(TgoCassisModuleKernels, TgoCassisSingleFrameletProjection) {
  QTemporaryDir prefix;

  // run tgocassis2isis and spiceinit on pan framelet.
  QString panFileName = prefix.path() + "/panframelet.cub";
  QVector<QString> tgocassis2isisArgs = {"from=data/tgoCassis/singleFrameletProj/CAS-MCO-2016-11-26T22.58.02.583-PAN-00020-B1.xml",
                                         "to=" + panFileName};

  UserInterface tgocassis2isisPan(TGOCASSIS2ISIS_XML, tgocassis2isisArgs);
  try {
    tgocassis2isis(tgocassis2isisPan);
  }
  catch (IException &e) {
    FAIL() << "Unable to run tgocassis2isis on pan image: " << e.what() << std::endl;
  }

  QVector<QString> spiceinitArgs = {"from=" + panFileName,
                                    "ck=" + binaryCkKernelsAsString,
                                    "spk=" + binarySpkKernelsAsString};
  UserInterface spiceinitPan(SPICEINIT_XML, spiceinitArgs);
  try {
    spiceinit(spiceinitPan);
  }
  catch (IException &e) {
    FAIL() << "Unable to run spiceinit on pan image: " << e.what() << std::endl;
  }

  // run tgocassis2isis and spiceinit on red framelet.
  QString redFileName = prefix.path() + "/redframelet.cub";
  tgocassis2isisArgs = {"from=data/tgoCassis/singleFrameletProj/CAS-MCO-2016-11-26T22.58.02.583-RED-01020-B1.xml",
                        "to=" + redFileName};
  UserInterface tgocassis2isisRed(TGOCASSIS2ISIS_XML, tgocassis2isisArgs);
  try {
    tgocassis2isis(tgocassis2isisRed);
  }
  catch (IException &e) {
    FAIL() << "Unable to run tgocassis2isis on pan image: " << e.what() << std::endl;
  }

  spiceinitArgs = {"from=" + redFileName,
                   "ck=" + binaryCkKernelsAsString,
                   "spk=" + binarySpkKernelsAsString};
  UserInterface spiceinitRed(SPICEINIT_XML, spiceinitArgs);
  try {
    spiceinit(spiceinitRed);
  }
  catch (IException &e) {
    FAIL() << "Unable to run spiceinit on pan image: " << e.what() << std::endl;
  }

  // run tgocassis2isis and spiceinit on blu framelet.
  QString bluFileName = prefix.path() + "/bluframelet.cub";
  tgocassis2isisArgs = {"from=data/tgoCassis/singleFrameletProj/CAS-MCO-2016-11-26T22.58.02.583-BLU-03020-B1.xml",
                        "to=" + bluFileName};
  UserInterface tgocassis2isisBlu(TGOCASSIS2ISIS_XML, tgocassis2isisArgs);
  try {
    tgocassis2isis(tgocassis2isisBlu);
  }
  catch (IException &e) {
    FAIL() << "Unable to run tgocassis2isis on blu image: " << e.what() << std::endl;
  }

  spiceinitArgs = {"from=" + bluFileName,
                   "ck=" + binaryCkKernelsAsString,
                   "spk=" + binarySpkKernelsAsString};
  UserInterface spiceinitBlu(SPICEINIT_XML, spiceinitArgs);
  try {
    spiceinit(spiceinitBlu);
  }
  catch (IException &e) {
    FAIL() << "Unable to run spiceinit on blu image: " << e.what() << std::endl;
  }

  // run tgocassis2isis and spiceinit on nir framelet.
  QString nirFileName = prefix.path() + "/nirframelet.cub";
  tgocassis2isisArgs = {"from=data/tgoCassis/singleFrameletProj/CAS-MCO-2016-11-26T22.58.02.583-NIR-02020-B1.xml",
                        "to=" + nirFileName};
  UserInterface tgocassis2isisNir(TGOCASSIS2ISIS_XML, tgocassis2isisArgs);
  try {
    tgocassis2isis(tgocassis2isisNir);
  }
  catch (IException &e) {
    FAIL() << "Unable to run tgocassis2isis on nir image: " << e.what() << std::endl;
  }

  spiceinitArgs = {"from=" + nirFileName,
                   "ck=" + binaryCkKernelsAsString,
                   "spk=" + binarySpkKernelsAsString};
  UserInterface spiceinitNir(SPICEINIT_XML, spiceinitArgs);
  try {
    spiceinit(spiceinitNir);
  }
  catch (IException &e) {
    FAIL() << "Unable to run spiceinit  on nir image: " << e.what() << std::endl;
  }

  // run mosrange on cube list
  FileList *cubeList = new FileList();
  cubeList->append(panFileName);
  cubeList->append(redFileName);
  cubeList->append(bluFileName);
  cubeList->append(nirFileName);

  QString cubeListFile = prefix.path() + "/cubelist.lis";
  cubeList->write(cubeListFile);

  QString mapFile = prefix.path() + "/equi.map";
  QVector<QString> mosrangeArgs = {"fromlist=" + cubeListFile, "to=" + mapFile};
  UserInterface mosrangeOptions(MOSRANGE_XML, mosrangeArgs);

  try {
    mosrange(mosrangeOptions);
  }
  catch (IException &e) {
    FAIL() << "Unable to run mosrange with cube list: " << e.what() << std::endl;
  }

  // run cam2map on pan cube
  QString panEquiFile = prefix.path() + "/pan_equi.cub";
  QVector<QString> cam2mapArgs = {"from=" + panFileName,
                                  "to=" + panEquiFile,
                                  "map=" + mapFile};
  UserInterface cam2mapPan(CAM2MAP_XML, cam2mapArgs);
  try {
    cam2map(cam2mapPan);
  }
  catch (IException &e) {
    FAIL() << "Unable to run cam2map on pan image: " << e.what() << std::endl;
  }

  // run cam2map on nir cube
  QString nirEquiFile = prefix.path() + "/nir_equi.cub";
  cam2mapArgs = {"from=" + nirFileName, "to=" + nirEquiFile, "map=" + mapFile};
  UserInterface cam2mapNir(CAM2MAP_XML, cam2mapArgs);
  try {
    cam2map(cam2mapNir);
  }
  catch (IException &e) {
    FAIL() << "Unable to run cam2map on nir image: " << e.what() << std::endl;
  }

  // run cam2map on blu cube
  QString bluEquiFile = prefix.path() + "/blu_equi.cub";
  cam2mapArgs = {"from=" + bluFileName, "to=" + bluEquiFile, "map=" + mapFile};
  UserInterface cam2mapBlu(CAM2MAP_XML, cam2mapArgs);
  try {
    cam2map(cam2mapBlu);
  }
  catch (IException &e) {
    FAIL() << "Unable to run cam2map on blu image: " << e.what() << std::endl;
  }

  // run cam2map on red cube
  QString redEquiFile = prefix.path() + "/red_equi.cub";
  cam2mapArgs = {"from=" + redFileName, "to=" + redEquiFile, "map=" + mapFile};
  UserInterface cam2mapRed(CAM2MAP_XML, cam2mapArgs);
  try {
    cam2map(cam2mapRed);
  }
  catch (IException &e) {
    FAIL() << "Unable to run cam2map on red image: " << e.what() << std::endl;
  }

  // PAN Cube
  Cube panCube(panEquiFile);
  Pvl *panLabel = panCube.label();

  // Instrument Group
  PvlGroup &inst = panLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ(inst["SpacecraftName"][0], "TRACE GAS ORBITER");
  EXPECT_EQ(inst["InstrumentId"][0], "CaSSIS");
  EXPECT_EQ(inst["TargetName"][0], "Mars" );
  EXPECT_EQ(inst["StartTime"][0], "2016-11-26T22:58:02.583");
  EXPECT_EQ(inst["SpacecraftClockStartCount"][0], "2f0154373db1aa13");
  EXPECT_EQ(inst["ExposureDuration"][0], "1.920e-003");
  EXPECT_EQ(int(inst["SummingMode"]), 0);
  EXPECT_EQ(inst["Filter"][0], "PAN");

  // Archive Group
  PvlGroup &archive = panLabel->findGroup("Archive", Pvl::Traverse);
  EXPECT_EQ(archive["DataSetId"][0], "TBD");
  EXPECT_EQ(archive["ProductVersionId"][0], "UNK");
  EXPECT_EQ(archive["ProductCreationTime"][0], "2017-10-03T10:50:26");
  EXPECT_DOUBLE_EQ(double(archive["ScalingFactor"]), 1.0);
  EXPECT_DOUBLE_EQ(double(archive["Offset"]), 0.0);
  EXPECT_DOUBLE_EQ(double(archive["PredictMaximumExposureTime"]), 3.4317);
  EXPECT_DOUBLE_EQ(double(archive["CassisOffNadirAngle"]), 32.2820);
  EXPECT_DOUBLE_EQ(double(archive["PredictedRepetitionFrequency"]), 790.7);
  EXPECT_DOUBLE_EQ(double(archive["GroundTrackVelocity"]), 2.3616);
  EXPECT_DOUBLE_EQ(double(archive["ForwardRotationAngle"]), 47.93);
  EXPECT_DOUBLE_EQ(double(archive["SpiceMisalignment"]), 174.295);
  EXPECT_DOUBLE_EQ(double(archive["FocalLength"]), 0.8770);
  EXPECT_DOUBLE_EQ(double(archive["FNumber"]), 6.50);
  EXPECT_EQ(int(archive["ExposureTimeCommand"]), 200);
  EXPECT_EQ(int(archive["FrameletNumber"]), 20);
  EXPECT_EQ(int(archive["NumberOfFramelets"]), 40);
  EXPECT_EQ(int(archive["ImageFrequency"]), 1200000);
  EXPECT_EQ(int(archive["NumberOfWindows"]), 6);
  EXPECT_EQ(int(archive["UniqueIdentifier"]), 100799468);
  EXPECT_EQ(archive["ExposureTimestamp"][0], "2f0154373db1aa13");
  EXPECT_DOUBLE_EQ(double(archive["ExposureTimePEHK"]), 0.00192);
  EXPECT_DOUBLE_EQ(double(archive["PixelsPossiblySaturated"]), 0.00);
  EXPECT_EQ(int(archive["WindowCount"]), 0);
  EXPECT_EQ(int(archive["Window1Binning"]), 0);
  EXPECT_EQ(int(archive["Window1StartSample"]), 0);
  EXPECT_EQ(int(archive["Window1EndSample"]), 2047);
  EXPECT_EQ(int(archive["Window1StartLine"]), 354);
  EXPECT_EQ(int(archive["Window1EndLine"]), 633);
  EXPECT_EQ(int(archive["Window2Binning"]), 0);
  EXPECT_EQ(int(archive["Window2StartSample"]), 0);
  EXPECT_EQ(int(archive["Window2EndSample"]), 2047);
  EXPECT_EQ(int(archive["Window2StartLine"]), 712);
  EXPECT_EQ(int(archive["Window2EndLine"]), 966);
  EXPECT_EQ(int(archive["Window3Binning"]), 0);
  EXPECT_EQ(int(archive["Window3StartSample"]), 0);
  EXPECT_EQ(int(archive["Window3EndSample"]), 2047);
  EXPECT_EQ(int(archive["Window3StartLine"]), 1048);
  EXPECT_EQ(int(archive["Window3EndLine"]), 1302);
  EXPECT_EQ(int(archive["Window4Binning"]), 0);
  EXPECT_EQ(int(archive["Window4StartSample"]), 0);
  EXPECT_EQ(int(archive["Window4EndSample"]), 2047);
  EXPECT_EQ(int(archive["Window4StartLine"]), 1409);
  EXPECT_EQ(int(archive["Window4EndLine"]), 1662);
  EXPECT_EQ(int(archive["Window5Binning"]), 0);
  EXPECT_EQ(int(archive["Window5StartSample"]), 640);
  EXPECT_EQ(int(archive["Window5EndSample"]), 767);
  EXPECT_EQ(int(archive["Window5StartLine"]), 200);
  EXPECT_EQ(int(archive["Window5EndLine"]), 208);
  EXPECT_EQ(int(archive["Window6Binning"]), 0);
  EXPECT_EQ(int(archive["Window6StartSample"]), 1280);
  EXPECT_EQ(int(archive["Window6EndSample"]), 1407);
  EXPECT_EQ(int(archive["Window6StartLine"]), 1850);
  EXPECT_EQ(int(archive["Window6EndLine"]), 1858);
  EXPECT_EQ(int(archive["YearDoy"]), 2016331);
  EXPECT_EQ(archive["ObservationId"][0], "CRUS_049218_251_0");

  // BandBin Group
  PvlGroup &bandbin = panLabel->findGroup("BandBin", Pvl::Traverse);
  EXPECT_EQ(bandbin["FilterName"][0], "PAN");
  EXPECT_DOUBLE_EQ(double(bandbin["Center"]), 678.2);
  EXPECT_DOUBLE_EQ(double(bandbin["Width"]), 231.9);
  EXPECT_EQ(bandbin["NaifIkCode"][0], "-143421");

  // Kernels Group
  PvlGroup &kernels = panLabel->findGroup("Kernels", Pvl::Traverse);
  EXPECT_EQ(int(kernels["NaifFrameCode"]), -143400);

  Histogram *hist = panCube.histogram();

  EXPECT_NEAR(hist->Average(), 0.082351300138231429, 0.0001);
  EXPECT_NEAR(hist->Sum(), 70857.19977273792, 0.0001);
  EXPECT_EQ(hist->ValidPixels(), 860426);
  EXPECT_NEAR(hist->StandardDeviation(), 0.0010547865346787659, 0.0001);

  // NIR Cube
  Cube nirCube(nirEquiFile);
  Pvl *nirLabel = nirCube.label();

  // Instrument Group
  inst = nirLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ(inst["SpacecraftName"][0], "TRACE GAS ORBITER");
  EXPECT_EQ(inst["InstrumentId"][0], "CaSSIS");
  EXPECT_EQ(inst["TargetName"][0], "Mars" );
  EXPECT_EQ(inst["StartTime"][0], "2016-11-26T22:58:02.583");
  EXPECT_EQ(inst["SpacecraftClockStartCount"][0], "2f0154373db1aa13");
  EXPECT_EQ(inst["ExposureDuration"][0], "1.920e-003");
  EXPECT_EQ(int(inst["SummingMode"]), 0);
  EXPECT_EQ(inst["Filter"][0], "NIR");

  // Archive Group
  archive = nirLabel->findGroup("Archive", Pvl::Traverse);
  EXPECT_EQ(archive["DataSetId"][0], "TBD");
  EXPECT_EQ(archive["ProductVersionId"][0], "UNK");
  EXPECT_EQ(archive["ProductCreationTime"][0], "2017-10-03T10:50:26");
  EXPECT_DOUBLE_EQ(double(archive["ScalingFactor"]), 1.0);
  EXPECT_DOUBLE_EQ(double(archive["Offset"]), 0.0);
  EXPECT_DOUBLE_EQ(double(archive["PredictMaximumExposureTime"]), 3.4317);
  EXPECT_DOUBLE_EQ(double(archive["CassisOffNadirAngle"]), 32.2820);
  EXPECT_DOUBLE_EQ(double(archive["PredictedRepetitionFrequency"]), 790.7);
  EXPECT_DOUBLE_EQ(double(archive["GroundTrackVelocity"]), 2.3616);
  EXPECT_DOUBLE_EQ(double(archive["ForwardRotationAngle"]), 47.93);
  EXPECT_DOUBLE_EQ(double(archive["SpiceMisalignment"]), 174.295);
  EXPECT_DOUBLE_EQ(double(archive["FocalLength"]), 0.8770);
  EXPECT_DOUBLE_EQ(double(archive["FNumber"]), 6.50);
  EXPECT_EQ(int(archive["ExposureTimeCommand"]), 200);
  EXPECT_EQ(int(archive["FrameletNumber"]), 20);
  EXPECT_EQ(int(archive["NumberOfFramelets"]), 40);
  EXPECT_EQ(int(archive["ImageFrequency"]), 1200000);
  EXPECT_EQ(int(archive["NumberOfWindows"]), 6);
  EXPECT_EQ(int(archive["UniqueIdentifier"]), 100799468);
  EXPECT_EQ(archive["ExposureTimestamp"][0], "2f0154373db1aa13");
  EXPECT_DOUBLE_EQ(double(archive["ExposureTimePEHK"]), 0.00192);
  EXPECT_DOUBLE_EQ(double(archive["PixelsPossiblySaturated"]), 0.00);
  EXPECT_EQ(int(archive["WindowCount"]), 2);
  EXPECT_EQ(int(archive["Window1Binning"]), 0);
  EXPECT_EQ(int(archive["Window1StartSample"]), 0);
  EXPECT_EQ(int(archive["Window1EndSample"]), 2047);
  EXPECT_EQ(int(archive["Window1StartLine"]), 354);
  EXPECT_EQ(int(archive["Window1EndLine"]), 632);
  EXPECT_EQ(int(archive["Window2Binning"]), 0);
  EXPECT_EQ(int(archive["Window2StartSample"]), 0);
  EXPECT_EQ(int(archive["Window2EndSample"]), 2047);
  EXPECT_EQ(int(archive["Window2StartLine"]), 712);
  EXPECT_EQ(int(archive["Window2EndLine"]), 966);
  EXPECT_EQ(int(archive["Window3Binning"]), 0);
  EXPECT_EQ(int(archive["Window3StartSample"]), 0);
  EXPECT_EQ(int(archive["Window3EndSample"]), 2047);
  EXPECT_EQ(int(archive["Window3StartLine"]), 1048);
  EXPECT_EQ(int(archive["Window3EndLine"]), 1303);
  EXPECT_EQ(int(archive["Window4Binning"]), 0);
  EXPECT_EQ(int(archive["Window4StartSample"]), 0);
  EXPECT_EQ(int(archive["Window4EndSample"]), 2047);
  EXPECT_EQ(int(archive["Window4StartLine"]), 1409);
  EXPECT_EQ(int(archive["Window4EndLine"]), 1662);
  EXPECT_EQ(int(archive["Window5Binning"]), 0);
  EXPECT_EQ(int(archive["Window5StartSample"]), 640);
  EXPECT_EQ(int(archive["Window5EndSample"]), 767);
  EXPECT_EQ(int(archive["Window5StartLine"]), 200);
  EXPECT_EQ(int(archive["Window5EndLine"]), 208);
  EXPECT_EQ(int(archive["Window6Binning"]), 0);
  EXPECT_EQ(int(archive["Window6StartSample"]), 1280);
  EXPECT_EQ(int(archive["Window6EndSample"]), 1407);
  EXPECT_EQ(int(archive["Window6StartLine"]), 1850);
  EXPECT_EQ(int(archive["Window6EndLine"]), 1858);
  EXPECT_EQ(int(archive["YearDoy"]), 2016331);
  EXPECT_EQ(archive["ObservationId"][0], "CRUS_049218_251_0");

  // BandBin Group
  bandbin = nirLabel->findGroup("BandBin", Pvl::Traverse);
  EXPECT_EQ(bandbin["FilterName"][0], "NIR");
  EXPECT_DOUBLE_EQ(double(bandbin["Center"]), 939.3);
  EXPECT_DOUBLE_EQ(double(bandbin["Width"]), 121.8);
  EXPECT_EQ(bandbin["NaifIkCode"][0], "-143423");

  // Kernels Group
  kernels = nirLabel->findGroup("Kernels", Pvl::Traverse);
  EXPECT_EQ(int(kernels["NaifFrameCode"]), -143400);

  hist = nirCube.histogram();

  EXPECT_NEAR(hist->Average(), 0.096215370187754598, 0.0001);
  EXPECT_NEAR(hist->Sum(), 78150.645788893104, 0.0001);
  EXPECT_EQ(hist->ValidPixels(), 812247);
  EXPECT_NEAR(hist->StandardDeviation(), 0.0015024999314775509, 0.0001);

  // RED Cube
  Cube redCube(redEquiFile);
  Pvl *redLabel = redCube.label();

  // Instrument Group
  inst = redLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ(inst["SpacecraftName"][0], "TRACE GAS ORBITER");
  EXPECT_EQ(inst["InstrumentId"][0], "CaSSIS");
  EXPECT_EQ(inst["TargetName"][0], "Mars" );
  EXPECT_EQ(inst["StartTime"][0], "2016-11-26T22:58:02.583");
  EXPECT_EQ(inst["SpacecraftClockStartCount"][0], "2f0154373db1aa13");
  EXPECT_EQ(inst["ExposureDuration"][0], "1.920e-003");
  EXPECT_EQ(int(inst["SummingMode"]), 0);
  EXPECT_EQ(inst["Filter"][0], "RED");

  // Archive Group
  archive = redLabel->findGroup("Archive", Pvl::Traverse);
  EXPECT_EQ(archive["DataSetId"][0], "TBD");
  EXPECT_EQ(archive["ProductVersionId"][0], "UNK");
  EXPECT_EQ(archive["ProductCreationTime"][0], "2017-10-03T10:50:26");
  EXPECT_DOUBLE_EQ(double(archive["ScalingFactor"]), 1.0);
  EXPECT_DOUBLE_EQ(double(archive["Offset"]), 0.0);
  EXPECT_DOUBLE_EQ(double(archive["PredictMaximumExposureTime"]), 3.4317);
  EXPECT_DOUBLE_EQ(double(archive["CassisOffNadirAngle"]), 32.2820);
  EXPECT_DOUBLE_EQ(double(archive["PredictedRepetitionFrequency"]), 790.7);
  EXPECT_DOUBLE_EQ(double(archive["GroundTrackVelocity"]), 2.3616);
  EXPECT_DOUBLE_EQ(double(archive["ForwardRotationAngle"]), 47.93);
  EXPECT_DOUBLE_EQ(double(archive["SpiceMisalignment"]), 174.295);
  EXPECT_DOUBLE_EQ(double(archive["FocalLength"]), 0.8770);
  EXPECT_DOUBLE_EQ(double(archive["FNumber"]), 6.50);
  EXPECT_EQ(int(archive["ExposureTimeCommand"]), 200);
  EXPECT_EQ(int(archive["FrameletNumber"]), 20);
  EXPECT_EQ(int(archive["NumberOfFramelets"]), 40);
  EXPECT_EQ(int(archive["ImageFrequency"]), 1200000);
  EXPECT_EQ(int(archive["NumberOfWindows"]), 6);
  EXPECT_EQ(int(archive["UniqueIdentifier"]), 100799468);
  EXPECT_EQ(archive["ExposureTimestamp"][0], "2f0154373db1aa13");
  EXPECT_DOUBLE_EQ(double(archive["ExposureTimePEHK"]), 0.00192);
  EXPECT_DOUBLE_EQ(double(archive["PixelsPossiblySaturated"]), 0.00);
  EXPECT_EQ(int(archive["WindowCount"]), 1);
  EXPECT_EQ(int(archive["Window1Binning"]), 0);
  EXPECT_EQ(int(archive["Window1StartSample"]), 0);
  EXPECT_EQ(int(archive["Window1EndSample"]), 2047);
  EXPECT_EQ(int(archive["Window1StartLine"]), 354);
  EXPECT_EQ(int(archive["Window1EndLine"]), 632);
  EXPECT_EQ(int(archive["Window2Binning"]), 0);
  EXPECT_EQ(int(archive["Window2StartSample"]), 0);
  EXPECT_EQ(int(archive["Window2EndSample"]), 2047);
  EXPECT_EQ(int(archive["Window2StartLine"]), 712);
  EXPECT_EQ(int(archive["Window2EndLine"]), 967);
  EXPECT_EQ(int(archive["Window3Binning"]), 0);
  EXPECT_EQ(int(archive["Window3StartSample"]), 0);
  EXPECT_EQ(int(archive["Window3EndSample"]), 2047);
  EXPECT_EQ(int(archive["Window3StartLine"]), 1048);
  EXPECT_EQ(int(archive["Window3EndLine"]), 1302);
  EXPECT_EQ(int(archive["Window4Binning"]), 0);
  EXPECT_EQ(int(archive["Window4StartSample"]), 0);
  EXPECT_EQ(int(archive["Window4EndSample"]), 2047);
  EXPECT_EQ(int(archive["Window4StartLine"]), 1409);
  EXPECT_EQ(int(archive["Window4EndLine"]), 1662);
  EXPECT_EQ(int(archive["Window5Binning"]), 0);
  EXPECT_EQ(int(archive["Window5StartSample"]), 640);
  EXPECT_EQ(int(archive["Window5EndSample"]), 767);
  EXPECT_EQ(int(archive["Window5StartLine"]), 200);
  EXPECT_EQ(int(archive["Window5EndLine"]), 208);
  EXPECT_EQ(int(archive["Window6Binning"]), 0);
  EXPECT_EQ(int(archive["Window6StartSample"]), 1280);
  EXPECT_EQ(int(archive["Window6EndSample"]), 1407);
  EXPECT_EQ(int(archive["Window6StartLine"]), 1850);
  EXPECT_EQ(int(archive["Window6EndLine"]), 1858);
  EXPECT_EQ(int(archive["YearDoy"]), 2016331);
  EXPECT_EQ(archive["ObservationId"][0], "CRUS_049218_251_0");

  // BandBin Group
  bandbin = redLabel->findGroup("BandBin", Pvl::Traverse);
  EXPECT_EQ(bandbin["FilterName"][0], "RED");
  EXPECT_DOUBLE_EQ(double(bandbin["Center"]), 836);
  EXPECT_DOUBLE_EQ(double(bandbin["Width"]), 98.5);
  EXPECT_EQ(bandbin["NaifIkCode"][0], "-143422");

  // Kernels Group
  kernels = redLabel->findGroup("Kernels", Pvl::Traverse);
  EXPECT_EQ(int(kernels["NaifFrameCode"]), -143400);

  hist = redCube.histogram();

  EXPECT_NEAR(hist->Average(), 0.098812884362865061, 0.0001);
  EXPECT_NEAR(hist->Sum(), 78810.883871480823, 0.0001);
  EXPECT_EQ(hist->ValidPixels(), 797577);
  EXPECT_NEAR(hist->StandardDeviation(), 0.0020888136703382234, 0.0001);


  // BLU Cube
  Cube bluCube(bluEquiFile);
  Pvl *bluLabel = bluCube.label();

  // Instrument Group
  inst = bluLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ(inst["SpacecraftName"][0], "TRACE GAS ORBITER");
  EXPECT_EQ(inst["InstrumentId"][0], "CaSSIS");
  EXPECT_EQ(inst["TargetName"][0], "Mars" );
  EXPECT_EQ(inst["StartTime"][0], "2016-11-26T22:58:02.583");
  EXPECT_EQ(inst["SpacecraftClockStartCount"][0], "2f0154373db1aa13");
  EXPECT_EQ(inst["ExposureDuration"][0], "1.920e-003");
  EXPECT_EQ(int(inst["SummingMode"]), 0);
  EXPECT_EQ(inst["Filter"][0], "BLU");

  // Archive Group
  archive = bluLabel->findGroup("Archive", Pvl::Traverse);
  EXPECT_EQ(archive["DataSetId"][0], "TBD");
  EXPECT_EQ(archive["ProductVersionId"][0], "UNK");
  EXPECT_EQ(archive["ProductCreationTime"][0], "2017-10-03T10:50:26");
  EXPECT_DOUBLE_EQ(double(archive["ScalingFactor"]), 1.0);
  EXPECT_DOUBLE_EQ(double(archive["Offset"]), 0.0);
  EXPECT_DOUBLE_EQ(double(archive["PredictMaximumExposureTime"]), 3.4317);
  EXPECT_DOUBLE_EQ(double(archive["CassisOffNadirAngle"]), 32.2820);
  EXPECT_DOUBLE_EQ(double(archive["PredictedRepetitionFrequency"]), 790.7);
  EXPECT_DOUBLE_EQ(double(archive["GroundTrackVelocity"]), 2.3616);
  EXPECT_DOUBLE_EQ(double(archive["ForwardRotationAngle"]), 47.93);
  EXPECT_DOUBLE_EQ(double(archive["SpiceMisalignment"]), 174.295);
  EXPECT_DOUBLE_EQ(double(archive["FocalLength"]), 0.8770);
  EXPECT_DOUBLE_EQ(double(archive["FNumber"]), 6.50);
  EXPECT_EQ(int(archive["ExposureTimeCommand"]), 200);
  EXPECT_EQ(int(archive["FrameletNumber"]), 20);
  EXPECT_EQ(int(archive["NumberOfFramelets"]), 40);
  EXPECT_EQ(int(archive["ImageFrequency"]), 1200000);
  EXPECT_EQ(int(archive["NumberOfWindows"]), 6);
  EXPECT_EQ(int(archive["UniqueIdentifier"]), 100799468);
  EXPECT_EQ(archive["ExposureTimestamp"][0], "2f0154373db1aa13");
  EXPECT_DOUBLE_EQ(double(archive["ExposureTimePEHK"]), 0.00192);
  EXPECT_DOUBLE_EQ(double(archive["PixelsPossiblySaturated"]), 0.00);
  EXPECT_EQ(int(archive["WindowCount"]), 3);
  EXPECT_EQ(int(archive["Window1Binning"]), 0);
  EXPECT_EQ(int(archive["Window1StartSample"]), 0);
  EXPECT_EQ(int(archive["Window1EndSample"]), 2047);
  EXPECT_EQ(int(archive["Window1StartLine"]), 354);
  EXPECT_EQ(int(archive["Window1EndLine"]), 632);
  EXPECT_EQ(int(archive["Window2Binning"]), 0);
  EXPECT_EQ(int(archive["Window2StartSample"]), 0);
  EXPECT_EQ(int(archive["Window2EndSample"]), 2047);
  EXPECT_EQ(int(archive["Window2StartLine"]), 712);
  EXPECT_EQ(int(archive["Window2EndLine"]), 966);
  EXPECT_EQ(int(archive["Window3Binning"]), 0);
  EXPECT_EQ(int(archive["Window3StartSample"]), 0);
  EXPECT_EQ(int(archive["Window3EndSample"]), 2047);
  EXPECT_EQ(int(archive["Window3StartLine"]), 1048);
  EXPECT_EQ(int(archive["Window3EndLine"]), 1302);
  EXPECT_EQ(int(archive["Window4Binning"]), 0);
  EXPECT_EQ(int(archive["Window4StartSample"]), 0);
  EXPECT_EQ(int(archive["Window4EndSample"]), 2047);
  EXPECT_EQ(int(archive["Window4StartLine"]), 1409);
  EXPECT_EQ(int(archive["Window4EndLine"]), 1660);
  EXPECT_EQ(int(archive["Window5Binning"]), 0);
  EXPECT_EQ(int(archive["Window5StartSample"]), 640);
  EXPECT_EQ(int(archive["Window5EndSample"]), 767);
  EXPECT_EQ(int(archive["Window5StartLine"]), 200);
  EXPECT_EQ(int(archive["Window5EndLine"]), 208);
  EXPECT_EQ(int(archive["Window6Binning"]), 0);
  EXPECT_EQ(int(archive["Window6StartSample"]), 1280);
  EXPECT_EQ(int(archive["Window6EndSample"]), 1407);
  EXPECT_EQ(int(archive["Window6StartLine"]), 1850);
  EXPECT_EQ(int(archive["Window6EndLine"]), 1858);
  EXPECT_EQ(int(archive["YearDoy"]), 2016331);
  EXPECT_EQ(archive["ObservationId"][0], "CRUS_049218_251_0");

  // BandBin Group
  bandbin = bluLabel->findGroup("BandBin", Pvl::Traverse);
  EXPECT_EQ(bandbin["FilterName"][0], "BLU");
  EXPECT_DOUBLE_EQ(double(bandbin["Center"]), 494.8);
  EXPECT_DOUBLE_EQ(double(bandbin["Width"]), 133.6);
  EXPECT_EQ(bandbin["NaifIkCode"][0], "-143424");

  // Kernels Group
  kernels = bluLabel->findGroup("Kernels", Pvl::Traverse);
  EXPECT_EQ(int(kernels["NaifFrameCode"]), -143400);

  hist = bluCube.histogram();

  EXPECT_NEAR(hist->Average(), 0.051942847688226532, 0.0001);
  EXPECT_NEAR(hist->Sum(), 42226.834142448381, 0.0001);
  EXPECT_EQ(hist->ValidPixels(), 812948);
  EXPECT_NEAR(hist->StandardDeviation(), 0.00085567958401590197, 0.0001);
}


TEST(TgoCassisModuleTests, TgoCassisIngestReingest) {
  QTemporaryDir prefix;

  // run tgocassis2isis on red framelet.
  QString redFileName = prefix.path() + "/redframelet.cub";
  QString digestedFile = prefix.path() + "/redframelet.img";
  QVector<QString> tgocassis2isisArgs = {
                        "from=data/tgoCassis/singleFrameletProj/CAS-MCO-2016-11-26T22.58.02.583-RED-01020-B1.xml",
                        "to=" + redFileName};
  UserInterface tgocassis2isisRed(TGOCASSIS2ISIS_XML, tgocassis2isisArgs);
  try {
    tgocassis2isis(tgocassis2isisRed);
  }
  catch (IException &e) {
    FAIL() << "Unable to run tgocassis2isis on red image: " << e.what() << std::endl;
  }

  // run tgocassisrdrgen on red framelet.
  QVector<QString> rdrgenArgs = {"from=" + redFileName,  "to=" + digestedFile};
  UserInterface rdrgenRed(RDRGEN_XML, rdrgenArgs);
  try {
    tgocassisrdrgen(rdrgenRed);
  }
  catch (IException &e) {
    FAIL() << "Unable to run tgocassisrdrgen on red image: " << e.what() << std::endl;
  }

  // run tgocassis2isis on digested red framelet.
  QString reingestedFile = prefix.path() + "/redframelet.reingest.cub";
  QString digestedXML = prefix.path() + "/redframelet.xml";
  tgocassis2isisArgs = {"from=" + digestedXML, "to=" + reingestedFile};
  UserInterface tgocassis2isisReingest(TGOCASSIS2ISIS_XML, tgocassis2isisArgs);
  try {
    tgocassis2isis(tgocassis2isisReingest);
  }
  catch (IException &e) {
    FAIL() << "Unable to run tgocassis2isis on red image: " << e.what() << std::endl;
  }

  // RED Cube
  Cube redCube(reingestedFile);
  Pvl *redLabel = redCube.label();

  // Instrument Group
  PvlGroup &inst = redLabel->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ(inst["SpacecraftName"][0], "TRACE GAS ORBITER");
  EXPECT_EQ(inst["InstrumentId"][0], "CaSSIS");
  EXPECT_EQ(inst["TargetName"][0], "Mars" );
  EXPECT_EQ(inst["StartTime"][0], "2016-11-26T22:58:02.583");
  EXPECT_EQ(inst["ExposureDuration"][0], "1.920e-003");
  EXPECT_EQ(int(inst["SummingMode"]), 0);
  EXPECT_EQ(inst["Filter"][0], "RED");

  // Archive Group
  PvlGroup &archive = redLabel->findGroup("Archive", Pvl::Traverse);
  EXPECT_DOUBLE_EQ(double(archive["ProductVersionId"]), 1.0);
  EXPECT_DOUBLE_EQ(double(archive["ScalingFactor"]), 1.0);
  EXPECT_EQ(int(archive["YearDoy"]), 2016331);
  EXPECT_EQ(archive["ObservationId"][0], "CRUS_049218_251_0");

  // BandBin Group
  PvlGroup &bandbin = redLabel->findGroup("BandBin", Pvl::Traverse);
  EXPECT_EQ(bandbin["FilterName"][0], "RED");
  EXPECT_DOUBLE_EQ(double(bandbin["Center"]), 836);
  EXPECT_DOUBLE_EQ(double(bandbin["Width"]), 98.5);
  EXPECT_EQ(bandbin["NaifIkCode"][0], "-143422");

  // Kernels Group
  PvlGroup &kernels = redLabel->findGroup("Kernels", Pvl::Traverse);
  EXPECT_EQ(int(kernels["NaifFrameCode"]), -143400);

  Histogram *hist = redCube.histogram();

  EXPECT_NEAR(hist->Average(), 0.098812884362865061, 0.0001);
  EXPECT_NEAR(hist->Sum(), 51800.457383409142, 0.0001);
  EXPECT_EQ(hist->ValidPixels(), 524288);
  EXPECT_NEAR(hist->StandardDeviation(), 0.0020888136703382234, 0.0001);
}


TEST_F(TgoCassisModuleKernels, TgoCassisTestColorMosaic) {
  QTemporaryDir prefix;

  // run tgocassis2isis and spiceinit on pan framelet.
  QString panFileName = prefix.path() + "/panframelet.cub";
  QVector<QString> tgocassis2isisArgs = {"from=data/tgoCassis/CAS-MCO-2016-11-26T22.50.27.381-PAN-00005-B1.xml",
                                         "to=" + panFileName};

  UserInterface tgocassis2isisPan(TGOCASSIS2ISIS_XML, tgocassis2isisArgs);
  try {
    tgocassis2isis(tgocassis2isisPan);
  }
  catch (IException &e) {
    FAIL() << "Unable to run tgocassis2isis on pan image: " << e.what() << std::endl;
  }

  QVector<QString> spiceinitArgs = {"from=" + panFileName,
                                    "ck=" + binaryCkKernelsAsString,
                                    "spk=" + binarySpkKernelsAsString};
  UserInterface spiceinitPan(SPICEINIT_XML, spiceinitArgs);
  try {
    spiceinit(spiceinitPan);
  }
  catch (IException &e) {
    FAIL() << "Unable to run spiceinit on pan image: " << e.what() << std::endl;
  }

  // run tgocassis2isis and spiceinit on red framelet.
  QString redFileName = prefix.path() + "/redframelet.cub";
  tgocassis2isisArgs = {"from=data/tgoCassis/CAS-MCO-2016-11-26T22.50.27.381-RED-01005-B1.xml",
                        "to=" + redFileName};
  UserInterface tgocassis2isisRed(TGOCASSIS2ISIS_XML, tgocassis2isisArgs);
  try {
    tgocassis2isis(tgocassis2isisRed);
  }
  catch (IException &e) {
    FAIL() << "Unable to run tgocassis2isis on pan image: " << e.what() << std::endl;
  }

  spiceinitArgs = {"from=" + redFileName,
                   "ck=" + binaryCkKernelsAsString,
                   "spk=" + binarySpkKernelsAsString};
  UserInterface spiceinitRed(SPICEINIT_XML, spiceinitArgs);
  try {
    spiceinit(spiceinitRed);
  }
  catch (IException &e) {
    FAIL() << "Unable to run spiceinit on pan image: " << e.what() << std::endl;
  }

  // run tgocassis2isis and spiceinit on blu framelet.
  QString bluFileName = prefix.path() + "/bluframelet.cub";
  tgocassis2isisArgs = {"from=data/tgoCassis/CAS-MCO-2016-11-26T22.50.27.381-BLU-03005-B1.xml",
                        "to=" + bluFileName};
  UserInterface tgocassis2isisBlu(TGOCASSIS2ISIS_XML, tgocassis2isisArgs);
  try {
    tgocassis2isis(tgocassis2isisBlu);
  }
  catch (IException &e) {
    FAIL() << "Unable to run tgocassis2isis on blu image: " << e.what() << std::endl;
  }

  spiceinitArgs = {"from=" + bluFileName,
                   "ck=" + binaryCkKernelsAsString,
                   "spk=" + binarySpkKernelsAsString};
  UserInterface spiceinitBlu(SPICEINIT_XML, spiceinitArgs);
  try {
    spiceinit(spiceinitBlu);
  }
  catch (IException &e) {
    FAIL() << "Unable to run spiceinit on blu image: " << e.what() << std::endl;
  }

  // run tgocassis2isis and spiceinit on nir framelet.
  QString nirFileName = prefix.path() + "/nirframelet.cub";
  tgocassis2isisArgs = {"from=data/tgoCassis/CAS-MCO-2016-11-26T22.50.27.381-NIR-02005-B1.xml",
                        "to=" + nirFileName};
  UserInterface tgocassis2isisNir(TGOCASSIS2ISIS_XML, tgocassis2isisArgs);
  try {
    tgocassis2isis(tgocassis2isisNir);
  }
  catch (IException &e) {
    FAIL() << "Unable to run tgocassis2isis on nir image: " << e.what() << std::endl;
  }

  spiceinitArgs = {"from=" + nirFileName,
                   "ck=" + binaryCkKernelsAsString,
                   "spk=" + binarySpkKernelsAsString};
  UserInterface spiceinitNir(SPICEINIT_XML, spiceinitArgs);
  try {
    spiceinit(spiceinitNir);
  }
  catch (IException &e) {
    FAIL() << "Unable to run spiceinit  on nir image: " << e.what() << std::endl;
  }

  // run mosrange on cube list
  FileList *cubeList = new FileList();
  cubeList->append(panFileName);
  cubeList->append(redFileName);
  cubeList->append(bluFileName);
  cubeList->append(nirFileName);

  QString cubeListFile = prefix.path() + "/cubelist.lis";
  cubeList->write(cubeListFile);

  QString mapFile = prefix.path() + "/equi.map";
  QVector<QString> mosrangeArgs = {"fromlist=" + cubeListFile, "to=" + mapFile};
  UserInterface mosrangeOptions(MOSRANGE_XML, mosrangeArgs);

  try {
    mosrange(mosrangeOptions);
  }
  catch (IException &e) {
    FAIL() << "Unable to run mosrange with cube list: " << e.what() << std::endl;
  }

  // run cam2map and cassismos on pan cube
  QString panEquiFile = prefix.path() + "/pan_equi.cub";
  QVector<QString> cam2mapArgs = {"from=" + panFileName,
                                  "to=" + panEquiFile,
                                  "map=" + mapFile,
                                  "defaultrange=map",
                                  "pixres=mpp",
                                  "resolution=200"};
  UserInterface cam2mapPan(CAM2MAP_XML, cam2mapArgs);
  try {
    cam2map(cam2mapPan);
  }
  catch (IException &e) {
    FAIL() << "Unable to run cam2map on pan image: " << e.what() << std::endl;
  }

  FileList *panMosaicList = new FileList();
  panMosaicList->append(panEquiFile);
  QString panListFile = prefix.path() + "/panMosaic.lis";
  panMosaicList->write(panListFile);

  QString panCassisMosaic = prefix.path() + "/panCassisMosaic.cub";
  QVector<QString> cassismosArgs = {"fromlist=" + panListFile, "to=" + panCassisMosaic};
  UserInterface tgocassismosPan(MOS_XML, cassismosArgs);
  try {
    tgocassismos(tgocassismosPan);
  }
  catch (IException &e) {
    FAIL() << "Unable to run tgocassismos on pan image: " << e.what() << std::endl;
  }

  // run cam2map and cassismos on nir cube
  QString nirEquiFile = prefix.path() + "/nir_equi.cub";
  cam2mapArgs = {"from=" + nirFileName,
                 "to=" + nirEquiFile,
                 "map=" + mapFile,
                 "defaultrange=map",
                 "pixres=mpp",
                 "resolution=200"};
  UserInterface cam2mapNir(CAM2MAP_XML, cam2mapArgs);
  try {
    cam2map(cam2mapNir);
  }
  catch (IException &e) {
    FAIL() << "Unable to run cam2map on nir image: " << e.what() << std::endl;
  }

  FileList *nirMosaicList = new FileList();
  nirMosaicList->append(nirEquiFile);
  QString nirListFile = prefix.path() + "/nirMosaic.lis";
  nirMosaicList->write(nirListFile);

  QString nirCassisMosaic = prefix.path() + "/nirCassisMosaic.cub";
  cassismosArgs = {"fromlist=" + nirListFile, "to=" + nirCassisMosaic};
  UserInterface tgocassismosNir(MOS_XML, cassismosArgs);
  try {
    tgocassismos(tgocassismosNir);
  }
  catch (IException &e) {
    FAIL() << "Unable to run tgocassismos on nir image: " << e.what() << std::endl;
  }

  // run cam2map and cassismos on blu cube
  QString bluEquiFile = prefix.path() + "/blu_equi.cub";
  cam2mapArgs = {"from=" + bluFileName,
                 "to=" + bluEquiFile,
                 "map=" + mapFile,
                 "defaultrange=map",
                 "pixres=mpp",
                 "resolution=200"};
  UserInterface cam2mapBlu(CAM2MAP_XML, cam2mapArgs);
  try {
    cam2map(cam2mapBlu);
  }
  catch (IException &e) {
    FAIL() << "Unable to run cam2map on blu image: " << e.what() << std::endl;
  }

  FileList *bluMosaicList = new FileList();
  bluMosaicList->append(bluEquiFile);
  QString bluListFile = prefix.path() + "/bluMosaic.lis";
  bluMosaicList->write(bluListFile);

  QString bluCassisMosaic = prefix.path() + "/bluCassisMosaic.cub";
  cassismosArgs = {"fromlist=" + bluListFile, "to=" + bluCassisMosaic};
  UserInterface tgocassismosBlu(MOS_XML, cassismosArgs);
  try {
    tgocassismos(tgocassismosBlu);
  }
  catch (IException &e) {
    FAIL() << "Unable to run tgocassismos on blu image: " << e.what() << std::endl;
  }

  // run cam2map and cassismos on red cube
  QString redEquiFile = prefix.path() + "/red_equi.cub";
  cam2mapArgs = {"from=" + redFileName,
                  "to=" + redEquiFile,
                  "map=" + mapFile,
                  "defaultrange=map",
                  "pixres=mpp",
                  "resolution=200"};
  UserInterface cam2mapRed(CAM2MAP_XML, cam2mapArgs);
  try {
    cam2map(cam2mapRed);
  }
  catch (IException &e) {
    FAIL() << "Unable to run cam2map on red image: " << e.what() << std::endl;
  }

  FileList *redMosaicList = new FileList();
  redMosaicList->append(redEquiFile);
  QString redListFile = prefix.path() + "/redMosaic.lis";
  redMosaicList->write(redListFile);

  QString redCassisMosaic = prefix.path() + "/redCassisMosaic.cub";
  cassismosArgs = {"fromlist=" + redListFile, "to=" + redCassisMosaic};
  UserInterface tgocassismosRed(MOS_XML, cassismosArgs);
  try {
    tgocassismos(tgocassismosRed);
  }
  catch (IException &e) {
    FAIL() << "Unable to run tgocassismos on red image: " << e.what() << std::endl;
  }


  // run cubeit
  FileList *MosaicList = new FileList();
  MosaicList->append(redCassisMosaic);
  MosaicList->append(bluCassisMosaic);
  MosaicList->append(nirCassisMosaic);
  MosaicList->append(panCassisMosaic);
  QString mosListFile = prefix.path() + "/mosaicList.lis";
  MosaicList->write(mosListFile);

  QString coloredMosaic = prefix.path() + "/coloredMosaic.cub";
  QVector<QString> cubeitArgs = {"fromlist=" + mosListFile, "to=" + coloredMosaic};
  UserInterface cubeitUI(CUBEIT_XML, cubeitArgs);
  try {
    cubeit(cubeitUI);
  }
  catch (IException &e) {
    FAIL() << "Unable to run cubeit on mosaic list: " << e.what() << std::endl;
  }

  // Mosaic Cube
  Cube mosCube(coloredMosaic);
  Pvl *outLabel = mosCube.label();

  std::istringstream mos(R"(
    Group = Mosaic
      SpacecraftName            = "TRACE GAS ORBITER"
      InstrumentId              = CaSSIS
      ObservationId             = CRUS_049218_201_0
      StartTime                 = 2016-11-26T22:50:27.381
      StopTime                  = 2016-11-26T22:50:27.382
      SpacecraftClockStartCount = 2f015435767e275a
      IncidenceAngle            = 44.946650468616 <degrees>
      EmissionAngle             = 11.637754697441 <degrees>
      PhaseAngle                = 44.136937967978 <degrees>
      LocalTime                 = 14.429448515306
      SolarLongitude            = 269.1366003982 <degrees>
      SubSolarAzimuth           = 139.56469945225 <degrees>
      NorthAzimuth              = 270.0 <degrees>
    End_Group
  )");

  PvlGroup truthMosGroup;
  mos >> truthMosGroup;
  PvlGroup &mosGroup = outLabel->findGroup("Mosaic", Pvl::Traverse);

  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, mosGroup, truthMosGroup);

  std::istringstream arss(R"(
    Group = Archive
      DataSetId                    = TBD
      ProductVersionId             = UNK
      ProducerId                   = UBE
      ProducerName                 = "Nicolas Thomas"
      ProductCreationTime          = 2017-10-03T10:50:12
      FileName                     = CAS-MCO-2016-11-26T22.50.27.381-RED-01005-B1
      ScalingFactor                = 1.00
      Offset                       = 0.00
      PredictMaximumExposureTime   = 1.5952 <ms>
      CassisOffNadirAngle          = 10.032 <deg>
      PredictedRepetitionFrequency = 367.5 <ms>
      GroundTrackVelocity          = 3.4686 <km/s>
      ForwardRotationAngle         = 52.703 <deg>
      SpiceMisalignment            = 185.422 <deg>
      FocalLength                  = 0.8770 <m>
      FNumber                      = 6.50
      ExposureTimeCommand          = 150
      FrameletNumber               = 5
      NumberOfFramelets            = 40
      ImageFrequency               = 400000 <ms>
      NumberOfWindows              = 6
      UniqueIdentifier             = 100799268
      UID                          = 100799268
      ExposureTimestamp            = 2f015435767e275a
      ExposureTimePEHK             = 1.440e-003 <ms>
      PixelsPossiblySaturated      = 0.16
      IFOV                         = 1.140e-005
      IFOVUnit                     = rad/px
      FiltersAvailable             = "BLU RED NIR PAN"
      FocalLengthUnit              = M
      TelescopeType                = "Three-mirror anastigmat with powered fold mirror"
      DetectorDescription          = "2D Array"
      PixelHeight                  = 10.0
      PixelHeightUnit              = MICRON
      PixelWidth                   = 10.0
      PixelWidthUnit               = MICRON
      DetectorType                 = 'SI CMOS HYBRID (OSPREY 2K)'
      ReadNoise                    = 61.0
      ReadNoiseUnit                = ELECTRON
      MissionPhase                 = MCO
      SubInstrumentIdentifier      = 61.0
      WindowCount                  = 1
      Window1Binning               = 0
      Window1StartSample           = 0
      Window1EndSample             = 2047
      Window1StartLine             = 354
      Window1EndLine               = 632
      Window2Binning               = 0
      Window2StartSample           = 0
      Window2EndSample             = 2047
      Window2StartLine             = 712
      Window2EndLine               = 967
      Window3Binning               = 1
      Window3StartSample           = 0
      Window3EndSample             = 2047
      Window3StartLine             = 1048
      Window3EndLine               = 1302
      Window4Binning               = 0
      Window4StartSample           = 1024
      Window4EndSample             = 1087
      Window4StartLine             = 1409
      Window4EndLine               = 1662
      Window5Binning               = 0
      Window5StartSample           = 640
      Window5EndSample             = 767
      Window5StartLine             = 200
      Window5EndLine               = 208
      Window6Binning               = 0
      Window6StartSample           = 1280
      Window6EndSample             = 1407
      Window6StartLine             = 1850
      Window6EndLine               = 1858
      YearDoy                      = 2016331
      ObservationId                = CRUS_049218_201_0
    End_Group
  )");

  PvlGroup truthArchiveGroup;
  arss >> truthArchiveGroup;

  PvlGroup &archiveGroup = outLabel->findGroup("Archive", Pvl::Traverse);

  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, archiveGroup, truthArchiveGroup);

  std::istringstream bbss(R"(
    Group = BandBin
      FilterName = (RED, BLU, NIR, PAN)
      Center     = (836.0, 494.8, 939.3, 678.2) <nm>
      Width      = (98.5, 133.6, 121.8, 231.9) <nm>
      NaifIkCode = (-143422, -143424, -143423, -143421)
    End_Group
  )");

  PvlGroup truthBandBinGroup;
  bbss >> truthBandBinGroup;

  PvlGroup &bandBinGroup = outLabel->findGroup("BandBin", Pvl::Traverse);

  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, bandBinGroup, truthBandBinGroup);

  std::istringstream map(R"(
    Group = Mapping
      ProjectionName       = Equirectangular
      CenterLongitude      = 266.21338321885
      TargetName           = Mars
      EquatorialRadius     = 3396190.0 <meters>
      PolarRadius          = 3376200.0 <meters>
      LatitudeType         = Planetocentric
      LongitudeDirection   = PositiveEast
      LongitudeDomain      = 360
      MinimumLatitude      = 2.465491209879
      MaximumLatitude      = 2.703757297152
      MinimumLongitude     = 266.13827437353
      MaximumLongitude     = 266.28849206417
      UpperLeftCornerX     = -4600.0 <meters>
      UpperLeftCornerY     = 160400.0 <meters>
      PixelResolution      = 200.0 <meters/pixel>
      Scale                = 296.3699086728 <pixels/degree>
      CenterLatitude       = 2.584624253516
      CenterLatitudeRadius = 3396148.9883258
    End_Group
  )");

  PvlGroup truthMappingGroup;
  map >> truthMappingGroup;

  PvlGroup &mappingGroup = outLabel->findGroup("Mapping", Pvl::Traverse);

  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, mappingGroup, truthMappingGroup);

  Histogram *hist = mosCube.histogram();

  EXPECT_NEAR(hist->Average(), 0.29920571615330949, 0.0001);
  EXPECT_NEAR(hist->Sum(), 183.71230971813202, 0.0001);
  EXPECT_EQ(hist->ValidPixels(), 614);
  EXPECT_NEAR(hist->StandardDeviation(), 0.0054483425167489693, 0.0001);
}


TEST_F(TgoCassisModuleKernels, TgoCassisMapProjectedReingested) {
  QTemporaryDir prefix;

  // run tgocassis2isis on red framelet.
  QString outputCubeName = prefix.path() + "CAS-M01-2018-05-05T23.11.48.767-RED-01029-B1.cub";
  QString digestedFile = prefix.path() + "/CAS-M01-2018-05-05T23.11.48.767-RED-01029-B1.equi.img";
  QVector<QString> tgocassis2isisArgs = {
                        "from=data/tgoCassis/mapProjectedReingested/CAS-M01-2018-05-05T23.11.48.767-RED-01029-B1.xml",
                        "to=" + outputCubeName};
  UserInterface tgocassis2isisUi(TGOCASSIS2ISIS_XML, tgocassis2isisArgs);
  try {
    tgocassis2isis(tgocassis2isisUi);
  }
  catch (IException &e) {
    FAIL() << "Unable to run tgocassis2isis on image: " << e.what() << std::endl;
  }

  // run spiceinit on framelet.
  QVector<QString> spiceinitArgs = {"from=" + outputCubeName,
                                    "ck=" + binaryCkKernelsAsString,
                                    "spk=" + binarySpkKernelsAsString};
  UserInterface spiceinitUi(SPICEINIT_XML, spiceinitArgs);
  try {
    spiceinit(spiceinitUi);
  }
  catch (IException &e) {
    FAIL() << "Unable to run spiceinit on image: " << e.what() << std::endl;
  }

  // run cam2map on pan cube
  QString projCubeName = prefix.path() + "/CAS-M01-2018-05-05T23.11.48.767-RED-01029-B1.equi.cub";
  QString mapFile = "data/tgoCassis/mapProjectedReingested/equi.map";
  QVector<QString> cam2mapArgs = {"from=" + outputCubeName,
                                  "to=" + projCubeName,
                                  "map=" + mapFile,
                                  "pixres=mpp",
                                  "resolution=200"};
  UserInterface cam2mapUi(CAM2MAP_XML, cam2mapArgs);
  try {
    cam2map(cam2mapUi);
  }
  catch (IException &e) {
    FAIL() << "Unable to run cam2map on image: " << e.what() << std::endl;
  }

  // run tgocassisrdrgen on image.
  QVector<QString> rdrgenArgs = {"from=" + projCubeName,  "to=" + digestedFile};
  UserInterface rdrgenUi(RDRGEN_XML, rdrgenArgs);
  try {
    tgocassisrdrgen(rdrgenUi);
  }
  catch (IException &e) {
    FAIL() << "Unable to run tgocassisrdrgen on image: " << e.what() << std::endl;
  }

  // run tgocassis2isis on digested red framelet.
  QString digestedXML = prefix.path() + "/CAS-M01-2018-05-05T23.11.48.767-RED-01029-B1.equi.xml";
  QString reingestedFile = prefix.path() + "/CAS-M01-2018-05-05T23.11.48.767-RED-01029-B1.equi.reingested.cub";
  tgocassis2isisArgs = {"from=" + digestedXML, "to=" + reingestedFile};
  UserInterface tgocassis2isisReingest(TGOCASSIS2ISIS_XML, tgocassis2isisArgs);
  try {
    tgocassis2isis(tgocassis2isisReingest);
  }
  catch (IException &e) {
    FAIL() << "Unable to run tgocassis2isis on red image: " << e.what() << std::endl;
  }

  // RED Cube
  Cube reingestCube(reingestedFile);
  Pvl *reingestLabel = reingestCube.label();

  // Instrument Group
  std::istringstream iss(R"(
    Group = Instrument
      SpacecraftName   = "TRACE GAS ORBITER"
      InstrumentId     = CaSSIS
      Expanded         = 1
      TargetName       = Mars
      StartTime        = 2018-05-05T23:11:48.767
      ExposureDuration = 1.488e-003 <seconds>
      Filter           = RED
      Expanded         = 1
      SummingMode      = 0
    End_Group
  )");

  PvlGroup truthInstGroup;
  iss >> truthInstGroup;
  PvlGroup &instGroup = reingestLabel->findGroup("Instrument", Pvl::Traverse);

  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, instGroup, truthInstGroup);

  // Archive Group
  std::istringstream arss(R"(
    Group = Archive
      ObservationId    = MY34_002002_211_2
      DataSetId        = urn:esa:psa:em16_tgo_cas:data_projected:my34_002002_211_2
      ProductVersionId = 1.0
      FileName         = CAS-M01-2018-05-05T23.11.48.767-RED-01029-B1.equi.img
      ScalingFactor    = 1.0
      YearDoy          = 2018125
    End_Group
  )");

  PvlGroup truthArchiveGroup;
  arss >> truthArchiveGroup;

  PvlGroup &archiveGroup = reingestLabel->findGroup("Archive", Pvl::Traverse);

  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, archiveGroup, truthArchiveGroup);

  // BandBin Group
  std::istringstream bbss(R"(
    Group = BandBin
      FilterName = RED
      Center     = 836.0 <nm>
      Width      = 98.5 <nm>
      NaifIkCode = -143422
    End_Group
  )");

  PvlGroup truthBandBinGroup;
  bbss >> truthBandBinGroup;

  PvlGroup &bandBinGroup = reingestLabel->findGroup("BandBin", Pvl::Traverse);

  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, bandBinGroup, truthBandBinGroup);

  // Kernels Group
  PvlGroup &kernels = reingestLabel->findGroup("Kernels", Pvl::Traverse);
  EXPECT_EQ(int(kernels["NaifFrameCode"]), -143400);

  Histogram *hist = reingestCube.histogram();

  EXPECT_NEAR(hist->Average(), 0.11603580358533563, 0.0001);
  EXPECT_NEAR(hist->Sum(), 26.108894683420658, 0.0001);
  EXPECT_EQ(hist->ValidPixels(), 225);
  EXPECT_NEAR(hist->StandardDeviation(), 0.0031002995166270952, 0.0001);
}


TEST_F(TgoCassisModuleKernels, TgoCassisSingleColorMosaicReingest) {
  QTemporaryDir prefix;

  // run tgocassis2isis and spiceinit on pan framelet.
  QString panFileName = prefix.path() + "/panframelet.cub";
  QVector<QString> tgocassis2isisArgs = {"from=data/tgoCassis/CAS-MCO-2016-11-26T22.50.27.381-PAN-00005-B1.xml",
                                         "to=" + panFileName};

  UserInterface tgocassis2isisPan(TGOCASSIS2ISIS_XML, tgocassis2isisArgs);
  try {
    tgocassis2isis(tgocassis2isisPan);
  }
  catch (IException &e) {
    FAIL() << "Unable to run tgocassis2isis on pan image: " << e.what() << std::endl;
  }

  QVector<QString> spiceinitArgs = {"from=" + panFileName,
                                    "ck=" + binaryCkKernelsAsString,
                                    "spk=" + binarySpkKernelsAsString};
  UserInterface spiceinitPan(SPICEINIT_XML, spiceinitArgs);
  try {
    spiceinit(spiceinitPan);
  }
  catch (IException &e) {
    FAIL() << "Unable to run spiceinit on pan image: " << e.what() << std::endl;
  }

  // run mosrange on cube list
  FileList *cubeList = new FileList();
  cubeList->append(panFileName);

  QString cubeListFile = prefix.path() + "/cubelist.lis";
  cubeList->write(cubeListFile);

  QString mapFile = prefix.path() + "/equi.map";
  QVector<QString> mosrangeArgs = {"fromlist=" + cubeListFile, "to=" + mapFile};
  UserInterface mosrangeOptions(MOSRANGE_XML, mosrangeArgs);

  try {
    mosrange(mosrangeOptions);
  }
  catch (IException &e) {
    FAIL() << "Unable to run mosrange with cube list: " << e.what() << std::endl;
  }

  // run cam2map and cassismos on pan cube
  QString panEquiFile = prefix.path() + "/pan_equi.cub";
  QVector<QString> cam2mapArgs = {"from=" + panFileName,
                                  "to=" + panEquiFile,
                                  "map=" + mapFile,
                                  "defaultrange=map",
                                  "pixres=mpp",
                                  "resolution=200"};
  UserInterface cam2mapPan(CAM2MAP_XML, cam2mapArgs);
  try {
    cam2map(cam2mapPan);
  }
  catch (IException &e) {
    FAIL() << "Unable to run cam2map on pan image: " << e.what() << std::endl;
  }

  FileList *mosaicList = new FileList();
  mosaicList->append(panEquiFile);
  QString listFile = prefix.path() + "/cubelist.lis";
  mosaicList->write(listFile);

  QString mosaicCubeFile = prefix.path() + "/mosaic.cub";
  QVector<QString> cassismosArgs = {"fromlist=" + listFile, "to=" + mosaicCubeFile};
  UserInterface options(MOS_XML, cassismosArgs);
  try {
    tgocassismos(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to run tgocassismos on mosaic list: " << e.what() << std::endl;
  }

  // run tgocassisrdrgen on color mosaic.
  QString digestedFile = prefix.path() + "/mosaic.img";
  QVector<QString> rdrgenArgs = {"from=" + mosaicCubeFile,  "to=" + digestedFile};
  UserInterface rdrgen(RDRGEN_XML, rdrgenArgs);
  try {
    tgocassisrdrgen(rdrgen);
  }
  catch (IException &e) {
    FAIL() << "Unable to run tgocassisrdrgen on color mosiac image: " << e.what() << std::endl;
  }

  // run tgocassis2isis on digested color mosaic.
  QString reingestedFile = prefix.path() + "/mosaic.reingest.cub";
  QString digestedXML = prefix.path() + "/mosaic.xml";
  tgocassis2isisArgs = {"from=" + digestedXML, "to=" + reingestedFile};
  UserInterface tgocassis2isisReingest(TGOCASSIS2ISIS_XML, tgocassis2isisArgs);
  try {
    tgocassis2isis(tgocassis2isisReingest);
  }
  catch (IException &e) {
    FAIL() << "Unable to run tgocassis2isis on color mosaic image: " << e.what() << std::endl;
  }

  // Mosaic Cube
  Cube mosCube(reingestedFile);
  Pvl *outLabel = mosCube.label();

  std::istringstream inst(R"(
    Group = Instrument
      SpacecraftName = "TRACE GAS ORBITER"
      InstrumentId   = CaSSIS
      Expanded       = 1
      TargetName     = Mars
      StartTime      = 2016-11-26T22:50:27.381
      Filter         = PAN
      Expanded       = 1
      SummingMode    = 0
    End_Group
  )");

  PvlGroup truthInstGroup;
  inst >> truthInstGroup;
  PvlGroup &instGroup = outLabel->findGroup("Instrument", Pvl::Traverse);

  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, instGroup, truthInstGroup);

  std::istringstream arss(R"(
    Group = Archive
      ObservationId    = CRUS_049218_201_0
      DataSetId        = urn:esa:psa:em16_tgo_cas:data_derived:crus_049218_201_0
      ProductVersionId = 1.0
      FileName         = mosaic.img
      ScalingFactor    = 1.0
      YearDoy          = 2016331
    End_Group
  )");

  PvlGroup truthArchiveGroup;
  arss >> truthArchiveGroup;

  PvlGroup &archiveGroup = outLabel->findGroup("Archive", Pvl::Traverse);

  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, archiveGroup, truthArchiveGroup);

  std::istringstream bbss(R"(
    Group = BandBin
      FilterName = PAN
      Center     = 678.2 <nm>
      Width      = 231.9 <nm>
      NaifIkCode = -143421
    End_Group
  )");

  PvlGroup truthBandBinGroup;
  bbss >> truthBandBinGroup;

  PvlGroup &bandBinGroup = outLabel->findGroup("BandBin", Pvl::Traverse);

  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, bandBinGroup, truthBandBinGroup);

  std::istringstream map(R"(
    Group = Mapping
      ProjectionName     = Equirectangular
      CenterLongitude    = 266.15724842165
      TargetName         = Mars
      EquatorialRadius   = 3396190.0
      PolarRadius        = 3376200.0
      LatitudeType       = Planetocentric
      LongitudeDirection = PositiveEast
      LongitudeDomain    = 360
      MinimumLatitude    = 2.4698863724983
      MaximumLatitude    = 2.7060776922727
      MinimumLongitude   = 266.1741364076
      MaximumLongitude   = 266.13698283851
      UpperLeftCornerX   = -1200.0
      UpperLeftCornerY   = 160400.0
      PixelResolution    = 200.0
      Scale              = 296.36990921958
      CenterLatitude     = 0.0
    End_Group
  )");

  PvlGroup truthMappingGroup;
  map >> truthMappingGroup;

  PvlGroup &mappingGroup = outLabel->findGroup("Mapping", Pvl::Traverse);

  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, mappingGroup, truthMappingGroup);

  Histogram *hist = mosCube.histogram();

  EXPECT_NEAR(hist->Average(), 0.20770993546981495, 0.0001);
  EXPECT_NEAR(hist->Sum(), 137.29626734554768, 0.0001);
  EXPECT_EQ(hist->ValidPixels(), 661);
  EXPECT_NEAR(hist->StandardDeviation(), 0.0022430344774779496, 0.0001);
}


TEST(TgoCassisModuleTests, TgoCassisUncontrolledSingleColorMosaic) {
  QTemporaryDir prefix;

  // run tgocassis2isis and spiceinit on pan framelet.
  QString panFileName = prefix.path() + "/panframelet.cub";
  QVector<QString> tgocassis2isisArgs = {"from=data/tgoCassis/CAS-MCO-2016-11-26T22.50.27.381-PAN-00005-B1.xml",
                                         "to=" + panFileName};

  UserInterface tgocassis2isisPan(TGOCASSIS2ISIS_XML, tgocassis2isisArgs);
  try {
    tgocassis2isis(tgocassis2isisPan);
  }
  catch (IException &e) {
    FAIL() << "Unable to run tgocassis2isis on pan image: " << e.what() << std::endl;
  }

  QVector<QString> spiceinitArgs = {"from=" + panFileName,  "ckp=t", "spkp=t"};
  UserInterface spiceinitPan(SPICEINIT_XML, spiceinitArgs);
  try {
    spiceinit(spiceinitPan);
  }
  catch (IException &e) {
    FAIL() << "Unable to run spiceinit on pan image: " << e.what() << std::endl;
  }

  // run mosrange on cube list
  FileList *cubeList = new FileList();
  cubeList->append(panFileName);

  QString cubeListFile = prefix.path() + "/cubelist.lis";
  cubeList->write(cubeListFile);

  QString mapFile = prefix.path() + "/equi.map";
  QVector<QString> mosrangeArgs = {"fromlist=" + cubeListFile, "to=" + mapFile};
  UserInterface mosrangeOptions(MOSRANGE_XML, mosrangeArgs);

  try {
    mosrange(mosrangeOptions);
  }
  catch (IException &e) {
    FAIL() << "Unable to run mosrange with cube list: " << e.what() << std::endl;
  }

  // run cam2map and cassismos on pan cube
  QString panEquiFile = prefix.path() + "/pan_equi.cub";
  QVector<QString> cam2mapArgs = {"from=" + panFileName,
                                  "to=" + panEquiFile,
                                  "map=" + mapFile,
                                  "defaultrange=map",
                                  "pixres=mpp",
                                  "resolution=200"};
  UserInterface cam2mapPan(CAM2MAP_XML, cam2mapArgs);
  try {
    cam2map(cam2mapPan);
  }
  catch (IException &e) {
    FAIL() << "Unable to run cam2map on pan image: " << e.what() << std::endl;
  }

  FileList *mosaicList = new FileList();
  mosaicList->append(panEquiFile);
  QString listFile = prefix.path() + "/cubelist.lis";
  mosaicList->write(listFile);

  QString mosaicCubeFile = prefix.path() + "/mosaic.cub";
  QVector<QString> cassismosArgs = {"fromlist=" + listFile, "to=" + mosaicCubeFile};
  UserInterface options(MOS_XML, cassismosArgs);
  try {
    tgocassismos(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to run tgocassismos on mosaic list: " << e.what() << std::endl;
  }

  // Mosaic Cube
  Cube mosCube(mosaicCubeFile);
  Pvl *outLabel = mosCube.label();

  std::istringstream arss(R"(
    Group = Archive
        DataSetId                    = TBD
        ProductVersionId             = UNK
        ProducerId                   = UBE
        ProducerName                 = "Nicolas Thomas"
        ProductCreationTime          = 2017-10-03T10:50:12
        FileName                     = CAS-MCO-2016-11-26T22.50.27.381-PAN-00005--
                                       B1
        ScalingFactor                = 1.00
        Offset                       = 0.00
        PredictMaximumExposureTime   = 1.5952 <ms>
        CassisOffNadirAngle          = 10.032 <deg>
        PredictedRepetitionFrequency = 367.5 <ms>
        GroundTrackVelocity          = 3.4686 <km/s>
        ForwardRotationAngle         = 52.703 <deg>
        SpiceMisalignment            = 185.422 <deg>
        FocalLength                  = 0.8770 <m>
        FNumber                      = 6.50
        ExposureTimeCommand          = 150
        FrameletNumber               = 5
        NumberOfFramelets            = 40
        ImageFrequency               = 400000 <ms>
        NumberOfWindows              = 6
        UniqueIdentifier             = 100799268
        UID                          = 100799268
        ExposureTimestamp            = 2f015435767e275a
        ExposureTimePEHK             = 1.440e-003 <ms>
        PixelsPossiblySaturated      = 29.17
        IFOV                         = 1.140e-005
        IFOVUnit                     = rad/px
        FiltersAvailable             = "BLU RED NIR PAN"
        FocalLengthUnit              = M
        TelescopeType                = "Three-mirror anastigmat with powered fold
                                        mirror"
        DetectorDescription          = "2D Array"
        PixelHeight                  = 10.0
        PixelHeightUnit              = MICRON
        PixelWidth                   = 10.0
        PixelWidthUnit               = MICRON
        DetectorType                 = 'SI CMOS HYBRID (OSPREY 2K)'
        ReadNoise                    = 61.0
        ReadNoiseUnit                = ELECTRON
        MissionPhase                 = MCO
        SubInstrumentIdentifier      = 61.0
        WindowCount                  = 0
        Window1Binning               = 0
        Window1StartSample           = 0
        Window1EndSample             = 2047
        Window1StartLine             = 354
        Window1EndLine               = 633
        Window2Binning               = 0
        Window2StartSample           = 0
        Window2EndSample             = 2047
        Window2StartLine             = 712
        Window2EndLine               = 966
        Window3Binning               = 1
        Window3StartSample           = 0
        Window3EndSample             = 2047
        Window3StartLine             = 1048
        Window3EndLine               = 1302
        Window4Binning               = 0
        Window4StartSample           = 1024
        Window4EndSample             = 1087
        Window4StartLine             = 1409
        Window4EndLine               = 1662
        Window5Binning               = 0
        Window5StartSample           = 640
        Window5EndSample             = 767
        Window5StartLine             = 200
        Window5EndLine               = 208
        Window6Binning               = 0
        Window6StartSample           = 1280
        Window6EndSample             = 1407
        Window6StartLine             = 1850
        Window6EndLine               = 1858
        YearDoy                      = 2016331
        ObservationId                = CRUS_049218_201_0
      End_Group
  )");

  PvlGroup truthArchiveGroup;
  arss >> truthArchiveGroup;

  PvlGroup &archiveGroup = outLabel->findGroup("Archive", Pvl::Traverse);

  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, archiveGroup, truthArchiveGroup);

  std::istringstream bbss(R"(
    Group = BandBin
      FilterName = PAN
      Center     = 678.2 <nm>
      Width      = 231.9 <nm>
      NaifIkCode = -143421
    End_Group
  )");

  PvlGroup truthBandBinGroup;
  bbss >> truthBandBinGroup;

  PvlGroup &bandBinGroup = outLabel->findGroup("BandBin", Pvl::Traverse);

  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, bandBinGroup, truthBandBinGroup);

  std::istringstream map(R"(
    Group = Mapping
      ProjectionName       = Equirectangular
      CenterLongitude      = 266.15724842165
      TargetName           = Mars
      EquatorialRadius     = 3396190.0 <meters>
      PolarRadius          = 3376200.0 <meters>
      LatitudeType         = Planetocentric
      LongitudeDirection   = PositiveEast
      LongitudeDomain      = 360
      MinimumLatitude      = 2.465960911303
      MaximumLatitude      = 2.702892431819
      MinimumLongitude     = 266.13827437353
      MaximumLongitude     = 266.17622246977
      UpperLeftCornerX     = -1200.0 <meters>
      UpperLeftCornerY     = 160400.0 <meters>
      PixelResolution      = 200.0 <meters/pixel>
      Scale                = 296.36990921958 <pixels/degree>
      CenterLatitude       = 2.584426671561
      CenterLatitudeRadius = 3396148.9945915
    End_Group
  )");

  PvlGroup truthMappingGroup;
  map >> truthMappingGroup;

  PvlGroup &mappingGroup = outLabel->findGroup("Mapping", Pvl::Traverse);

  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, mappingGroup, truthMappingGroup);

  std::istringstream mos(R"(
    Group = Mosaic
      SpacecraftName            = "TRACE GAS ORBITER"
      InstrumentId              = CaSSIS
      ObservationId             = CRUS_049218_201_0
      StartTime                 = 2016-11-26T22:50:27.381
      StopTime                  = 2016-11-26T22:50:27.382
      SpacecraftClockStartCount = 2f015435767e275a
      IncidenceAngle            = 44.903865525262 <degrees>
      EmissionAngle             = 11.357161002382 <degrees>
      PhaseAngle                = 44.334625021078 <degrees>
      LocalTime                 = 14.425706195493
      SolarLongitude            = 269.1366003982 <degrees>
      SubSolarAzimuth           = 139.52581194362 <degrees>
      NorthAzimuth              = 270.0 <degrees>
    End_Group
  )");

  PvlGroup truthMosaicGroup;
  mos >> truthMosaicGroup;

  PvlGroup &mosaicGroup = outLabel->findGroup("Mosaic", Pvl::Traverse);

  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, mosaicGroup, truthMosaicGroup);

  Histogram *hist = mosCube.histogram();

  EXPECT_NEAR(hist->Average(), 0.20770993546981495, 0.0001);
  EXPECT_NEAR(hist->Sum(), 137.29626734554768, 0.0001);
  EXPECT_EQ(hist->ValidPixels(), 661);
  EXPECT_NEAR(hist->StandardDeviation(), 0.0022430344774779496, 0.0001);
}


TEST_F(TgoCassisModuleKernels, TgoCassisTestProjSingleStitchedFrame) {
  QTemporaryDir prefix;
  prefix.setAutoRemove(false);

  // run tgocassis2isis and spiceinit on pan framelet.
  QString panFileName = prefix.path() + "/panframelet.cub";
  QVector<QString> tgocassis2isisArgs = {"from=data/tgoCassis/CAS-MCO-2016-11-26T22.50.27.381-PAN-00005-B1.xml",
                                         "to=" + panFileName};

  UserInterface tgocassis2isisPan(TGOCASSIS2ISIS_XML, tgocassis2isisArgs);
  try {
    tgocassis2isis(tgocassis2isisPan);
  }
  catch (IException &e) {
    FAIL() << "Unable to run tgocassis2isis on pan image: " << e.what() << std::endl;
  }

  QVector<QString> spiceinitArgs = {"from=" + panFileName,
                                    "ck=" + binaryCkKernelsAsString,
                                    "spk=" + binarySpkKernelsAsString};
  UserInterface spiceinitPan(SPICEINIT_XML, spiceinitArgs);
  try {
    spiceinit(spiceinitPan);
  }
  catch (IException &e) {
    FAIL() << "Unable to run spiceinit on pan image: " << e.what() << std::endl;
  }

  // run tgocassis2isis and spiceinit on red framelet.
  QString redFileName = prefix.path() + "/redframelet.cub";
  tgocassis2isisArgs = {"from=data/tgoCassis/CAS-MCO-2016-11-26T22.50.27.381-RED-01005-B1.xml",
                        "to=" + redFileName};
  UserInterface tgocassis2isisRed(TGOCASSIS2ISIS_XML, tgocassis2isisArgs);
  try {
    tgocassis2isis(tgocassis2isisRed);
  }
  catch (IException &e) {
    FAIL() << "Unable to run tgocassis2isis on red image: " << e.what() << std::endl;
  }

  spiceinitArgs = {"from=" + redFileName,
                   "ck=" + binaryCkKernelsAsString,
                   "spk=" + binarySpkKernelsAsString};
  UserInterface spiceinitRed(SPICEINIT_XML, spiceinitArgs);
  try {
    spiceinit(spiceinitRed);
  }
  catch (IException &e) {
    FAIL() << "Unable to run spiceinit on red image: " << e.what() << std::endl;
  }

  // run tgocassis2isis and spiceinit on blu framelet.
  QString bluFileName = prefix.path() + "/bluframelet.cub";
  tgocassis2isisArgs = {"from=data/tgoCassis/CAS-MCO-2016-11-26T22.50.27.381-BLU-03005-B1.xml",
                        "to=" + bluFileName};
  UserInterface tgocassis2isisBlu(TGOCASSIS2ISIS_XML, tgocassis2isisArgs);
  try {
    tgocassis2isis(tgocassis2isisBlu);
  }
  catch (IException &e) {
    FAIL() << "Unable to run tgocassis2isis on blu image: " << e.what() << std::endl;
  }

  spiceinitArgs = {"from=" + bluFileName,
                   "ck=" + binaryCkKernelsAsString,
                   "spk=" + binarySpkKernelsAsString};
  UserInterface spiceinitBlu(SPICEINIT_XML, spiceinitArgs);
  try {
    spiceinit(spiceinitBlu);
  }
  catch (IException &e) {
    FAIL() << "Unable to run spiceinit on blu image: " << e.what() << std::endl;
  }

  // run tgocassis2isis and spiceinit on nir framelet.
  QString nirFileName = prefix.path() + "/nirframelet.cub";
  tgocassis2isisArgs = {"from=data/tgoCassis/CAS-MCO-2016-11-26T22.50.27.381-NIR-02005-B1.xml",
                        "to=" + nirFileName};
  UserInterface tgocassis2isisNir(TGOCASSIS2ISIS_XML, tgocassis2isisArgs);
  try {
    tgocassis2isis(tgocassis2isisNir);
  }
  catch (IException &e) {
    FAIL() << "Unable to run tgocassis2isis on nir image: " << e.what() << std::endl;
  }

  spiceinitArgs = {"from=" + nirFileName,
                   "ck=" + binaryCkKernelsAsString,
                   "spk=" + binarySpkKernelsAsString};
  UserInterface spiceinitNir(SPICEINIT_XML, spiceinitArgs);
  try {
    spiceinit(spiceinitNir);
  }
  catch (IException &e) {
    FAIL() << "Unable to run spiceinit  on nir image: " << e.what() << std::endl;
  }

  // run stitch on cube list
  FileList *cubeList = new FileList();
  cubeList->append(panFileName);
  cubeList->append(bluFileName);
  cubeList->append(redFileName);
  cubeList->append(nirFileName);

  QString cubeListFile = prefix.path() + "/cubelist.lis";
  cubeList->write(cubeListFile);

  QVector<QString> stitchArgs = {"fromlist=" + cubeListFile,
                "outputprefix=" + prefix.path() + "/stitched"};
  UserInterface stitchOptions(STITCH_XML, stitchArgs);

  try {
    tgocassisstitch(stitchOptions);
  }
  catch (IException &e) {
    FAIL() << "Unable to run tgocassisstitch with cube list: " << e.what() << std::endl;
  }

  // run mosrange on cube list
  QString tgocassisstitchOutput = prefix.path() + "/stitched-2016-11-26T22:50:27.381.cub";
  FileList *mosrangeCubeList = new FileList();
  mosrangeCubeList->append(tgocassisstitchOutput);

  QString mosrangeCubeListFile = prefix.path() + "/cubelist.lis";
  mosrangeCubeList->write(mosrangeCubeListFile);

  QString mapFile = prefix.path() + "/stitched.map";
  QVector<QString> mosrangeArgs = {"fromlist=" + mosrangeCubeListFile, "to=" + mapFile};
  UserInterface mosrangeOptions(MOSRANGE_XML, mosrangeArgs);

  try {
    mosrange(mosrangeOptions);
  }
  catch (IException &e) {
    FAIL() << "Unable to run mosrange with cube list: " << e.what() << std::endl;
  }

  // run cam2map on stitched cube
  QString projectedFile = prefix.path() + "/projected.cub";
  QVector<QString> cam2mapArgs = {"from=" + tgocassisstitchOutput,
                                  "to=" + projectedFile,
                                  "map=" + mapFile,
                                  "pixres=map"};
  UserInterface cam2mapOptions(CAM2MAP_XML, cam2mapArgs);
  try {
    cam2map(cam2mapOptions);
  }
  catch (IException &e) {
    FAIL() << "Unable to run cam2map on stitched image: " << e.what() << std::endl;
  }

  // run tgocassisrdrgen on red framelet.
  QVector<QString> rdrgenArgs = {"from=" + projectedFile,
                                 "to=" + prefix.path() + "/exported.img"};
  UserInterface rdrgen(RDRGEN_XML, rdrgenArgs);
  try {
    tgocassisrdrgen(rdrgen);
  }
  catch (IException &e) {
    FAIL() << "Unable to run tgocassisrdrgen on projected image: " << e.what() << std::endl;
  }
  
  // Mosaic Cube
  Cube exportCube(projectedFile);
  Pvl *outLabel = exportCube.label();

  std::istringstream arnirss(R"(
    Group = ArchiveNIR
      DataSetId                    = TBD
      ProductVersionId             = UNK
      ProducerId                   = UBE
      ProducerName                 = 'Nicolas Thomas'
      ProductCreationTime          = 2017-10-03T10:50:12
      FileName                     = CAS-MCO-2016-11-26T22.50.27.381-NIR-02005--
                                     B1
      ScalingFactor                = 1.00
      Offset                       = 0.00
      PredictMaximumExposureTime   = 1.5952 <ms>
      CassisOffNadirAngle          = 10.032 <deg>
      PredictedRepetitionFrequency = 367.5 <ms>
      GroundTrackVelocity          = 3.4686 <km/s>
      ForwardRotationAngle         = 52.703 <deg>
      SpiceMisalignment            = 185.422 <deg>
      FocalLength                  = 0.8770 <m>
      FNumber                      = 6.50
      ExposureTimeCommand          = 150
      FrameletNumber               = 5
      NumberOfFramelets            = 40
      ImageFrequency               = 400000 <ms>
      NumberOfWindows              = 6
      UniqueIdentifier             = 100799268
      UID                          = 100799268
      ExposureTimestamp            = 2f015435767e275a
      ExposureTimePEHK             = 1.440e-003 <ms>
      PixelsPossiblySaturated      = 0.00
      IFOV                         = 1.140e-005
      IFOVUnit                     = rad/px
      FiltersAvailable             = "BLU RED NIR PAN"
      FocalLengthUnit              = M
      TelescopeType                = "Three-mirror anastigmat with powered fold
                                      mirror"
      DetectorDescription          = "2D Array"
      PixelHeight                  = 10.0
      PixelHeightUnit              = MICRON
      PixelWidth                   = 10.0
      PixelWidthUnit               = MICRON
      DetectorType                 = 'SI CMOS HYBRID (OSPREY 2K)'
      ReadNoise                    = 61.0
      ReadNoiseUnit                = ELECTRON
      MissionPhase                 = MCO
      SubInstrumentIdentifier      = 61.0
      WindowCount                  = 2
      Window1Binning               = 0
      Window1StartSample           = 0
      Window1EndSample             = 2047
      Window1StartLine             = 354
      Window1EndLine               = 632
      Window2Binning               = 0
      Window2StartSample           = 0
      Window2EndSample             = 2047
      Window2StartLine             = 712
      Window2EndLine               = 966
      Window3Binning               = 1
      Window3StartSample           = 0
      Window3EndSample             = 2047
      Window3StartLine             = 1048
      Window3EndLine               = 1303
      Window4Binning               = 0
      Window4StartSample           = 1024
      Window4EndSample             = 1087
      Window4StartLine             = 1409
      Window4EndLine               = 1662
      Window5Binning               = 0
      Window5StartSample           = 640
      Window5EndSample             = 767
      Window5StartLine             = 200
      Window5EndLine               = 208
      Window6Binning               = 0
      Window6StartSample           = 1280
      Window6EndSample             = 1407
      Window6StartLine             = 1850
      Window6EndLine               = 1858
      YearDoy                      = 2016331
      ObservationId                = CRUS_049218_201_0
    End_Group
  )");

  PvlGroup truthNIRArchiveGroup;
  arnirss >> truthNIRArchiveGroup;

  PvlGroup &archiveGroup = outLabel->findGroup("ArchiveNIR", Pvl::Traverse);

  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, archiveGroup, truthNIRArchiveGroup);

  std::istringstream arredss(R"(
    Group = ArchiveRED
      DataSetId                    = TBD
      ProductVersionId             = UNK
      ProducerId                   = UBE
      ProducerName                 = 'Nicolas Thomas'
      ProductCreationTime          = 2017-10-03T10:50:12
      FileName                     = CAS-MCO-2016-11-26T22.50.27.381-RED-01005--
                                     B1
      ScalingFactor                = 1.00
      Offset                       = 0.00
      PredictMaximumExposureTime   = 1.5952 <ms>
      CassisOffNadirAngle          = 10.032 <deg>
      PredictedRepetitionFrequency = 367.5 <ms>
      GroundTrackVelocity          = 3.4686 <km/s>
      ForwardRotationAngle         = 52.703 <deg>
      SpiceMisalignment            = 185.422 <deg>
      FocalLength                  = 0.8770 <m>
      FNumber                      = 6.50
      ExposureTimeCommand          = 150
      FrameletNumber               = 5
      NumberOfFramelets            = 40
      ImageFrequency               = 400000 <ms>
      NumberOfWindows              = 6
      UniqueIdentifier             = 100799268
      UID                          = 100799268
      ExposureTimestamp            = 2f015435767e275a
      ExposureTimePEHK             = 1.440e-003 <ms>
      PixelsPossiblySaturated      = 0.16
      IFOV                         = 1.140e-005
      IFOVUnit                     = rad/px
      FiltersAvailable             = "BLU RED NIR PAN"
      FocalLengthUnit              = M
      TelescopeType                = "Three-mirror anastigmat with powered fold
                                      mirror"
      DetectorDescription          = "2D Array"
      PixelHeight                  = 10.0
      PixelHeightUnit              = MICRON
      PixelWidth                   = 10.0
      PixelWidthUnit               = MICRON
      DetectorType                 = 'SI CMOS HYBRID (OSPREY 2K)'
      ReadNoise                    = 61.0
      ReadNoiseUnit                = ELECTRON
      MissionPhase                 = MCO
      SubInstrumentIdentifier      = 61.0
      WindowCount                  = 1
      Window1Binning               = 0
      Window1StartSample           = 0
      Window1EndSample             = 2047
      Window1StartLine             = 354
      Window1EndLine               = 632
      Window2Binning               = 0
      Window2StartSample           = 0
      Window2EndSample             = 2047
      Window2StartLine             = 712
      Window2EndLine               = 967
      Window3Binning               = 1
      Window3StartSample           = 0
      Window3EndSample             = 2047
      Window3StartLine             = 1048
      Window3EndLine               = 1302
      Window4Binning               = 0
      Window4StartSample           = 1024
      Window4EndSample             = 1087
      Window4StartLine             = 1409
      Window4EndLine               = 1662
      Window5Binning               = 0
      Window5StartSample           = 640
      Window5EndSample             = 767
      Window5StartLine             = 200
      Window5EndLine               = 208
      Window6Binning               = 0
      Window6StartSample           = 1280
      Window6EndSample             = 1407
      Window6StartLine             = 1850
      Window6EndLine               = 1858
      YearDoy                      = 2016331
      ObservationId                = CRUS_049218_201_0
    End_Group
  )");

  PvlGroup truthREDArchiveGroup;
  arredss >> truthREDArchiveGroup;

  archiveGroup = outLabel->findGroup("ArchiveRED", Pvl::Traverse);

  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, archiveGroup, truthREDArchiveGroup);

  std::istringstream arbluss(R"(
    Group = ArchiveBLU
      DataSetId                    = TBD
      ProductVersionId             = UNK
      ProducerId                   = UBE
      ProducerName                 = 'Nicolas Thomas'
      ProductCreationTime          = 2017-10-03T10:50:12
      FileName                     = CAS-MCO-2016-11-26T22.50.27.381-BLU-03005--
                                     B1
      ScalingFactor                = 1.00
      Offset                       = 0.00
      PredictMaximumExposureTime   = 1.5952 <ms>
      CassisOffNadirAngle          = 10.032 <deg>
      PredictedRepetitionFrequency = 367.5 <ms>
      GroundTrackVelocity          = 3.4686 <km/s>
      ForwardRotationAngle         = 52.703 <deg>
      SpiceMisalignment            = 185.422 <deg>
      FocalLength                  = 0.8770 <m>
      FNumber                      = 6.50
      ExposureTimeCommand          = 150
      FrameletNumber               = 5
      NumberOfFramelets            = 40
      ImageFrequency               = 400000 <ms>
      NumberOfWindows              = 6
      UniqueIdentifier             = 100799268
      UID                          = 100799268
      ExposureTimestamp            = 2f015435767e275a
      ExposureTimePEHK             = 1.440e-003 <ms>
      PixelsPossiblySaturated      = 0.00
      IFOV                         = 1.140e-005
      IFOVUnit                     = rad/px
      FiltersAvailable             = "BLU RED NIR PAN"
      FocalLengthUnit              = M
      TelescopeType                = "Three-mirror anastigmat with powered fold
                                      mirror"
      DetectorDescription          = "2D Array"
      PixelHeight                  = 10.0
      PixelHeightUnit              = MICRON
      PixelWidth                   = 10.0
      PixelWidthUnit               = MICRON
      DetectorType                 = 'SI CMOS HYBRID (OSPREY 2K)'
      ReadNoise                    = 61.0
      ReadNoiseUnit                = ELECTRON
      MissionPhase                 = MCO
      SubInstrumentIdentifier      = 61.0
      WindowCount                  = 3
      Window1Binning               = 0
      Window1StartSample           = 0
      Window1EndSample             = 2047
      Window1StartLine             = 354
      Window1EndLine               = 632
      Window2Binning               = 0
      Window2StartSample           = 0
      Window2EndSample             = 2047
      Window2StartLine             = 712
      Window2EndLine               = 966
      Window3Binning               = 1
      Window3StartSample           = 0
      Window3EndSample             = 2047
      Window3StartLine             = 1048
      Window3EndLine               = 1302
      Window4Binning               = 0
      Window4StartSample           = 1024
      Window4EndSample             = 1087
      Window4StartLine             = 1409
      Window4EndLine               = 1626
      Window5Binning               = 0
      Window5StartSample           = 640
      Window5EndSample             = 767
      Window5StartLine             = 200
      Window5EndLine               = 208
      Window6Binning               = 0
      Window6StartSample           = 1280
      Window6EndSample             = 1407
      Window6StartLine             = 1850
      Window6EndLine               = 1858
      YearDoy                      = 2016331
      ObservationId                = CRUS_049218_201_0
    End_Group
  )");

  PvlGroup truthBLUArchiveGroup;
  arbluss >> truthBLUArchiveGroup;

  archiveGroup = outLabel->findGroup("ArchiveBLU", Pvl::Traverse);

  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, archiveGroup, truthBLUArchiveGroup);

  std::istringstream arpanss(R"(
    Group = ArchivePAN
      DataSetId                    = TBD
      ProductVersionId             = UNK
      ProducerId                   = UBE
      ProducerName                 = 'Nicolas Thomas'
      ProductCreationTime          = 2017-10-03T10:50:12
      FileName                     = CAS-MCO-2016-11-26T22.50.27.381-PAN-00005--
                                     B1
      ScalingFactor                = 1.00
      Offset                       = 0.00
      PredictMaximumExposureTime   = 1.5952 <ms>
      CassisOffNadirAngle          = 10.032 <deg>
      PredictedRepetitionFrequency = 367.5 <ms>
      GroundTrackVelocity          = 3.4686 <km/s>
      ForwardRotationAngle         = 52.703 <deg>
      SpiceMisalignment            = 185.422 <deg>
      FocalLength                  = 0.8770 <m>
      FNumber                      = 6.50
      ExposureTimeCommand          = 150
      FrameletNumber               = 5
      NumberOfFramelets            = 40
      ImageFrequency               = 400000 <ms>
      NumberOfWindows              = 6
      UniqueIdentifier             = 100799268
      UID                          = 100799268
      ExposureTimestamp            = 2f015435767e275a
      ExposureTimePEHK             = 1.440e-003 <ms>
      PixelsPossiblySaturated      = 29.17
      IFOV                         = 1.140e-005
      IFOVUnit                     = rad/px
      FiltersAvailable             = "BLU RED NIR PAN"
      FocalLengthUnit              = M
      TelescopeType                = "Three-mirror anastigmat with powered fold
                                      mirror"
      DetectorDescription          = "2D Array"
      PixelHeight                  = 10.0
      PixelHeightUnit              = MICRON
      PixelWidth                   = 10.0
      PixelWidthUnit               = MICRON
      DetectorType                 = 'SI CMOS HYBRID (OSPREY 2K)'
      ReadNoise                    = 61.0
      ReadNoiseUnit                = ELECTRON
      MissionPhase                 = MCO
      SubInstrumentIdentifier      = 61.0
      WindowCount                  = 0
      Window1Binning               = 0
      Window1StartSample           = 0
      Window1EndSample             = 2047
      Window1StartLine             = 354
      Window1EndLine               = 633
      Window2Binning               = 0
      Window2StartSample           = 0
      Window2EndSample             = 2047
      Window2StartLine             = 712
      Window2EndLine               = 966
      Window3Binning               = 1
      Window3StartSample           = 0
      Window3EndSample             = 2047
      Window3StartLine             = 1048
      Window3EndLine               = 1302
      Window4Binning               = 0
      Window4StartSample           = 1024
      Window4EndSample             = 1087
      Window4StartLine             = 1409
      Window4EndLine               = 1662
      Window5Binning               = 0
      Window5StartSample           = 640
      Window5EndSample             = 767
      Window5StartLine             = 200
      Window5EndLine               = 208
      Window6Binning               = 0
      Window6StartSample           = 1280
      Window6EndSample             = 1407
      Window6StartLine             = 1850
      Window6EndLine               = 1858
      YearDoy                      = 2016331
      ObservationId                = CRUS_049218_201_0
    End_Group
  )");

  PvlGroup truthPANArchiveGroup;
  arpanss >> truthPANArchiveGroup;

  archiveGroup = outLabel->findGroup("ArchivePAN", Pvl::Traverse);

  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, archiveGroup, truthPANArchiveGroup);

  std::istringstream sss(R"(
    Group = Stitch
      OriginalFilters    = (NIR, RED, BLU, PAN)
      FilterCenters      = (939.3, 836.0, 494.8, 678.2)
      FilterWidths       = (121.8, 98.5, 133.6, 231.9)
      FilterIkCodes      = (-143423, -143422, -143424, -143421)
      FilterStartSamples = (0.0, 0.0, 1024.0, 0.0)
      FilterSamples      = (2048, 2048, 64, 2048)
      FilterStartLines   = (1048.0, 712.0, 1409.0, 354.0)
      FilterLines        = (256, 256, 218, 280)
      FilterFileNames    = (CAS-MCO-2016-11-26T22.50.27.381-NIR-02005-B1,
                            CAS-MCO-2016-11-26T22.50.27.381-RED-01005-B1,
                            CAS-MCO-2016-11-26T22.50.27.381-BLU-03005-B1,
                            CAS-MCO-2016-11-26T22.50.27.381-PAN-00005-B1)
    End_Group
  )");

  PvlGroup truthStitchGroup;
  sss >> truthStitchGroup;

  PvlGroup &stitchGroup = outLabel->findGroup("Stitch", Pvl::Traverse);
  std::cout << stitchGroup << std::endl;

  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, stitchGroup, truthStitchGroup);

  std::istringstream map(R"(
    Group = Mapping
      ProjectionName     = Equirectangular
      CenterLongitude    = 266.21992961904
      TargetName         = Mars
      EquatorialRadius   = 3396190.0 <meters>
      PolarRadius        = 3376200.0 <meters>
      LatitudeType       = Planetocentric
      LongitudeDirection = PositiveEast
      LongitudeDomain    = 360
      MinimumLatitude    = 2.4644293339627
      MaximumLatitude    = 2.7054455166927
      MinimumLongitude   = 266.09781108551
      MaximumLongitude   = 266.34204815257
      UpperLeftCornerX   = -7233.2769909138 <meters>
      UpperLeftCornerY   = 160363.72421906 <meters>
      PixelResolution    = 6.80458795006 <meters/pixel>
      Scale              = 8710.8847730724 <pixels/degree>
      CenterLatitude     = 2.584937425328
    End_Group
  )");

  PvlGroup truthMappingGroup;
  map >> truthMappingGroup;

  PvlGroup &mappingGroup = outLabel->findGroup("Mapping", Pvl::Traverse);

  EXPECT_PRED_FORMAT2(AssertPvlGroupEqual, mappingGroup, truthMappingGroup);

  Histogram *hist = exportCube.histogram();

  EXPECT_NEAR(hist->Average(), 0.26625623495550205, 0.0001);
  EXPECT_NEAR(hist->Sum(), 441491.9287794265, 0.001);
  EXPECT_EQ(hist->ValidPixels(), 1658108);
  EXPECT_NEAR(hist->StandardDeviation(), 0.048925404459616698, 0.0001);
}
