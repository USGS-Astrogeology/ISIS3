/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"
#include "ProcessByLine.h"
#include "ProcessBySample.h"
#include "HiLab.h"
#include "MultivariateStatistics.h"
#include "PvlGroup.h"
#include "Pipeline.h"

#include <vector>
#include <QMap>
#include <QList>
#include <sstream>

//#define _DEBUG_

using namespace std;
using namespace Isis;

int channel;
double correlation;
struct furrow {
  int startSample;
  int endSample;
  int increment;
  vector<MultivariateStatistics> mvstats;
};
vector<furrow> furrows;

void furrowCheck(Buffer &in, Buffer &out);
void getStats(Buffer &in);

void GetFurrowThresholdValues(int piCcdId, int piBin);
void RemoveFurrows_Version_1_42(void);
void FurrowProcess(Buffer &in, Buffer &out);
void CopyTrimFilterOutput(Buffer &in, Buffer &out);

//const int cpmm2ccd[] = {0, 1, 2, 3, 12, 4, 10, 11, 5, 13, 6, 7, 8, 9};
enum eCcdId {RED0, RED1, RED2, RED3, RED4, RED5, RED6, RED7, RED8, RED9, IR10, IR11, BG12, BG13};

int iStartSample, iNumSamples, iLastSample;
QList<int> iFurrowThresholds;
int iFurrowSample = -1;
bool bFurrowsFound = false;

void IsisMain() {
  // We will be processing by line
  ProcessByLine p;
  UserInterface &ui = Application::GetUserInterface();

  if(ui.GetBoolean("NEW_VERSION")) {
    RemoveFurrows_Version_1_42();
    return;
  }
  // Setup the input and output cubes
  Cube *icube = p.SetInputCube("FROM");
  correlation = ui.GetDouble("CORRELATION");
  HiLab hiInfo(icube);
  channel = hiInfo.getChannel();

  // Bin 1 images have up to four furrows; others have only one
  if(hiInfo.getBin() == 1) {
    // Numbers derived from TRA_000827_0985
    furrows.resize(4);
    furrows[0].startSample = 5;
    furrows[0].endSample = 1;
    furrows[0].increment = -1;

    furrows[1].startSample = 255;
    furrows[1].endSample = 251;
    furrows[1].increment = -1;

    furrows[2].startSample = 518;
    furrows[2].endSample = 514;
    furrows[2].increment = -1;

    furrows[3].startSample = 781;
    furrows[3].endSample = 777;
    furrows[3].increment = -1;
  }
  else {
    furrows.resize(1);
    if(channel == 0) {
      furrows[0].startSample = 5;
      furrows[0].endSample = 1;
      furrows[0].increment = -1;
    }
    else if(channel == 1) {
      furrows[0].startSample = icube->sampleCount() - 5 + 1;
      furrows[0].endSample = icube->sampleCount();
      furrows[0].increment = 1;
    }
    else {
      string msg = "Cannot process merged images.";
      throw IException(IException::User, msg, _FILEINFO_);
    }
  }

  for(int i = 0; i < (int)furrows.size(); i++) {
    int n = abs(furrows[i].startSample - furrows[i].endSample);
    furrows[i].mvstats.resize(n);
  }

  p.StartProcess(getStats);
  PvlGroup stats("Correlations");
  p.SetOutputCube("TO");

  // Add correlation data to cube label
  for(int i = 0; i < (int)furrows.size(); i++) {
    for(int j = 0; j < (int)furrows[i].mvstats.size(); j++) {
      string label, begin, finish;
      stringstream ss;

      if(channel == 0) {
        ss << furrows[i].startSample - j;
        ss >> begin;
        ss.clear();
        ss << furrows[i].startSample - j - 1;
        ss >> finish;
      }
      else if(channel == 1) {
        ss << furrows[i].startSample + j;
        ss >> begin;
        ss.clear();
        ss << furrows[i].startSample + j + 1;
        ss >> finish;
      }
      label += "Column" + begin + "to" + finish;
      stats += PvlKeyword(label.c_str(), toString(furrows[i].mvstats[j].Correlation()));
    }
  }
  Application::Log(stats);

  p.StartProcess(furrowCheck);
  p.EndProcess();
}

