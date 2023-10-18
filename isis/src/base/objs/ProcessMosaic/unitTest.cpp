/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "Isis.h"

#include "Application.h"
#include "IString.h"
#include "Portal.h"
#include "ProcessMosaic.h"
#include "SpecialPixel.h"
#include "Table.h"

using namespace Isis;
using namespace std;

void testIn(int iss, int isl, int isb, int ins = 0, int inl = 0, int inb = 0);
void testOut(int piSamples, int piLines, int piBands, int piPriority,
             int originBand);
/**
 * Unit test for ProcessMosaic.
 *
 * Tests for correct area drop, tracking origin,  origin band,
 * priorities input, mosaic and band, options to allow HS, LS
 * and NULL pixels from input to mosaic, each time displaying
 * the contents of the input and mosaic pixels for the area
 * under consideration
 *
 * Also tests for exceptions like number of input and output images to
 * be exactly one each, band cannot be priority if Track is set off and
 * more
 *
 * @author 2009-10-14 Sharmila Prasad
 *
 *  @internal
 *   @history 2018-06-06 Jeannie Backer - Removed file paths from error message written to
 *                           test output.
 */
void IsisMain() {

  Preference::Preferences(true);

  qDebug() << "Testing ProcessMosaic Class ... ";

  // Create the default output cube
  Process p;
  p.SetOutputCube("TO", 5, 5, 1);
  p.EndProcess();

  // ***********************************************************
  // Drop a small area into the middle of the output
  qDebug() << "Create output mosaic with Tracking set to True";
  qDebug() << "1. Drop a small area into the middle of the output";
  ProcessMosaic m1;
  m1.SetTrackFlag(true);
  m1.SetCreateFlag(true);
  m1.SetImageOverlay(ProcessMosaic::PlaceImagesOnTop);

  m1.SetInputCube("FROM", 1, 1, 1, 10, 5, 1);

  Cube *mosaicCube1 = m1.SetOutputCube("TO");

  m1.StartProcess(5, 2, 1); // This should be overwritten by the next StartProcess call
  m1.StartProcess(2, 2, 1);

  // Test for "Tracking" group in the mosaic cube
  if (mosaicCube1->hasGroup("Tracking")) {
    qDebug() << "";
    qDebug() << "a. SUCCESS - \"Tracking\" Group Exists in [" << mosaicCube1->fileName() << "]";
  }
  else {
    qDebug() << "";
    qDebug() << "a. FAILURE - \"Tracking\" Group does not Exist in [" << mosaicCube1->fileName() << "]";
  }

  // Test for Tracking Table "InputImages" in the tracking cube
  QString trackingBase = FileName(mosaicCube1->fileName()).removeExtension().expanded().split("/").last();
  Cube *trackingCube1 = new Cube(FileName(trackingBase + "_tracking.cub"));
  try {
    Table trackTable = trackingCube1->readTable(ProcessMosaic::TRACKING_TABLE_NAME);
    qDebug() << "b. SUCCESS - Track Table Exists in [" << trackingCube1->fileName() << "]";
    qDebug().noquote() << QString::fromStdString(Table::toString( trackTable, "\t" ));
  }
  catch (IException&) {
    qDebug() << "b. FAILURE - Track Table does not Exist in [" << trackingCube1->fileName() << "]";
  }
  m1.EndProcess();
  testIn(1, 1, 1, 5, 5, 1);
  testOut(2, 2, 1, ProcessMosaic::PlaceImagesOnTop, 2);

  remove("isisMosaic_01.cub");
  remove("isisMosaic_01_tracking.cub");
  qDebug() << "***********************************************************************************";

  // ***********************************************************
  // Drop 2,2,1 into the lower right corner of band 2
  qDebug() << "2. Drop 2,2,1 into the lower right corner of band 2";
  qDebug() << "Tracking is set to False";
  ProcessMosaic m2;
  m2.SetTrackFlag(false);
  m2.SetCreateFlag(true);
  m2.SetImageOverlay(ProcessMosaic::PlaceImagesOnTop);

  m2.SetInputCube("FROM", 2, 2, 1, -1, -1, -1);

  p.SetOutputCube("TO", 5, 5, 1);
  p.EndProcess();
  m2.SetOutputCube("TO");

  m2.StartProcess(4, 4, 2);
  m2.EndProcess();

  testIn(2, 2, 1, 5, 5, 1);
  testOut(4, 4, 1, ProcessMosaic::PlaceImagesOnTop, 0);

  remove("isisMosaic_01.cub");
  qDebug() << "***********************************************************************************";

  // ***********************************************************
  // Drop 3,3,1 into the upper right corner of band 1
  qDebug() << "3. Drop 3,3,1 into the upper right corner of band 1";
  ProcessMosaic m3;
  m3.SetTrackFlag(true);
  m3.SetCreateFlag(true);
  m3.SetImageOverlay(ProcessMosaic::PlaceImagesBeneath);

  m3.SetInputCube("FROM", 3, 3, 1, 10, 1, 1);

  p.SetOutputCube("TO", 5, 5, 1);
  p.EndProcess();
  m3.SetOutputCube("TO");

  m3.StartProcess(5, 1, 1);
  m3.EndProcess();

  testIn(3, 3, 1, 5, 5, 1);
  testOut(5, 1, 1, ProcessMosaic::PlaceImagesBeneath, 2);

  remove("isisMosaic_01.cub");
  remove("isisMosaic_01_tracking.cub");
  qDebug() << "***********************************************************************************";

  // ***********************************************************
  // Drop the first 3x3x1  the upper left corner
  qDebug() << "4. Drop the first 3x3x1 to the upper left corner";
  ProcessMosaic m4;
  m4.SetTrackFlag(true);
  m4.SetCreateFlag(true);
  m4.SetImageOverlay(ProcessMosaic::PlaceImagesBeneath);

  m4.SetInputCube("FROM", 1, 1, 1, 3, 3, 1);

  p.SetOutputCube("TO", 5, 5, 1);
  p.EndProcess();
  m4.SetOutputCube("TO");

  m4.StartProcess(1, 1, 1);
  m4.EndProcess();

  testIn(1, 1, 1, 3, 3, 1);
  testOut(1, 1, 1, ProcessMosaic::PlaceImagesBeneath, 2);

  qDebug() << "***********************************************************************************";

  // Test for mosaic(beneath)  priority
  qDebug() << "5. Test for mosaic priority with existing mosaic";
  ProcessMosaic m5;
  m5.SetImageOverlay(ProcessMosaic::PlaceImagesBeneath);

  m5.SetInputCube("FROM", 1, 1, 1, 5, 5, 1);

  m5.SetOutputCube("TO");

  m5.StartProcess(1, 2, 1);
  m5.EndProcess();

  testIn(1, 1, 1, 5, 5, 1);
  testOut(1, 2, 1, ProcessMosaic::PlaceImagesBeneath, 2);

  qDebug() << "***********************************************************************************";


  // ***********************************************************
  // Test for band priority using Keywords for band id
  qDebug() << "6. Test for band priority with Keyname \"FilterName\" and value \"Red\" with "
              "Criteria \"Greater\" than in an existing mosaic";
  ProcessMosaic m6;
  m6.SetTrackFlag(true);
  m6.SetCreateFlag(true);
  m6.SetImageOverlay(ProcessMosaic::UseBandPlacementCriteria);
  m6.SetBandKeyword("FilterName", "red");
  m6.SetBandUseMaxValue(true);

  m6.SetInputCube("FROM", 3, 3, 1, 10, 1, 1);

  //p.SetOutputCube("TO", 5, 5, 3);
  //p.EndProcess();
  m6.SetOutputCube("TO");

  m6.StartProcess(1, 1, 1);
  m6.EndProcess();

  testIn(3, 3, 1, 10, 1, 1);
  testOut(1, 1, 1, ProcessMosaic::UseBandPlacementCriteria, 2);

  qDebug() << "***********************************************************************************";

  qDebug() << "7. Test for band priority for existing mosaic with Keyname \"OriginalBand\" and "
              "value \"1\" and Criteria \"Lesser\" than";
  ProcessMosaic m7;
  m7.SetTrackFlag(true);
  m7.SetCreateFlag(false);
  m7.SetImageOverlay(ProcessMosaic::UseBandPlacementCriteria);
  m7.SetBandKeyword("OriginalBand", "1");
  m7.SetBandUseMaxValue(false);
  m7.SetHighSaturationFlag(false);
  m7.SetLowSaturationFlag(false);
  m7.SetNullFlag(false);

  m7.SetInputCube("FROM", 1, 1, 1, 10, 1, 1);
  m7.SetOutputCube("TO");

  m7.StartProcess(1, 1, 1);
  m7.EndProcess();

  testIn(1, 1, 1, 10, 1, 1);
  testOut(1, 1, 1, ProcessMosaic::UseBandPlacementCriteria, 2);

  qDebug() << "***********************************************************************************";

  // ***********************************************************
  // Test for band priority using Band Number
  qDebug() << "8. Test for band priority with existing mosaic and BandNumber set";
  ProcessMosaic m8;
  m8.SetTrackFlag(true);
  m8.SetCreateFlag(false);
  m8.SetImageOverlay(ProcessMosaic::UseBandPlacementCriteria);
  m8.SetBandNumber(1);
  m8.SetBandUseMaxValue(false);
  m8.SetHighSaturationFlag(true);
  m8.SetLowSaturationFlag(false);
  m8.SetNullFlag(false);

  m8.SetInputCube("FROM", 1, 1, 1, 5, 5, 1);
  m8.SetOutputCube("TO");

  m8.StartProcess(1, 3, 1);
  m8.EndProcess();

  testIn(1, 1, 1, 5, 5, 1);
  testOut(1, 3, 1, ProcessMosaic::UseBandPlacementCriteria, 2);

  qDebug() << "***********************************************************************************";

  // ***********************************************************
  // Test for HS value set with existing mosaic
  qDebug() << "9. Test for Null flag set with existing mosaic";
  ProcessMosaic m9;
  m9.SetImageOverlay(ProcessMosaic::UseBandPlacementCriteria);
  m9.SetBandNumber(1);
  m9.SetBandUseMaxValue(true);
  m9.SetHighSaturationFlag(false);
  m9.SetLowSaturationFlag(false);
  m9.SetNullFlag(true);

  m9.SetInputCube("FROM", 1, 1, 1, 5, 5, 1);
  m9.SetOutputCube("TO");

  m9.StartProcess(1, 2, 1);
  m9.EndProcess();

  testIn(1, 1, 1, 5, 5, 1);
  testOut(1, 2, 1, ProcessMosaic::UseBandPlacementCriteria, 2);

  remove("isisMosaic_01.cub");
  remove("isisMosaic_01_tracking.cub");


  // ***********************************************************
  // Test Average Priority
  qDebug() << "";
  qDebug() << "10. Test Average Priority";
  // Create the default output cube
  p.SetOutputCube("TO_AVG", 5, 5, 2);
  p.EndProcess();

  ProcessMosaic m10;
  m10.SetTrackFlag(false);
  m10.SetImageOverlay(ProcessMosaic::AverageImageWithMosaic);
  m10.SetCreateFlag(true);

  m10.SetInputCube("FROM", 1, 1, 1, 5, 5, 1);
  m10.SetOutputCube("TO_AVG");
  m10.StartProcess(1, 1, 1);
  m10.EndProcess();
  testIn(1, 1, 1, 5, 5, 1);
  testOut(1, 1, 1, ProcessMosaic::AverageImageWithMosaic, 0);

  m10.SetInputCube("FROM", 1, 1, 1, 5, 5, 1);
  m10.SetOutputCube("TO_AVG");
  m10.SetTrackFlag(false);
  m10.SetImageOverlay(ProcessMosaic::AverageImageWithMosaic);
  m10.SetCreateFlag(false);

  m10.StartProcess(-1, -1, 1);
  m10.EndProcess();

  testOut(1, 1, 1, ProcessMosaic::AverageImageWithMosaic, 0);
  remove("isisMosaic_02.cub");

  qDebug() << "****** End Average **********************";

 // Test for band priority using Band Number
  qDebug() << "11. Test for band priority with Tracking Off and BandNumber set";
  ProcessMosaic m11;
  m11.SetTrackFlag(false);
  m11.SetCreateFlag(true);
  m11.SetImageOverlay(ProcessMosaic::UseBandPlacementCriteria);
  m11.SetBandNumber(1);
  m11.SetBandUseMaxValue(false);

  m11.SetInputCube("FROM", 1, 1, 1, 5, 5, 1);

  p.SetOutputCube("TO", 5, 5, 1);
  p.EndProcess();
  m11.SetOutputCube("TO");

  m11.StartProcess(1, 3, 1);
  m11.EndProcess();

  testIn(1, 1, 1, 5, 5, 1);
  testOut(1, 3, 1, ProcessMosaic::UseBandPlacementCriteria, 0);

  ProcessMosaic m12;
  m12.SetTrackFlag(false);
  m12.SetCreateFlag(true);
  m12.SetImageOverlay(ProcessMosaic::UseBandPlacementCriteria);
  m12.SetBandNumber(1);
  m12.SetBandUseMaxValue(false);

  m12.SetInputCube("FROM", 1, 1, 1, 5, 5, 1);
  m12.SetOutputCube("TO");
  m12.StartProcess(1, 1, 1);
  m12.EndProcess();

  testOut(1, 1, 1, ProcessMosaic::UseBandPlacementCriteria, 0);

  qDebug() << "********* Test imagePositions() ********";
  for (int i = 0; i <= m11.imagePositions().groups() - 1; i++) {
    qDebug() << "Name: " << m11.imagePositions().group(i).name();
    qDebug() << "File: " << FileName(m11.imagePositions().group(i).findKeyword("File")[0]).name();
    qDebug() << "StartSample: " << m11.imagePositions().group(i).findKeyword("StartSample")[0];
    qDebug() << "StartLine: " << m11.imagePositions().group(i).findKeyword("StartLine")[0];
  }
  qDebug() << "***********************************************************************************";

  // ***********************************************************
  // Testing Errors

  // Try to open two input cubes
  qDebug() << "";
  qDebug() << "*** Test Error Handling ***";
  qDebug() << "Test multiple input error";
  try {
    ProcessMosaic m;
    m.SetInputCube("FROM", 1, 1, 1, -1, -1, -1);
    m.SetInputCube("FROM", 1, 1, 1, -1, -1, -1);
    m.StartProcess(1, 1, 1);
    m.EndProcess();
  }
  catch (IException &e) {
    e.print();
    p.EndProcess();
    qDebug() << "";
  }

  // ***********************************************************
  // Try to open two output cubes
  qDebug() << "Test multiple output error";
  try {
    ProcessMosaic m;
    m.SetOutputCube("TO");
    m.SetOutputCube("TO");
    m.StartProcess(1, 1, 1);
    m.EndProcess();
  }
  catch (IException &e) {
    e.print();
    p.EndProcess();
    qDebug() << "";
  }

  // ***********************************************************
  // Drop the input completly outside the output
  qDebug() << "Test input does not overlap mosaic";
  try {
    ProcessMosaic m;
    m.SetInputCube("FROM", 1, 1, 1, -1, -1, -1);
    m.SetOutputCube("TO");
    m.StartProcess(-20, 0, 1);
    m.EndProcess();
  }
  catch (IException &e) {
    e.print();
    p.EndProcess();
    qDebug() << "";
  }

  qDebug() << "Test input does not overlap mosaic";
  try {
    ProcessMosaic m;
    m.SetInputCube("FROM", 1, 1, 1, -1, -1, -1);
    m.SetOutputCube("TO");
    m.StartProcess(54, 23, 1);
    m.EndProcess();
  }
  catch (IException &e) {
    e.print();
    p.EndProcess();
    qDebug() << "";
  }

  // ***********************************************************
  // Don't open an input cube
  qDebug() << "Test no input cube";
  try {
    ProcessMosaic m;
    m.SetOutputCube("TO");
    m.StartProcess(1, 1, 1);
    m.EndProcess();
  }
  catch (IException &e) {
    e.print();
    p.EndProcess();
    qDebug() << "";
  }

  //***********************************************************
  // Don't open an output cube
  qDebug() << "Test no output cube";
  try {
    ProcessMosaic m;
    m.SetInputCube("FROM", 1, 1, 1, -1, -1, -1);
    m.StartProcess(1, 1, 1);
    m.EndProcess();
  }
  catch (IException &e) {
    e.print();
    p.EndProcess();
    qDebug() << "";
  }

  //***********************************************************
  // Band cannot be a priority if Track is not set
  qDebug() << "Test Band cannot be a priority if Track is not set";
  try {
    ProcessMosaic m;
    m.SetTrackFlag(false);
    m.SetImageOverlay(ProcessMosaic::UseBandPlacementCriteria);
    m.SetBandNumber(1);

    m.SetOutputCube("TO");
    m.SetInputCube("FROM", 1, 1, 1, -1, -1, -1);
    m.StartProcess(1, 1, 1);
    m.EndProcess();
  }
  catch (IException &e) {
    e.print();
    p.EndProcess();
    qDebug() << "";
  }

  // ***********************************************************
  // Test tracking with ontop priotirty and multiple bands
  qDebug() << "Test tracking with ontop priotirty and multiple bands";
  try {
    ProcessMosaic m;
    m.SetTrackFlag(true);
    m.SetImageOverlay(ProcessMosaic::UseBandPlacementCriteria);
    m.SetBandNumber(10);

    m.SetOutputCube("TO");
    m.SetInputCube("FROM", 1, 1, 1, -1, -1, -1);
    m.StartProcess(1, 1, 1);
    m.EndProcess();
  }
  catch (IException &e) {
    e.print();
    p.EndProcess();
    qDebug() << "";
  }

  // ***********************************************************
  // Test Band not found with Band as Priority
  qDebug() << "Test Band not found with Band as Priority";
  try {
    ProcessMosaic m;
    m.SetTrackFlag(false);
    m.SetImageOverlay(ProcessMosaic::UseBandPlacementCriteria);
    m.SetBandNumber(10);

    m.SetOutputCube("TO");
    m.SetInputCube("FROM", 1, 1, 1, -1, -1, -1);
    m.StartProcess(1, 1, 1);
    m.EndProcess();
  }
  catch (IException &e) {
    e.print();
    p.EndProcess();
    qDebug() << "";
  }
  remove("isisMosaic_01.cub");
  remove("isisMosaic_01_tracking.cub");


  // ***********************************************************
  // Testing errors that can occur
  qDebug() << "***********************************************************************************";
  qDebug() << "Test Pvl Group [BandBin] for mismatch between input cube and established mosaic";
  qDebug() << "    Create output mosaic";
  qDebug() << "    Modify Group [BandBin] so it will differ";
  qDebug() << "    Mosaic the same cube to verify proper error is thrown";

  p.SetOutputCube("TO", 5, 5, 1);
  p.EndProcess();

  ProcessMosaic m13;
  m13.SetInputCube("FROM", 1, 1, 1, -1, -1, -1);
  Cube *c = m13.SetOutputCube("TO");
  m13.StartProcess(1, 1, 1);
  Pvl *pvl = c->label();
  PvlKeyword &key = pvl->findKeyword("OriginalBand", Pvl::Traverse);
  key[0] = "3";
  m13.EndProcess();

  try {
    ProcessMosaic m;
    m.SetOutputCube("TO");
    m.SetInputCube("FROM", 1, 1, 1, -1, -1, -1);
    m.StartProcess(1, 1, 1);
    m.EndProcess();
  }
  catch (IException &e) {
    QString message = e.toString();
    qDebug().noquote() << message.replace(QRegExp("cube.*base/unitTestData"), "cube [base/unitTestData");
    p.EndProcess();
    qDebug() << "";
  }
  remove("isisMosaic_01.cub");
}


