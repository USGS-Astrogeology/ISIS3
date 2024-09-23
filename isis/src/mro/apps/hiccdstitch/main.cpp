/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#define GUIHELPERS

#include "Isis.h"

#include <sstream>
#include <iostream>

#include "UserInterface.h"
#include "Cube.h"
#include "Portal.h"
#include "Interpolator.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "ProcessMosaic.h"
#include "ProcessByLine.h"
#include "Brick.h"
#include "FileList.h"
#include "IException.h"

using namespace std;
using namespace Isis;

//helper button functins in the code
void helperButtonLog();

map <QString, void *> GuiHelpers() {
  map <QString, void *> helper;
  helper ["helperButtonLog"] = (void *) helperButtonLog;
  return helper;
}


typedef struct {
  Cube *cube;         // Cube object
  Portal *portal;     // Portal for interpolation
  QString filename;    // Name of the CCD file
  QString ccdName;     // Name of ccd
  int ccdNumber;
  int mosOrder;       // Mosaic order, default is summing mode
  int summing;        // Summing mode
  int sumLines;       // iTime delay summing line offset
  int tdi;            // TDI mode
  int trimLines;      // Commanded trim lines
  int fpsamp;         // Location of focal plane sample
  int fpline;         // Location (offset, actually) of focal plane line
  int ns;             // Number of samples in the input CCD file
  int nl;             // Number of lines in the input CCD file
  int nb;             // Number bands in image
  int ss;             // Input CCD file Starting Sample (before expansion)
  //   of the piece to put into output file
  int es;             // Input CCD file Ending Sample (before expansion)
  //   of the piece to put into output file
  int expFactor;      // Expansion factor to be used
  int outss;          // Starting sample in the output file of where
  //   the piece should go
  int outsl;          // Output starting line
} HiriseCCD;

typedef struct {
  double sample;
  double startLine;
  double lineInc;
  Portal *portal;
  Cube *cube;
} HiriseCCDLocation;

vector<HiriseCCD> CCDlist;
vector<HiriseCCD>::size_type CCDindex;

vector<HiriseCCDLocation> CCDlocation(21000);

bool compareCcd(const HiriseCCD &x, const HiriseCCD &y);
bool compareMos(const HiriseCCD &x, const HiriseCCD &y);
void InitCCDLocation(int outns);
void PlaceCCDs(Buffer &buf);

Interpolator *interp;

