/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#define GUIHELPERS

#include "Isis.h"

#include "AutoReg.h"
#include "AutoRegFactory.h"
#include "Chip.h"
#include "ControlMeasure.h"
#include "ControlMeasureLogData.h"
#include "ControlNet.h"
#include "ControlPoint.h"
#include "Cube.h"
#include "ProgramLauncher.h"
#include "Progress.h"
#include "PvlGroup.h"
#include "SerialNumber.h"
#include "Statistics.h"
#include "UserInterface.h"
#include "IException.h"
#include "IString.h"
#include "iTime.h"

using namespace std;
using namespace Isis;

//helper button functins in the code
void helperButtonLog();

map <QString, void *> GuiHelpers() {
  map <QString, void *> helper;
  helper ["helperButtonLog"] = (void *) helperButtonLog;
  return helper;
}

void IsisMain() {
  // Get user interface
  UserInterface &ui = Application::GetUserInterface();

  // Make sure the correct parameters are entered
  if (ui.WasEntered("TO")) {
    if (ui.GetString("TRANSFORM") == "WARP") {
      if (!ui.WasEntered("ONET")) {
        std::string msg = "A Control Net file must be entered if the TO parameter is ";
        msg += "entered";
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }
  }

  // Open the first cube.  It will be matched to the second input cube.
  Cube trans;
  trans.open(ui.GetCubeName("FROM"), "r");

  // Open the second cube, it is held in place.  We will be matching the
  // first to this one by attempting to compute a sample/line translation
  Cube match;
  match.open(ui.GetCubeName("MATCH"), "r");

  // Input cube Lines and Samples must be the same and each must have only
  // one band
  if ((trans.lineCount() != match.lineCount()) ||
      (trans.sampleCount() != match.sampleCount())) {
    std::string msg = "Input Cube Lines and Samples must be equal!";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  if (trans.bandCount() != 1 || match.bandCount() != 1) {
    std::string msg = "Input Cubes must have only one band!";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  // Get serial number
  QString serialTrans = SerialNumber::Compose(trans, true);
  QString serialMatch = SerialNumber::Compose(match, true);

//  This still precludes band to band registrations.
  if (serialTrans == serialMatch) {
    std::string sTrans = FileName(trans.fileName().toStdString()).name();
    std::string sMatch = FileName(match.fileName().toStdString()).name();
    if (sTrans == sMatch) {
      std::string msg = "Cube Serial Numbers must be unique - FROM=" + serialTrans.toStdString() +
                   ", MATCH=" + serialMatch.toStdString();
      throw IException(IException::User, msg, _FILEINFO_);
    }
    serialTrans = QString::fromStdString(sTrans);
    serialMatch = QString::fromStdString(sMatch);
  }


  // We need to get a user definition of how to auto correlate around each
  // of the control points.
  Pvl regdef;
  regdef.read(ui.GetFileName("DEFFILE").toStdString());
  AutoReg *ar = AutoRegFactory::Create(regdef);

  // We want to create a grid of control points that is N rows by M columns.
  // Get row and column variables, if not entered, default to 1% of the input
  // image size
  int rows, cols;
  if (ui.WasEntered("ROWS")) {
    rows = ui.GetInteger("ROWS");
  }
  else {
    rows = (int)((trans.lineCount() - 1) / ar->SearchChip()->Lines() + 1);
  }
  if (ui.WasEntered("COLUMNS")) {
    cols = ui.GetInteger("COLUMNS");
  }
  else {
    cols = (int)((trans.sampleCount() - 1)
                 / ar->SearchChip()->Samples() + 1);
  }

  // Display the progress...10% 20% etc.
  Progress prog;
  prog.SetMaximumSteps(rows * cols);
  prog.CheckStatus();

  // Calculate spacing for the grid of points
  double lSpacing = (double)trans.lineCount() / rows;
  double sSpacing = (double)trans.sampleCount() / cols;

  // Initialize control point network and set target name (only required
  // field)
  ControlNet cn;
  cn.SetNetworkId("Coreg");
  if (match.hasGroup("Instrument")) {
    PvlGroup inst = match.group("Instrument");
    cn.SetTarget(*trans.label());
  }

  // Loop through grid of points and get statistics to compute
  // translation values
  Statistics sStats, lStats;
  for (int r = 0; r < rows; r++) {
    for (int c = 0; c < cols; c++) {
      int line = (int)(lSpacing / 2.0 + lSpacing * r + 0.5);
      int samp = (int)(sSpacing / 2.0 + sSpacing * c + 0.5);
      ar->PatternChip()->TackCube(samp, line);
      ar->PatternChip()->Load(match);
      ar->SearchChip()->TackCube(samp, line);
      ar->SearchChip()->Load(trans);

      // Set up ControlMeasure for cube to translate
      ControlMeasure * cmTrans = new ControlMeasure;
      cmTrans->SetCubeSerialNumber(serialTrans);
      cmTrans->SetCoordinate(samp, line, ControlMeasure::Candidate);
      cmTrans->SetChooserName("coreg");

      // Set up ControlMeasure for the pattern/Match cube
      ControlMeasure * cmMatch = new ControlMeasure;
      cmMatch->SetCubeSerialNumber(serialMatch);
      cmMatch->SetCoordinate(samp, line, ControlMeasure::RegisteredPixel);
      cmMatch->SetChooserName("coreg");

      ar->Register();

      // Match found
      if (ar->Success()) {
        double sDiff = samp - ar->CubeSample();
        double lDiff = line - ar->CubeLine();
        sStats.AddData(&sDiff, (unsigned int)1);
        lStats.AddData(&lDiff, (unsigned int)1);
        cmTrans->SetCoordinate(ar->CubeSample(), ar->CubeLine(),
                              ControlMeasure::RegisteredPixel);
        cmTrans->SetResidual(sDiff, lDiff);
        cmTrans->SetLogData(ControlMeasureLogData(
              ControlMeasureLogData::GoodnessOfFit,
              ar->GoodnessOfFit()));
      }

      // Add the measures to a control point
      QString str = "Row_" + QString::number(r) + "_Column_" + QString::number(c);
      ControlPoint * cp = new ControlPoint(str);
      cp->SetType(ControlPoint::Free);
      cp->Add(cmTrans);
      cp->Add(cmMatch);
      cp->SetRefMeasure(cmMatch);
      if (!cmTrans->IsMeasured()) cp->SetIgnored(true);
      cn.AddPoint(cp);
      prog.CheckStatus();
    }
  }

  // Write translation to log
  PvlGroup results("Translation");
  double sMin = (int)(sStats.Minimum() * 100.0) / 100.0;
  double sTrans = (int)(sStats.Average() * 100.0) / 100.0;
  double sMax = (int)(sStats.Maximum() * 100.0) / 100.0;
  double sDev = (int)(sStats.StandardDeviation() * 100.0) / 100.0;
  double lMin = (int)(lStats.Minimum() * 100.0) / 100.0;
  double lTrans = (int)(lStats.Average() * 100.0) / 100.0;
  double lMax = (int)(lStats.Maximum() * 100.0) / 100.0;
  double lDev = (int)(lStats.StandardDeviation() * 100.0) / 100.0;

  results += PvlKeyword("SampleMinimum", Isis::toString(sMin));
  results += PvlKeyword("SampleAverage", Isis::toString(sTrans));
  results += PvlKeyword("SampleMaximum", Isis::toString(sMax));
  results += PvlKeyword("SampleStandardDeviation", Isis::toString(sDev));
  results += PvlKeyword("LineMinimum", Isis::toString(lMin));
  results += PvlKeyword("LineAverage", Isis::toString(lTrans));
  results += PvlKeyword("LineMaximum", Isis::toString(lMax));
  results += PvlKeyword("LineStandardDeviation", Isis::toString(lDev));
  Application::Log(results);

  Pvl arPvl = ar->RegistrationStatistics();

  for (int i = 0; i < arPvl.groups(); i++) {
    Application::Log(arPvl.group(i));
  }

  // add the auto registration information to print.prt
  PvlGroup autoRegTemplate = ar->RegTemplate();
  Application::Log(autoRegTemplate);

  // If none of the points registered, throw an error
  if (sStats.TotalPixels() < 1) {
    std::string msg = "Coreg was unable to register any points. Check your algorithm definition.";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  // Don't need the cubes opened anymore
  trans.close();
  match.close();

  // If a cnet file was entered, write the ControlNet pvl to the file
  if (ui.WasEntered("ONET")) {
    cn.Write(ui.GetFileName("ONET"));
  }

  // If flatfile was entered, create the flatfile
  // The flatfile is comma seperated and can be imported into an excel
  // spreadsheet
  if (ui.WasEntered("FLATFILE")) {
    std::string fFile = FileName(ui.GetFileName("FLATFILE").toStdString()).expanded();
    ofstream os;
    os.open(fFile.c_str(), ios::out);
    os << "Sample,Line,TranslatedSample,TranslatedLine," <<
       "SampleDifference,LineDifference,GoodnessOfFit" << endl;
    for (int i = 0; i < cn.GetNumPoints(); i++) {
      const ControlPoint * cp = cn[i];
      if (cp->IsIgnored()) continue;
      const ControlMeasure * cmTrans = cp->GetMeasure(0);
      const ControlMeasure * cmMatch = cp->GetMeasure(1);

      double goodnessOfFit = cmTrans->GetLogData(
          ControlMeasureLogData::GoodnessOfFit).GetNumericalValue();

      os << cmTrans->GetSample() << "," << cmTrans->GetLine() << ","
         << cmMatch->GetSample() << "," << cmMatch->GetLine() << ","
         << cmTrans->GetSampleResidual() << "," << cmTrans->GetLineResidual()
         << "," << goodnessOfFit << endl;
    }
  }

  // If a TO parameter was specified, apply the average translation found to the
  // second input image
  if (ui.WasEntered("TO")) {
    if (ui.GetString("TRANSFORM") == "TRANSLATE") {
      QString params = " from="   + ui.GetCubeName("FROM") +
                      " to="     + ui.GetCubeName("TO") +
                      " strans=" + QString::number(sTrans) +
                      " ltrans=" + QString::number(lTrans) +
                      " interp=" + ui.GetString("INTERP");
      ProgramLauncher::RunIsisProgram("translate", params);
    }
    else {
      QString params = " from="    + ui.GetCubeName("FROM") +
                      " to="     + ui.GetCubeName("TO") +
                      " cube="   + ui.GetCubeName("MATCH") +
                      " cnet="   + ui.GetFileName("ONET") +
                      " interp=" + ui.GetString("INTERP") +
                      " degree=" + QString::number(ui.GetInteger("DEGREE"));
      ProgramLauncher::RunIsisProgram("warp", params);
    }
  }
}

//Helper function to output the regdeft file to log.
void helperButtonLog() {
  UserInterface &ui = Application::GetUserInterface();
  QString file(ui.GetFileName("DEFFILE"));
  Pvl p;
  p.read(file.toStdString());
  Application::GuiLog(p);
}
//...........end of helper function ........
