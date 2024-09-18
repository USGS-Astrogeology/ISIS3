#include <QFileInfo> 
#include <QString>
#include <QTemporaryDir>

#include "cnetedit.h"
#include "ControlMeasure.h"
#include "ControlNet.h"
#include "ControlPoint.h"
#include "FileList.h"
#include "Progress.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlObject.h"
#include "LineManager.h"
#include "spiceinit.h"
#include "TempFixtures.h"
#include "TestUtilities.h"

#include "gtest/gtest.h"

using namespace Isis;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/cnetedit.xml").expanded());

class CneteditCheckValid : public TempTestingFiles {
  protected:
    QString cnet11File;
    QString chkValidCubeListFile;
    QString defFile;

    void SetUp() override {
      TempTestingFiles::SetUp();

      chkValidCubeListFile = tempDir.path() + "/chkValidCubeList.lis";
      cnet11File = "data/cnetedit/cnet_11pts.pvl";
      defFile = tempDir.path() + "/defFile.def";

      std::ifstream label1Strm("data/cnetedit/PSP_002733_1880_RED4.crop.pvl");
      std::ifstream label2Strm("data/cnetedit/PSP_002733_1880_RED5.crop.pvl");

      Pvl label1;
      Pvl label2;

      label1Strm >> label1;
      label2Strm >> label2;
      
      Cube cube1;
      Cube cube2;

      cube1.fromLabel(tempDir.path().toStdString() + "/PSP_002733_1880_RED4.crop.cub",
                    label1, "rw");
      cube2.fromLabel(tempDir.path().toStdString() + "/PSP_002733_1880_RED5.crop.cub",
                    label2, "rw");

      LineManager line(cube1);
      LineManager line2(cube2);

      for(line.begin(); !line.end(); line++) {
          for(int i = 0; i < line.size(); i++) {
            line[i] = (double)(i+1);
          }
          cube1.write(line);
      }
      for(line2.begin(); !line2.end(); line2++) {
          for(int i = 0; i < line2.size(); i++) {
            line2[i] = (double)(i+1);
          }
          cube2.write(line2);
      }
      cube1.reopen("rw");
      cube2.reopen("rw");

      // set up cube list for checkValid tests
      FileList chkValidCubeList;
      chkValidCubeList.append(cube1.fileName().toStdString());
      chkValidCubeList.append(cube2.fileName().toStdString());
      chkValidCubeList.write(chkValidCubeListFile.toStdString());

      // set up pvl def file
      PvlGroup validMeasureGroup("ValidMeasure");
      validMeasureGroup.addKeyword(PvlKeyword("MinDN", "-1000000")); 
      validMeasureGroup.addKeyword(PvlKeyword("MaxDN", "1000000")); 
      validMeasureGroup.addKeyword(PvlKeyword("MinEmission", "0")); 
      validMeasureGroup.addKeyword(PvlKeyword("MaxEmission", "135")); 
      validMeasureGroup.addKeyword(PvlKeyword("MinIncidence", "0")); 
      validMeasureGroup.addKeyword(PvlKeyword("MaxIncidence", "135")); 
      validMeasureGroup.addKeyword(PvlKeyword("MinResolution", "0")); 
      validMeasureGroup.addKeyword(PvlKeyword("MaxResolution", "1000")); 
      validMeasureGroup.addKeyword(PvlKeyword("PixelsFromEdge", "5")); 
      validMeasureGroup.addKeyword(PvlKeyword("SampleResidual", "5")); 
      validMeasureGroup.addKeyword(PvlKeyword("LineResidual", "5")); 
      validMeasureGroup.addKeyword(PvlKeyword("SampleShift", "3")); 
      validMeasureGroup.addKeyword(PvlKeyword("LineShift", "5")); 
      Pvl p;
      p.addGroup(validMeasureGroup);
      p.write(defFile.toStdString());  
    }
};

class CneteditMeasureList : public TempTestingFiles {
  protected:
    QString cnet35File;
    QString badMeasureListFile1;
    QString badMeasureListFile2;

    void SetUp() override {
      TempTestingFiles::SetUp();

      cnet35File = "data/cnetedit/cnet_35pts.pvl";
      badMeasureListFile1 = tempDir.path() + "badMeasureList1.lis";
      badMeasureListFile2 = tempDir.path() + "badMeasureList2.lis";

      // setup badMeasureLists with pvl label files
      FileList badMeasureList1;
      badMeasureList1.append("I24827003RDR_bndry_32,data/cnetedit/I10101002RDR.dstr.cub.label.pvl");
      badMeasureList1.append("I24827003RDR_bndry_32,data/cnetedit/I10413004RDR.dstr.cub.label.pvl");
      badMeasureList1.append("I24827003RDR_bndry_35,data/cnetedit/I07873009RDR.dstr.cub.label.pvl");
      badMeasureList1.append("I24827003RDR_bndry_35,data/cnetedit/I23604003RDR.dstr.cub.label.pvl");
      badMeasureList1.append("I24827003RDR_bndry_35,data/cnetedit/I24827003RDR.dstr.cub.label.pvl");
      badMeasureList1.append("I24827003RDR_bndry_36,data/cnetedit/I07873009RDR.dstr.cub.label.pvl");
      badMeasureList1.append("I24827003RDR_bndry_36,data/cnetedit/I24827003RDR.dstr.cub.label.pvl");
      badMeasureList1.append("I24827003RDR_bndry_50,data/cnetedit/I24827003RDR.dstr.cub.label.pvl");
      badMeasureList1.append("I24827003RDR_bndry_53,data/cnetedit/I24827003RDR.dstr.cub.label.pvl");
      badMeasureList1.append("I24827003RDR_bndry_56,data/cnetedit/I24827003RDR.dstr.cub.label.pvl");
      badMeasureList1.append("I24827003RDR_bndry_8,data/cnetedit/I07873009RDR.dstr.cub.label.pvl");
      badMeasureList1.write(badMeasureListFile1.toStdString());

      FileList badMeasureList2(badMeasureList1);
      badMeasureList2.append("I24827003RDR_bndry_11,data/cnetedit/I24827003RDR.dstr.cub.label.pvl");
      badMeasureList2.write(badMeasureListFile2.toStdString());
    }
};


class Cnetedit : public TempTestingFiles {
  protected:
    QString cnet108File;
    QString pointListFile;
    QString cubeListFile;
    QString measureListFile;

    void SetUp() override {
      TempTestingFiles::SetUp();

      cnet108File = "data/cnetedit/cnet_108pts.pvl";
      pointListFile = tempDir.path() + "/pointList.lis";
      cubeListFile = tempDir.path() + "/cubeList.lis";
      measureListFile = tempDir.path() + "measureList.lis";
     
      // set up cube list
      FileList cubeList;
      cubeList.append("data/cnetedit/e0902065.cal.sub.cub");
      cubeList.write(cubeListFile.toStdString());

      // set up measureList
      FileList measureList;
      measureList.append("new0001,data/cnetedit/e0902065.cal.sub.cub");
      measureList.write(measureListFile.toStdString());

      // set up point list
      FileList pointList;
      pointList.append("new0007");
      pointList.append("new0050");
      pointList.append("new0001");
      pointList.append("new0036");
      pointList.append("new0020");
      pointList.append("new0008");
      pointList.write(pointListFile.toStdString());
    }
};


/**
   * FunctionalTestCneteditCheckValid
   * 
   * Cnetedit test of check valid functionality.
   * Input ...
   *   1) ControlNet with 11 points  (data/cnetedit/cnet_11pts.pvl)
   *   2) two image cube list file   (data/cnetedit/PSP_002733_1880_RED4.crop.cub;
   *                                  data/cnetedit/PSP_002733_1880_RED5.crop.cub)
   *   3) def file
   *   4) CHECKVALID = yes
   *   5) RETAIN_REFERENCE=yes
   * 
   * Output ...
   *    1) edited ControlNet
   *    2) Pvl log file.
   */
