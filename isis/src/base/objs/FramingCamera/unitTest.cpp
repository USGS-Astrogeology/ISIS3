/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <iostream>

#include "FramingCamera.h"
#include "IException.h"
#include "iTime.h"
#include "Preference.h"
#include "Pvl.h"
#include "PvlGroup.h"

using namespace std;
using namespace Isis;

class MyCamera : public FramingCamera {
  public:
    MyCamera(Cube &cube) : FramingCamera(cube) { }

    /**
     * This is a pure virtual method from the Camera parent class and must be
     * overriden.
     */
    virtual int CkFrameId() const {
      string msg = "CK Frame ID is unqiue to mission-specific cameras";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }

    /**
     * This is a pure virtual method from the Camera parent class and must be
     * overriden.
     */
    virtual int CkReferenceId() const {
      string msg = "CK Reference ID is unique to mission-specific cameras";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }

    /**
     * This is a pure virtual method from the Camera parent class and must be
     * overriden.
     */
    virtual int SpkReferenceId() const {
      string msg = "SPK Reference ID is unique to mission-specific cameras";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }
    
    // These are pure virtual within Sensor that must be overriden
    virtual QString instrumentNameLong() const { return QString("Framing"); }
    virtual QString instrumentNameShort() const { return QString("F"); }
    virtual QString spacecraftNameLong() const { return QString("Framing 1"); }
    virtual QString spacecraftNameShort() const { return QString("F1"); }

    /**
     * This is a pure virtual method from the FramingCamera class and must be
     * overriden. We will use the default implementation from the FramingCamera
     * class.
     * @author Jeannie Walldren
     * @internal
     *   @history 2011-05-03 Jeannie Walldren - Original version.
     */
    pair <iTime, iTime> ShutterOpenCloseTimes(double exposureDuration,
                                              double time) {
      return FramingCamera::ShutterOpenCloseTimes(exposureDuration, time);
    }
};

int main() {
  Preference::Preferences(true);
  //NOTE: The following cube is not from a framing camera.  The test returns
  //true for framing camera type since MyCamera is a child class of FramingCamera
  try {
    Cube cube("$ISISTESTDATA/isis/src/base/unitTestData/ab102401_ideal.cub", "r");
    MyCamera cam(cube);

    // test camera type
    cout << "Camera = Framing?   " << (cam.GetCameraType() == Camera::Framing) << endl;
    cout << "Camera = LineScan?  " << (cam.GetCameraType() == Camera::LineScan) << endl;
    cout << "Camera = PushFrame? " << (cam.GetCameraType() == Camera::PushFrame) << endl;
    cout << "Camera = Radar?     " << (cam.GetCameraType() == Camera::Radar) << endl;

    // test ShutterOpenCloseTimes() method
    PvlGroup inst = cube.label()->findGroup("Instrument", Pvl::Traverse);
    QString startTime = QString::fromStdString(inst["StartTime"]);
    double eTime = 0.0;
    utc2et_c(startTime.toLatin1().data(), &eTime);
    double expoDur = ((double) inst["ExposureDuration"])/1000; // in seconds
    pair <iTime, iTime> octime = cam.ShutterOpenCloseTimes(eTime,expoDur);
    cout.precision(12);
    cout << "StartTime                 =      " << startTime << endl;
    cout << "StartTime (et)            =      " << eTime << endl;
    cout << "ExposureDuration (in sec) =      " << expoDur << endl;
    cout << "shutter open              =      " << octime.first.Et() << endl;
    cout << "shutter close             =      " << octime.second.Et() << endl;
  }
  catch (IException &e) {
    cout << endl << endl;
    IException(e, IException::Programmer,
              "\n------------FramingCamera Unit Test Failed.------------",
              _FILEINFO_).print();
  }
}