/**
 * Display the contents of Input image with starting and number of
 * samples, lines,bands
 *
 * @author sprasad (10/14/2009)
 *
 * @param iss  - input starting sample
 * @param isl  - input starting line
 * @param isb  - input starting band
 * @param ins  - input number of samples
 * @param inl  - input number of lines
 * @param inb  - input number of bands
 */
void testIn(int iss, int isl, int isb, int ins, int inl, int inb) {
  Cube cInCube;
  UserInterface &ui = Isis::Application::GetUserInterface();
  QString sFrom = ui.GetCubeName("FROM");
  cInCube.open(sFrom);

  qDebug() << "";
  qDebug() << "***  Input Image  ***  ";
  if (ins == 0) ins = cInCube.sampleCount() - iss + 1;
  if (inl == 0) inl = cInCube.lineCount()   - isl + 1;
  if (inb == 0) inb = cInCube.bandCount()   - isb + 1;

  qDebug() << "Stats " << isl << ", " << iss << ", " << isb << ", "
                       << inl << ", " << ins << ", " << inb;

  int iS;
  Portal ciPortal(ins, 1, cInCube.pixelType());
  for (int band = isb; band <= (isb + inb - 1); band++) {
    for (int line = isl; line <= (isl + inl - 1); line++) {
      iS = iss;
      ciPortal.SetPosition(iss, line, band);  //sample, line, band position
      cInCube.read(ciPortal);
      for (int iPixel = 0; iPixel < ciPortal.size(); iPixel++) {
        if (iPixel == 5) {
          qDebug() << "";
        }
        qDebug() << "(" << Isis::toString(line) << "," << Isis::toString(iS++) << ","
                 << Isis::toString(band) << ")=" << Isis::toString((int)ciPortal[iPixel]);
      }
      qDebug() << "";
    }
    qDebug() << "";
  }
  cInCube.close();
}


