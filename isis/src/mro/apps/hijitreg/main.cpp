/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

#include <cmath>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include "AutoReg.h"
#include "AutoRegFactory.h"
#include "ControlMeasure.h"
#include "ControlMeasureLogData.h"
#include "ControlNet.h"
#include "ControlPoint.h"
#include "Chip.h"
#include "Cube.h"
#include "HiJitCube.h"
#include "IException.h"
#include "iTime.h"
#include "MultivariateStatistics.h"
#include "Progress.h"
#include "PvlGroup.h"
#include "SerialNumber.h"
#include "Statistics.h"
#include "UserInterface.h"

using namespace std;
using namespace Isis;


struct RegData {
  double fLine;
  double fSamp;
  double fLTime;
  double mLine;
  double mSamp;
  double mLTime;
  double regLine;
  double regSamp;
  double regCorr;
  double B0;              //<! Offset of linear regression
  double B1;              //<! Slope of linear regression
  double Bcorr;           //<! Regression Correlation
};

struct JitterParms {
  HiJitCube::JitInfo fromJit;
  HiJitCube::JitInfo matchJit;
  HiJitCube::Corners fromCorns;
  HiJitCube::Corners matchCorns;
  QString regFile;
  int rows;
  int cols;
  double lSpacing;
  double sSpacing;
  Statistics sStats;
  Statistics lStats;
  int nSuspects;
};


typedef vector<RegData> RegList;

static ostream &dumpResults(ostream &out, const RegList &regs,
                            const JitterParms &jparms, const AutoReg &ar);