// Populate mvstats vector with column data (first or last five columns)
void getStats(Buffer &in) {
  for(int i = 0; i < (int)furrows.size(); i++) {
    for(int j = 0; j < (int)furrows[i].mvstats.size(); j++) {
      int index = furrows[i].startSample + j * furrows[i].increment - 1;
      furrows[i].mvstats[j].AddData(&in[index],
                                    &in[index+furrows[i].increment], 1);
    }
  }
}

void furrowCheck(Buffer &in, Buffer &out) {
  bool bFurrows = false;
  // Copy first << then correct furrows
  for(int i = 0; i < in.size(); i++) {
    out[i] = in[i];
  }

  for(int i = 0; i < (int)furrows.size(); i++) {
    for(int j = (int)furrows[i].mvstats.size(); j > 0; j--) {
      if(furrows[i].mvstats[j-1].Correlation() < correlation || bFurrows == true) {
        bFurrows = true;
        if(channel == 0) {
          out[furrows[i].startSample - j - 1] = Isis::Null;
        }
        else {
          out[furrows[i].startSample + j - 1] = Isis::Null;
        }
      }
    }
  }
}

/**
 * The processing has been taken from the Version 1.42 of
 * the HiCal pipeline. Furrows are Nulled in the columns specified
 * based on Channel and Summing modes and if the DN are not in the threshold
 * range. If furrows are found, then trimfilter and lowpass applications are
 * run in the pipeline to smooth the edges which were nulled due to the furrows.
 *
 * @author Sharmila Prasad (1/31/2011)
 */
void RemoveFurrows_Version_1_42(void)
{
  //int iCh0_Bin2_Samples[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  //int iCh1_Bin2_Samples[] = { 512, 511, 510, 509, 508, 507, 506, 505, 504, 503};
  //int iCh0_Bin4_Samples[] = { 1, 2, 3, 4, 5, 6};
  //int iCh1_Bin4_Samples[] = { 256, 255, 254, 253, 252, 251};

  ProcessBySample procSample;

  UserInterface &ui = Application::GetUserInterface();

  Cube *inCube = procSample.SetInputCube("FROM");
  HiLab hiInfo(inCube);
  int iChannel = hiInfo.getChannel();
  int iBin     = hiInfo.getBin();
  int iCcdId   = hiInfo.getCcd();

#ifdef _DEBUG_
  cout << "Channel=" << iChannel << "  Bin=" << iBin << "  CCID=" << iCcdId << endl;
#endif

  if(iBin != 2 && iBin != 4) {
    string sErrMsg = "Unsupported Summing Mode";
    throw IException(IException::User, sErrMsg, _FILEINFO_);
  }

  GetFurrowThresholdValues(iCcdId, iBin);

  if(iBin == 2) {
    if(iChannel == 0) {
      iStartSample = 11;
      iNumSamples  = 502;
    }
    else {
      iStartSample = 1;
      iNumSamples  = 502;
    }
  }
  else if(iBin == 4) {
    if(iChannel == 0) {
      iStartSample = 7;
      iNumSamples  = 250;
    }
    else {
      iStartSample = 1;
      iNumSamples  = 250;
    }
  }

  iLastSample = iStartSample + iNumSamples - 1;
#ifdef _DEBUG_
  cerr << "\n*** Samples not to check for Furrows ***\n";
  cerr << "Start Sample=" << iStartSample << "  Num=" << iNumSamples << "  Last=" << iLastSample << endl;
#endif

  procSample.SetOutputCube("TO");
  procSample.StartProcess(FurrowProcess);
  procSample.EndProcess();

#ifdef _DEBUG_
  cerr << "Furrows Found=" << bFurrowsFound << endl;;
#endif

  // apply trim filter if furrows found
  QString sTempFile("./FixFurrows.cub");
  if(bFurrowsFound) {
    Pipeline p;
    p.SetInputFile(FileName(ui.GetCubeName("TO").toStdString()));
    p.SetOutputFile(FileName(sTempFile.toStdString()));
    p.KeepTemporaryFiles(false);

    p.AddToPipeline("trimfilter");
    p.Application("trimfilter").SetInputParameter ("FROM",    false);
    p.Application("trimfilter").SetOutputParameter("TO",      "trim");
    p.Application("trimfilter").AddConstParameter ("LINES",   "3");
    p.Application("trimfilter").AddConstParameter ("SAMPLES", "3");
    p.Application("trimfilter").AddConstParameter ("MINOPT",  "COUNT");
    p.Application("trimfilter").AddConstParameter ("MINIMUM", "5");

    if(ui.GetBoolean("LOWPASS")) {
      p.AddToPipeline("lowpass");
      p.Application("lowpass").SetInputParameter ("FROM",    false);
      p.Application("lowpass").SetOutputParameter("TO",      "lpf");
      p.Application("lowpass").AddConstParameter ("LINES",   "3");
      p.Application("lowpass").AddConstParameter ("SAMPLES", "3");
      p.Application("lowpass").AddConstParameter ("MINOPT",  "COUNT");
      p.Application("lowpass").AddConstParameter ("MINIMUM", "5");
      p.Application("lowpass").AddConstParameter ("FILTER",  "OUTSIDE");
    }

#ifdef _DEBUG_
    cout << p;
#endif

    p.Run();

    // Copy trimfilter output file to Output file specified by the user
    CubeAttributeInput inAtt;
    procSample.SetInputCube(sTempFile, inAtt);
    procSample.SetOutputCube("TO");
    procSample.StartProcess(CopyTrimFilterOutput);
    procSample.EndProcess();

    // clean up
    remove(sTempFile.toLatin1().data());
  }
}

