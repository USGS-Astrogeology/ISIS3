/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <iostream>
#include <sstream>
#include <string>

#include <SpiceUsr.h>

#include "Camera.h"
#include "FileName.h"
#include "HiJitCube.h"
#include "IException.h"
#include "Instrument.hh"
#include "iTime.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "NaifStatus.h"
#include "RestfulSpice.h"

using namespace UA::HiRISE;
using std::endl;
using std::ostringstream;


namespace Isis {


  static  geos::geom::GeometryFactory::Ptr geosFactory = geos::geom::GeometryFactory::create();
  bool HiJitCube::naifLoaded = false;
  int npSamp0[] = {0, 1971, 3964, 5963, 7970, 7971, 7971, 9975, 9976, 9976, 11981, 13986, 15984, 17982};
  int npSamps[] = {2021, 2043, 2048, 2052, 2055, 2053, 2053, 2053, 2054, 2055, 2051, 2049, 2043, 2018};
  bool sampinit = false;
  bool originst;

  /**
   *  @brief Default constructor with no cube
   */
  HiJitCube::HiJitCube() {
    initLocal();
  }

  /**
   *  @brief Constructor with file to open
   */
  HiJitCube::HiJitCube(const QString &filename) {
    initLocal();
    OpenCube(filename);
  }

  /**
   *  @brief Constructor with file to open and potential shift applied
   */
  HiJitCube::HiJitCube(const QString &filename, PvlObject &shift) {
    initLocal();
    OpenCube(filename, shift);
  }


  /**
   *  @brief Destructor
   */
  HiJitCube::~HiJitCube() {
    delete fpGeom;
    close();
  }

  void HiJitCube::setSampleOffset(int soff) {
    jdata.sampOffset = soff;
    if(isOpen()) computePoly();
    return;
  }

  void HiJitCube::setLineOffset(int loff) {
    jdata.lineOffset = loff;
    if(isOpen()) computePoly();
    return;
  }


  void HiJitCube::OpenCube(const QString &filename) {
    open(filename);
    Init();
    return;
  }

  void HiJitCube::OpenCube(const QString &filename, PvlObject &shift) {
    OpenCube(filename);

    //  Determine if a shift of the CCD exists in the definitions file
    if(shift.hasGroup(jdata.ccdName)) {
      PvlGroup &ccddef = shift.findGroup(jdata.ccdName, Pvl::Traverse);
      if(ccddef.hasKeyword("SampleOffset")) {
        jdata.sampOffset = (int) ccddef["SampleOffset"];
      }
      if(ccddef.hasKeyword("LineOffset")) {
        jdata.lineOffset = (int) ccddef["LineOffset"];
      }
      computePoly();
    }

    return;
  }

  double HiJitCube::getLineTime(double line) const {
    return (((line - 1.0) * jdata.linerate) + jdata.obsStartTime);
  }

  void HiJitCube::Compatable(HiJitCube &cube) {
    JitInfo other = cube.GetInfo();

    if(jdata.summing != other.summing) {
      ostringstream msg;
      msg << "Summing mode (" << jdata.summing
          << ") in file " << fileName() << " is not equal to summing mode ("
          << other.summing << ") in file " << cube.fileName() << endl;
      throw IException(IException::User, msg.str(), _FILEINFO_);
    }
    return;
  }


  bool HiJitCube::intersects(const HiJitCube &cube) const {
    return (fpGeom->intersects(cube.Poly()));
  }

  bool HiJitCube::overlap(const HiJitCube &cube, Corners &ovlCorners) {
    geos::geom::Geometry *ovl = fpGeom->intersection(cube.Poly()).release();
//  cout << "Overlap: " << ovl->toString() << std::endl;
    ovlCorners = FocalPlaneToImage(getCorners(*ovl));
    delete ovl;
    return (ovlCorners.good);
  }