TEST_F(CneteditCheckValid, FunctionalTestCneteditCheckValid) {
  QTemporaryDir tempDir;

  QVector<QString> args = {"cnet=" + cnet11File,
                           "log=" + tempDir.path() + "/log.txt",
                           "checkvalid=yes",
                           "fromlist=" + chkValidCubeListFile,
                           "retain_reference=yes",
                           "deffile=" + defFile,
                           "onet=" + tempDir.path() + "/out.net"
                           };

  UserInterface ui(APP_XML, args);

  Pvl cneteditLog;

  try {
    cneteditLog = cnetedit(ui);
  }
  catch(IException &e) {
    FAIL() <<  e.toString().c_str() << std::endl;
  }

  // read back log file
  Pvl log;
  try {
    log.read(tempDir.path().toStdString() + "/log.txt");
  }
  catch (IException &e) {
    FAIL() << "Unable to open log file: " << e.what() << std::endl;
  }

  // check number of deleted points and measures
  EXPECT_EQ((int)log.findKeyword("PointsDeleted"), 4);
  EXPECT_EQ((int)log.findKeyword("MeasuresDeleted"), 8);

  // no edit locked points or measures
  EXPECT_EQ(log.findObject("EditLocked").findObject("Points").keywords(), 0);
  EXPECT_EQ(log.findObject("EditLocked").findObject("Measures").keywords(), 0);

  // check retained references
  PvlObject retainedRefs = log.findObject("RetainedReferences");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, retainedRefs.findKeyword("pointregTest0001"),
                      "Validity Check failed:   Sample Residual is greater than"
                      " tolerance 5");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, retainedRefs.findKeyword("pointregTest0007"),
                      "Validity Check failed:   Sample Residual is greater than"
                      " tolerance 5");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, retainedRefs.findKeyword("pointregTest0008"),
                      "Validity Check failed:   Pixels From Edge is less than"
                      " tolerance 5");

  // check deleted points
  PvlObject deletedPoints = log.findObject("Deleted").findObject("Points");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("pointregTest0002"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("pointregTest0003"),
                      "Too few measures");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("pointregTest0004"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("pointregTest0011"),
                      "Too few measures");

  // check deleted measures
  PvlObject deletedMeasures = log.findObject("Deleted").findObject("Measures");
  PvlGroup pointregTest0002 = deletedMeasures.findGroup("pointregTest0002");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, pointregTest0002.
                      findKeyword("MRO/HIRISE/856864216:41044/RED4/2"),
                      "Point deleted");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, pointregTest0002.
                      findKeyword("MRO/HIRISE/856864216:41044/RED5/2"),
                      "Point deleted");
  PvlGroup pointregTest0003 = deletedMeasures.findGroup("pointregTest0003");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, pointregTest0003.
                      findKeyword("MRO/HIRISE/856864216:41044/RED5/2"),
                      "Validity Check failed:   Pixels From Edge is less than"
                      " tolerance 5 Line Residual is greater than tolerance 5");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, pointregTest0003.
                      findKeyword("MRO/HIRISE/856864216:41044/RED4/2"),
                      "Point deleted");
  PvlGroup pointregTest0004 = deletedMeasures.findGroup("pointregTest0004");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, pointregTest0004.
                      findKeyword("MRO/HIRISE/856864216:41044/RED4/2"),
                      "Point deleted");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, pointregTest0004.
                      findKeyword("MRO/HIRISE/856864216:41044/RED5/2"),
                      "Point deleted");
  PvlGroup pointregTest0011 = deletedMeasures.findGroup("pointregTest0011");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, pointregTest0011.
                      findKeyword("MRO/HIRISE/856864216:41044/RED5/2"),
                      "Validity Check failed:   Pixels From Edge is less than"
                      " tolerance 5");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, pointregTest0011.
                      findKeyword("MRO/HIRISE/856864216:41044/RED4/2"),
                      "Point deleted");

  // check output ControlNetwork
  ControlNet outNet(tempDir.path() + "/out.net");
  EXPECT_EQ(outNet.GetNumPoints(), 7);
  EXPECT_EQ(outNet.GetNumValidPoints(), 7);
  EXPECT_EQ(outNet.GetNumEditLockPoints(), 0);
  EXPECT_EQ(outNet.GetNumMeasures(), 14);
  EXPECT_EQ(outNet.GetNumIgnoredMeasures(), 0);
  EXPECT_EQ(outNet.GetNumEditLockMeasures(), 0);
}


/**
   * FunctionalTestCneteditCheckValidIgnoreAll
   * 
   * Cnetedit test of check valid functionality with IGNOREALL=YES.
   * Input ...
   *   1) ControlNet with 11 points  (data/cnetedit/cnet_11pts.pvl)
   *   2) two image cube list file   (data/cnetedit/PSP_002733_1880_RED4.crop.cub;
   *                                  data/cnetedit/PSP_002733_1880_RED5.crop.cub)
   *   3) def file
   *   4) CHECKVALID = yes
   *   5) IGNOREALL=yes
   * 
   * Output ...
   *   1) edited ControlNet
   *   2) Pvl log file.
   */
TEST_F(CneteditCheckValid, FunctionalTestCneteditCheckValidIgnoreAll) {
  QTemporaryDir tempDir;

  QVector<QString> args = {"cnet=" + cnet11File,
                           "log=" + tempDir.path() + "/log.txt",
                           "checkvalid=yes",
                           "ignoreall=yes",
                           "fromlist=" + chkValidCubeListFile,
                           "deffile=" + defFile,
                           "onet=" + tempDir.path() + "/out.net"
                           };

  UserInterface ui(APP_XML, args);

  Pvl cneteditLog;

  try {
    cneteditLog = cnetedit(ui);
  }
  catch(IException &e) {
    FAIL() <<  e.toString().c_str() << std::endl;
  }

  // read back log file
  Pvl log;
  try {
    log.read(tempDir.path().toStdString() + "/log.txt");
  }
  catch (IException &e) {
    FAIL() << "Unable to open log file: " << e.what() << std::endl;
  }

  // check number of deleted points and measures
  EXPECT_EQ((int)log.findKeyword("PointsDeleted"), 7);
  EXPECT_EQ((int)log.findKeyword("MeasuresDeleted"), 14);

  // no edit locked points or measures
  EXPECT_EQ(log.findObject("EditLocked").findObject("Points").keywords(), 0);
  EXPECT_EQ(log.findObject("EditLocked").findObject("Measures").keywords(), 0);

  // no retained references
  EXPECT_EQ(log.findObject("RetainedReferences").keywords(), 0);

  // check deleted points
  PvlObject deletedPoints = log.findObject("Deleted").findObject("Points");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("pointregTest0001"),
                      "Reference measure ignored");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("pointregTest0002"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("pointregTest0003"),
                      "Too few measures");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("pointregTest0004"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("pointregTest0007"),
                      "Reference measure ignored");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("pointregTest0008"),
                      "Reference measure ignored");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("pointregTest0011"),
                      "Too few measures");

  // check deleted measures
  PvlObject deletedMeasures = log.findObject("Deleted").findObject("Measures");
  PvlGroup pointregTest0001 = deletedMeasures.findGroup("pointregTest0001");

  EXPECT_PRED_FORMAT2(AssertStringsEqual, pointregTest0001.
                      findKeyword("MRO/HIRISE/856864216:41044/RED4/2"),
                      "Validity Check failed:   Sample Residual is greater than "
                      "tolerance 5");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, pointregTest0001.
                      findKeyword("MRO/HIRISE/856864216:41044/RED5/2"),
                      "Reference ignored");
  PvlGroup pointregTest0002 = deletedMeasures.findGroup("pointregTest0002");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, pointregTest0002.
                      findKeyword("MRO/HIRISE/856864216:41044/RED4/2"),
                      "Point deleted");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, pointregTest0002.
                      findKeyword("MRO/HIRISE/856864216:41044/RED5/2"),
                      "Reference ignored");
  PvlGroup pointregTest0003 = deletedMeasures.findGroup("pointregTest0003");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, pointregTest0003.
                      findKeyword("MRO/HIRISE/856864216:41044/RED5/2"),
                      "Validity Check failed:   Pixels From Edge is less than"
                      " tolerance 5 Line Residual is greater than tolerance 5");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, pointregTest0003.
                      findKeyword("MRO/HIRISE/856864216:41044/RED4/2"),
                      "Point deleted");
  PvlGroup pointregTest0004 = deletedMeasures.findGroup("pointregTest0004");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, pointregTest0004.
                      findKeyword("MRO/HIRISE/856864216:41044/RED4/2"),
                      "Point deleted");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, pointregTest0004.
                      findKeyword("MRO/HIRISE/856864216:41044/RED5/2"),
                      "Reference ignored");
  PvlGroup pointregTest0007 = deletedMeasures.findGroup("pointregTest0007");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, pointregTest0007.
                      findKeyword("MRO/HIRISE/856864216:41044/RED4/2"),
                      "Validity Check failed:   Sample Residual is greater than "
                      "tolerance 5");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, pointregTest0007.
                      findKeyword("MRO/HIRISE/856864216:41044/RED5/2"),
                      "Reference ignored");
  PvlGroup pointregTest0008 = deletedMeasures.findGroup("pointregTest0008");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, pointregTest0008.
                      findKeyword("MRO/HIRISE/856864216:41044/RED4/2"),
                      "Validity Check failed:   Pixels From Edge is less than "
                      "tolerance 5");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, pointregTest0008.
                      findKeyword("MRO/HIRISE/856864216:41044/RED5/2"),
                      "Reference ignored");
  PvlGroup pointregTest0011 = deletedMeasures.findGroup("pointregTest0011");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, pointregTest0011.
                      findKeyword("MRO/HIRISE/856864216:41044/RED5/2"),
                      "Validity Check failed:   Pixels From Edge is less than"
                      " tolerance 5");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, pointregTest0011.
                      findKeyword("MRO/HIRISE/856864216:41044/RED4/2"),
                      "Point deleted");

  // check output ControlNetwork
  ControlNet outNet(tempDir.path() + "/out.net");
  EXPECT_EQ(outNet.GetNumPoints(), 4);
  EXPECT_EQ(outNet.GetNumValidPoints(), 4);
  EXPECT_EQ(outNet.GetNumEditLockPoints(), 0);
  EXPECT_EQ(outNet.GetNumMeasures(), 8);
  EXPECT_EQ(outNet.GetNumIgnoredMeasures(), 0);
  EXPECT_EQ(outNet.GetNumEditLockMeasures(), 0);
}


/**
   * CneteditDefault
   * 
   * Input ...
   *   1) ControlNet with 108 points (data/cnetedit/cnet_108pts.pvl)
   * 
   * Output ...
   *   1) edited ControlNet
   *   2) Pvl log file.
   */