/**
 * Determine the Furrows and set their values to NULL. Specified
 * columns are skipped depending on the Channel and Summing modes.
 * The columns that are not skipped, if the DN value is not in the
 * threshold range then it is set to NULL.
 *
 * @author Sharmila Prasad (1/31/2011)
 *
 * @param in  - input buffer
 * @param out - output buffer
 */
void FurrowProcess(Buffer &in, Buffer &out) {
  int iCurrentSample = in.Sample();

  if(iCurrentSample < iStartSample || iCurrentSample > iLastSample) {
    iFurrowSample++;
  }

#ifdef _DEBUG_
  cerr << iCurrentSample << "  FurrowSampleIndex=" << iFurrowSample << endl;
#endif

  // Loop and move appropriate samples
  for(int i = 0; i < in.size(); i++) {
    if((iCurrentSample >= iStartSample && iCurrentSample <= iLastSample)) {
      out[i] = in[i];
    }
    else {
      if(in[i] < 0 || in[i] > iFurrowThresholds[iFurrowSample] ) {
        out[i]  = Isis::Null;
        bFurrowsFound = true;
      }
      else {
        out[i] = in[i];
      }
    }
  }
}

/**
 * Copy the input to the output cube - used to copy the
 * trimfilter result to the final output
 *
 * @author Sharmila Prasad (2/1/2011)
 *
 * @param in - input buffer
 * @param out - output buffer
 */
void CopyTrimFilterOutput(Buffer &in, Buffer &out)
{
  for(int i = 0; i < in.size(); i++)
    out[i] = in[i];
}
/**
 * Get the Max Thresholds  for DN value to be considered a Furrow,
 * based on CCID and Summing modes(2/4)
 *
 * @author Sharmila Prasad (1/31/2011)
 *
 * @param piCcdId - CCID
 * @param piBin   - Summing Mode
 *
 * @return None
 */