void IsisMain() {


  // X offset (pixels) of each CCD relative to CCD 10
  const char *const ccdNames[] = { "RED0", "RED1", "RED2", "RED3", "RED4",
                                   "RED5", "RED6", "RED7", "RED8", "RED9",
                                   "IR10", "IR11", "BG12", "BG13"
                                 };

  //  Initial locations of samples and lines for each CCD
  const int xoffset[] = { -8000, -6000, -4004, -2003, 0, 2000, 4000, 6000, 8000, 10000,
                          0, 2000, 0, 2000
                        };
  const int yoffset[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

  // For converting a CPMM number to a CCD number
  const int cpmm2ccd[] = {0, 1, 2, 3, 12, 4, 10, 11, 5, 13, 6, 7, 8, 9};

  //  Line delays for summing modes that cause shifts.  All shifts are relative
  //  to bin mode 1.  They appear to be multiples of 180 * BinMode.
  const int SummingModeLineOffsets[] = { 0, 0, 180, 360, 540, 0, 0, 0, 1260,
                                         0, 0, 0, 0, 0, 0, 0, 2700
                                       };

  typedef vector<HiriseCCD>::size_type vec_sz;

  // Get the list of names of input CCD cubes to stitch together
  FileList list;
  UserInterface &ui = Application::GetUserInterface();
  list.read(ui.GetFileName("FROMLIST").toStdString());
  if(list.size() < 1) {
    std::string msg = "The list file[" + ui.GetFileName("FROMLIST").toStdString() +
                 " does not contain any filenames";
    throw IException(IException::User, msg, _FILEINFO_);
  }

#if 0
//  Commented out as HiRISE Team is requesting a single file to be processed
//  to simplify pipeline logic
  if(list.size() == 1) {
    std::string msg = "The list file[" + ui.GetFileName("FROMLIST") +
                 " must contain at least two filenames";
    throw iException::Message(iException::User, msg, _FILEINFO_);
  }
#endif

  //  What type of interpolator is needed
  if(ui.GetString("INTERP") == "NEARESTNEIGHBOR") {
    interp = new Interpolator(Interpolator::NearestNeighborType);
  }
  else if(ui.GetString("INTERP") == "BILINEAR") {
    interp = new Interpolator(Interpolator::BiLinearType);
  }
  else if(ui.GetString("INTERP") == "CUBICCONVOLUTION") {
    interp = new Interpolator(Interpolator::CubicConvolutionType);
  }
  else {
    std::string msg = "Unknow value for INTERP [" +
                 ui.GetString("INTERP").toStdString() + "]";
    throw IException(IException::User, msg, _FILEINFO_);
  }

//  Open the shift definitions file
  Pvl shiftdef;
  shiftdef.read(ui.GetFileName("SHIFTDEF").toStdString());

  PvlObject &stitch = shiftdef.findObject("Hiccdstitch", Pvl::Traverse);

  // Get information about each of the input cubes
  bool gotRed = false;
  bool gotNir = false;
  bool gotBg = false;
  bool first = true;
  QString obsId;     // ObservationId keyword value
  int maxBands = 0;

  for(int i = 0; i < list.size(); i++) {
    HiriseCCD CCDinfo;
    Cube *cube = new Cube();
    cube->open(QString::fromStdString(list[i].toString()));

    PvlGroup arch = cube->label()->findGroup("Archive", Pvl::Traverse);
    if(first) {
      obsId = QString::fromStdString(arch["ObservationId"]);
      first = false;
    }
    else {
      if(obsId != QString::fromStdString(arch["ObservationId"])) {
        std::string msg = "Input file " + list[i].toString()
                     + " has a different ObservationId";
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }

    PvlGroup inst = cube->label()->findGroup("Instrument", Pvl::Traverse);
    int chan = inst["ChannelNumber"];
    if(chan != 2) {
      std::string msg = "Input file " + list[i].toString() + " contains a single channel";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    int cpmm = inst["CpmmNumber"];
    int ccd = cpmm2ccd[cpmm];
    if(ccd >= 0 && ccd <= 9) {
      gotRed = true;
    }
    else if(ccd == 10 || ccd == 11) {
      gotNir = true;
    }
    else {
      gotBg = true;
    }

    CCDinfo.cube = cube;
    CCDinfo.filename = QString::fromStdString(list[i].toString());
    CCDinfo.ccdName = ccdNames[ccd];
    CCDinfo.ccdNumber = ccd;
    CCDinfo.summing = inst["Summing"];
    CCDinfo.sumLines = SummingModeLineOffsets[CCDinfo.summing];
    CCDinfo.tdi     = inst["Tdi"];
    CCDinfo.trimLines = arch["TrimLines"];
    CCDinfo.fpsamp = xoffset[ccd];
    CCDinfo.fpline = yoffset[ccd];
    CCDinfo.ns = cube->sampleCount();
    CCDinfo.nl = cube->lineCount();
    CCDinfo.nb = cube->bandCount();
    if(CCDinfo.nb > maxBands) maxBands = CCDinfo.nb;
    CCDinfo.outss = 1;
    CCDinfo.outsl = 1;
    CCDinfo.ss = 1;
    //  The sort routine compare uses the > operator on mosOrder (highest mosOrder
    //  stitched first.
    //
    //  Scale summing so that if user enters mosaicOrder in def file,
    //  which would have values of -1, or 1-14, that order takes precedent
    //  over summing modes.  For example:
    //   Red9 : mosaicOrder = -1
    //   Red0 : mosaicOrder = 1
    //   Red3 : mosaicOrder = 2
    //   Red5 : mosaicOrder = 3
    // Rest of ccd's laid down highest summing mode first, then by ccd number.
    //    Using summing, mosOrder will have default values of 25,20,18,17,16,15
    //    for ascending sort.
    CCDinfo.mosOrder = (int)(1. / (float)CCDinfo.summing * 10. + 14.);

    //  Determine if a shift of the CCD exists in the definitions file
    //  Combine summing/tdi into a QString
    QString sumTdi = QString::number(CCDinfo.summing) + "/" +
                     QString::number(CCDinfo.tdi);

    QString ccdId = ccdNames[ccd];
    if(stitch.hasObject(ccdId.toStdString())) {
      PvlObject &ccddef = stitch.findObject(ccdId.toStdString(), Pvl::Traverse);
      if(ccddef.hasKeyword("MosaicOrder")) {
        CCDinfo.mosOrder = (int) ccddef["MosaicOrder"];
      }
      if(ccddef.hasKeyword("SampleOffset")) {
        CCDinfo.fpsamp = xoffset[ccd] + (int) ccddef["SampleOffset"];
      }
      if(ccddef.hasKeyword("LineOffset")) {
        CCDinfo.fpline = yoffset[ccd] + (int) ccddef["LineOffset"];
      }
      //  See if there is a binning group
      if(ccddef.hasGroup(sumTdi.toStdString())) {
        PvlGroup &sumGroup = ccddef.findGroup(sumTdi.toStdString());
        if(sumGroup.hasKeyword("SampleOffset")) {
          CCDinfo.fpsamp = xoffset[ccd] + (int) sumGroup["SampleOffset"];
        }
        if(sumGroup.hasKeyword("LineOffset")) {
          CCDinfo.fpline = yoffset[ccd] + (int) sumGroup["LineOffset"];
        }
      }
    }

    //  Set up portal
    CCDinfo.portal = new Portal(interp->Samples(), interp->Lines(),
                                cube->pixelType(),
                                interp->HotSample(), interp->HotLine());
    CCDlist.push_back(CCDinfo);

//cout << "CCD: " << ccd << " Summing: " << CCDinfo.summing << endl;

//    cube->close();
  }

  // Check for consistent filters
  if((gotRed && gotNir) || (gotRed && gotBg) || (gotNir && gotBg)) {
    std::string msg = "Cannot stitch together different filter images";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  // Sort the list of CCD info structs according to ascending CCD numbers
  sort(CCDlist.begin(), CCDlist.end(), compareCcd);

#if defined(FORCE_ADJACENT)
  // Check to make sure we have a set of adjacent CCDs
  int prevCCD = CCDlist[0].ccdNumber;
  for(vec_sz i = 1; i < CCDlist.size(); ++i) {
    if(CCDlist[i].ccdNumber != prevCCD + 1) {
      std::string msg = "CCD numbers are not adjacent";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    prevCCD = CCDlist[i].ccdNumber;
  }
#endif

  // Determine expansion factors for transferring data to output file
  int minSum = 100;                  // Minimum summing factor
  int minLine(CCDlist[0].fpline);
  for(vec_sz i = 0; i < CCDlist.size(); ++i) {
    minSum = min(minSum, CCDlist[i].summing);
    minLine = min(minLine, CCDlist[i].fpline);
  }

  //  Set the summing mode offset line as to the minimum summing mode found
  int minLineOffset = SummingModeLineOffsets[minSum];

  //  Adjust for summing mode 3 and force it to summing mode 1.
  if(CCDlist.size() != 1) {
    if(minSum == 3) minSum = 1;
  }

  for(vec_sz i = 0; i < CCDlist.size(); ++i) {
    CCDlist[i].expFactor = CCDlist[i].summing / minSum;
  }

  // Determine number of lines in output file
  // 2006-09-27 account for summing mode line time delay in lineOffset (KJB)
  int outnl = 0;
  for(vec_sz i = 0; i < CCDlist.size(); ++i) {
    int lineOffset = (CCDlist[i].fpline - minLine) / minSum +
                     ((CCDlist[i].sumLines - minLineOffset) / minSum);
    int ccdLines = CCDlist[i].nl * CCDlist[i].expFactor + lineOffset;
    outnl = max(outnl, ccdLines);
    CCDlist[i].outsl = lineOffset + 1;
  }

  // Determine the starting input sample numbers of the pieces of each input
  // file to be used and where these pieces should go in the ouput file
  CCDlist[0].ss = 1;                 // First CCD starts at input sample 1
  CCDlist[0].outss = 1;              // It goes to sample 1 in output file
  int nBandErrs = 0;                 //  Good a place as any to check this
  for(vec_sz i = 1; i < CCDlist.size(); i++) {
    CCDlist[i].ss = 1;
    CCDlist[i].outss = ((CCDlist[i].fpsamp - CCDlist[0].fpsamp) / minSum) + 1;
    // Check for appropriate bands
    if(CCDlist[i].nb != maxBands) {
      ostringstream mess;
      mess << "File " << CCDlist[i].filename.toStdString() << " does not have the required "
           << maxBands << " bands, but only " << CCDlist[i].nb;
      IException(IException::User, mess.str(), _FILEINFO_);
      nBandErrs++;
    }
  }

//  If we find any band count inconsistancies, gotta give up the ghost
  if(nBandErrs > 0) {
    std::string mess = "Band count inconsistancies exist in input cubes!";
    throw IException(IException::User, mess, _FILEINFO_);
  }

  // Compute number of samples in output file
  int outns = CCDlist[CCDlist.size()-1].outss + (2048 / minSum) - 1;

  //  Set up which input cube will be used to propagate labels
  //  before sorting is done so that the lowest ccd is used.
  ProcessByLine placing;
  CubeAttributeInput att;
  placing.SetInputCube(CCDlist[0].filename, att);
  placing.PropagateLabels(true);
  Cube *ocube = placing.SetOutputCube("TO", outns, outnl, maxBands);

  //  Delete ChannelNumber and CpmmNumber so that the output cannot be projected.
  PvlGroup oinst = ocube->group("Instrument");
  oinst.deleteKeyword("ChannelNumber");
  oinst.deleteKeyword("CpmmNumber");
  ocube->putGroup(oinst);

  placing.ClearInputCubes();


  // Sort the list of CCD info structs according to ascending mosaicOrder
  sort(CCDlist.begin(), CCDlist.end(), compareMos);

  //  Initialiize the ccdLocation arrays
  InitCCDLocation(outns);

  PvlObject results("Hiccdstitch");
  // Write ccd order to results
  for(CCDindex = 0; CCDindex < CCDlist.size(); CCDindex++) {

    PvlGroup ccdGroup(CCDlist[CCDindex].ccdName.toStdString());

    ccdGroup += PvlKeyword("File", CCDlist[CCDindex].filename.toStdString());
    ccdGroup += PvlKeyword("FocalPlaneSample", toString(CCDlist[CCDindex].fpsamp));
    ccdGroup += PvlKeyword("FocalPlaneLine", toString(CCDlist[CCDindex].fpline));
    ccdGroup += PvlKeyword("ImageSample", toString(CCDlist[CCDindex].outss));
    ccdGroup += PvlKeyword("ImageLine", toString(CCDlist[CCDindex].outsl));

    int ccd =  CCDlist[CCDindex].ccdNumber;
    ccdGroup += PvlKeyword("SampleOffset", toString(CCDlist[CCDindex].fpsamp - xoffset[ccd]));
    ccdGroup += PvlKeyword("LineOffset", toString(CCDlist[CCDindex].fpline - yoffset[ccd]));

    results.addGroup(ccdGroup);
  }

  // Process by output file
  placing.Progress()->SetText("Stitching ");
  placing.StartProcess(PlaceCCDs);
  placing.EndProcess();

  // close all inputs
  for(vec_sz i = 0; i < CCDlist.size(); ++i) {
    CCDlist[i].cube->close();
  }

//  Write the object if requested
  if(ui.WasEntered("PLACEMENT")) {
    QString placefile = ui.GetFileName("PLACEMENT");
    std::ofstream pfile;
    pfile.open(placefile.toLatin1().data(), std::ios::out | std::ios::trunc);
    pfile << results << endl;
    pfile.close();
  }

} // End of IsisMain



//    c o m p a r e    CCD's
bool compareCcd(const HiriseCCD &x, const HiriseCCD &y) {
  return x.ccdNumber < y.ccdNumber;
}



//    c o m p a r e     MosaicOrder ascending
bool compareMos(const HiriseCCD &x, const HiriseCCD &y) {
  if(x.mosOrder < y.mosOrder) {
    return true;
  }
  else if(x.mosOrder == y.mosOrder) {
    if(x.summing > y.summing) {
      return true;
    }
    else if(x.summing == y.summing && x.ccdNumber < y.ccdNumber) {
      return true;
    }
  }

  return false;

}


//    C r e a t e S t i t c h
void CreateStitch(Buffer &buf) {
  for(int i = 0; i < buf.size(); i++) {
    buf[i] = NULL8;
  }
}


void InitCCDLocation(int outputNS) {

  CCDlocation.resize(outputNS);
  for(int i = 0; i < (int)CCDlocation.size(); i++) {
    CCDlocation[i].cube = NULL;
  }
  for(CCDindex = 0; CCDindex < CCDlist.size(); CCDindex++) {

    int osamp = CCDlist[CCDindex].outss - 1;
    //  ???  should it be i<=ss+ns-1  ???
    for(int i = CCDlist[CCDindex].ss; i <= CCDlist[CCDindex].ns; i++) {
      for(int j = 0; j != CCDlist[CCDindex].expFactor; j++) {

        CCDlocation[osamp].sample = (i - 0.5) +
                                    (0.5 / CCDlist[CCDindex].expFactor) +
                                    j * (1.0 / CCDlist[CCDindex].expFactor);


        CCDlocation[osamp].startLine = 0.5 +
                                       (0.5 / CCDlist[CCDindex].expFactor) -
                                       ((CCDlist[CCDindex].outsl - 1.0) *
                                        (1.0 / CCDlist[CCDindex].expFactor));

        CCDlocation[osamp].lineInc = 1.0 / CCDlist[CCDindex].expFactor;
        CCDlocation[osamp].portal = CCDlist[CCDindex].portal;
        CCDlocation[osamp].cube = CCDlist[CCDindex].cube;
        osamp++;
      }
    }
  }
}

//  Process by line with output instead of input
void PlaceCCDs(Buffer &obuf) {

  for(int i = 0; i < obuf.size(); i++) {
    if(CCDlocation[i].cube == NULL) {
      obuf[i] = Isis::Null;
    }
    else {
      double inSamp = CCDlocation[i].sample;
      double inLine = CCDlocation[i].startLine +
                      (CCDlocation[i].lineInc * (obuf.Line() - 1.0));
      CCDlocation[i].portal->SetPosition(inSamp, inLine, obuf.Band());
      CCDlocation[i].cube->read(*(CCDlocation[i].portal));
      obuf[i] = interp->Interpolate(inSamp, inLine,
                                    CCDlocation[i].portal->DoubleBuffer());
    }
  }
}

#if 0
//    P l a c e C C D
void PlaceCCD(Buffer &buf) {
  // Copy input pixels into the output Brick buffer.  This includes
  // inserting multiple copies of the pixel by applying the expansion factor.
  int bindex = 0;         // Index into the output Brick
  for(int i = CCDlist[CCDindex].ss; i <= CCDlist[CCDindex].ns; i++) {
    // Process an input pixel
    for(int j = 0; j != CCDlist[CCDindex].expFactor; j++) {
      // Put a copy of input pixel into the output Brick
      (*brick)[bindex++] = buf[i-1];
    }
  }

  // Write the output Brick buffer line to the ouput file.  This includes
  // writing multiple copies by applying the expansion factor.
  int outLine = ((buf.Line() - 1) * CCDlist[CCDindex].expFactor) +
                CCDlist[CCDindex].outsl;
  for(int j = 0; j != CCDlist[CCDindex].expFactor; j++) {
    brick->SetBaseLine(outLine++);
    stitchCube->Write(*brick);
  }
}

#endif

//Helper function to output the regdeft file to log.
void helperButtonLog() {
  UserInterface &ui = Application::GetUserInterface();
  QString file(ui.GetFileName("SHIFTDEF"));
  Pvl p;
  p.read(file.toStdString());
  Application::GuiLog(p);
}
//...........end of helper function ........
