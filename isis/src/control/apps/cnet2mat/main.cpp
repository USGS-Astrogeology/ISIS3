/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

#include <map>
#include <iomanip>
#include <sstream>

#include "Application.h"
#include "ControlMeasure.h"
#include "ControlNet.h"
#include "ControlPoint.h"
#include "FileList.h"
#include "IException.h"
#include "IString.h"
#include "Pvl.h"
#include "PvlObject.h"
#include "SerialNumberList.h"
#include "SpecialPixel.h"
#include "TextFile.h"
#include "UserInterface.h"

using namespace std;
using namespace Isis;

void IsisMain() {

  std::map <int, QString> fscMap;
  std::map <QString, int> snMap;

  UserInterface &ui = Application::GetUserInterface();

  FileList list2(ui.GetFileName("LIST2").toStdString());

  SerialNumberList snl(ui.GetFileName("LIST3"));
  for (int f = 0; f < list2.size(); f++) {
    QString currFile(QString::fromStdString(list2[f].toString()));
    Pvl lab(currFile.toStdString());
    PvlObject qube(lab.findObject("QUBE"));
    QString fsc;
    if(qube.hasKeyword("IMAGE_NUMBER")) {
      fsc = QString::fromStdString(qube.findKeyword("IMAGE_NUMBER")[0]);
    }
    else if(qube.hasKeyword("IMAGE_ID")) {
      fsc = QString::fromStdString(qube.findKeyword("IMAGE_ID")[0]);
    }
    else {
      std::string msg = "Unable to find keyword [\"IMAGE_NUMBER\" or \"IMAGE_ID\"] in file [";
      msg += fsc.toStdString() + "]";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    QString sn(snl.serialNumber(f));
    fscMap.insert(std::pair<int, QString>(f, fsc));
    snMap.insert(std::pair<QString, int>(sn, f));
  }

  ControlNet cnet(ui.GetFileName("CNET"));

  int mpTotal = 0;

  for(int i = 0; i < cnet.GetNumPoints(); i++) {
    mpTotal += cnet.GetPoint(i)->GetNumMeasures();
  }

  TextFile mpFile(ui.GetFileName("MATCH"), "Overwrite", "");

  ostringstream str;
  QString textLine;

  textLine = "Matchpoint total =    ";
  textLine += QString::number(mpTotal);
  mpFile.PutLine(textLine);
  str.clear();
  str.str("");
  str.width(40);
  str.setf(ios::left);
  str << "Point ID";
  textLine = str.str().c_str();

  str.clear();
  str.str("");
  str.width(7);
  str.setf(ios::left);
  str << "FSC";
  textLine += str.str().c_str();

  str.clear();
  str.str("");
  str.width(8);
  str.setf(ios::left);
  str << "LINE";
  textLine += str.str().c_str();

  str.clear();
  str.str("");
  str.width(5);
  str.setf(ios::left);
  str << "SAMP";
  textLine += str.str().c_str();


  str.clear();
  str.str("");
  str.width(14);
  str.setf(ios::left);
  str << "CLASS";
  textLine += str.str().c_str();

  str.clear();
  str.str("");
  str.width(8);
  str.setf(ios::left);
  str << "DIAMETER";
  textLine += str.str().c_str();

  mpFile.PutLine(textLine);

  // Loop for each point in the control network
  for(int i = 0; i < cnet.GetNumPoints(); i++) {
    ControlPoint *currPoint = cnet.GetPoint(i);

    // Loop for each measure in the control point
    for(int m = 0; m < currPoint->GetNumMeasures(); m++) {
      ostringstream formatter;
      ControlMeasure *currMeas = currPoint->GetMeasure(m);

      //Set Point ID
      formatter.clear();
      formatter.str("");
      formatter.width(30);
      formatter.setf(ios::left);
      formatter << currPoint->GetId().toStdString() << " ";
      textLine = formatter.str().c_str();

      //Set FSC
      formatter.clear();
      formatter.str("");
      formatter.width(12);
      formatter.setf(ios::right);
      QString sn = currMeas->GetCubeSerialNumber();
      QString fsc = fscMap[snMap[sn]];
      formatter << fsc.toStdString() << " ";
      textLine += formatter.str().c_str();

      //Set Line
      formatter.clear();
      formatter.str("");
      formatter.width(7);
      formatter.setf(ios::right);
      formatter.setf(ios::fixed);
      formatter.precision(2);
      formatter << currMeas->GetLine() << " ";
      textLine += formatter.str().c_str();

      //Set Sample
      formatter.clear();
      formatter.str("");
      formatter.width(7);
      formatter.setf(ios::right);
      formatter.setf(ios::fixed);
      formatter.precision(2);
      formatter << currMeas->GetSample() << "   ";
      textLine += formatter.str().c_str();

      //Set Class
      QString ptClass;
      ControlMeasure::MeasureType mType = currMeas->GetType();

      if(currMeas->IsIgnored() || currPoint->IsIgnored()) {
        ptClass = "U   "; //Unmeasured
      }
      else if(currPoint->GetRefMeasure() == currMeas) {
        ptClass = "T   "; //Truth
      }
      else if(mType == ControlMeasure::RegisteredSubPixel) {
        ptClass = "S   "; //SubPixel
      }
      else if(mType == ControlMeasure::RegisteredPixel
              || mType == ControlMeasure::Manual) {
        ptClass = "M   "; //Measured
      }
      else { // if(mType == ControlMeasure::Candidate) {
        ptClass = "A   "; //Approximate
      }
      textLine += ptClass;

      //Set Diameter
      formatter.clear();
      formatter.str("");
      formatter.width(16);
      formatter.setf(ios::right);
      IString diam;
      if(currMeas->GetDiameter() == Isis::Null) {
        diam = 0.0;
      }
      else {
        diam = currMeas->GetDiameter();
      }
      formatter << diam;
      textLine += formatter.str().c_str();

      mpFile.PutLine(textLine);
    }
  }
}