void GetFurrowThresholdValues(int piCcdId, int piBin)
{
  iFurrowThresholds.clear();
  switch(piCcdId) {
    case RED0:
      if(piBin == 2)
        iFurrowThresholds << 8000 << 8100 << 8700 << 9200 << 9600 << 10000 << 12000 << 12000 << 12000 << 12000;
      else if(piBin == 4) {
        iFurrowThresholds << 8000 << 9000 << 9500 <<  9900 <<  9900 << 10000;
      }
      break;
    case RED1:
      if(piBin == 2)
        iFurrowThresholds << 7200 << 7200 << 7800 <<  8400 << 9000 << 9500 << 12000 << 12000 << 12000 << 12000;
      else if(piBin == 4) {
        iFurrowThresholds << 8000 << 8100 << 9200 <<  9600 <<  9800 << 10000;
      }
      break;
    case RED2:
      if(piBin == 2)
        iFurrowThresholds << 7800 << 7800 << 8400 <<  9000 <<  9600 << 10000 << 12000 << 12000 << 12000 << 12000;
      else if(piBin == 4) {
        iFurrowThresholds << 8000 << 8700 << 9500 <<  9800 <<  9900 << 10000;
      }
      break;
    case RED3:
      if(piBin == 2)
        iFurrowThresholds << 7800 << 8100 << 8300 <<  9200 <<  9600 << 10000 << 12000 << 12000 << 12000 << 12000;
      else if(piBin == 4) {
        iFurrowThresholds << 7900 << 9200 << 9700 <<  9900 << 10000 << 10500;
      }
      break;
    case RED4:
      if(piBin == 2)
        iFurrowThresholds << 7800 << 7800 << 8300 <<  9000 <<  9500 <<  9900 << 12000 << 12000 << 12000 << 12000;
      else if(piBin == 4) {
        iFurrowThresholds << 8000 << 8700 << 9700 << 10000 << 10300 << 10600;
      }
    case RED5:
      if(piBin == 2)
        iFurrowThresholds << 7900 << 8200 << 8600 <<  9200 <<  9600 << 10000 << 12000 << 12000 << 12000 << 12000;
      else if(piBin == 4) {
        iFurrowThresholds << 8000 << 9300 << 9700 <<  9900 << 10200 << 10700;
      }
      break;
    case RED6:
      if(piBin == 2)
        iFurrowThresholds << 7500 << 7500 << 8100 <<  8500 <<  9200 << 10000 << 12000 << 12000 << 12000 << 12000;
      else if(piBin == 4) {
        iFurrowThresholds << 8000 << 8400 << 9700 << 10000 << 10500 << 10700;
      }
      break;
    case RED7:
      if(piBin == 2)
        iFurrowThresholds << 7600 << 8300 << 8900 <<  9400 <<  9900 << 11000 << 12000 << 12000 << 12000 << 12000;
      else if(piBin == 4) {
        iFurrowThresholds << 7700 << 9600 << 10000 << 10200 << 11000 << 12000;
      }
      break;
    case RED8:
      if(piBin == 2)
        iFurrowThresholds << 7200 << 7200 << 7900 <<  8500 <<  9000 <<  9400 << 12000 << 12000 << 12000 << 12000;
      else if(piBin == 4) {
        iFurrowThresholds << 8000 << 8200 << 9100 <<  9300 <<  9600 << 11000;
      }
      break;
    case RED9:
      if(piBin == 2)
        iFurrowThresholds << 7600 << 8300 << 8600 <<  9200 <<  9600 << 10000 << 12000 << 12000 << 12000 << 12000;
      else if(piBin == 4) {
        iFurrowThresholds << 8000 << 8800 << 9200 <<  9400 <<  9800 << 10500;
      }
      break;
    case IR10:
      if(piBin == 2)
        iFurrowThresholds << 8000 << 8100 << 8700 <<  9200 <<  9600 << 10000 << 12000 << 12000 << 12000 << 12000;
      else if(piBin == 4) {
        iFurrowThresholds << 7600 << 8300 << 9000 << 10000 << 10500 << 12000;
      }
      break;
    case IR11:
      if(piBin == 2)
        iFurrowThresholds << 8000 << 8100 << 8700 <<  9200 <<  9600 << 10000 << 12000 << 12000 << 12000 << 12000;
      else if(piBin == 4) {
        iFurrowThresholds << 7600 << 8300 << 9000 << 10000 << 10500 << 12000;
      }
      break;
    case BG12:
      if(piBin == 2)
        iFurrowThresholds << 8000 << 8100 << 8700 <<  9200 <<  9600 << 10000 << 12000 << 12000 << 12000 << 12000;
      else if(piBin == 4) {
        iFurrowThresholds << 7600 << 8300 << 9000 << 10000 << 10500 << 12000;
      }
      break;
    case BG13:
      if(piBin == 2)
        iFurrowThresholds << 8000 << 8100 << 8700 <<  9200 <<  9600 << 10000 << 12000 << 12000 << 12000 << 12000;
      else if(piBin == 4) {
        iFurrowThresholds << 7600 << 8300 << 9000 << 10000 << 10500 << 12000;
      }
      break;
    }

#ifdef _DEBUG_
  QListIterator<int> i(iFurrowThresholds);
  while (i.hasNext())
     cerr << i.next() << ", ";
  cerr << endl;
#endif

}
