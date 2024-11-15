/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "VimsCamera.h"
#include "VimsGroundMap.h"
#include "VimsSkyMap.h"

#include <fstream>
#include <iostream>
#include <iomanip>

#include <QDebug>
#include <QList>
#include <QPointF>
#include <QString>

#include "CameraDetectorMap.h"
#include "CameraDistortionMap.h"
#include "CameraFocalPlaneMap.h"
#include "Constants.h"
#include "FileName.h"
#include "IException.h"
#include "IString.h"
#include "iTime.h"
#include "NaifStatus.h"
#include "SpecialPixel.h"

using namespace std;

namespace Isis {
  /**
   * Constructor for the Cassini Vims Camera Model
   *
   * @param [in] lab   (Pvl &)  Label used to create camera model
   *
   * @internal
   *   @history 2007-12-12  Tracie Sucharski,  After creating spice cache with
   *                           padding, reset et by calling SetImage(1,1) so that
   *                           et is initialized properly at beginning of image
   *                           without padding.
   * @internal
   *   @history 2011-05-03 Jeannie Walldren - Added NAIF error check.
   *   @history 2018-03-14 Adam Goins - Changed cache calculations with LoadCache() call.
   *                           This fixes an error where VimsCamera caused spiceinit to
   *                           fail when TargetName == SKY. Fixes #5353.
   *
   */
  VimsCamera::VimsCamera(Cube &cube) : Camera(cube) {
    m_instrumentNameLong = "Visible and Infrared Mapping Spectrometer";
    m_instrumentNameShort = "VIMS";
    m_spacecraftNameLong = "Cassini Huygens";
    m_spacecraftNameShort = "Cassini";

    NaifStatus::CheckErrors();

    Pvl &lab = *cube.label();
    PvlGroup &inst = lab.findGroup("Instrument", Pvl::Traverse);
    QString channel = (QString) inst ["Channel"];

    //  Vims pixel pitch is not always square, but ISISdoes not have the ability to store
    //  more than a single value for pixel pitch.  Member variables for pixelPitch x and y
    //  were created for proper calculation of ifov.
    if (channel == "VIS") {
      //LoadFrameMounting ("CASSINI_SC_COORD","CASSINI_VIMS_V");

      SetFocalLength(143.0);
      if (QString((QString)inst["SamplingMode"]).toUpper() == "NORMAL") {
        SetPixelPitch(3 * .024);
        // Should this .506?  According to 2002 paper ground calibration shows .506 +/- .003 mrad
        m_pixelPitchX = 0.024 * 3;
        m_pixelPitchY = 0.024 * 3;
      }
      else if (QString((QString)inst["SamplingMode"]).toUpper() == "HI-RES") {
        SetPixelPitch(.024);
        m_pixelPitchX = 0.024;
        m_pixelPitchY = 0.024;
      }
      else if (QString((QString)inst["SamplingMode"]).toUpper() == "UNDER") {
        QString msg = "Isis cannot process images with a SamplingMode = \"UNDER\" (or NYQUIST)";
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }
      else {
        QString msg = "Unknown SamplingMode [" + (QString) inst["SamplingMode"] + "]";
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }
    }
    else if (channel == "IR") {
      //LoadFrameMounting ("CASSINI_SC_COORD","CASSINI_VIMS_IR");

      SetFocalLength(426.0);
      SetPixelPitch(.2);
      if (QString((QString)inst["SamplingMode"]).toUpper() == "NORMAL") {
        m_pixelPitchX = 0.2;
        m_pixelPitchY = 0.2;
      }
      else if (QString((QString)inst["SamplingMode"]).toUpper() == "HI-RES") {
        m_pixelPitchX = 0.103;
        m_pixelPitchY = 0.2;
      }
      else if (QString((QString)inst["SamplingMode"]).toUpper() == "UNDER") {
        QString msg = "Isis cannot process images with a SamplingMode = \"UNDER\" (or NYQUIST)";
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }
      else {
        QString msg = "Unknown SamplingMode [" + (QString) inst["SamplingMode"] + "]";
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }
    }

    // Get the start time in et
    QString stime = inst ["NativeStartTime"][0];
    QString intTime = stime.split(".").first();
    stime = stime.split(".").last();

    // Get the end time in et
    QString etime = (QString) inst ["NativeStopTime"];
    intTime = etime.split(".").first();
    etime = etime.split(".").last();

    //  Setup detector map
    new CameraDetectorMap(this);

    // Setup focal plane map
    new CameraFocalPlaneMap(this, naifIkCode());

    // Setup distortion map
    new CameraDistortionMap(this);

    // Setup the ground and sky map
    new VimsGroundMap(this, lab);
    new VimsSkyMap(this, lab);

    ((VimsGroundMap *)GroundMap())->Init(lab);
    ((VimsSkyMap *)SkyMap())->Init(lab);

    LoadCache();

    IgnoreProjection(true);
    SetImage(1, 1);
    IgnoreProjection(false);
    NaifStatus::CheckErrors();
    return;
  }


  /**
   * Returns the pixel ifov offsets from center of pixel.  For vims this will be a rectangle or
   * square, depending on the sampling mode.  The first vertex is the top left.
   *
   * @internal
   *   @history 2013-08-09 Tracie Sucharski - Add more vertices along each edge.  This might need
   *                          to be a user parameter evenually?  Might be dependent on resolution.
   */
   QList<QPointF> VimsCamera::PixelIfovOffsets() {

     QList<QPointF> offsets;

     //  Create 100 pts on each edge of pixel
     int npts = 100;

     //  Top edge of pixel
     for (double x = -m_pixelPitchX / 2.0; x <= m_pixelPitchX / 2.0; x += m_pixelPitchX / (npts-1)) {
       offsets.append(QPointF(x, -m_pixelPitchY / 2.0));
     }
     //  Right edge of pixel
     for (double y = -m_pixelPitchY / 2.0; y <= m_pixelPitchY / 2.0; y += m_pixelPitchY / (npts-1)) {
       offsets.append(QPointF(m_pixelPitchX / 2.0, y));
     }
     //  Bottom edge of pixel
     for (double x = m_pixelPitchX / 2.0; x >= -m_pixelPitchX / 2.0; x -= m_pixelPitchX / (npts-1)) {
       offsets.append(QPointF(x, m_pixelPitchY / 2.0));
     }
     //  Left edge of pixel
     for (double y = m_pixelPitchY / 2.0; y >= -m_pixelPitchY / 2.0; y -= m_pixelPitchY / (npts-1)) {
       offsets.append(QPointF(-m_pixelPitchX / 2.0, y));
     }

     return offsets;
   }
}

// Plugin
/**
 * This is the function that is called in order to instantiate a VimsCamera object.
 *
 * @param lab Cube labels
 *
 * @return Isis::Camera* VimsCamera
 * @internal
 *   @history 2011-05-03 Jeannie Walldren - Added documentation.  Removed
 *            Cassini namespace.
 */
extern "C" Isis::Camera *VimsCameraPlugin(Isis::Cube &cube) {
  return new Isis::VimsCamera(cube);
}
