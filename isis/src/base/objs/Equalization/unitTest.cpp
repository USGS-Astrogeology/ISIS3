/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <iomanip>

#include <QString>
#include <QStringList>

#include "Buffer.h"
#include "Equalization.h"
#include "IException.h"
#include "OverlapNormalization.h"
#include "OverlapStatistics.h"
#include "Preference.h"
#include "ProcessByLine.h"
#include "PvlGroup.h"
#include "QRegularExpression"

using namespace std;
using namespace Isis;

void ReportError(QString err) {
  cout << err.replace(QRegularExpression("(\\/[\\w\\-\\. ]*)+\\/unitTestData"), "unitTestData") << endl;
}

class TestFunctor {
  public:
    TestFunctor(const Equalization *equalizer, int lineCount, int imageIndex) {
      m_equalizer = equalizer;
      m_imageIndex = imageIndex;
      m_lineCount = lineCount;
    }

    void operator()(Buffer &in) const {
      int lineIndex = ((in.Band() - 1) * m_lineCount + (in.Line() - 1));
      int sampleIndex = lineIndex % in.size();

      if (IsValidPixel(in[sampleIndex])) {
        int bandIndex = in.Band() - 1;

        cout << "sample " << sampleIndex + 1 << ", ";
        cout << "line " << (lineIndex + 1) << ": ";
        cout << in[sampleIndex] << " => ";

        cout << m_equalizer->evaluate(
            in[sampleIndex], m_imageIndex, bandIndex) << endl;
      }
    }

  private:
    const Equalization *m_equalizer;
    int m_imageIndex;
    int m_lineCount;
};