TEST_F(Cnetedit, FunctionalTestCneteditDefault) {
  QTemporaryDir tempDir;

  QVector<QString> args = {"cnet=" + cnet108File,
                           "log=" + tempDir.path() + "/log.txt",
                           "onet=" + tempDir.path() + "/out.net"
                           };

  UserInterface ui(APP_XML, args);

  Pvl cneteditLog;

  try {
    cneteditLog = cnetedit(ui);
  }
  catch(IException &e) {
    FAIL() <<  e.toString().c_str() << std::endl;
  }

  // read back log file
  Pvl log;
  try {
    log.read(tempDir.path().toStdString() + "/log.txt");
  }
  catch (IException &e) {
    FAIL() << "Unable to open log file: " << e.what() << std::endl;
  }

  // check number of deleted points and measures
  EXPECT_EQ((int)log.findKeyword("PointsDeleted"), 8);
  EXPECT_EQ((int)log.findKeyword("MeasuresDeleted"), 21);

  // no edit locked points or measures
  EXPECT_EQ(log.findObject("EditLocked").findObject("Points").keywords(), 0);
  EXPECT_EQ(log.findObject("EditLocked").findObject("Measures").keywords(), 0);

  // no retained references
  EXPECT_EQ(log.findObject("RetainedReferences").keywords(), 0);

  // check deleted points
  PvlObject deletedPoints = log.findObject("Deleted").findObject("Points");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0031"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0032"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0038"),
                      "Too few measures");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0039"),
                      "Too few measures");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0064"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0065"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0067"),
                      "Too few measures");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0068"),
                      "Ignored from input");

  // check deleted measures
  PvlObject deletedMeasures = log.findObject("Deleted").findObject("Measures");
  PvlGroup new0031 = deletedMeasures.findGroup("new0031");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, new0031.
                      findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Point deleted");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, new0031.
                      findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Point deleted");
  PvlGroup new0032 = deletedMeasures.findGroup("new0032");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, new0032.
                      findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Point deleted");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, new0032.
                      findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Point deleted");
  PvlGroup new0038 = deletedMeasures.findGroup("new0038");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, new0038.
                      findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, new0038.
                      findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Point deleted");
  PvlGroup new0039 = deletedMeasures.findGroup("new0039");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, new0039.
                      findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, new0039.
                      findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Ignored from input");
  PvlGroup new0064 = deletedMeasures.findGroup("new0064");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, new0064.
                      findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Point deleted");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, new0064.
                      findKeyword("MGS/688540926:0/MOC-WA/RED"),
                      "Point deleted");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, new0064.
                      findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Point deleted");
  PvlGroup new0065 = deletedMeasures.findGroup("new0065");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, new0065.
                      findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, new0065.
                      findKeyword("MGS/688540926:0/MOC-WA/RED"),
                      "Point deleted");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, new0065.
                      findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Point deleted");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0066").
                      findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Ignored from input");
  PvlGroup new0067 = deletedMeasures.findGroup("new0067");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, new0067.
                      findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, new0067.
                      findKeyword("MGS/688540926:0/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, new0067.
                      findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Point deleted");
  PvlGroup new0068 = deletedMeasures.findGroup("new0068");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, new0068.
                      findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, new0068.
                      findKeyword("MGS/688540926:0/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, new0068.
                      findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Ignored from input");

  // check output ControlNetwork
  ControlNet outNet(tempDir.path() + "/out.net");
  EXPECT_EQ(outNet.GetNumPoints(), 100);
  EXPECT_EQ(outNet.GetNumValidPoints(), 100);
  EXPECT_EQ(outNet.GetNumEditLockPoints(), 0);
  EXPECT_EQ(outNet.GetNumMeasures(), 240);
  EXPECT_EQ(outNet.GetNumIgnoredMeasures(), 0);
  EXPECT_EQ(outNet.GetNumEditLockMeasures(), 0);
}


/**
   * CneteditEditlock
   * 
   * Cnetedit test of edit lock functionality.
   * Input ...
   *   1) ControlNet with 108 points (data/cnetedit/cnet_108pts.pvl)
   *   2) one image cube list file   (data/cnetedit/e0902065.cal.sub.cub)
   *   3) points list file
   *   4) ignore=no
   *   5) delete=no
   *   6) lock=yes
   * 
   * Output ...
   *   1) edited ControlNet
   *   2) Pvl log file.
   */
TEST_F(Cnetedit, FunctionalTestCneteditEditlock) {
  QTemporaryDir tempDir;

  QVector<QString> args = {"cnet=" + cnet108File,
                           "log=" + tempDir.path() + "/log.txt",
                           "cubelist=" + cubeListFile,
                           "pointlist=" + pointListFile,
                           "ignore=no",
                           "delete=no",
                           "lock=yes",
                           "onet=" + tempDir.path() + "/out.net"
                           };

  UserInterface ui(APP_XML, args);

  Pvl cneteditLog;

  try {
    cneteditLog = cnetedit(ui);
  }
  catch(IException &e) {
    FAIL() <<  e.toString().c_str() << std::endl;
  }

  // read back log file
  Pvl log;
  try {
    log.read(tempDir.path().toStdString() + "/log.txt");
  }
  catch (IException &e) {
    FAIL() << "Unable to open log file: " << e.what() << std::endl;
  }

  // check number of deleted points and measures
  EXPECT_EQ((int)log.findKeyword("PointsDeleted"), 0);
  EXPECT_EQ((int)log.findKeyword("MeasuresDeleted"), 0);

  // no edit locked points or measures
  EXPECT_EQ(log.findObject("EditLocked").findObject("Points").keywords(), 0);
  EXPECT_EQ(log.findObject("EditLocked").findObject("Measures").groups(), 0);

  // no retained references
  EXPECT_EQ(log.findObject("RetainedReferences").keywords(), 0);

  // check deleted points
  PvlObject ignoredPoints = log.findObject("Ignored").findObject("Points");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredPoints.findKeyword("new0031"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredPoints.findKeyword("new0032"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredPoints.findKeyword("new0064"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredPoints.findKeyword("new0065"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredPoints.findKeyword("new0068"),
                      "Ignored from input");

  // check ignored measures
  PvlObject ignoredMeasures = log.findObject("Ignored").findObject("Measures");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0038").
                      findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Ignored from input");
  PvlGroup new0039 = ignoredMeasures.findGroup("new0039");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, new0039.
                      findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, new0039.
                      findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0065").
                      findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0066").
                      findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Ignored from input");
  PvlGroup new0067 = ignoredMeasures.findGroup("new0067");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, new0067.
                      findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, new0067.
                      findKeyword("MGS/688540926:0/MOC-WA/RED"),
                      "Ignored from input");
  PvlGroup new0068 = ignoredMeasures.findGroup("new0068");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, new0068.
                      findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, new0068.
                      findKeyword("MGS/688540926:0/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, new0068.
                      findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Ignored from input");

  // check output ControlNetwork
  ControlNet outNet(tempDir.path() + "/out.net");
  EXPECT_EQ(outNet.GetNumPoints(), 108);
  EXPECT_EQ(outNet.GetNumValidPoints(), 103);
  EXPECT_EQ(outNet.GetNumEditLockPoints(), 6); //should be 6
  EXPECT_EQ(outNet.GetNumMeasures(), 261);
  EXPECT_EQ(outNet.GetNumIgnoredMeasures(), 10);  
  EXPECT_EQ(outNet.GetNumEditLockMeasures(), 52);  
}


/**
   * CneteditEditUnlock
   * 
   * Cnetedit test of edit lock functionality with unlock=yes.
   * Input ...
   *   1) ControlNet with 108 points (data/cnetedit/cnet_108pts.pvl)
   *   2) one image cube list file   (data/cnetedit/e0902065.cal.sub.cub)
   *   3) points list file
   *   4) ignore=no
   *   5) delete=no
   *   6) unlock=yes
   * 
   * Output ...
   *   1) edited ControlNet
   *   2) Pvl log file.
   */
TEST_F(Cnetedit, FunctionalTestCneteditEditUnlock) {
  QTemporaryDir tempDir;

  QVector<QString> args = {"cnet=" + cnet108File,
                           "log=" + tempDir.path() + "/log.txt",
                           "cubelist=" + cubeListFile,
                           "pointlist=" + pointListFile,
                           "ignore=no",
                           "delete=no",
                           "unlock=yes",
                           "onet=" + tempDir.path() + "/out.net"
                           };

  UserInterface ui(APP_XML, args);

  Pvl cneteditLog;

  try {
    cneteditLog = cnetedit(ui);
  }
  catch(IException &e) {
    FAIL() <<  e.toString().c_str() << std::endl;
  }

  // read back log file
  Pvl log;
  try {
    log.read(tempDir.path().toStdString() + "/log.txt");
  }
  catch (IException &e) {
    FAIL() << "Unable to open log file: " << e.what() << std::endl;
  }

  // check number of deleted points and measures
  EXPECT_EQ((int)log.findKeyword("PointsDeleted"), 0);
  EXPECT_EQ((int)log.findKeyword("MeasuresDeleted"), 0);

  // no edit locked points or measures
  EXPECT_EQ(log.findObject("EditLocked").findObject("Points").keywords(), 0);
  EXPECT_EQ(log.findObject("EditLocked").findObject("Measures").keywords(), 0);

  // no retained references
  EXPECT_EQ(log.findObject("RetainedReferences").keywords(), 0);

  // check deleted points
  PvlObject ignoredPoints = log.findObject("Ignored").findObject("Points");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredPoints.findKeyword("new0031"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredPoints.findKeyword("new0032"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredPoints.findKeyword("new0064"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredPoints.findKeyword("new0065"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredPoints.findKeyword("new0068"),
                      "Ignored from input");

  // check ignored measures
  PvlObject ignoredMeasures = log.findObject("Ignored").findObject("Measures");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0038").
                      findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Ignored from input");
  PvlGroup new0039 = ignoredMeasures.findGroup("new0039");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, new0039.
                      findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, new0039.
                      findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0065").
                      findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0066").
                      findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Ignored from input");
  PvlGroup new0067 = ignoredMeasures.findGroup("new0067");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, new0067.
                      findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, new0067.
                      findKeyword("MGS/688540926:0/MOC-WA/RED"),
                      "Ignored from input");
  PvlGroup new0068 = ignoredMeasures.findGroup("new0068");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, new0068.
                      findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, new0068.
                      findKeyword("MGS/688540926:0/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, new0068.
                      findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Ignored from input");

  // check output ControlNetwork
  ControlNet outNet(tempDir.path() + "/out.net");
  EXPECT_EQ(outNet.GetNumPoints(), 108);
  EXPECT_EQ(outNet.GetNumValidPoints(), 103);
  EXPECT_EQ(outNet.GetNumEditLockPoints(), 0);
  EXPECT_EQ(outNet.GetNumMeasures(), 261);
  EXPECT_EQ(outNet.GetNumIgnoredMeasures(), 10);
  EXPECT_EQ(outNet.GetNumEditLockMeasures(), 0);
}


/**
   * CneteditError
   * 
   * Cnetedit test given a nonexistant input ControlNet.
   */
TEST_F(Cnetedit, FunctionalTestCneteditError) {

  QVector<QString> args = {"cnet=cnet.net",
                           "onet=cnet.net",
                           };

  UserInterface ui(APP_XML, args);

  Pvl log;

  try {
    log = cnetedit(ui);
    FAIL() << "Expected Exception for an invalid control network";
  }
  catch(IException &e) {
    EXPECT_TRUE(e.toString().find("Invalid control network"))
      <<  e.toString();
  }
}


/**
   * CneteditIgnore
   * 
   * Cnetedit test with IGNOREALL=YES
   * Input ...
   *   1) ControlNet with 108 points (data/cnetedit/cnet_108pts.pvl)
   *   2) one image cube list file   (data/cnetedit/e0902065.cal.sub.cub)
   *   3) measure list file          (new0001,data/cnetedit/e0902065.cal.sub.cub)
   *   4) delete=no
   *   5) ignoreall=yes
   * 
   * Output ...
   *   1) edited ControlNet
   *   2) Pvl log file. 
   */
TEST_F(Cnetedit, CneteditIgnore) {
  QTemporaryDir tempDir;

  QVector<QString> args = {"cnet=" + cnet108File,
                           "cubelist=" + cubeListFile,
                           "measurelist=" + measureListFile,
                           "onet=" + tempDir.path() + "/out.net",
//                           "onet=/Users/kledmundson/ISISDev/cnetedit/Oct242023cne/ISIS3/isis/tests/data/cnetedit/ignore/truth/gtestCnet1.net",
                           "delete=no",
                           "ignoreall=yes",
                           "log=" + tempDir.path() + "/log.txt"
//                           "log=/Users/kledmundson/ISISDev/cnetedit/Oct242023cne/ISIS3/isis/tests/data/cnetedit/ignore/truth/gtestLog1.txt",
                           };

  UserInterface ui(APP_XML, args);

  Pvl cneteditlog;

  try {
    cneteditlog = cnetedit(ui);
  }
  catch(IException &e) {
    FAIL() <<  e.toString().c_str() << std::endl;
  }

    // read back log file
  Pvl log;
  try {
    log.read(tempDir.path().toStdString() + "/log.txt");
  }
  catch (IException &e) {
    FAIL() << "Unable to open log file: " << e.what() << std::endl;
  }

  // check number of deleted points and measures
  EXPECT_EQ((int)log.findKeyword("PointsDeleted"), 0);
  EXPECT_EQ((int)log.findKeyword("MeasuresDeleted"), 0);

  // no edit locked points or measures
  EXPECT_EQ(log.findObject("EditLocked").findObject("Points").keywords(), 0);
  EXPECT_EQ(log.findObject("EditLocked").findObject("Measures").keywords(), 0);

  // no retained references
  EXPECT_EQ(log.findObject("RetainedReferences").keywords(), 0);

  // check ignored points
  PvlObject ignoredPoints = log.findObject("Ignored").findObject("Points");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredPoints.findKeyword("new0001"),
                      "Reference measure ignored");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredPoints.findKeyword("new0002"),
                      "Reference measure ignored");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredPoints.findKeyword("new0031"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredPoints.findKeyword("new0032"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredPoints.findKeyword("new0038"),
                      "Reference measure ignored");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredPoints.findKeyword("new0039"),
                      "Reference measure ignored");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredPoints.findKeyword("new0064"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredPoints.findKeyword("new0065"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredPoints.findKeyword("new0067"),
                      "Reference measure ignored");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredPoints.findKeyword("new0068"),
                      "Ignored from input");

  // check ignored measures
  PvlObject ignoredMeasures = log.findObject("Ignored").findObject("Measures");
  PvlGroup new0001 = ignoredMeasures.findGroup("new0001");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0001.findKeyword("MGS/688540926:0/MOC-WA/RED"),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0001.findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Reference ignored");

  new0001.deleteKeyword("MGS/688540926:0/MOC-WA/RED");

  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0001.findKeyword("MGS/688540926:0/MOC-WA/RED"),
                      "Measure in MEASURELIST");

  PvlGroup new0002 = ignoredMeasures.findGroup("new0002");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0002.findKeyword("MGS/688540926:0/MOC-WA/RED"),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0002.findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Reference ignored");
  PvlGroup new0038 = ignoredMeasures.findGroup("new0038");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0038.findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0038.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Reference ignored");

  new0038.deleteKeyword("MGS/718369703:160/MOC-WA/RED");

  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0038.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Ignored from input");

  PvlGroup new0039 = ignoredMeasures.findGroup("new0039");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0039.findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0039.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0064").
                      findKeyword("MGS/688540926:0/MOC-WA/RED"),
                      "Serial Number in CUBELIST");
  PvlGroup new0065 = ignoredMeasures.findGroup("new0065");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0065.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0065.findKeyword("MGS/688540926:0/MOC-WA/RED"),
                      "Reference ignored");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0065.findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Reference ignored");

  new0065.deleteKeyword("MGS/688540926:0/MOC-WA/RED");
  new0065.deleteKeyword("MGS/691204200:96/MOC-WA/RED");

  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0065.findKeyword("MGS/688540926:0/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0065.findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Ignored from input");

  PvlGroup new0066 = ignoredMeasures.findGroup("new0066");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0066.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0066.findKeyword("MGS/688540926:0/MOC-WA/RED"),
                      "Reference ignored");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0066.findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Reference ignored");

  new0066.deleteKeyword("MGS/688540926:0/MOC-WA/RED");
  new0066.deleteKeyword("MGS/691204200:96/MOC-WA/RED");

  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0066.findKeyword("MGS/688540926:0/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0066.findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Ignored from input");

  PvlGroup new0067 = ignoredMeasures.findGroup("new0067");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0067.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0067.findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Reference ignored");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0067.findKeyword("MGS/688540926:0/MOC-WA/RED"),
                      "Ignored from input");

  new0067.deleteKeyword("MGS/691204200:96/MOC-WA/RED");

  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0067.findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Ignored from input");

  PvlGroup new0068 = ignoredMeasures.findGroup("new0068");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0068.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0068.findKeyword("MGS/688540926:0/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0068.findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0069").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0070").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0071").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0072").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0073").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0074").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0075").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0076").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0077").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0078").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0079").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0080").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0081").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0082").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0083").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0084").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0085").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0086").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0087").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0088").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0089").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0090").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0091").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0092").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0093").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0094").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0095").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0096").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0097").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0098").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0099").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0100").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0101").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0102").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0103").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0104").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0105").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0106").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0107").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0108").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");

  // check output ControlNetwork
  ControlNet outNet(tempDir.path() + "/out.net");
  EXPECT_EQ(outNet.GetNumPoints(), 108);
  EXPECT_EQ(outNet.GetNumValidPoints(), 97);
  EXPECT_EQ(outNet.GetNumEditLockPoints(), 0);
  EXPECT_EQ(outNet.GetNumMeasures(), 261);
  EXPECT_EQ(outNet.GetNumIgnoredMeasures(), 61);
  EXPECT_EQ(outNet.GetNumEditLockMeasures(), 0);
}


