/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "ControlNetVersioner.h"

#include <QString>
#include <QTime>

#include "ControlNet.h"
#include "FileName.h"
#include "IException.h"
#include "IString.h"
#include "Preference.h"
#include "Progress.h"
#include "Pvl.h"

using namespace std;
using namespace Isis;

void TestNetwork(const QString &filename, Progress *progress, bool printNetwork = true, bool pvlInput = false);

/**
 * Unit test for ControlNetVersioner class
 *
 * @author ????-??-?? Unknown
 *
 *  @internal
 *   @history 2018-06-06 Jeannie Backer - Removed file paths from error message written to
 *                           test output.
 *
 */
int main(int argc, char *argv[]) {
  Preference::Preferences(true);
  Progress *testProgress = new Progress();
  cout << "Test ControlNetVersioner";

  // No target
  TestNetwork("$ISISTESTDATA/isis/src/control/unitTestData/unitTest_ControlNetVersioner_PvlNetwork2_PvlV0001.net",
              testProgress, false); // no print network here because the datetimes will change

  // Really odd keywords with target
  TestNetwork("$ISISTESTDATA/isis/src/control/unitTestData/unitTest_ControlNetVersioner_PvlNetwork3_PvlV0001.net",
              testProgress);

  // Another set of odd keywords
  TestNetwork("$ISISTESTDATA/isis/src/control/unitTestData/unitTest_ControlNetVersioner_PvlNetwork1_PvlV0001.net",
              testProgress);

  // Binary V1
  TestNetwork("$ISISTESTDATA/isis/src/control/unitTestData/unitTest_ControlNetVersioner_ProtoNetwork1_ProtoV0001.net",
              testProgress);

  // Corrupted (based off of oldNetwork2.net)
  TestNetwork("$ISISTESTDATA/isis/src/control/unitTestData/unitTest_ControlNetVersioner_BadNetwork_ProtoV0001.net",
              testProgress);

  // Binary V2
  TestNetwork("$ISISTESTDATA/isis/src/control/unitTestData/unitTest_ControlNetVersioner_ProtoNetwork2_ProtoV0002.net",
              testProgress,
              false);

  // Network with rejected jigsaw points
  TestNetwork("$ISISTESTDATA/isis/src/control/unitTestData/unitTest_ControlNetVersioner_PvlNetwork8_PvlV0005.pvl",
              testProgress,
              true,
              true);

  // Network full of weird test cases (based on PvlNetwork4)
  TestNetwork("$ISISTESTDATA/isis/src/control/unitTestData/unitTest_ControlNetVersioner_PvlNetwork5_PvlV0003.pvl",
              testProgress,
              false,
              false);

  // Test Network 1 created for code coverage.
  TestNetwork("$ISISTESTDATA/isis/src/control/unitTestData/PvlNet_TestNetwork1_V2.net",
              testProgress,
              false,
              false);

  // Test Network 2 created for code coverage.
  TestNetwork("$ISISTESTDATA/isis/src/control/unitTestData/PvlNet_TestNetwork2_V3.net",
              testProgress,
              false,
              false);

  // Re-test each version without progress
  TestNetwork("$ISISTESTDATA/isis/src/control/unitTestData/unitTest_ControlNetVersioner_PvlNetwork3_PvlV0001.net",
              0,
              false);
  TestNetwork("$ISISTESTDATA/isis/src/control/unitTestData/unitTest_ControlNetVersioner_ProtoNetwork1_ProtoV0001.net",
              0,
              false);
  TestNetwork("$ISISTESTDATA/isis/src/control/unitTestData/unitTest_ControlNetVersioner_ProtoNetwork2_ProtoV0002.net",
              0,
              false);
  TestNetwork("$ISISTESTDATA/isis/src/control/unitTestData/unitTest_ControlNetVersioner_PvlNetwork4_PvlV0003.pvl",
              0,
              false);

  cout << endl << "Test writing from ControlNet objects" << endl << endl;
  QString cnetv2 = "$ISISTESTDATA/isis/src/control/unitTestData/unitTest_ControlNetVersioner_ProtoNetwork2_ProtoV0002.net";
  ControlNet *binaryV2Net = new ControlNet(cnetv2,testProgress);
  ControlNetVersioner *binV2Versioner = new ControlNetVersioner(binaryV2Net);
  binV2Versioner->write("./binaryV2tmp.net");
  remove("./binaryV2tmp.net");
  delete binV2Versioner;
  binV2Versioner = NULL;

  cout << endl << "Test reading version 1 protobuf network" << endl << endl;
  QString cnetv1 = "$ISISTESTDATA/isis/src/control/unitTestData/unitTest_ControlNetVersioner_ProtoNetwork1_ProtoV0001.net";
  ControlNetVersioner *binV1Versioner = new ControlNetVersioner(FileName(cnetv1.toStdString()), testProgress);
  cout << "Take all of the control points and delete them." << endl;
  int pointsTaken = 0;
  ControlPoint *readPoint = binV1Versioner->takeFirstPoint();
  while (readPoint != NULL) {
    pointsTaken++;
    cout << "  " << pointsTaken << (pointsTaken > 1 ? " points taken" : " point taken") << endl;
    delete readPoint;
    readPoint = binV1Versioner->takeFirstPoint();
  }
  delete binV1Versioner;
  binV1Versioner = NULL;

  cout << endl << "Test reading version 5 protobuf network" << endl << endl;
  QString cnetv5 = "$ISISTESTDATA/isis/src/control/unitTestData/unitTest_ControlNetVersioner_ProtoNetwork3_ProtoV0005.net";
  ControlNetVersioner *binV5Versioner = new ControlNetVersioner(FileName(cnetv5.toStdString()), testProgress);
  delete binV5Versioner;
  binV5Versioner = NULL;

  cout << endl << "Test writing with invalid target" << endl << endl;
  try {
    binaryV2Net->SetTarget("INVALID_TARGET_NAME");
    binV2Versioner = new ControlNetVersioner(binaryV2Net);
  }
  catch (IException &e) {
    e.print();
    if (binV2Versioner) {
      delete binV2Versioner;
      binV2Versioner = NULL;
    }
  }

  cout << endl << "Test reading a random PVL file" << endl << endl;
  try {
    ControlNetVersioner invalidVersioner("$ISISROOT/appdata/templates/maps/equirectangular.map");
  }
  catch (IException &e) {
    e.print();
  }

  cout << endl << "Test reading a PVL file with missing header information" << endl << endl;
  QString badCnetName = "";
  try {
    badCnetName = "$ISISTESTDATA/isis/src/control/unitTestData/unitTest_ControlNetVersioner_PvlNetwork_BadHeaderV1.net";
    ControlNetVersioner invalidVersionerV1(badCnetName.toStdString());
  }
  catch (IException &e) {
    QString message = QString::fromStdString(e.toString());
    cout << message.replace(QRegExp("file.*control/unitTestData"), "file [control/unitTestData").toStdString();
    cout << endl;
  }
  try {
    badCnetName = "$ISISTESTDATA/isis/src/control/unitTestData/unitTest_ControlNetVersioner_PvlNetwork_BadHeaderV2.net";
    ControlNetVersioner invalidVersionerV2(badCnetName.toStdString());
  }
  catch (IException &e) {
    QString message = QString::fromStdString(e.toString());
    cout << message.replace(QRegExp("file.*control/unitTestData"), "file [control/unitTestData").toStdString();
    cout << endl;
  }
  try {
    badCnetName = "$ISISTESTDATA/isis/src/control/unitTestData/unitTest_ControlNetVersioner_PvlNetwork_BadHeaderV3.net";
    ControlNetVersioner invalidVersionerV3(badCnetName.toStdString());
  }
  catch (IException &e) {
    QString message = QString::fromStdString(e.toString());
    cout << message.replace(QRegExp("file.*control/unitTestData"), "file [control/unitTestData").toStdString();
    cout << endl;
  }
  try {
    badCnetName = "$ISISTESTDATA/isis/src/control/unitTestData/unitTest_ControlNetVersioner_PvlNetwork_BadHeaderV4.net";
    ControlNetVersioner invalidVersionerV4(badCnetName.toStdString());
  }
  catch (IException &e) {
    QString message = QString::fromStdString(e.toString());
    cout << message.replace(QRegExp("file.*control/unitTestData"), "file [control/unitTestData").toStdString();
    cout << endl;
  }
  try {
    badCnetName = "$ISISTESTDATA/isis/src/control/unitTestData/unitTest_ControlNetVersioner_PvlNetwork_BadHeaderV5.net";
    ControlNetVersioner invalidVersionerV5(badCnetName.toStdString());
  }
  catch (IException &e) {
    QString message = QString::fromStdString(e.toString());
    cout << message.replace(QRegExp("file.*control/unitTestData"), "file [control/unitTestData").toStdString();
    cout << endl;
  }

  cout << endl << "Test reading a protobuf file with a bad version number" << endl << endl;
  try {
    badCnetName = "$ISISTESTDATA/isis/src/control/unitTestData/unitTest_ControlNetVersioner_ProtoNetwork_BadVersion.net";
    ControlNetVersioner invalidVersioner(badCnetName.toStdString());
  }
  catch (IException &e) {
    e.print();
  }

  cout << endl << "Test reading a protobuf file with no version number" << endl << endl;
  try {
    badCnetName = "$ISISTESTDATA/isis/src/control/unitTestData/unitTest_ControlNetVersioner_ProtoNetwork_NoVersion.net";
    ControlNetVersioner invalidVersioner(badCnetName.toStdString());

  }
  catch (IException &e) {
    e.print();
  }

  delete binaryV2Net;
}