/**
 * Display the contents of Ouput image and display the sample, line and band
 * stats for the mosaic being tested
 *
 * @author sprasad (10/14/2009)
 *
 * @param iss  - input starting sample
 * @param isl  - input starting line
 * @param isb  - input starting band
 */
void testOut(int piSamples, int piLines,
             int piBands, int piPriority, int originBand) {
  Cube cOutCube;
  Cube trackingCube;
  UserInterface &ui = Isis::Application::GetUserInterface();
  QString sTo;
  if (piPriority == ProcessMosaic::AverageImageWithMosaic)
    sTo = ui.GetCubeName("TO_AVG");
  else
    sTo = ui.GetCubeName("TO");
  cOutCube.open(sTo);

  int iBands = cOutCube.bandCount();

  qDebug() << "";
  qDebug() << "***  Mosaic Image  ***  ";
  qDebug() << "Start Stats " << Isis::toString(piLines) << ", " << Isis::toString(piSamples)
           << ", " << Isis::toString(piBands);
  qDebug() << "Total Bands=" << Isis::toString(iBands);
  Portal coPortal(5, 1, cOutCube.pixelType());
  int band = piBands;
  while (band <= iBands) {
    for (int line = 1; line <= 5; line++) {
      coPortal.SetPosition(1, line, band);  //sample, line, band position
      cOutCube.read(coPortal);
      for (int iPixel = 0; iPixel < coPortal.size(); iPixel++) {
        int iDefault = 0;
        int iFileIndexOffset = 0;

        switch (SizeOf(cOutCube.pixelType())) {
          case 1:
            iDefault = NULL1;
            iFileIndexOffset = -VALID_MIN1;
            break;

          case 2:
            iDefault = NULL2;
            iFileIndexOffset = -VALID_MIN2;
            break;

          case 4:
            iDefault = INULL4;
            iFileIndexOffset = -ProcessMosaic::FLOAT_STORE_INT_PRECISELY_MIN_VALUE;
            break;
        }

        int iFileIndex = 0;
        if (piPriority != ProcessMosaic::AverageImageWithMosaic && band == originBand &&
           coPortal[iPixel] != iDefault) {
          iFileIndex = (int)coPortal[iPixel] + iFileIndexOffset + 1;
        }
        if (band == originBand && piPriority != ProcessMosaic::AverageImageWithMosaic) {//orig band
          qDebug() << "(" << Isis::toString(line) << "," << Isis::toString(iPixel + 1)
                   << "," << Isis::toString(band) << ")=" << Isis::toString((int)coPortal[iPixel])
                   << ", " << iFileIndex;
        }
        else {
          qDebug() << "(" << Isis::toString(line) << "," << Isis::toString(iPixel + 1)
                   << "," << Isis::toString(band) << ")=" << Isis::toString((int)coPortal[iPixel]);
        }
      }
      qDebug() << "";
    }
    qDebug() << "";
    band++;
    if (band > iBands) {
      break;
    }
  }

  // Test the tracking cube
  if (cOutCube.hasGroup("Tracking")) {

    qDebug() << "";

    qDebug() << "***  Tracking Cube  ***  ";

    QString trackingBase = FileName(cOutCube.fileName()).removeExtension().expanded().split("/").last();
    trackingCube.open(trackingBase + "_tracking.cub");
    Portal trackingPortal(5, 1, trackingCube.pixelType());

    for (int line = 1; line <= 5; line++) {
      trackingPortal.SetPosition(1, line, 1);  //sample, line, band position
      trackingCube.read(trackingPortal);
      for (int iPixel = 0; iPixel < trackingPortal.size(); iPixel++) {

        QString pixelString;
        QString fileIndex;

        if (IsSpecial(trackingPortal[iPixel])) {
          if (trackingPortal[iPixel] == Isis::Null) {
            pixelString = "Null";
          }
          else if (trackingPortal[iPixel] == Isis::Lrs) {
            pixelString = "Lrs";
          }
          else if (trackingPortal[iPixel] == Isis::Lis) {
            pixelString = "Lis";
          }
          else if (trackingPortal[iPixel] == Isis::Hrs) {
            pixelString = "Hrs";
          }
          else if (trackingPortal[iPixel] == Isis::His) {
            pixelString = "His";
          }
          else {
            pixelString = "Unknown";
          }

          fileIndex = "Unknown";
        }
        else {
          pixelString = Isis::toString((unsigned int)trackingPortal[iPixel]);
          fileIndex = Isis::toString((unsigned int)trackingPortal[iPixel] - 2);
        }

        qDebug() << "(" << Isis::toString(line)
                 << "," << Isis::toString(iPixel + 1)
                 << ")=" << pixelString
                 << ", " << fileIndex;
      }
      qDebug() << "";
    }
    qDebug() << "";
  }

  cOutCube.close();
  trackingCube.close();
}
