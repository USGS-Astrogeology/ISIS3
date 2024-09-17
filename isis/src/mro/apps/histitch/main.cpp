/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <cmath>
#include "Isis.h"
#include "QuickFilter.h"
#include "NumericalApproximation.h"
#include "ProcessByLine.h"
#include "SpecialPixel.h"
#include "Pvl.h"

#include "Buffer.h"
#include "Statistics.h"
#include "MultivariateStatistics.h"
#include "Table.h"

#include "tnt_array1d.h"

typedef TNT::Array1D<double> HiVector;       //!<  1-D Buffer

using namespace Isis;
using namespace std;

void histitch(vector<Buffer *> &in, vector<Buffer *> &out);
void getStats(std::vector<Isis::Buffer *> &in,
              std::vector<Isis::Buffer *> &out);

Statistics stats0;
Statistics stats1;
MultivariateStatistics stats;

struct ChannelInfo {
  int ChnNumber;
  unsigned nLines;
  unsigned nSamples;
  unsigned offset;
  HiVector mult, add;
  ChannelInfo() : ChnNumber(1), nLines(0), nSamples(0), offset(0) { }
};

static ChannelInfo fromData[2];
double average0 = Isis::Null;
double average1 = Isis::Null;
HiVector f0LineAvg;
HiVector f1LineAvg;
double coeff;
QString balance;
int seamSize;
int skipSize;


inline HiVector filter(const HiVector &v, int width) {
  QuickFilter lowpass(v.dim(), width, 1);
  lowpass.AddLine(&v[0]);
  HiVector vout(v.dim());
  for(int i = 0 ; i < v.dim() ; i ++) {
    vout[i] = lowpass.Average(i);
  }
  return (vout);
}

//2008-11-05 Jeannie Walldren Replaced references to DataInterp class with NumericalApproximation.
inline HiVector filler(const HiVector &v, int &nfilled) {
  NumericalApproximation spline(NumericalApproximation::CubicNatural);
  for(int i = 0 ; i < v.dim() ; i++) {
    if(!IsSpecial(v[i])) {
      spline.AddData(i, v[i]);
    }
  }

  //  Compute the spline and fill missing data
  HiVector vout(v.dim());
  nfilled = 0;
  for(int j = 0 ; j < v.dim() ; j++) {
    if(IsSpecial(v[j])) {
      vout[j] = spline.Evaluate(j, NumericalApproximation::NearestEndpoint);
      nfilled++;
    }
    else {
      vout[j] = v[j];
    }
  }
  return (vout);
}

HiVector compRatio(const HiVector &c0, const HiVector &c1, int &nNull) {
  nNull = 0;
  HiVector vout(c0.dim());
  for(int i = 0 ; i < c0.dim() ; i++) {
    if(IsSpecial(c0[i]) || IsSpecial(c1[i]) || (c1[i] == 0.0)) {
      vout[i] = 1.0;
      nNull++;
    }
    else {
      vout[i] = c0[i] / c1[i];
    }
  }
  return (vout);
}

HiVector compAdd(const HiVector &c0, const HiVector &c1, int &nNull) {
  nNull = 0;
  HiVector vout(c0.dim());
  for(int i = 0 ; i < c0.dim() ; i++) {
    if(IsSpecial(c0[i]) || IsSpecial(c1[i])) {
      vout[i] = 0.0;
      nNull++;
    }
    else {
      vout[i] = c0[i] - c1[i];
    }
  }
  return (vout);
}