int main(int argc, char *argv[]) {
  Preference::Preferences(true);
  cout << setprecision(9);
  QString nonOverlapStats;      // Used in validateInputStatistics exception (1) test
  QString nonOverlapList;       // Used in validateInputStatistics exception (2) test
  QString allOverlapStats;      // Used in validateInputStatistics exception (2) test
                                // and loadOutput tests
  QStringList tmpFilesToRemove; // Keeps track of the temporary files we want removed
  PvlGroup results; // Results pvl
  FileName outputStatsFile("$TEMPORARY/results.tmp.pvl"); // Results file

  QString fromList = "FromList.lst";
  QString holdList = "HoldList.lst";
  QString toList   = "ToList.lst";
  // Create the non-overlapping list of two images
  nonOverlapList = "$TEMPORARY/nonOverlaps.tmp.lst";
  tmpFilesToRemove << nonOverlapList;

  FileList nonOverlaps(fromList);
  nonOverlaps.removeOne("$ISISTESTDATA/isis/src/odyssey/unitTestData/I01523019RDR.lev2.cub");
  nonOverlaps.write(nonOverlapList);

  // Default constructor is protected, won't be tested
//     cout << "UnitTest for Equalization" << endl;
//     cout << "Create object without fromlist file." << endl;
//     Equalization equalizerNoFromList();

  cout << "Create object using fromlist file (non-overlapping)" << endl;
  Equalization equalizer(OverlapNormalization::Both, nonOverlapList);

  double percent = 100.0;
  int mincount = 1000;
  bool weight = false;
  LeastSquares::SolveMethod solvemethod = LeastSquares::QRD;

  try {
    cout << "     Calculate statistics for contrast and brightness using QRD method..." << endl;
    equalizer.calculateStatistics(percent, mincount, weight, solvemethod);
  }
  // Non-overlaps will throw an exception
  catch (IException &e) {
    e.print();
  }

  results = equalizer.getResults();
  cout << "Results:" << endl;
  for (int i = 0; i < results["NonOverlaps"].size(); i++) {
    QString::fromStdString(results["NonOverlaps"][i]).replace(QRegularExpression("(\\/[\\w\\-\\. ]*)+\\/odyssey"), "odyssey");
  }
  cout << results << endl;

  cout << "     Write results to file..." << endl;
  tmpFilesToRemove << outputStatsFile.expanded();
  equalizer.write(outputStatsFile.expanded());
  nonOverlapStats = outputStatsFile.expanded();

  // Create an equalizer from the fixed from list
  Equalization equalizerFixed(OverlapNormalization::Both, fromList);

  cout << "     Add hold list..." << endl;
  equalizerFixed.addHolds(holdList);

  // Fix the non-overlaps and recalculate
  cout << "     Recalculating the statistics with all overlapping files now..." << endl;
  equalizerFixed.recalculateStatistics(outputStatsFile.expanded());

  results = equalizerFixed.getResults();
  cout << "Results:" << endl;
  for (int i = 0; i < results.keywords(); i++) {
    if (results[i].isNamed("FileName")) {
      results[i].setValue(QString::fromStdString(results[i]).replace(QRegularExpression("(\\/[\\w\\-\\. ]*)+\\/odyssey"), "odyssey").toStdString());
    }
  }
  cout << results << endl;

  cout << "     Write results to file..." << endl;
  FileName outputStatsFixedFile("$TEMPORARY/resultsFixed.tmp.pvl");
  tmpFilesToRemove << outputStatsFixedFile.expanded();
  equalizerFixed.write(outputStatsFixedFile.expanded());

  // Set allOverlapStats so we can use later in exceptions testing
  allOverlapStats = outputStatsFixedFile.expanded();

  // Open input cube
  FileList imageList(fromList);
  for (int i = 0; i < imageList.size(); i++) {
    ProcessByLine p;
    CubeAttributeInput att;
    const QString inp = imageList[i].toString();
    Cube *inputCube = p.SetInputCube(inp, att);
    TestFunctor func(&equalizerFixed, inputCube->lineCount(), i);
    p.ProcessCubeInPlace(func, false);
  }

  cout << "     Apply corrections to temporary output cubes..." << endl;
  equalizerFixed.applyCorrection(toList);

  cout << "     Import statistics from results file..." << endl;
  equalizerFixed.importStatistics(outputStatsFixedFile.expanded());
  for (int i = 0; i < imageList.size(); i++) {
    ProcessByLine p;
    CubeAttributeInput att;
    const QString inp = imageList[i].toString();
    Cube *inputCube = p.SetInputCube(inp, att);
    TestFunctor func(&equalizerFixed, inputCube->lineCount(), i);
    p.ProcessCubeInPlace(func, false);
  }

  FileList toFileList(toList);
  for (int i = 0; i < toFileList.size(); i++) {
    remove(toFileList[i].expanded().toLatin1());
  }


  // Test exceptions
  // Testing addHolds exceptions
  try {
    cout << endl << "Testing addHolds exception (1)..." << endl;
    QString tmpList = "$TEMPORARY/tooManyHoldList.tmp.lst";
    tmpFilesToRemove << tmpList;
    FileList holds("FromList.lst");
    holds.append("tooManyCubes.cub");
    holds.write(tmpList);
    Equalization equalizer(OverlapNormalization::Both, fromList);
    equalizer.addHolds(tmpList);
  }
  catch (IException &e) {
    e.print();
  }

  try {
    cout << endl << "Testing addHolds exception (2)..." << endl;
    QString tmpList = "$TEMPORARY/holdFileMissing.tmp.lst";
    tmpFilesToRemove << tmpList;
    FileList holds("HoldList.lst");
    holds.append("fileDNE.cub");
    holds.write(tmpList);
    Equalization equalizer(OverlapNormalization::Both, fromList);
    equalizer.addHolds(tmpList);
  }
  catch (IException &e) {
    e.print();
  }

  // Testing applyCorrection exception
  try {
    cout << endl << "Testing applyCorrection exception ..." << endl;
    Equalization equalizer(OverlapNormalization::Both, fromList);
    equalizer.applyCorrection("");
  }
  catch (IException &e) {
    e.print();
  }

  // Testing loadInputs exception
  try {
    cout << endl << "Testing loadInputs exception ..." << endl;
    QString oneInput = "HoldList.lst"; // There is only one file, so this will work
    Equalization equalizer(OverlapNormalization::Both, oneInput);
  }
  catch (IException &e) {
    e.print();
  }

  // Testing errorCheck exceptions
  try {
    cout << endl << "Testing errorCheck exception (1)..." << endl;
    QString tmpList = "$TEMPORARY/nonMatchingBands.tmp.lst";
    tmpFilesToRemove << tmpList;
    FileList inputs(fromList);
    inputs.append("$ISISTESTDATA/isis/src/base/unitTestData/isisTruth.cub");
    inputs.write(tmpList);
    Equalization equalizer(OverlapNormalization::Both, tmpList);
  }
  catch (IException &e) {
    ReportError(e.toString());
  }

  try {
    cout << endl << "Testing errorCheck exception (2)..." << endl;
    QString tmpList = "$TEMPORARY/nonMatchingMap.tmp.lst";
    tmpFilesToRemove << tmpList;
    FileList inputs(fromList);
    inputs.append("$ISISTESTDATA/isis/src/odyssey/unitTestData/I56632006EDR.lev2.cub");
    inputs.write(tmpList);
    Equalization equalizer(OverlapNormalization::Both, tmpList);
  }
  catch (IException &e) {
    ReportError(e.toString());
  }

  // Testing loadOutputs exceptions TODO

  // Testing validateInputStatistics exceptions
  try {
    cout << endl << "Testing validateInputStatistics exception (1)..." << endl;
    Equalization equalizer(OverlapNormalization::Both, fromList);
    equalizer.importStatistics(nonOverlapStats);
  }
  catch (IException &e) {
    ReportError(e.toString());
  }

  try {
    cout << endl << "Testing validateInputStatistics exception (2)..." << endl;
    Equalization equalizer(OverlapNormalization::Both, nonOverlapList);
    equalizer.importStatistics(allOverlapStats);
  }
  catch (IException &e) {
    ReportError(e.toString());
  }

  // Remove all the temporary files
  foreach (FileName file, tmpFilesToRemove) {
    remove(file.expanded().toLatin1().data());
  }

}