  void HiJitCube::computeStartTime() {

//  Compute the unbinned and binned linerates in seconds
    jdata.unBinnedRate = ((Instrument::LINE_TIME_PRE_OFFSET +
                           (jdata.dltCount / Instrument::HIRISE_CLOCK_SUBTICK_MICROS))
                          / 1000000.0);
    jdata.linerate = jdata.unBinnedRate * ((double) jdata.summing);

    if(!jdata.scStartTime.isEmpty()) {

      try {
        Camera *cam;
        cam = camera();
        // This SetImage at (1,1) is used to match the non-camera code below. (0.5, 0.5) should match the start
        // clock count of the image, but instead (1, 1) matches. This suggests something odd in the Camera
        cam->SetImage (1.0, 1.0);
        jdata.obsStartTime = cam->time().Et();
      } catch (IException &e) {
        try {
          QString scStartTimeString = jdata.scStartTime;
          jdata.obsStartTime = Isis::RestfulSpice::strSclkToEt(-74999, scStartTimeString.toLatin1().data(), "hirise");
        } catch (IException &e) {
            QString message = "Start time of the image can not be determined.";
            throw IException(e, IException::User, message, _FILEINFO_);
        }
      }

      // Adjust the start time so that it is the effective time for
      // the first line in the image file
      jdata.obsStartTime -= (jdata.unBinnedRate * (((double(jdata.tdiMode / 2.0)
                             - 0.5))));
      // Effective observation
      // time for all the TDI lines used for the
      // first line before doing binning
      jdata.obsStartTime += (jdata.unBinnedRate * (((double) jdata.summing / 2.0)
                             - 0.5));
    }
    return;
  }

  void HiJitCube::initLocal() {
    jdata.filename = "_none_";
    jdata.productId = "__undetermined__";
    jdata.lines = 0;
    jdata.samples = 0;
    jdata.sampOffset = 0;
    jdata.lineOffset = 0;
    jdata.tdiMode = 0;
    jdata.summing = 0;
    jdata.channelNumber = 0;
    jdata.cpmmNumber = 0;
    jdata.ccdName = "_unknown_";
    jdata.dltCount = 0.0;
    jdata.UTCStartTime = "";
    jdata.scStartTime = "";
    jdata.obsStartTime = 0.0;
    jdata.unBinnedRate = 0.0;
    jdata.linerate = 0.0;
    jdata.fpSamp0 = 0;
    jdata.fpLine0 = 0;
    jdata.pixpitch = 0.0;

    fpGeom = 0;
  }

  void HiJitCube::Init() {
    // Get required keywords from instrument group
    Pvl *labelPvl(label());
    Isis::PvlGroup inst;
    Isis::PvlGroup idinst;
    jdata.filename = fileName();
    Isis::PvlGroup &archive = labelPvl->findGroup("Archive", Isis::Pvl::Traverse);
    jdata.productId = (QString) archive["ProductId"];

    jdata.lines = lineCount();
    if(labelPvl->findObject("IsisCube").hasGroup("OriginalInstrument")) {
      inst = labelPvl->findGroup("OriginalInstrument", Isis::Pvl::Traverse);
      originst = true;
    }
    else {
      inst = labelPvl->findGroup("Instrument", Isis::Pvl::Traverse);
      originst = false;
    }
    jdata.tdiMode = inst["Tdi"];
    jdata.summing = inst["Summing"];
    if(labelPvl->findObject("IsisCube").hasGroup("Instrument") && originst) {
      idinst = labelPvl->findGroup("Instrument", Isis::Pvl::Traverse);
      if (idinst.hasKeyword("PixelPitch")) {
        jdata.pixpitch = idinst["PixelPitch"];
      }
      else {
        jdata.pixpitch = labelPvl->findObject("NaifKeywords")["IDEAL_PIXEL_PITCH"];
      }
      jdata.summing = (int)(jdata.pixpitch / .012);
    }
    if(originst && jdata.summing != 1 && !sampinit) {
      for(int i = 0; i < 14; i++) {
        npSamps[i] = (int)(npSamps[i] / (float)jdata.summing + 0.5);
        npSamp0[i] = (int)(npSamp0[i] / (float)jdata.summing + 0.5);
      }
      sampinit = true;
    }
    jdata.channelNumber = inst["ChannelNumber"];
    jdata.cpmmNumber = inst["CpmmNumber"];
    if(originst) {
      jdata.samples = npSamps[jdata.cpmmNumber];
    }
    else {
      jdata.samples = sampleCount();
    }
    jdata.ccdName = Instrument::CCD_NAMES[jdata.cpmmNumber];
    jdata.dltCount = inst["DeltaLineTimerCount"];
    jdata.UTCStartTime = (QString) inst["StartTime"];
    jdata.scStartTime = (QString) inst["SpacecraftClockStartCount"];

    try {
      if(originst) {
        jdata.fpSamp0 = npSamp0[jdata.cpmmNumber];
      }
      else {
        jdata.fpSamp0 = Instrument::focal_plane_x_offset(jdata.cpmmNumber,
                        jdata.summing);
      }
    }
    catch(Exception &hiExc) {
      ostringstream msg;
      msg << "Summing mode (" << jdata.summing
          << ") is illegal (must be > 0) or CPMM number (" << jdata.cpmmNumber
          << ") is invalid in file " << fileName() << endl;
      throw IException(IException::User, msg.str(), _FILEINFO_);
    }

//  It is assumed all images start at the line location in the focal plane
    jdata.fpLine0 = 0;

//  Validate channel number and adjust starting sample
    if((jdata.channelNumber > 2) || (jdata.channelNumber < 0)) {
      ostringstream msg;
      msg << "Channel number (" << jdata.channelNumber
          << ") is invalid (must be 0, 1 or 2) in file " << fileName()
          << endl;
      throw IException(IException::User, msg.str(), _FILEINFO_);
    }
    else {
      if(originst) {
        if(jdata.channelNumber == 0) jdata.fpSamp0 += npSamps[jdata.cpmmNumber];
      }
      else {
        if(jdata.channelNumber == 0) jdata.fpSamp0 += sampleCount();
      }
    }

// Determine starting time of image and compute the binning rates
    computeStartTime();

//  Compute the focal plane polygon for this image
    computePoly();
    return;
  }