/**
   * CneteditIgnoreMeasuresPoints
   * 
   * Cnetedit test with ignore measures and points lists.
   * Input ...
   *   1) ControlNet with 108 points (data/cnetedit/cnet_108pts.pvl)
   *   2) one image cube list file   (data/cnetedit/e0902065.cal.sub.cub)
   *   3) point list file
   * 
   * Output ...
   *   1) edited ControlNet
   *   2) Pvl log file.
   */
TEST_F(Cnetedit, CneteditIgnoreMeasuresPoints) {
  QTemporaryDir tempDir;

  QVector<QString> args = {"cnet=" + cnet108File,
                           "cubelist=" + cubeListFile,
                           "log=" + tempDir.path() + "/log.txt",
                           "pointlist=" + pointListFile,
                           "onet=" + tempDir.path() + "/out.net",
                           };

  UserInterface ui(APP_XML, args);

  Pvl cneteditlog;

  try {
    cneteditlog = cnetedit(ui);
  }
  catch(IException &e) {
    FAIL() <<  e.toString().c_str() << std::endl;
  }

    // read back log file
  Pvl log;
  try {
    log.read(tempDir.path().toStdString() + "/log.txt");
  }
  catch (IException &e) {
    FAIL() << "Unable to open log file: " << e.what() << std::endl;
  }

  // check number of deleted points and measures
  EXPECT_EQ((int)log.findKeyword("PointsDeleted"), 16);
  EXPECT_EQ((int)log.findKeyword("MeasuresDeleted"), 77);

  // no edit locked points or measures
  EXPECT_EQ(log.findObject("EditLocked").findObject("Points").keywords(), 0);
  EXPECT_EQ(log.findObject("EditLocked").findObject("Measures").keywords(), 0);

  // no retained references
  EXPECT_EQ(log.findObject("RetainedReferences").keywords(), 0);

  // check deleted points
  PvlObject deletedPoints = log.findObject("Deleted").findObject("Points");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0001"),
                      "Point ID in POINTLIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0002"),
                      "Too few measures");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0007"),
                      "Point ID in POINTLIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0008"),
                      "Point ID in POINTLIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0020"),
                      "Point ID in POINTLIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0031"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0032"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0036"),
                      "Point ID in POINTLIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0038"),
                      "Too few measures");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0039"),
                      "Too few measures");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0050"),
                      "Point ID in POINTLIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0064"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0065"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0066"),
                      "Too few measures");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0067"),
                      "Too few measures");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0068"),
                      "Ignored from input");

  // check deleted measures
  PvlObject deletedMeasures = log.findObject("Deleted").findObject("Measures");
  PvlGroup new0001 = deletedMeasures.findGroup("new0001");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0001.findKeyword("MGS/688540926:0/MOC-WA/RED"),
                      "Point deleted");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0001.findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Point deleted");
  PvlGroup new0002 = deletedMeasures.findGroup("new0002");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0002.findKeyword("MGS/688540926:0/MOC-WA/RED"),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0002.findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Point deleted");
  PvlGroup new0007 = deletedMeasures.findGroup("new0007");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0007.findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Point deleted");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0007.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Point deleted");
  PvlGroup new0008 = deletedMeasures.findGroup("new0008");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0008.findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Point deleted");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0008.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Point deleted");
  PvlGroup new0020 = deletedMeasures.findGroup("new0020");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0020.findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Point deleted");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0020.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Point deleted");
  PvlGroup new0031 = deletedMeasures.findGroup("new0031");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0031.findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Point deleted");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0031.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Point deleted");
  PvlGroup new0032 = deletedMeasures.findGroup("new0032");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0032.findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Point deleted");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0032.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Point deleted");
  PvlGroup new0036 = deletedMeasures.findGroup("new0036");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0036.findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Point deleted");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0036.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Point deleted");
  PvlGroup new0038 = deletedMeasures.findGroup("new0038");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0038.findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0038.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Point deleted");
  PvlGroup new0039 = deletedMeasures.findGroup("new0039");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0039.findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0039.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Ignored from input");
  PvlGroup new0050 = deletedMeasures.findGroup("new0050");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0050.findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Point deleted");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0050.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Point deleted");
  PvlGroup new0064 = deletedMeasures.findGroup("new0064");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0064.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Point deleted");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0064.findKeyword("MGS/688540926:0/MOC-WA/RED"),
                      "Point deleted");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0064.findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Point deleted");
  PvlGroup new0065 = deletedMeasures.findGroup("new0065");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0065.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0065.findKeyword("MGS/688540926:0/MOC-WA/RED"),
                      "Point deleted");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0065.findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Point deleted");
  PvlGroup new0066 = deletedMeasures.findGroup("new0066");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0066.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0066.findKeyword("MGS/688540926:0/MOC-WA/RED"),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0066.findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Point deleted");
  PvlGroup new0067 = deletedMeasures.findGroup("new0067");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0067.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0067.findKeyword("MGS/688540926:0/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0067.findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Point deleted");
  PvlGroup new0068 = deletedMeasures.findGroup("new0068");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0068.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0068.findKeyword("MGS/688540926:0/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0068.findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0069").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0070").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0071").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0072").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0073").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0074").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0075").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0076").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0077").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0078").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0079").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0080").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0081").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0082").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0083").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0084").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0085").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0086").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0087").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0088").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0089").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0090").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0091").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0092").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0093").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0094").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0095").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0096").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0097").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0098").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0099").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0100").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0101").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0102").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0103").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0104").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0105").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0106").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0107").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0108").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");

  // check output ControlNetwork
  ControlNet outNet(tempDir.path() + "/out.net");
  EXPECT_EQ(outNet.GetNumPoints(), 92);
  EXPECT_EQ(outNet.GetNumValidPoints(), 92);
  EXPECT_EQ(outNet.GetNumEditLockPoints(), 0);
  EXPECT_EQ(outNet.GetNumMeasures(), 184);
  EXPECT_EQ(outNet.GetNumIgnoredMeasures(), 0);
  EXPECT_EQ(outNet.GetNumEditLockMeasures(), 0);
}