/**
 * Runs various test on the given network.
 *
 * @param filename Name of the control network file.
 * @param progress
 * @param printNetwork Indicates whether to print the network as
 *                     a PVL.
 * @param pvlInput Indicates whether the given network is in PVL
 *                 format.
 */
void TestNetwork(const QString &filename, Progress *progress, bool printNetwork, bool pvlInput) {
  cout << "\nReading: " << filename.toStdString() << "...\n";
  FileName networkFileName(filename.toStdString());

  ControlNetVersioner *test = NULL;
  ControlNetVersioner *test2 = NULL;

  try {

    // If we're reading in a Pvl file, this will call the Pvl update cycle, then
    //   convert to binary, then convert back to Pvl.
    // If we're reading in a binary file, this will call the binary read, then
    //   convert to Pvl, then update, then convert to binary, and back to pvl.
    //   The reason for the intermediate Pvl is described in
    //   ControlNetVersioner.h.
    cout << "\nRead network..." << endl;
    test = new ControlNetVersioner(networkFileName, progress);

    if(printNetwork) {
      cout << "Converted directly to Pvl:" << endl;
      Pvl pvlVersion(test->toPvl());

      // cout does not support this operation on a pvl
      cout << pvlVersion << endl;
      pvlVersion.write("./tmp.pvl");
    }

    // Test the latest binary read/write and Pvl conversion
    cout << "Write the network and re-read it..." << endl;
    test->write( FileName("./tmp") );
    try {
      test2 = new ControlNetVersioner( FileName("./tmp") );
    }
    catch(IException &e) {
      remove("./tmp");
      throw;
    }

    cout << "After reading and writing to a binary form does Pvl match?" << endl;

    if(printNetwork) {
      Pvl pvlVersion2(test2->toPvl());
      pvlVersion2.write("./tmp2.pvl");
      if(system("cmp ./tmp.pvl ./tmp2.pvl")) {
        cout << "Reading/Writing results in Pvl differences!" << endl;
      }
      else {
        cout << "Conversion to Pvl stays consistent" << endl;
      }
    }

    test2->write(FileName("./tmp2"));
    if(system("cmp ./tmp ./tmp2")) {
      cout << "Reading/Writing control network results in binary differences!" << endl;
    }
    else {
      cout << "Reading/Writing control network is consistent" << endl;
    }

    if (pvlInput) {

      ControlNetVersioner *cNet2 = NULL;

      cout << "Check conversions between the binary format and the pvl format." << endl;
      /*
       * When the input is a pvl, ./tmp is the binary form of the initial input. (pvl1->bin1)
       * Furthermore, ./tmp.pvl is the first binary conversion reverted back to pvl.
       * (pvl1->bin1->pvl2)
       * cNet1 is the binary version of the second pvl. (pvl1->bin1->pvl2->bin2)
       *
       *                                  a       b       c
       *                            (pvl1 -> bin1 -> pvl2 -> bin2)
       *
       * if (pvl1 != pvl2)
       *        a or b is broken but we don't know which yet
       *        if(bin1 != bin2)
       *                bin->pvl is broken (b) because the error happened after bin1 was created.
       *        else
       *                pvl-bin is broken (a/c) because the error happened before bin1 was created
       *                        and was propagated to bin2 via c.
       * else
       *        The conversions are up to date and correct because neither a nor b broke.
       *
       *
       */
      cNet2 = new ControlNetVersioner(FileName("./tmp.pvl"));

      cNet2->write( FileName("./tmpCNet2") );

      //if there are differences between the pvls.
      std::string cmd = "diff -bB --suppress-common-lines -I 'Version.*' " + networkFileName.expanded() + " ./tmp.pvl";
      if(system(cmd.c_str())) {

        //if the binary files are different.
        if(system("diff -bB --suppress-common-lines ./tmp ./tmpCNet2")){
          cout << "The conversion from binary to pvl is incorrect." << endl;
        }
        else {
          cout << "The conversion from pvl to binary is incorrect." << endl;
        }
      }
      else {
        cout << "The conversion methods for pvl->bin and bin->pvl are correct." << endl;
      }

      remove("./tmpCNet2");
      delete cNet2;
      cNet2 = NULL;
    }

    remove("./tmp");
    remove("./tmp2");

    if(printNetwork) {
      remove("./tmp.pvl");
      remove("./tmp2.pvl");
    }
  }
  catch(IException &e) {
    QStringList errors = QString::fromStdString(e.toString()).split("\n");
    errors.removeLast();
    cout << errors.join("\n").toStdString() << endl;
  }

  if(test) {
    delete test;
    test = NULL;
  }

  if(test2) {
    delete test2;
    test2 = NULL;
  }

}