void IsisMain() {
//  Get user interface to test for input conditions
  UserInterface &ui = Application::GetUserInterface();
  balance = ui.GetString("BALANCE");
  seamSize = ui.GetInteger("SEAMSIZE");
  skipSize = ui.GetInteger("SKIP");
  int filterWidth = ui.GetInteger("WIDTH");
  bool fillNull = ui.GetBoolean("FILL");
  int hiChannel = ui.GetInteger("CHANNEL");
  QString fixop = ui.GetString("OPERATOR");
  coeff = 1;

  // Define the processing to be by line
  ProcessByLine p;

  //Set string to gather ProductIds.
  QString stitchedProductIds;

// Obtain lines and samples of the input files.  Note that conditions
// are obtained from the first input file only unless provided from a
// second file
  Cube *icube1 = p.SetInputCube("FROM1");
  fromData[0].nLines   = fromData[1].nLines   = icube1->lineCount();
  fromData[0].nSamples = fromData[1].nSamples = icube1->sampleCount();
  fromData[0].mult = HiVector(icube1->lineCount(), 1.0);
  fromData[0].add = HiVector(icube1->lineCount(), 0.0);

  if(seamSize + skipSize > icube1->sampleCount()) {
    std::string msg = "SEAMSIZE [" + toString(seamSize) + "] + SKIP [" + toString(skipSize) + "] must ";
    msg += " be less than the number of samples [" + toString(icube1->sampleCount()) + "] in ";
    msg += "[" + ui.GetAsString("FROM1").toStdString() + "]";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  PvlGroup &from1Archive = icube1->group("ARCHIVE");
  PvlGroup &from1Instrument = icube1->group("INSTRUMENT");
  fromData[0].ChnNumber = from1Instrument["ChannelNumber"];

  stitchedProductIds = QString::fromStdString(from1Archive["ProductId"][0]);

//  Set initial conditions for one input file
  if(fromData[0].ChnNumber == 1) {
    fromData[0].offset = 0;
  }
  else  {
    if(fromData[0].ChnNumber != 0) {
      string msg = "FROM1 channel number must be 1 or 2";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    fromData[0].offset = fromData[0].nSamples;
  }

//  Only get the second input file if entered by the user
  if(ui.WasEntered("FROM2")) {
    Cube *icube2 = p.SetInputCube("FROM2");
    fromData[1].nLines   = icube2->lineCount();
    fromData[1].nSamples = icube2->sampleCount();
    fromData[1].mult = HiVector(icube2->lineCount(), 1.0);
    fromData[1].add = HiVector(icube2->lineCount(), 0.0);

    if(seamSize + skipSize > icube2->sampleCount()) {
      std::string msg = "SEAMSIZE [" + toString(seamSize) + "] + SKIP [" + toString(skipSize) + "] must ";
      msg += " be less than the number of samples [" + toString(icube2->sampleCount()) + " in ";
      msg += "[" + ui.GetAsString("FROM2").toStdString() + "]";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    //Test to make sure input files are compatable
    PvlGroup &from2Archive = icube2->group("ARCHIVE");

    //Make sure observation id's are the same
    QString from1ObsId = QString::fromStdString(from1Archive["ObservationId"]);
    QString from2ObsId = QString::fromStdString(from2Archive["ObservationId"]);
    if(from1ObsId != from2ObsId) {
      std::string msg = "The input files Observation Id's are not compatable";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    stitchedProductIds = "(" + stitchedProductIds + ", " +
                         QString::fromStdString(from2Archive["ProductId"][0]) + ")";

    PvlGroup &from2Instrument = icube2->group("INSTRUMENT");

    //Make sure CCD Id's are the same
    QString from1CcdId = QString::fromStdString(from1Instrument["CCDId"]);
    QString from2CcdId = QString::fromStdString(from2Instrument["CCDId"]);
    if(from1CcdId != from2CcdId) {
      string msg = "The input files CCD Id's are not compatable";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    //Make sure channel numbers are equal to 0 & 1
    fromData[1].ChnNumber = from2Instrument["ChannelNumber"];
    if(!((fromData[0].ChnNumber == 0) && (fromData[1].ChnNumber == 1)) &&
        !((fromData[0].ChnNumber == 1) && (fromData[1].ChnNumber == 0))) {
      string msg = "The input files Channel numbers must be equal to 0 and 1";
      throw IException(IException::User, msg, _FILEINFO_);
    }

//  Set up offsets
    if(fromData[0].ChnNumber == 1) {
      fromData[0].offset = 0;
      fromData[1].offset = fromData[0].nSamples;
    }
    else {
      fromData[1].offset = 0;
      fromData[0].offset = fromData[1].nSamples;
    }
  }

  unsigned int LinesOut = max(fromData[0].nLines, fromData[1].nLines);
  unsigned int SampsOut = fromData[0].nSamples + fromData[1].nSamples;
  unsigned int BandsOut = 1;

  Cube *ocube = p.SetOutputCube("TO", SampsOut, LinesOut, BandsOut);

  // Change Channel Number on output cube to 2
  PvlGroup &InstrumentOut = ocube->group("INSTRUMENT");
  InstrumentOut["ChannelNumber"] = "2";

  // Set StitchedChannels and Stitched ProductIds keywords
  if(ui.WasEntered("FROM2")) {
    InstrumentOut += PvlKeyword("StitchedChannels", "(0,1)");
    InstrumentOut += PvlKeyword("StitchedProductIds", stitchedProductIds.toStdString());
  }
  else {
    InstrumentOut += PvlKeyword("StitchedChannels", std::to_string(fromData[0].ChnNumber));
    InstrumentOut += PvlKeyword("StitchedProductIds", stitchedProductIds.toStdString());
  }

  //  Do balance correction
  PvlGroup results("Results");
  results += PvlKeyword("Balance", balance.toStdString());
  if((balance == "TRUE") || (balance == "EQUALIZE")) {
    ProcessByLine pAvg;

    if(ui.WasEntered("FROM2")) {


      int ch0Index = 0;
      int ch1Index = 1;
      if(fromData[0].ChnNumber == 0) {
        pAvg.SetInputCube("FROM1");
        pAvg.SetInputCube("FROM2");
      }

      if(fromData[1].ChnNumber == 0) {
        ch0Index = 1;
        ch1Index = 0;
        pAvg.SetInputCube("FROM2");
        pAvg.SetInputCube("FROM1");
      }

      stats.Reset();
      f0LineAvg = HiVector(icube1->lineCount());
      f1LineAvg = HiVector(icube1->lineCount());
      pAvg.StartProcess(getStats);
      pAvg.EndProcess();

      if(balance == "TRUE") {
        average0 = stats.X().Average();
        average1 = stats.Y().Average();
        if(hiChannel == 0) {
          if(average1 != Isis::Null) {
            coeff = average0 / average1;
            fromData[ch1Index].mult = coeff;
          }
        }
        else {
          if(average0 != Isis::Null) {
            coeff = average1 / average0;
            fromData[ch0Index].mult = coeff;
          }
        }
        results += PvlKeyword("TruthChannel", std::to_string(hiChannel));
        results += PvlKeyword("BalanceRatio", std::to_string(coeff));
      }
      else {
        //  Store off original averages for table
        HiVector ch0_org = f0LineAvg;
        HiVector ch1_org = f1LineAvg;

        results += PvlKeyword("FilterWidth", std::to_string(filterWidth));
        if(filterWidth > 0) {
          f0LineAvg = filter(f0LineAvg, filterWidth);
          f1LineAvg = filter(f1LineAvg, filterWidth);
        }

        results += PvlKeyword("Fill", ((fillNull) ? "TRUE" : "FALSE"));
        if(fillNull) {
          int nfilled;
          f0LineAvg = filler(f0LineAvg, nfilled);
          results += PvlKeyword("Channel0Filled", std::to_string(nfilled));
          f1LineAvg = filler(f1LineAvg, nfilled);
          results += PvlKeyword("Channel1Filled", std::to_string(nfilled));
        }

        results += PvlKeyword("TruthChannel", std::to_string(hiChannel));
        results += PvlKeyword("Operator", fixop.toStdString());
        int nunfilled(0);
        HiVector ch0_fixed(icube1->lineCount(), 1.0);
        HiVector ch1_fixed(icube1->lineCount(), 1.0);
        if(fixop == "MULTIPLY") {
          if(hiChannel == 0) {
            fromData[ch1Index].mult = compRatio(f0LineAvg, f1LineAvg, nunfilled);
            ch1_fixed = fromData[ch1Index].mult;
          }
          else {
            fromData[ch0Index].mult = compRatio(f1LineAvg, f0LineAvg, nunfilled);
            ch0_fixed = fromData[ch0Index].mult;
          }
        }
        else {
          if(hiChannel == 0) {
            fromData[ch1Index].add = compAdd(f0LineAvg, f1LineAvg, nunfilled);
            ch1_fixed = fromData[ch1Index].add;
            ch0_fixed = 0.0;
          }
          else {
            fromData[ch0Index].add = compAdd(f1LineAvg, f0LineAvg, nunfilled);
            ch0_fixed = fromData[ch0Index].mult;
            ch1_fixed = 0.0;
          }
        }
        results += PvlKeyword("UnFilled", std::to_string(nunfilled));

        //  Add a table to the output file of the data values
        TableField f1("Channel1Original", Isis::TableField::Double);
        TableField f2("Channel0Original", Isis::TableField::Double);
        TableField f3("Channel1Correction", Isis::TableField::Double);
        TableField f4("Channel0Correction", Isis::TableField::Double);
        TableRecord rec;
        rec += f1;
        rec += f2;
        rec += f3;
        rec += f4;
        Table table("HistitchStats", rec);
        for(int i = 0 ; i < ch1_org.dim() ; i++) {
          rec[0] = ch1_org[i];
          rec[1] = ch0_org[i];
          rec[2] = ch1_fixed[i];
          rec[3] = ch0_fixed[i];
          table += rec;
        }

        PvlGroup stitch = results;
        stitch.setName("HiStitch");
        table.Label().addGroup(stitch);
        ocube->write(table);
      }
    }

  } // end if balance = TRUE

  // Begin processing the input cubes to output cube.
  p.StartProcess(histitch);
  // All Done
  PvlGroup stitch = results;
  stitch.setName("HiStitch");
  ocube->putGroup(stitch);
  p.EndProcess();
  Application::Log(results);
}

void getStats(std::vector<Buffer *> &in, std::vector<Buffer *> &out) {
  Buffer &channel0 = *in[0];
  Buffer &channel1 = *in[1];
  double x, y;

  Statistics c0, c1;
  for(int i = 0; i < seamSize + 1; i++) {

    // set the x value
    x = channel0[skipSize + i];
    c0.AddData(x);

    // set the y value
    y = channel1[channel1.size() - (skipSize + 1) - i] ;
    c1.AddData(y);

    stats.AddData(&x, &y, 1);
  }

  f0LineAvg[channel0.Line()-1] = c0.Average();
  f1LineAvg[channel1.Line()-1] = c1.Average();
}

// Line processing routine
void histitch(vector<Buffer *> &in, vector<Buffer *> &out) {
  Buffer &ot = *out[0];

//  Initialize the buffer
  for(int n = 0 ; n < ot.size() ; n++) ot[n] = NULL8;

//  Place the channel data into the output buffer
  vector<Buffer *>::iterator ibuf;
  int ifrom;
  int line = ot.Line() - 1;
  for(ibuf = in.begin(), ifrom = 0 ; ibuf != in.end() ; ++ibuf, ++ifrom) {
    Buffer &inbuf = *(*ibuf);
    const HiVector &mult = fromData[ifrom].mult;
    const HiVector &add = fromData[ifrom].add;

    unsigned int oIndex(fromData[ifrom].offset);
    for(int i = 0; i < inbuf.size(); i++, oIndex++) {
      if(Isis::IsSpecial(inbuf[i])) {
        ot[oIndex] = inbuf[i];
      }
      else {
        ot[oIndex] =  inbuf[i] * mult[line] + add[line];
      }
    }
  }
  return;
}