/**
   * CneteditIgnorePoints
   * 
   * Cnetedit test with ignore points list.
   * Input ...
   *   1) ControlNet with 108 points (data/cnetedit/cnet_108pts.pvl)
   *   2) point list file
   * 
   * Output ...
   *   1) edited ControlNet
   *   2) Pvl log file.
   */
TEST_F(Cnetedit, CneteditIgnorePoints) {
  QTemporaryDir tempDir;

  QVector<QString> args = {"cnet=" + cnet108File,
                           "log=" + tempDir.path() + "/log.txt",
                           "pointlist=" + pointListFile,
                           "onet=" + tempDir.path() + "/out.net"
                           };

  UserInterface ui(APP_XML, args);

  Pvl cneteditlog;

  try {
    cneteditlog = cnetedit(ui);
  }
  catch(IException &e) {
    FAIL() <<  e.toString().c_str() << std::endl;
  }

    // read back log file
  Pvl log;
  try {
    log.read(tempDir.path().toStdString() + "/log.txt");
  }
  catch (IException &e) {
    FAIL() << "Unable to open log file: " << e.what() << std::endl;
  }

  // check number of deleted points and measures
  EXPECT_EQ((int)log.findKeyword("PointsDeleted"), 14);
  EXPECT_EQ((int)log.findKeyword("MeasuresDeleted"), 33);

  // no edit locked points or measures
  EXPECT_EQ(log.findObject("EditLocked").findObject("Points").keywords(), 0);
  EXPECT_EQ(log.findObject("EditLocked").findObject("Measures").keywords(), 0);

  // no retained references
  EXPECT_EQ(log.findObject("RetainedReferences").keywords(), 0);

  // check deleted points
  PvlObject deletedPoints = log.findObject("Deleted").findObject("Points");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0001"),
                      "Point ID in POINTLIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0007"),
                      "Point ID in POINTLIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0008"),
                      "Point ID in POINTLIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0020"),
                      "Point ID in POINTLIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0031"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0032"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0036"),
                      "Point ID in POINTLIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0038"),
                      "Too few measures");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0039"),
                      "Too few measures");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0050"),
                      "Point ID in POINTLIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0064"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0065"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0067"),
                      "Too few measures");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0068"),
                      "Ignored from input");

  // check deleted measures
  PvlObject deletedMeasures = log.findObject("Deleted").findObject("Measures");
  PvlGroup new0001 = deletedMeasures.findGroup("new0001");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0001.findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Point deleted");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0001.findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Point deleted");
  PvlGroup new0007 = deletedMeasures.findGroup("new0007");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0007.findKeyword("MGS/691204200:96/MOC-WA/RED "),
                      "Point deleted");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0007.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Point deleted");
  PvlGroup new0008 = deletedMeasures.findGroup("new0008");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0008.findKeyword("MGS/691204200:96/MOC-WA/RED "),
                      "Point deleted");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0008.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Point deleted");
  PvlGroup new0020 = deletedMeasures.findGroup("new0020");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0020.findKeyword("MGS/691204200:96/MOC-WA/RED "),
                      "Point deleted");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0020.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Point deleted");
  PvlGroup new0031 = deletedMeasures.findGroup("new0031");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0031.findKeyword("MGS/691204200:96/MOC-WA/RED "),
                      "Point deleted");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0031.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Point deleted");
  PvlGroup new0032 = deletedMeasures.findGroup("new0032");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0032.findKeyword("MGS/691204200:96/MOC-WA/RED "),
                      "Point deleted");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0032.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Point deleted");
  PvlGroup new0036 = deletedMeasures.findGroup("new0036");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0036.findKeyword("MGS/691204200:96/MOC-WA/RED "),
                      "Point deleted");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0036.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Point deleted");
  PvlGroup new0038 = deletedMeasures.findGroup("new0038");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0038.findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0038.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Point deleted");
  PvlGroup new0039 = deletedMeasures.findGroup("new0039");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0039.findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0039.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Ignored from input");
  PvlGroup new0050 = deletedMeasures.findGroup("new0050");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0050.findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Point deleted");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0050.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Point deleted");
  PvlGroup new0064 = deletedMeasures.findGroup("new0064");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0064.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Point deleted");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0064.findKeyword("MGS/688540926:0/MOC-WA/RED"),
                      "Point deleted");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0064").
                      findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Point deleted");
  PvlGroup new0065 = deletedMeasures.findGroup("new0065");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0065.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0065.findKeyword("MGS/688540926:0/MOC-WA/RED"),
                      "Point deleted");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0065.findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Point deleted");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0066").
                      findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Ignored from input");
  PvlGroup new0067 = deletedMeasures.findGroup("new0067");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0067.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0067.findKeyword("MGS/688540926:0/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0067.findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Point deleted");
  PvlGroup new0068 = deletedMeasures.findGroup("new0068");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0068.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0068.findKeyword("MGS/688540926:0/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0068.findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Ignored from input");

  // check output ControlNetwork
  ControlNet outNet(tempDir.path() + "/out.net");
  EXPECT_EQ(outNet.GetNumPoints(), 94);
  EXPECT_EQ(outNet.GetNumValidPoints(), 94);
  EXPECT_EQ(outNet.GetNumEditLockPoints(), 0);
  EXPECT_EQ(outNet.GetNumMeasures(), 228);
  EXPECT_EQ(outNet.GetNumIgnoredMeasures(), 0);
  EXPECT_EQ(outNet.GetNumEditLockMeasures(), 0);
}


/**
   * CneteditIgnoreAllPoints
   * 
   * Cnetedit test with ignore points list and IGNOREALL=YES.
   * Input ...
   *   1) ControlNet with 108 points (data/cnetedit/cnet_108pts.pvl)
   *   2) point list file
   *   3) ignoreall=yes
   * 
   * Output ...
   *   1) edited ControlNet
   *   2) Pvl log file.
   */
