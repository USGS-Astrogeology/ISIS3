/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

#include "Brick.h"
#include "Camera.h"
#include "ControlMeasure.h"
#include "ControlNet.h"
#include "ControlPoint.h"
#include "Cube.h"
#include "Distance.h"
#include "ID.h"
#include "IException.h"
#include "Latitude.h"
#include "Longitude.h"
#include "Progress.h"
#include "SerialNumber.h"
#include "SpecialPixel.h"
#include "SurfacePoint.h"
#include "UserInterface.h"

using namespace std;
using namespace Isis;

void IsisMain() {

  UserInterface &ui = Application::GetUserInterface();

  ControlNet cnet;
  if (ui.WasEntered("NETWORKID")) {
    cnet.SetNetworkId(ui.GetString("NETWORKID"));
  }
  if (ui.WasEntered("DESCRIPTION")) {
    cnet.SetDescription(ui.GetString("DESCRIPTION"));
  }
  cnet.SetUserName(Application::Name());

  QString filename = ui.GetCubeName("FROM");
  Cube inputCube;
  inputCube.open(filename, "r");

  QString locFilename = ui.GetCubeName("LOC");
  Cube locCube;
  locCube.open(locFilename, "r");

  if (inputCube.label()->hasKeyword("TargetName", PvlObject::Traverse)) {
    PvlGroup inst = inputCube.label()->findGroup("Instrument", PvlObject::Traverse);
    QString targetName = QString::fromStdString(inst["TargetName"]);
    cnet.SetTarget(targetName);
  }
  else {
    QString msg = "Input cube does not have target.";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  //  Create serial number list
  QString serialNumber = SerialNumber::Compose(inputCube);

  int sampInc = ui.GetInteger("SAMPLEINC");
  int lineInc = ui.GetInteger("LINEINC");

  // Set up an automatic id generator for the point ids
  ID pointId = ID(ui.GetString("POINTID"));

//Progress gridStatus;
//
//int maxSteps = (inputCube.sampleCount() / sampInc + 1) * (inputCube.lineCount() / lineInc + 1)+100;
//
//if (maxSteps > 0) {
//  gridStatus.SetMaximumSteps(maxSteps);
//  gridStatus.SetText("Creating Ground Points");
//  gridStatus.CheckStatus();
//}

  //  Set up brick to read a line from the LOC file
  Brick locBrick(locCube, locCube.sampleCount(), 1, 3);

  for (int line = 0; line < inputCube.lineCount(); line += lineInc) {
    locBrick.SetBasePosition(1, line + 1, 1);
    locCube.read(locBrick);
    for (int samp = 0; samp < inputCube.sampleCount(); samp += sampInc) {
      qDebug()<<"samp : line = "<<samp<<" : "<<line;
      ControlPoint *point = new ControlPoint(pointId.Next());
      point->SetId(pointId.Next());
      point->SetType(ControlPoint::Fixed);
      double lon = locBrick.at(samp);
      double lat = locBrick.at(locCube.sampleCount() + samp);
      double radius = locBrick.at((locCube.sampleCount() * 2) + samp);

      if (!IsValidPixel(lon) || !IsValidPixel(lat) || !IsValidPixel(radius)) {
        continue;
      }

      try {
        SurfacePoint pt(Latitude(lat, Angle::Degrees),
                        Longitude(lon, Angle::Degrees),
                        Distance(radius, Distance::Meters));
        point->SetAprioriSurfacePoint(pt);
      }
      catch (IException &e) {
        continue;
      }

      ControlMeasure *measure = new ControlMeasure;
      measure->SetCubeSerialNumber(serialNumber);
      measure->SetCoordinate(samp + 1, line + 1);
      measure->SetType(ControlMeasure::Candidate);
      measure->SetDateTime();
      measure->SetChooserName(Application::Name());
      point->Add(measure);

      cnet.AddPoint(point);

      //  Make sure last sample is always included
      if ((samp != (inputCube.sampleCount() - 1)) && ((samp + sampInc) >= inputCube.sampleCount())) {
        samp = inputCube.sampleCount() - sampInc - 1;
        qDebug()<<"lastSamp = "<<samp;
      }

//      gridStatus.CheckStatus();
    }
    //  Make sure last line is always included
    if ((line != (inputCube.lineCount() - 1)) && ((line + lineInc) >= inputCube.lineCount())) {
      line = inputCube.lineCount() - lineInc - 1;
      qDebug()<<"lastLine = "<<line;
    }
  }

  cnet.Write(ui.GetFileName("ONET"));
}