void IsisMain() {

  // Get user interface
  UserInterface &ui = Application::GetUserInterface();

//  Open the shift definitions file
  Pvl shiftdef;
  if (ui.WasEntered("SHIFTDEF")) {
    shiftdef.read(ui.GetFileName("SHIFTDEF").toStdString());
  }
  else {
    shiftdef.addObject(PvlObject("Hiccdstitch"));
  }

  PvlObject &stitch = shiftdef.findObject("Hiccdstitch", Pvl::Traverse);


  // Open the first cube.  It will be matched to the second input cube.
  HiJitCube trans;
  CubeAttributeInput &attTrans = ui.GetInputAttribute("FROM");
  vector<QString> bandTrans = attTrans.bands();
  trans.setVirtualBands(bandTrans);
  trans.OpenCube(ui.GetCubeName("FROM"), stitch);


  // Open the second cube, it is held in place.  We will be matching the
  // first to this one by attempting to compute a sample/line translation
  HiJitCube match;
  CubeAttributeInput &attMatch = ui.GetInputAttribute("MATCH");
  vector<QString> bandMatch = attMatch.bands();
  match.setVirtualBands(bandMatch);
  match.OpenCube(ui.GetCubeName("MATCH"), stitch);

//  Ensure only one band
  if ((trans.bandCount() != 1) || (match.bandCount() != 1)) {
    std::string msg = "Input Cubes must have only one band!";
    throw IException(IException::User, msg, _FILEINFO_);
  }

//  Now test compatability (basically summing)
  trans.Compatable(match);

//  Determine intersection
  if (!trans.intersects(match)) {
    std::string msg = "Input Cubes do not overlap!";
    throw IException(IException::User, msg, _FILEINFO_);
  }

//  Get overlapping regions of each cube
  HiJitCube::Corners fcorns, mcorns;
  trans.overlap(match, fcorns);
  match.overlap(trans, mcorns);

#if defined(ISIS_DEBUG)
  cout << "FROM Poly:  " << trans.PolyToString() << std::endl;
  cout << "MATCH Poly: " << match.PolyToString() << std::endl;
  cout << "From Overlap:  (" << fcorns.topLeft.sample << ","
       << fcorns.topLeft.line   << "), ("
       << fcorns.lowerRight.sample << ","
       << fcorns.lowerRight.line << ")\n" ;
  cout << "Match Overlap: (" << mcorns.topLeft.sample << ","
       << mcorns.topLeft.line   << "), ("
       << mcorns.lowerRight.sample << ","
       << mcorns.lowerRight.line << ")\n" ;
#endif


  // We need to get a user definition of how to auto correlate around each
  // of the grid points.
  Pvl regdef;
  FileName regFile(ui.GetFileName("REGDEF").toStdString());
  regdef.read(regFile.expanded());
  AutoReg *ar = AutoRegFactory::Create(regdef);


  double flines(fcorns.lowerRight.line - fcorns.topLeft.line + 1.0);
  double fsamps(fcorns.lowerRight.sample - fcorns.topLeft.sample + 1.0);

  // We want to create a grid of control points that is N rows by M columns.
  // Get row and column variables, if not entered, default to 1% of the input
  // image size
  int rows(1), cols(1);
  if (ui.WasEntered("ROWS")) {
    rows = ui.GetInteger("ROWS");
  }
  else {
    rows = (int)(((flines - 1.0) / ar->SearchChip()->Lines()) + 1);
  }

  cols = ui.GetInteger("COLUMNS");
  if (cols == 0) {
    cols = (int)(((fsamps - 1.0) / ar->SearchChip()->Samples()) + 1);
  }

  // Calculate spacing for the grid of points
  double lSpacing = floor(flines / rows);
  double sSpacing = floor(fsamps / cols);

#if defined(ISIS_DEBUG)
  cout << "# Samples in Overlap: " << fsamps << endl;
  cout << "# Lines in Overlap  : " << flines << endl;
  cout << "# Rows:    " << rows << endl;
  cout << "# Columns: " << cols << endl;
  cout << "Line Spacing:   " << lSpacing << endl;
  cout << "Sample Spacing: " << sSpacing << endl;
#endif

  // Display the progress...10% 20% etc.
  Progress prog;
  prog.SetMaximumSteps(rows * cols);
  prog.CheckStatus();

  // Initialize control point network
  ControlNet cn;
  cn.SetUserName(Application::UserName());
  cn.SetCreatedDate(iTime::CurrentLocalTime());

  //  Get serial numbers for input cubes
  QString transSN = SerialNumber::Compose(trans, true);
  QString matchSN = SerialNumber::Compose(match, true);

  cn.SetTarget(*trans.label());
  cn.SetDescription("Records s/c jitter between two adjacent HiRISE images");

//  Set up results parameter saves
  JitterParms jparms;
  jparms.fromCorns = fcorns;
  jparms.fromJit = trans.GetInfo();
  jparms.matchCorns = mcorns;
  jparms.matchJit = match.GetInfo();
  jparms.regFile = QString::fromStdString(regFile.expanded());
  jparms.cols = cols;
  jparms.rows = rows;
  jparms.lSpacing = lSpacing;
  jparms.sSpacing = sSpacing;
  jparms.nSuspects = 0;

  // Loop through grid of points and get statistics to compute
  // translation values
  RegList reglist;
  double fline0(fcorns.topLeft.line - 1.0), fsamp0(fcorns.topLeft.sample - 1.0);
  double mline0(mcorns.topLeft.line - 1.0), msamp0(mcorns.topLeft.sample - 1.0);

  for (int r = 0; r < rows; r++) {
    int line = (int)(lSpacing / 2.0 + lSpacing * r + 0.5);
    for (int c = 0; c < cols; c++) {
      int samp = (int)(sSpacing / 2.0 + sSpacing * c + 0.5);

      ar->PatternChip()->TackCube(msamp0 + samp, mline0 + line);
      ar->PatternChip()->Load(match);
      ar->SearchChip()->TackCube(fsamp0 + samp, fline0 + line);
      ar->SearchChip()->Load(trans);

      // Set up ControlMeasure for cube to translate
      ControlMeasure * cmTrans = new ControlMeasure;
      cmTrans->SetCubeSerialNumber(transSN);
      cmTrans->SetCoordinate(msamp0 + samp, mline0 + line,
                            ControlMeasure::Candidate);
      cmTrans->SetChooserName("hijitreg");

      // Set up ControlMeasure for the pattern/Match cube
      ControlMeasure * cmMatch = new ControlMeasure;
      cmMatch->SetCubeSerialNumber(matchSN);
      cmMatch->SetCoordinate(fsamp0 + samp, fline0 + line,
                            ControlMeasure::Candidate);
      cmMatch->SetChooserName("hijitreg");

      ar->Register();

      // Match found
      if (ar->Success()) {
        RegData reg;
        reg.fLine = fline0 + line;
        reg.fSamp = fsamp0 + samp;
        reg.fLTime = trans.getLineTime(reg.fLine);
        reg.mLine = mline0 + line;
        reg.mSamp = msamp0 + samp;
        reg.mLTime = match.getLineTime(reg.mLine);
        reg.regLine = ar->CubeLine();
        reg.regSamp = ar->CubeSample();
        reg.regCorr = ar->GoodnessOfFit();


        if (fabs(reg.regCorr) > 1.0) jparms.nSuspects++;

        double sDiff = reg.fSamp - reg.regSamp;
        double lDiff = reg.fLine - reg.regLine;
        jparms.sStats.AddData(&sDiff, (unsigned int)1);
        jparms.lStats.AddData(&lDiff, (unsigned int)1);

//  Record the translation in the control point
        cmTrans->SetCoordinate(ar->CubeSample(), ar->CubeLine(),
                              ControlMeasure::RegisteredPixel);
        cmTrans->SetResidual(sDiff, lDiff);
        ControlMeasureLogData gof(ControlMeasureLogData::GoodnessOfFit,
            ar->GoodnessOfFit());
        cmTrans->SetLogData(gof);

//  Reread the chip location centering the offset and compute
//  linear regression statistics
        try {
          Chip &pchip(*ar->PatternChip());
          Chip fchip(pchip.Samples(), pchip.Lines());
          fchip.TackCube(ar->CubeSample(), ar->CubeLine());
          fchip.Load(trans);

//  Writes correlated chips to files for visual inspection
#if defined(ISIS_DEBUG)
          ostringstream tstr;
          tstr << "R" << r << "C" << c << "_chip.cub";
          QString fcname("from"  + tstr.str());
          QString mcname("match" + tstr.str());

          pchip.Write(mcname);
          fchip.Write(fcname);
#endif

          MultivariateStatistics mstats;
          for (int line = 1 ; line <= fchip.Lines() ; line++) {
            for (int sample = 1; sample < fchip.Samples(); sample++) {
              double fchipValue = fchip.GetValue(sample, line);
              double pchipValue = pchip.GetValue(sample, line);
              mstats.AddData(&fchipValue, &pchipValue, 1);
            }
          }

//  Get regression and correlation values
          mstats.LinearRegression(reg.B0, reg.B1);
          reg.Bcorr = mstats.Correlation();
          if (IsSpecial(reg.B0)) throw 1;
          if (IsSpecial(reg.B1)) throw 2;
          if (IsSpecial(reg.Bcorr)) throw 3;
        }
        catch (...) {
//  If fails, flag this condition
          reg.B0 = 0.0;
          reg.B1 = 0.0;
          reg.Bcorr = 0.0;
        }

        reglist.push_back(reg);
      }

      // Add the measures to a control point
      QString str = "Row " + QString::number(r) + " Column " + QString::number(c);
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

  // If flatfile was entered, create the flatfile
  // The flatfile is comma seperated and can be imported into an excel
  // spreadsheet
  if (ui.WasEntered("FLATFILE")) {
    QString fFile = ui.GetFileName("FLATFILE");
    ofstream os;
    QString fFileExpanded = QString::fromStdString(FileName(fFile.toStdString()).expanded());
    os.open(fFileExpanded.toLatin1().data(), ios::out);
    dumpResults(os, reglist, jparms, *ar);
  }

  // If a cnet file was entered, write the ControlNet pvl to the file
  if (ui.WasEntered("CNETFILE")) {
    cn.Write(ui.GetFileName("CNETFILE"));
  }

  // Don't need the cubes opened anymore
  trans.close();
  match.close();


  // Write translation to log
  PvlGroup results("AverageTranslation");
  if (jparms.sStats.ValidPixels() > 0) {
    double sTrans = (int)(jparms.sStats.Average() * 100.0) / 100.0;
    double lTrans = (int)(jparms.lStats.Average() * 100.0) / 100.0;
    results += PvlKeyword("Sample", std::to_string(sTrans));
    results += PvlKeyword("Line", std::to_string(lTrans));
    results += PvlKeyword("NSuspects", std::to_string(jparms.nSuspects));
  }
  else {
    results += PvlKeyword("Sample", "NULL");
    results += PvlKeyword("Line", "NULL");
  }

  Application::Log(results);

  // add the auto registration information to print.prt
  PvlGroup autoRegTemplate = ar->RegTemplate();
  Application::Log(autoRegTemplate);

  return;
}


static ostream &dumpResults(ostream &out, const RegList &regs,
                            const JitterParms &jparms, const AutoReg &ar) {

  std::ios::fmtflags oldFlags = out.flags();
  out.setf(std::ios::fixed);

  const HiJitCube::JitInfo &fJit = jparms.fromJit;
  const HiJitCube::JitInfo &mJit = jparms.matchJit;
  const HiJitCube::Corners &fcorns = jparms.fromCorns;
  const HiJitCube::Corners &mcorns = jparms.matchCorns;

  out << "#          Hijitreg ISIS Application Results" << endl;
  out << "#    Coordinates are (Sample, Line) unless indicated" << endl;
  out << "#           RunDate:  " << iTime::CurrentLocalTime().toStdString() << endl;
  out << "#\n#    ****  Image Input Information ****\n";
  out << "#  FROM:  " << fJit.filename.toStdString() << endl;
  out << "#    Lines:       " << setprecision(0) << fJit.lines << endl;
  out << "#    Samples:     " << setprecision(0) << fJit.samples << endl;
  out << "#    FPSamp0:     " << setprecision(0) << fJit.fpSamp0 << endl;
  out << "#    SampOffset:  " << fJit.sampOffset << endl;
  out << "#    LineOffset:  " << fJit.lineOffset << endl;
  out << "#    CPMMNumber:  " << fJit.cpmmNumber << endl;
  out << "#    Summing:     " << fJit.summing << endl;
  out << "#    TdiMode:     " << fJit.tdiMode << endl;
  out << "#    Channel:     " << fJit.channelNumber << endl;
  out << "#    LineRate:    " << setprecision(8) << fJit.linerate
      << " <seconds>" << endl;
  out << "#    TopLeft:     " << setw(7) << setprecision(0)
      << fcorns.topLeft.sample << " "
      << setw(7) << setprecision(0)
      << fcorns.topLeft.line << endl;
  out << "#    LowerRight:  " << setw(7) << setprecision(0)
      << fcorns.lowerRight.sample << " "
      << setw(7) << setprecision(0)
      << fcorns.lowerRight.line << endl;
  out << "#    StartTime:   " << fJit.UTCStartTime.toStdString() << " <UTC>" << endl;
  out << "#    SCStartTime: " << fJit.scStartTime.toStdString() << " <SCLK>" << endl;
  out << "#    StartTime:   " << setprecision(8) << fJit.obsStartTime
      << " <seconds>" << endl;
  out << "\n";
  out << "#  MATCH: " << mJit.filename.toStdString() << endl;
  out << "#    Lines:       " << setprecision(0) << mJit.lines << endl;
  out << "#    Samples:     " << setprecision(0) << mJit.samples << endl;
  out << "#    FPSamp0:     " << setprecision(0) << mJit.fpSamp0 << endl;
  out << "#    SampOffset:  " << mJit.sampOffset << endl;
  out << "#    LineOffset:  " << mJit.lineOffset << endl;
  out << "#    CPMMNumber:  " << mJit.cpmmNumber << endl;
  out << "#    Summing:     " << mJit.summing << endl;
  out << "#    TdiMode:     " << mJit.tdiMode << endl;
  out << "#    Channel:     " << mJit.channelNumber << endl;
  out << "#    LineRate:    " << setprecision(8) << mJit.linerate
      << " <seconds>" << endl;
  out << "#    TopLeft:     " << setw(7) << setprecision(0)
      << mcorns.topLeft.sample << " "
      << setw(7) << setprecision(0)
      << mcorns.topLeft.line << endl;
  out << "#    LowerRight:  " << setw(7) << setprecision(0)
      << mcorns.lowerRight.sample  << " "
      << setw(7) << setprecision(0)
      << mcorns.lowerRight.line << endl;
  out << "#    StartTime:   " << mJit.UTCStartTime.toStdString() << " <UTC>" << endl;
  out << "#    SCStartTime: " << mJit.scStartTime.toStdString() << " <SCLK>" << endl;
  out << "#    StartTime:   " << setprecision(8) << mJit.obsStartTime
      << " <seconds>" << endl;
  out << "\n";

  double nlines(fcorns.lowerRight.line - fcorns.topLeft.line + 1);
  double nsamps(fcorns.lowerRight.sample - fcorns.topLeft.sample + 1);
  out << "\n#  **** Registration Data ****\n";
  out << "#   RegFile: " << jparms.regFile.toStdString() << endl;
  out << "#   OverlapSize:      " << setw(7) << (int) nsamps << " "
      << setw(7) << (int) nlines << "\n";
  out << "#   Sample Spacing:   " << setprecision(1) << jparms.sSpacing << endl;
  out << "#   Line Spacing:     " << setprecision(1) << jparms.lSpacing << endl;
  out << "#   Columns, Rows:    " << jparms.cols << " " << jparms.rows << endl;
  out << "#   Corr. Algorithm:  " << ar.AlgorithmName().toStdString() << endl;
  out << "#   Corr. Tolerance:  " << setprecision(2) << ar.Tolerance() << endl;
  out << "#   Total Registers:  " << regs.size() << " of "
      << (jparms.rows * jparms.cols) << endl;
  out << "#   Number Suspect:   " << jparms.nSuspects << endl;
  if (jparms.sStats.ValidPixels() > 0) {
    out << "#   Average Sample Offset: " << setprecision(4)
        << jparms.sStats.Average()
        << "  StdDev: " << setprecision(4) << jparms.sStats.StandardDeviation()
        << endl;
    out << "#   Average Line Offset:   " << setprecision(4)
        << jparms.lStats.Average()
        << " StdDev: " << setprecision(4) << jparms.lStats.StandardDeviation()
        << endl;
  }
  else {
    out << "#   Average Sample Offset: " << "NULL\n";
    out << "#   Average Line Offset:   " << "NULL\n";
  }

  out << "\n#  Column Headers and Data\n";

//  Write headers
  out
      << setw(20) << "FromTime"
      << setw(10) << "FromSamp"
      << setw(10) << "FromLine"
      << setw(20) << "MatchTime"
      << setw(10) << "MatchSamp"
      << setw(10) << "MatchLine"
      << setw(15) << "RegSamp"
      << setw(15) << "RegLine"
      << setw(10) << "RegCorr"
      << setw(15) << "B0_Offset"
      << setw(15) << "B1_Slope"
      << setw(10) << "B_RCorr"
      << endl;

  RegList::const_iterator reg;
  for (reg = regs.begin() ; reg != regs.end() ; ++reg) {
    out << setw(20) << setprecision(8) << reg->fLTime
        << setw(10) << setprecision(0) << reg->fSamp
        << setw(10) << setprecision(0) << reg->fLine
        << setw(20) << setprecision(8) << reg->mLTime
        << setw(10) << setprecision(0) << reg->mSamp
        << setw(10) << setprecision(0) << reg->mLine
        << setw(15) << setprecision(4) << reg->regSamp
        << setw(15) << setprecision(4) << reg->regLine
        << setw(10) << setprecision(6) << reg->regCorr
        << setw(15) << setprecision(6) << reg->B0
        << setw(15) << setprecision(6) << reg->B1
        << setw(10) << setprecision(6) << reg->Bcorr
        << std::endl;
  }

  out.setf(oldFlags);
  return (out);
}