TEST_F(Cnetedit, CneteditIgnoreAllPoints) {
  QTemporaryDir tempDir;

  QVector<QString> args = {"cnet=" + cnet108File,
                           "log=" + tempDir.path() + "/log.txt",
                           "pointlist=" + pointListFile,
                           "ignoreall=yes",
                           "onet=" + tempDir.path() + "/out.net"
                           };

  UserInterface ui(APP_XML, args);

  Pvl cneteditlog;

  try {
    cneteditlog = cnetedit(ui);
  }
  catch(IException &e) {
    FAIL() <<  e.toString().c_str() << std::endl;
  }

    // read back log file
  Pvl log;
  try {
    log.read(tempDir.path().toStdString() + "/log.txt");
  }
  catch (IException &e) {
    FAIL() << "Unable to open log file: " << e.what() << std::endl;
  }

  // check number of deleted points and measures
  EXPECT_EQ((int)log.findKeyword("PointsDeleted"), 15);
  EXPECT_EQ((int)log.findKeyword("MeasuresDeleted"), 35);

  // no edit locked points or measures
  EXPECT_EQ(log.findObject("EditLocked").findObject("Points").keywords(), 0);
  EXPECT_EQ(log.findObject("EditLocked").findObject("Measures").keywords(), 0);

  // no retained references
  EXPECT_EQ(log.findObject("RetainedReferences").keywords(), 0);

  // check deleted points
  PvlObject deletedPoints = log.findObject("Deleted").findObject("Points");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0001"),
                      "Point ID in POINTLIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0007"),
                      "Point ID in POINTLIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0008"),
                      "Point ID in POINTLIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0020"),
                      "Point ID in POINTLIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0031"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0032"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0036"),
                      "Point ID in POINTLIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0038"),
                      "Reference measure ignored");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0039"),
                      "Reference measure ignored");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0050"),
                      "Point ID in POINTLIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0064"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0065"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0066"),
                      "Reference measure ignored");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0067"),
                      "Reference measure ignored");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0068"),
                      "Ignored from input");

  // check deleted measures
  PvlObject deletedMeasures = log.findObject("Deleted").findObject("Measures");
  PvlGroup new0001 = deletedMeasures.findGroup("new0001");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0001.findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Point deleted");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0001.findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Reference ignored");
  PvlGroup new0007 = deletedMeasures.findGroup("new0007");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0007.findKeyword("MGS/691204200:96/MOC-WA/RED "),
                      "Point deleted");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0007.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Reference ignored");
  PvlGroup new0008 = deletedMeasures.findGroup("new0008");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0008.findKeyword("MGS/691204200:96/MOC-WA/RED "),
                      "Point deleted");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0008.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Reference ignored");
  PvlGroup new0020 = deletedMeasures.findGroup("new0020");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0020.findKeyword("MGS/691204200:96/MOC-WA/RED "),
                      "Point deleted");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0020.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Reference ignored");
  PvlGroup new0031 = deletedMeasures.findGroup("new0031");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0031.findKeyword("MGS/691204200:96/MOC-WA/RED "),
                      "Point deleted");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0031.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Reference ignored");
  PvlGroup new0032 = deletedMeasures.findGroup("new0032");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0032.findKeyword("MGS/691204200:96/MOC-WA/RED "),
                      "Point deleted");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0032.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Reference ignored");
  PvlGroup new0036 = deletedMeasures.findGroup("new0036");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0036.findKeyword("MGS/691204200:96/MOC-WA/RED "),
                      "Point deleted");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0036.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Reference ignored");
  PvlGroup new0038 = deletedMeasures.findGroup("new0038");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0038.findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0038.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Reference ignored");

  new0038.deleteKeyword("MGS/718369703:160/MOC-WA/RED");
                      
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0038.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Ignored from input");
  PvlGroup new0039 = deletedMeasures.findGroup("new0039");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0039.findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0039.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Ignored from input");
  PvlGroup new0050 = deletedMeasures.findGroup("new0050");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0050.findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Point deleted");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0050.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Reference ignored");
  PvlGroup new0064 = deletedMeasures.findGroup("new0064");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0064.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Point deleted");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0064.findKeyword("MGS/688540926:0/MOC-WA/RED"),
                      "Reference ignored");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0064.findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Reference ignored");
  PvlGroup new0065 = deletedMeasures.findGroup("new0065");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0065.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0065.findKeyword("MGS/688540926:0/MOC-WA/RED"),
                      "Reference ignored");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0065.findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Reference ignored");

  new0065.deleteKeyword("MGS/688540926:0/MOC-WA/RED");
  new0065.deleteKeyword("MGS/691204200:96/MOC-WA/RED");

  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0065.findKeyword("MGS/688540926:0/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0065.findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Ignored from input");
  PvlGroup new0066 = deletedMeasures.findGroup("new0066");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0066.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0066.findKeyword("MGS/688540926:0/MOC-WA/RED"),
                      "Reference ignored");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0066.findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Reference ignored");

  new0066.deleteKeyword("MGS/688540926:0/MOC-WA/RED");
  new0066.deleteKeyword("MGS/691204200:96/MOC-WA/RED");

  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0066.findKeyword("MGS/688540926:0/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0066.findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Ignored from input");
  PvlGroup new0067 = deletedMeasures.findGroup("new0067");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0067.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0067.findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Reference ignored");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0067.findKeyword("MGS/688540926:0/MOC-WA/RED"),
                      "Ignored from input");

  new0067.deleteKeyword("MGS/691204200:96/MOC-WA/RED");

  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0067.findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Ignored from input");
  PvlGroup new0068 = deletedMeasures.findGroup("new0068");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0068.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0068.findKeyword("MGS/688540926:0/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0068.findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Ignored from input");

  // check output ControlNetwork
  ControlNet outNet(tempDir.path() + "/out.net");
  EXPECT_EQ(outNet.GetNumPoints(), 93);
  EXPECT_EQ(outNet.GetNumValidPoints(), 93);
  EXPECT_EQ(outNet.GetNumEditLockPoints(), 0);
  EXPECT_EQ(outNet.GetNumMeasures(), 226);
  EXPECT_EQ(outNet.GetNumIgnoredMeasures(), 0);
  EXPECT_EQ(outNet.GetNumEditLockMeasures(), 0);
}

/**
   * CneteditNoDelete
   * 
   * Cnetedit test with DELETE=NO.
   * Input ...
   *   1) ControlNet with 108 points (data/cnetedit/cnet_108pts.pvl)
   *   2) one image cube list file   (data/cnetedit/e0902065.cal.sub.cub)
   *   3) point list file
   *   4) delete=no
   * 
   * Output ...
   *   1) edited ControlNet
   *   2) Pvl log file.
   */
TEST_F(Cnetedit, CneteditNoDelete) {
  QTemporaryDir tempDir;

  QVector<QString> args = {"cnet=" + cnet108File,
                           "cubelist=" + cubeListFile,
                           "log=" + tempDir.path() + "/log.txt",
                           "pointlist=" + pointListFile,
                           "delete=no",
                           "onet=" + tempDir.path() + "/out.net",
                           };

  UserInterface ui(APP_XML, args);

  Pvl cneteditlog;

  try {
    cneteditlog = cnetedit(ui);
  }
  catch(IException &e) {
    FAIL() <<  e.toString().c_str() << std::endl;
  }

    // read back log file
  Pvl log;
  try {
    log.read(tempDir.path().toStdString() + "/log.txt");
  }
  catch (IException &e) {
    FAIL() << "Unable to open log file: " << e.what() << std::endl;
  }

  // check number of deleted points and measures
  EXPECT_EQ((int)log.findKeyword("PointsDeleted"), 0);
  EXPECT_EQ((int)log.findKeyword("MeasuresDeleted"), 0);

  // no edit locked points or measures
  EXPECT_EQ(log.findObject("EditLocked").findObject("Points").keywords(), 0);
  EXPECT_EQ(log.findObject("EditLocked").findObject("Measures").keywords(), 0);

  // no retained references
  EXPECT_EQ(log.findObject("RetainedReferences").keywords(), 0);

  // check ignored points
  PvlObject ignoredPoints = log.findObject("Ignored").findObject("Points");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredPoints.findKeyword("new0001"),
                      "Point ID in POINTLIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredPoints.findKeyword("new0007"),
                      "Point ID in POINTLIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredPoints.findKeyword("new0008"),
                      "Point ID in POINTLIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredPoints.findKeyword("new0020"),
                      "Point ID in POINTLIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredPoints.findKeyword("new0031"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredPoints.findKeyword("new0032"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredPoints.findKeyword("new0036"),
                      "Point ID in POINTLIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredPoints.findKeyword("new0050"),
                      "Point ID in POINTLIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredPoints.findKeyword("new0064"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredPoints.findKeyword("new0065"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredPoints.findKeyword("new0068"),
                      "Ignored from input");

  // check ignored measures
  PvlObject ignoredMeasures = log.findObject("Ignored").findObject("Measures");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0001").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0002").
                      findKeyword("MGS/688540926:0/MOC-WA/RED"),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0038").
                      findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Ignored from input");
  PvlGroup new0039 = ignoredMeasures.findGroup("new0039");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0039.findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0039.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0064").
                      findKeyword("MGS/688540926:0/MOC-WA/RED"),
                      "Serial Number in CUBELIST");
  PvlGroup new0065 = ignoredMeasures.findGroup("new0065");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0065.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0065.findKeyword("MGS/688540926:0/MOC-WA/RED"),
                      "Serial Number in CUBELIST");
  PvlGroup new0066 = ignoredMeasures.findGroup("new0066");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0066.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0066.findKeyword("MGS/688540926:0/MOC-WA/RED"),
                      "Serial Number in CUBELIST");
  PvlGroup new0067 = ignoredMeasures.findGroup("new0067");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0067.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0067.findKeyword("MGS/688540926:0/MOC-WA/RED"),
                      "Ignored from input");
  PvlGroup new0068 = ignoredMeasures.findGroup("new0068");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0068.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0068.findKeyword("MGS/688540926:0/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0068.findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0069").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0070").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0071").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0072").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0073").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0074").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0075").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0076").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0077").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0078").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0079").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0080").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0081").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0082").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0083").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0084").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0085").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0086").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0087").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0088").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0089").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0090").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0091").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0092").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0093").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0094").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0095").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0096").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0097").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0098").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0099").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0100").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0101").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0102").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0103").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0104").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0105").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0106").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0107").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("new0108").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");

  // check output ControlNetwork
  ControlNet outNet(tempDir.path() + "/out.net");
  EXPECT_EQ(outNet.GetNumPoints(), 108);
  EXPECT_EQ(outNet.GetNumValidPoints(), 97);
  EXPECT_EQ(outNet.GetNumEditLockPoints(), 0);
  EXPECT_EQ(outNet.GetNumMeasures(), 261);
  EXPECT_EQ(outNet.GetNumIgnoredMeasures(), 55);
  EXPECT_EQ(outNet.GetNumEditLockMeasures(), 0);
}

/**
   * CneteditPreservePoints
   * 
   * Cnetedit test with PRESERVE=YES.
   * Input ...
   *   1) ControlNet with 108 points (data/cnetedit/cnet_108pts.pvl)
   *   2) one image cube list file   (data/cnetedit/e0902065.cal.sub.cub)
   *   3) point list file
   *   4) preserve=yes
   * 
   * Output ...
   *   1) edited ControlNet
   *   2) Pvl log file.
   */