  int HiJitCube::getBinModeIndex(int summing) const {
    for(unsigned int i = 0 ; i < Instrument::TOTAL_BINNING_FACTORS ; i++) {
      int binFactor = Instrument::BINNING_FACTORS[i];
      if(binFactor == summing) return (i);
    }

    ostringstream msg;
    msg << "Invalid summing mode (" << summing << ") for file " <<
        fileName() << std::endl;
    throw IException(IException::User, msg.str(), _FILEINFO_);
  }


  void HiJitCube::computePoly() {

//  Compute sample and line coordinates in the focal plane
    int samp0, sampN;
    if(originst) {
      samp0 = jdata.fpSamp0 + jdata.sampOffset;
      sampN = samp0 + npSamps[jdata.cpmmNumber] - 1;
    }
    else {
      samp0 = jdata.fpSamp0 + jdata.sampOffset;
      sampN = samp0 + sampleCount() - 1;
    }
    int line0(jdata.fpLine0 + jdata.lineOffset), lineN(line0 + lineCount() - 1);

//  Allocate a new coordinate sequence and define it
    geos::geom::CoordinateSequence pts;
    pts.add(geos::geom::Coordinate(samp0, lineN));
    pts.add(geos::geom::Coordinate(sampN, lineN));
    pts.add(geos::geom::Coordinate(sampN, line0));
    pts.add(geos::geom::Coordinate(samp0, line0));
    pts.add(geos::geom::Coordinate(samp0, lineN));


//  Make this reentrant and delete previous one if it exists and get the
//  new one
    delete fpGeom;
    fpGeom = geosFactory->createPolygon(geosFactory->createLinearRing(pts)).release();
    return;
  }

  HiJitCube::Corners HiJitCube::FocalPlaneToImage(const Corners &fp) const {
    Corners image;
    if(fp.good) {
      double samp0;
//  Compute sample and line coordinates in the focal plane
      if(originst) {
        samp0 = 0;
      }
      else {
        samp0 = jdata.fpSamp0 + jdata.sampOffset;
      }
      double line0(jdata.fpLine0 + jdata.lineOffset);

      image.topLeft.sample = fp.topLeft.sample - samp0 + 1.0;
      image.topLeft.line   = fp.topLeft.line   - line0 + 1.0;
      image.lowerRight.sample = fp.lowerRight.sample - samp0 + 1.0;
      image.lowerRight.line   = fp.lowerRight.line   - line0 + 1.0;
      image.good = true;
    }
    return (image);
  }

  HiJitCube::Corners HiJitCube::getCorners(const geos::geom::Geometry &poly) const {
    Corners corners;
    if((poly.isValid()) && (!poly.isEmpty())) {

//  Get the coordinate list
      geos::geom::CoordinateSequence *clist = poly.getCoordinates().release();
      const geos::geom::CoordinateXY *minpt = clist->minCoordinate();
//    cout << "MinPoint: " << minpt->x << ", " << minpt->y << std::endl;

      geos::geom::Coordinate maxpt(clist->getAt(0));
      for(unsigned int i = 1 ; i < clist->getSize() ; i++) {
        geos::geom::Coordinate pt = clist->getAt(i);
        if((pt.x >= maxpt.x) && (pt.y >= maxpt.y)) maxpt = pt;
      }
//    cout << "MaxPoint: " << maxpt.x << ", " << maxpt.y << std::endl;

      corners.topLeft.sample    = minpt->x;
      corners.topLeft.line      = minpt->y;
      corners.lowerRight.sample = maxpt.x;
      corners.lowerRight.line   = maxpt.y;
      corners.good = true;

      delete clist;
    }
    return (corners);
  }


}