TEST_F(Cnetedit, CneteditPreservePoints) {
  QTemporaryDir tempDir;

  QVector<QString> args = {"cnet=" + cnet108File,
                           "cubelist=" + cubeListFile,
                           "log=" + tempDir.path() + "/log.txt",
                           "pointlist=" + pointListFile,
                           "preserve=yes",
                           "onet=" + tempDir.path() + "/out.net",
                           };

  UserInterface ui(APP_XML, args);

  Pvl cneteditlog;

  try {
    cneteditlog = cnetedit(ui);
  }
  catch(IException &e) {
    FAIL() <<  e.toString().c_str() << std::endl;
  }

    // read back log file
  Pvl log;
  try {
    log.read(tempDir.path().toStdString() + "/log.txt");
  }
  catch (IException &e) {
    FAIL() << "Unable to open log file: " << e.what() << std::endl;
  }

  // check number of deleted points and measures
  EXPECT_EQ((int)log.findKeyword("PointsDeleted"), 12);
  EXPECT_EQ((int)log.findKeyword("MeasuresDeleted"), 73);

  // no edit locked points or measures
  EXPECT_EQ(log.findObject("EditLocked").findObject("Points").keywords(), 0);
  EXPECT_EQ(log.findObject("EditLocked").findObject("Measures").keywords(), 0);

  // no retained references
  EXPECT_EQ(log.findObject("RetainedReferences").keywords(), 0);

  // check deleted points
  PvlObject deletedPoints = log.findObject("Deleted").findObject("Points");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0001"),
                      "Point ID in POINTLIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0007"),
                      "Point ID in POINTLIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0008"),
                      "Point ID in POINTLIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0020"),
                      "Point ID in POINTLIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0031"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0032"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0036"),
                      "Point ID in POINTLIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0039"),
                      "Too few measures");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0050"),
                      "Point ID in POINTLIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0064"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0065"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("new0068"),
                      "Ignored from input");

  // check deleted measures
  PvlObject deletedMeasures = log.findObject("Deleted").findObject("Measures");
  PvlGroup new0001 = deletedMeasures.findGroup("new0001");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0001.findKeyword("MGS/688540926:0/MOC-WA/RED"),
                      "Point deleted");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0001.findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Point deleted");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0002").
                      findKeyword("MGS/688540926:0/MOC-WA/RED"),
                      "Serial Number in CUBELIST");
  PvlGroup new0007 = deletedMeasures.findGroup("new0007");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0007.findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Point deleted");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0007.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Point deleted");
  PvlGroup new0008 = deletedMeasures.findGroup("new0008");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0008.findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Point deleted");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0008.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Point deleted");
  PvlGroup new0020 = deletedMeasures.findGroup("new0020");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0020.findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Point deleted");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0020.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Point deleted");
  PvlGroup new0031 = deletedMeasures.findGroup("new0031");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0031.findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Point deleted");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0031.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Point deleted");
  PvlGroup new0032 = deletedMeasures.findGroup("new0032");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0032.findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Point deleted");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0032.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Point deleted");
  PvlGroup new0036 = deletedMeasures.findGroup("new0036");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0036.findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Point deleted");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0036.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Point deleted");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0038").
                      findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Ignored from input");
  PvlGroup new0039 = deletedMeasures.findGroup("new0039");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0039.findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Ignored from input");                      
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0039.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Ignored from input");
  PvlGroup new0050 = deletedMeasures.findGroup("new0050");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0050.findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Point deleted");                      
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0050.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Point deleted");
  PvlGroup new0064 = deletedMeasures.findGroup("new0064");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0064.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Point deleted");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0064.findKeyword("MGS/688540926:0/MOC-WA/RED"),
                      "Point deleted");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0064.findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Point deleted");                      
  PvlGroup new0065 = deletedMeasures.findGroup("new0065");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0065.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0065.findKeyword("MGS/688540926:0/MOC-WA/RED"),
                      "Point deleted");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0065.findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Point deleted");                      
  PvlGroup new0066 = deletedMeasures.findGroup("new0066");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0066.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0066.findKeyword("MGS/688540926:0/MOC-WA/RED"),
                      "Serial Number in CUBELIST");
  PvlGroup new0067 = deletedMeasures.findGroup("new0067");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0067.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0067.findKeyword("MGS/688540926:0/MOC-WA/RED"),
                      "Ignored from input");
  PvlGroup new0068 = deletedMeasures.findGroup("new0068");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0068.findKeyword("MGS/718369703:160/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0068.findKeyword("MGS/688540926:0/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      new0068.findKeyword("MGS/691204200:96/MOC-WA/RED"),
                      "Ignored from input");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0069").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0070").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0071").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0072").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0073").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0074").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0075").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0076").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0077").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0078").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0079").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0080").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0081").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0082").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0083").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0084").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0085").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0086").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0087").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0088").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0089").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0090").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0091").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0092").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0093").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0094").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0095").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0096").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0097").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0098").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0099").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0100").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0101").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0102").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0103").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0104").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0105").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0106").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0107").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedMeasures.findGroup("new0108").
                      findKeyword("MGS/688540926:0/MOC-WA/RED "),
                      "Serial Number in CUBELIST");

  // check output ControlNetwork
  ControlNet outNet(tempDir.path() + "/out.net");
  EXPECT_EQ(outNet.GetNumPoints(), 96);
  EXPECT_EQ(outNet.GetNumValidPoints(), 96);
  EXPECT_EQ(outNet.GetNumEditLockPoints(), 0);
  EXPECT_EQ(outNet.GetNumMeasures(), 188);
  EXPECT_EQ(outNet.GetNumIgnoredMeasures(), 0);
  EXPECT_EQ(outNet.GetNumEditLockMeasures(), 0);
}


/**
   * CneteditMeasureListGeneral
   * General cnetedit test with bad measures list.
   * Input ...
   *   1) ControlNet with 35 points (data/cnetedit/cnet_35pts.pvl)
   *   2) list file of bad measures
   *   3) delete=no
   * 
   * Output ...
   *   1) edited ControlNet
   *   2) Pvl log file.
   */
TEST_F(CneteditMeasureList, CneteditMeasureListGeneral) {
  QTemporaryDir tempDir;

  QVector<QString> args = {"cnet=" + cnet35File,
                           "log=" + tempDir.path() + "/log.txt",
                           "measurelist=" + badMeasureListFile1,
                           "delete=no",
                           "onet=" + tempDir.path() + "/out.net",
                           };

  UserInterface ui(APP_XML, args);

  Pvl cneteditlog;

  try {
    cneteditlog = cnetedit(ui);
  }
  catch(IException &e) {
    FAIL() <<  e.toString().c_str() << std::endl;
  }

    // read back log file
  Pvl log;
  try {
    log.read(tempDir.path().toStdString() + "/log.txt");
  }
  catch (IException &e) {
    FAIL() << "Unable to open log file: " << e.what() << std::endl;
  }

  // check number of deleted points and measures
  EXPECT_EQ((int)log.findKeyword("PointsDeleted"), 0);
  EXPECT_EQ((int)log.findKeyword("MeasuresDeleted"), 0);

  // no edit locked points or measures
  EXPECT_EQ(log.findObject("EditLocked").findObject("Points").keywords(), 0);
  EXPECT_EQ(log.findObject("EditLocked").findObject("Measures").keywords(), 0);

  // no retained references
  EXPECT_EQ(log.findObject("RetainedReferences").keywords(), 0);

  // no ignored points
  EXPECT_EQ(log.findObject("Ignored").findObject("Points").keywords(), 0);

  // check ignored measures
  PvlObject ignoredMeasures = log.findObject("Ignored").findObject("Measures");
  PvlGroup bndry_32 = ignoredMeasures.findGroup("I24827003RDR_bndry_32");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      bndry_32.findKeyword("Odyssey/THEMIS_IR/766864399.204"),
                      "Measure in MEASURELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      bndry_32.findKeyword("Odyssey/THEMIS_IR/764644820.000"),
                      "Measure in MEASURELIST");
  PvlGroup bndry_35 = ignoredMeasures.findGroup("I24827003RDR_bndry_35");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      bndry_35.findKeyword("Odyssey/THEMIS_IR/860700556.051"),
                      "Measure in MEASURELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      bndry_35.findKeyword("Odyssey/THEMIS_IR/748799078.000"),
                      "Measure in MEASURELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      bndry_35.findKeyword("Odyssey/THEMIS_IR/869400711.102"),
                      "Measure in MEASURELIST");
  PvlGroup bndry_36 = ignoredMeasures.findGroup("I24827003RDR_bndry_36");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      bndry_36.findKeyword("Odyssey/THEMIS_IR/748799078.000"),
                      "Measure in MEASURELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      bndry_36.findKeyword("Odyssey/THEMIS_IR/869400711.102"),
                      "Measure in MEASURELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("I24827003RDR_bndry_50").
                      findKeyword("Odyssey/THEMIS_IR/869400711.102"),
                      "Measure in MEASURELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("I24827003RDR_bndry_53").
                      findKeyword("Odyssey/THEMIS_IR/869400711.102"),
                      "Measure in MEASURELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("I24827003RDR_bndry_56").
                      findKeyword("Odyssey/THEMIS_IR/869400711.102"),
                      "Measure in MEASURELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("I24827003RDR_bndry_8").
                      findKeyword("Odyssey/THEMIS_IR/748799078.000"),
                      "Measure in MEASURELIST");

  // check output ControlNetwork
  ControlNet outNet(tempDir.path() + "/out.net");
  EXPECT_EQ(outNet.GetNumPoints(), 35);
  EXPECT_EQ(outNet.GetNumValidPoints(), 35);
  EXPECT_EQ(outNet.GetNumEditLockPoints(), 0);
  EXPECT_EQ(outNet.GetNumMeasures(), 91);
  EXPECT_EQ(outNet.GetNumIgnoredMeasures(), 11);
  EXPECT_EQ(outNet.GetNumEditLockMeasures(), 0);
}

/**
   * CneteditMeasureListIgnoreAll
   * 
   * Cnetedit test with bad measures list and IGNOREALL=YES; DELETE=NO.
   * Input ...
   *   1) ControlNet with 35 points (data/cnetedit/cnet_35pts.pvl)
   *   2) bad measure list
   *   3) ignoreall=yes
   *   4) delete=no
   * 
   * Output ...
   *   1) edited ControlNet
   *   2) Pvl log file.
   */
TEST_F(CneteditMeasureList, CneteditMeasureListIgnoreAll) {
  QTemporaryDir tempDir;

  QVector<QString> args = {"cnet=" + cnet35File,
                           "log=" + tempDir.path() + "/log.txt",
                           "measurelist=" + badMeasureListFile2,
                           "ignoreall=yes",
                           "delete=no",
                           "onet=" + tempDir.path() + "/out.net",
                           };

  UserInterface ui(APP_XML, args);

  Pvl cneteditlog;

  try {
    cneteditlog = cnetedit(ui);
  }
  catch(IException &e) {
    FAIL() <<  e.toString().c_str() << std::endl;
  }

    // read back log file
  Pvl log;
  try {
    log.read(tempDir.path().toStdString() + "/log.txt");
  }
  catch (IException &e) {
    FAIL() << "Unable to open log file: " << e.what() << std::endl;
  }

  // check number of deleted points and measures
  EXPECT_EQ((int)log.findKeyword("PointsDeleted"), 0);
  EXPECT_EQ((int)log.findKeyword("MeasuresDeleted"), 0);

  // no edit locked points or measures
  EXPECT_EQ(log.findObject("EditLocked").findObject("Points").keywords(), 0);
  EXPECT_EQ(log.findObject("EditLocked").findObject("Measures").keywords(), 0);

  // no retained references
  EXPECT_EQ(log.findObject("RetainedReferences").keywords(), 0);

  // no ignored points
  EXPECT_EQ(log.findObject("Ignored").findObject("Points").keywords(), 1);

  // check ignored measures
  PvlObject ignoredMeasures = log.findObject("Ignored").findObject("Measures");
  PvlGroup bndry_32 = ignoredMeasures.findGroup("I24827003RDR_bndry_32");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      bndry_32.findKeyword("Odyssey/THEMIS_IR/766864399.204"),
                      "Measure in MEASURELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      bndry_32.findKeyword("Odyssey/THEMIS_IR/764644820.000"),
                      "Measure in MEASURELIST");
  PvlGroup bndry_35 = ignoredMeasures.findGroup("I24827003RDR_bndry_35");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      bndry_35.findKeyword("Odyssey/THEMIS_IR/860700556.051"),
                      "Measure in MEASURELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      bndry_35.findKeyword("Odyssey/THEMIS_IR/748799078.000"),
                      "Measure in MEASURELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      bndry_35.findKeyword("Odyssey/THEMIS_IR/869400711.102"),
                      "Measure in MEASURELIST");
  PvlGroup bndry_36 = ignoredMeasures.findGroup("I24827003RDR_bndry_36");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      bndry_36.findKeyword("Odyssey/THEMIS_IR/748799078.000"),
                      "Measure in MEASURELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      bndry_36.findKeyword("Odyssey/THEMIS_IR/869400711.102"),
                      "Measure in MEASURELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("I24827003RDR_bndry_50").
                      findKeyword("Odyssey/THEMIS_IR/869400711.102"),
                      "Measure in MEASURELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("I24827003RDR_bndry_53").
                      findKeyword("Odyssey/THEMIS_IR/869400711.102"),
                      "Measure in MEASURELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("I24827003RDR_bndry_56").
                      findKeyword("Odyssey/THEMIS_IR/869400711.102"),
                      "Measure in MEASURELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, ignoredMeasures.findGroup("I24827003RDR_bndry_8").
                      findKeyword("Odyssey/THEMIS_IR/748799078.000"),
                      "Measure in MEASURELIST");

  // check output ControlNetwork
  ControlNet outNet(tempDir.path() + "/out.net");
  EXPECT_EQ(outNet.GetNumPoints(), 35);
  EXPECT_EQ(outNet.GetNumValidPoints(), 34);
  EXPECT_EQ(outNet.GetNumEditLockPoints(), 0);
  EXPECT_EQ(outNet.GetNumMeasures(), 91);
  EXPECT_EQ(outNet.GetNumIgnoredMeasures(), 13);
  EXPECT_EQ(outNet.GetNumEditLockMeasures(), 0);
}


/**
   * CneteditMeasureListDelete
   * 
   * Cnetedit test with bad measures list and delete=yes.
   * Input ...
   *   1) ControlNet with 35 points (data/cnetedit/cnet_35pts.pvl)
   *   2) bad measure list
   *   4) delete=yes (default)
   * 
   * Output ...
   *   1) edited ControlNet
   *   2) Pvl log file.
   */
TEST_F(CneteditMeasureList, CneteditMeasureListDelete) {
  QTemporaryDir tempDir;

  QVector<QString> args = {"cnet=" + cnet35File,
                           "log=" + tempDir.path() + "/log.txt",
                           "measurelist=" + badMeasureListFile1,
                           "onet=" + tempDir.path() + "/out.net",
                           };

  UserInterface ui(APP_XML, args);

  Pvl cneteditlog;

  try {
    cneteditlog = cnetedit(ui);
  }
  catch(IException &e) {
    FAIL() <<  e.toString().c_str() << std::endl;
  }

    // read back log file
  Pvl log;
  try {
    log.read(tempDir.path().toStdString() + "/log.txt");
  }
  catch (IException &e) {
    FAIL() << "Unable to open log file: " << e.what() << std::endl;
  }

  // check number of deleted points and measures
  EXPECT_EQ((int)log.findKeyword("PointsDeleted"), 5);
  EXPECT_EQ((int)log.findKeyword("MeasuresDeleted"),16);

  // no edit locked points or measures
  EXPECT_EQ(log.findObject("EditLocked").findObject("Points").keywords(), 0);
  EXPECT_EQ(log.findObject("EditLocked").findObject("Measures").keywords(), 0);

  // no retained references
  EXPECT_EQ(log.findObject("RetainedReferences").keywords(), 0);

  // check deleted points
  PvlObject deletedPoints = log.findObject("Deleted").findObject("Points");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("I24827003RDR_bndry_32"),
                      "Too few measures");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("I24827003RDR_bndry_50"),
                      "Too few measures");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("I24827003RDR_bndry_53"),
                      "Too few measures");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("I24827003RDR_bndry_56"),
                      "Too few measures");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, deletedPoints.findKeyword("I24827003RDR_bndry_8"),
                      "Too few measures");

  // check deleted measures
  PvlObject deletedMeasures = log.findObject("Deleted").findObject("Measures");
  PvlGroup bndry_32 = deletedMeasures.findGroup("I24827003RDR_bndry_32");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      bndry_32.findKeyword("Odyssey/THEMIS_IR/766864399.204"),
                      "Measure in MEASURELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      bndry_32.findKeyword("Odyssey/THEMIS_IR/764644820.000"),
                      "Measure in MEASURELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      bndry_32.findKeyword("Odyssey/THEMIS_IR/860700556.051"),
                      "Point deleted");
  PvlGroup bndry_35 = deletedMeasures.findGroup("I24827003RDR_bndry_35");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      bndry_35.findKeyword("Odyssey/THEMIS_IR/860700556.051"),
                      "Measure in MEASURELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      bndry_35.findKeyword("Odyssey/THEMIS_IR/748799078.000"),
                      "Measure in MEASURELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      bndry_35.findKeyword("Odyssey/THEMIS_IR/869400711.102"),
                      "Measure in MEASURELIST");
  PvlGroup bndry_36 = deletedMeasures.findGroup("I24827003RDR_bndry_36");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      bndry_36.findKeyword("Odyssey/THEMIS_IR/748799078.000"),
                      "Measure in MEASURELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      bndry_36.findKeyword("Odyssey/THEMIS_IR/869400711.102"),
                      "Measure in MEASURELIST");
  PvlGroup bndry_50 = deletedMeasures.findGroup("I24827003RDR_bndry_50");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      bndry_50.findKeyword("Odyssey/THEMIS_IR/869400711.102"),
                      "Measure in MEASURELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      bndry_50.findKeyword("Odyssey/THEMIS_IR/823680993.230"),
                      "Point deleted");
  PvlGroup bndry_53 = deletedMeasures.findGroup("I24827003RDR_bndry_53");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      bndry_53.findKeyword("Odyssey/THEMIS_IR/869400711.102"),
                      "Measure in MEASURELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      bndry_53.findKeyword("Odyssey/THEMIS_IR/823680993.230"),
                      "Point deleted");
  PvlGroup bndry_56 = deletedMeasures.findGroup("I24827003RDR_bndry_56");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      bndry_56.findKeyword("Odyssey/THEMIS_IR/869400711.102"),
                      "Measure in MEASURELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      bndry_56.findKeyword("Odyssey/THEMIS_IR/823680993.230"),
                      "Point deleted");
  PvlGroup bndry_8 = deletedMeasures.findGroup("I24827003RDR_bndry_8");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      bndry_8.findKeyword("Odyssey/THEMIS_IR/748799078.000"),
                      "Measure in MEASURELIST");
  EXPECT_PRED_FORMAT2(AssertStringsEqual,
                      bndry_8.findKeyword("Odyssey/THEMIS_IR/760206015.230"),
                      "Point deleted");

  // check output ControlNetwork
  ControlNet outNet(tempDir.path() + "/out.net");
  EXPECT_EQ(outNet.GetNumPoints(), 30);
  EXPECT_EQ(outNet.GetNumValidPoints(), 30);
  EXPECT_EQ(outNet.GetNumEditLockPoints(), 0);
  EXPECT_EQ(outNet.GetNumMeasures(), 75);
  EXPECT_EQ(outNet.GetNumIgnoredMeasures(), 0);
  EXPECT_EQ(outNet.GetNumEditLockMeasures(), 0);
}