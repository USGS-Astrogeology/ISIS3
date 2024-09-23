/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"
#include "Chip.h"
#include "UserInterface.h"
#include "FileName.h"
#include "Pvl.h"
#include "IString.h"
#include "IException.h"
#include "iTime.h"
#include "Apollo.h"
#include "PvlTranslationTable.h"
#include "Statistics.h"
#include "ProcessImportPds.h"

#include <cstdio>

using namespace std;
using namespace Isis;

bool FindCode();
void CalculateTransform();
void RefineCodeLocation();
void TranslateCode();
void TranslateApolloLabels (IString filename, Cube *pack);
bool IsValidCode();
QString FrameTime();
int Altitude();
double ShutterInterval();
QString FMC();

int codeSample, codeLine;
bool code[4][32];
bool codeFound = true;

Cube cube;
Apollo *apollo;
QString utcTime;

int RADIUS = 46;
double rotation, sampleTranslation, lineTranslation;

// Main program
void IsisMain() {
  ProcessImportPds p;
  Pvl pdsLabel;
  UserInterface &ui = Application::GetUserInterface();
  FileName inFile = ui.GetFileName("FROM").toStdString();

  p.SetPdsFile(QString::fromStdString(inFile.expanded()), "", pdsLabel);

  QString filename = QString::fromStdString(FileName(ui.GetFileName("FROM").toStdString()).baseName());
  FileName toFile = ui.GetCubeName("TO").toStdString();

  apollo = new Apollo(filename);

  utcTime = QString::fromStdString(pdsLabel["START_TIME"]);

  // Setup the output cube attributes for a 16-bit unsigned tiff
  Isis::CubeAttributeOutput cao;
  cao.setPixelType(Isis::Real);
  p.SetOutputCube(QString::fromStdString(toFile.expanded()), cao);

  // Import image
  p.StartProcess();
  p.EndProcess();

  cube.open(QString::fromStdString(toFile.expanded()), "rw");

  // Once the image is imported, we need to find and decrypt the code
  if (apollo->IsMetric() && FindCode())
    TranslateCode();

  CalculateTransform();
  // Once we have decrypted the code, we need to populate the image labels
  TranslateApolloLabels(filename.toStdString(), &cube);
  cube.close();
}

// Find the location of the code
bool FindCode() {
  // start by looking for a high value pixel
  int CENTER_SAMPLE = 1030,
       CENTER_LINE = 21350,
       DELTAX = 1000,
       DELTAY = 250;
  Chip chip(2*DELTAX+2*RADIUS, 2*DELTAY+4*RADIUS);
  chip.TackCube(CENTER_SAMPLE+RADIUS, CENTER_LINE+2*RADIUS);
  chip.Load(cube);
  for (int i=1; i<chip.Samples()-6*RADIUS-1; i+=RADIUS/8) {
    for (int j=1; j<=chip.Lines()-4*RADIUS-1; j+=RADIUS/8) {
      if ( chip.GetValue(i,j) > 60000 && chip.GetValue(i+2*RADIUS, j) > 60000 &&
                  chip.GetValue(i+2*RADIUS, j+2*RADIUS) < 60000 &&
                  chip.GetValue(i+2*RADIUS, j+4*RADIUS) > 60000)  {
        // Now that we've found one, let's refine the location
        codeFound = true;
        codeSample = CENTER_SAMPLE - DELTAX + i;
        codeLine = CENTER_LINE - DELTAY + j;
        RefineCodeLocation();
        return true;
      }
    }
  }

  // If the code was not found, we will use the default location
  codeFound = false;
  codeSample = CENTER_SAMPLE;
  codeLine = CENTER_LINE;
  return false;
}

// Calculate the translation and rotation of the scan
void CalculateTransform() {
  if (!apollo->IsMetric() || !codeFound) {
    sampleTranslation = 0.0;
    lineTranslation = 0.0;
    rotation = 0.0;
    return;
  }
  double sampleUL, lineUL,
      sampleUR, lineUR,
      sampleLL, lineLL,
      sampleLR, lineLR;

  RefineCodeLocation();
  sampleUL = codeSample;
  lineUL = codeLine;
  codeSample += 6*RADIUS;
  RefineCodeLocation();
  sampleUR = codeSample;
  lineUR = codeLine;
  codeLine += 62*RADIUS;
  RefineCodeLocation();
  sampleLR = codeSample;
  lineLR = codeLine;
  codeSample -= 6*RADIUS;
  RefineCodeLocation();
  sampleLL = codeSample;
  lineLL = codeLine;

  if (code[0][0] == 1 && code[0][31] ==1 && code[3][0] == 1 && code[3][31] == 1) {
  rotation = -1*atan((sampleLR-sampleUR+sampleLL-sampleUL)/(lineLR-lineUR+lineLL-lineUL)) - 0.00281891751001 - 0.000648055741779;
  sampleTranslation = (sampleUL+sampleUR+sampleLL+sampleLR)/4;
  lineTranslation = (lineUL+lineUR+lineLL+lineLR)/4;
  }
  else if (code[0][0] == 1 && code[0][31] == 1) {
          rotation = -1*atan((sampleLL-sampleUL)/(lineLL-lineUL)) - 0.00281891751001 - 0.000648055741779;
          sampleTranslation = (sampleUL + sampleLL)/2 + 3*RADIUS;
          lineTranslation = (lineUL + lineLL)/2;
  }
  else {
          rotation = -0.00281891751001 - 0.000648055741779;
          sampleTranslation = 600;
          lineTranslation = 22700;
  }

  if (apollo->IsApollo17())
          rotation += 0.0111002410269388;
}

void RefineCodeLocation() {
  Chip chip(2*RADIUS, 2*RADIUS);
  chip.TackCube(codeSample, codeLine);
  chip.Load(cube);
  int bestSample = 0,
      bestLine = 0;
  double max = 0.0;
  for (int s = RADIUS/2+1; s < 3*RADIUS/2; s++) {
    for (int l = RADIUS/2+1; l < 3*RADIUS/2; l++) {
      double value = 0.0;
      // Run a quick gaussian-esch filter
      for (int x=RADIUS/-2; x<=RADIUS/2; x++) {
        for (int y=RADIUS/-2; y<=RADIUS/2; y++) {
          //value += chip(s+x,l+y)/Exp(x*x + y*y);
          if (sqrt(x*x + y*y) < RADIUS/2)
            value += chip.GetValue(s+x,l+y);
        }
      }
      if (value > max) {
        bestSample = s;
        bestLine = l;
        max = value;
      }
    }
  }

  codeSample = codeSample - (chip.Samples()+1)/2 + bestSample;
  codeLine = codeLine - (chip.Lines()+1)/2 + bestLine;
}

// translate the code once it is found
void TranslateCode() {
  // read the code from the image
  Chip chip(8*RADIUS, 64*RADIUS);
  chip.TackCube(codeSample+3*RADIUS, codeLine+31*RADIUS);
  chip.Load(cube);
  for (int j=0; j<32; j++) {
    for (int i=0; i<4; i++) {
      Statistics stats;
      // Get the average of the subchip
      for (int x=1; x<=2*RADIUS; x++) {
        for (int y=1; y<=2*RADIUS; y++) {
          stats.AddData(chip.GetValue(i*2*RADIUS + x,j*2*RADIUS + y));
        }
      }
      // see if it is on or off
      if (stats.Average() > 20000)
        code[i][31-j] = true;
      else code[i][31-j] = false;
    }
  }

  for (int j=0; j<32; j++) {
    for (int i=0; i<4; i++) {
    }
  }
}

// Populate cube label using filname and film code
// Code decrypted as specified in film decoder document (July 23, 1971 Revision)
//     available at ASU Apollo Resources archive
void TranslateApolloLabels (IString filename, Cube *opack) {
  //Instrument group
  PvlGroup inst("Instrument");
  PvlGroup kern("Kernels");
  PvlGroup codeGroup("Code");

  inst += PvlKeyword("SpacecraftName", apollo->SpacecraftName().toStdString());
  inst += PvlKeyword("InstrumentId", apollo->InstrumentId().toStdString());
  inst += PvlKeyword("TargetName", apollo->TargetName().toStdString());

  if ( !IsValidCode() ){
    PvlGroup error("ERROR");
    error.addComment("The decrypted code is invalid.");
    for (int i=0; i<4; i++) {
      PvlKeyword keyword("Column"+Isis::toString(i+1));
      for (int j=0; j<32; j++) {
        keyword += Isis::toString((int)code[i][j]);
      }
      error.addKeyword(keyword);
      codeGroup += keyword;
    }
    Application::Log(error);
  }
  else {
    codeGroup += PvlKeyword("StartTime", FrameTime().toStdString());
    codeGroup += PvlKeyword("SpacecraftAltitude", Isis::toString(Altitude()),"meters");

    if (apollo->IsMetric()){
      codeGroup += PvlKeyword("ExposureDuration", Isis::toString(ShutterInterval()), "milliseconds");
      codeGroup += PvlKeyword("ForwardMotionCompensation", FMC().toStdString());
    }

    for (int i=0; i<4; i++) {
      PvlKeyword keyword("Column"+ Isis::toString(i+1));
      for (int j=0; j<32; j++) {
        keyword += Isis::toString((int)code[i][j]);
      }
      codeGroup += keyword;
    }
  }

  PvlGroup bandBin("BandBin");
  // There are no filters on the camera, so the default is clear with id # of 1
  // the BandBin group is only included to work with the spiceinit application
  bandBin += PvlKeyword("FilterName", "CLEAR");
  bandBin += PvlKeyword("FilterId", "1");

  kern += PvlKeyword("NaifFrameCode", apollo->NaifFrameCode().toStdString());

  // Set up the nominal reseaus group
  Isis::PvlGroup &dataDir = Isis::Preference::Preferences().findGroup("DataDirectory");
  Process p;
  PvlTranslationTable tTable("$ISISROOT/appdata/translations/MissionName2DataDir.trn");
  QString missionDir = QString::fromStdString(dataDir[tTable.Translate("MissionName", apollo->SpacecraftName()).toStdString()][0]);
  Pvl resTemplate(missionDir.toStdString() + "/reseaus/" + apollo->InstrumentId().toStdString() + "_NOMINAL.pvl");
  PvlGroup *reseaus = &resTemplate.findGroup("Reseaus");

  // Update reseau locations based on refined code location
  for (int i=0; i<(reseaus->findKeyword("Type")).size(); i++) {
    double x = Isis::toDouble(reseaus->findKeyword("Sample")[i]) + sampleTranslation + 2278,
           y = Isis::toDouble(reseaus->findKeyword("Line")[i]) + lineTranslation - 20231;

    if (apollo->IsApollo17()) {
        x += 50;
        y += 20;
    }

    reseaus->findKeyword("Sample")[i] = Isis::toString(
        cos(rotation)*(x-sampleTranslation) - sin(rotation)*(y-lineTranslation) + sampleTranslation);
    reseaus->findKeyword("Line")[i] = Isis::toString(
        sin(rotation)*(x-sampleTranslation) + cos(rotation)*(y-lineTranslation) + lineTranslation);
  }
  inst += PvlKeyword("StartTime", utcTime.toStdString());

  opack->putGroup(inst);
  opack->putGroup(bandBin);
  opack->putGroup(kern);
  opack->putGroup(*reseaus);
  opack->putGroup(codeGroup);
}

// Validates the code based on February 1971 Revision
bool IsValidCode() {
  return codeFound && ( code[0][0] || !code[0][3] || !code[0][5] || code[0][10] ||  !code[0][11] || !code[0][12] || code[0][15] || !code[0][16] || code[0][21] || !code[0][22] || !code[0][26] || code[0][31] ||
                         code[1][0] || !code[1][6] || !code[1][11] || code[1][12] || !code[1][13] || !code[1][14] || code[1][15] || !code[1][28] || code[1][29] || !code[1][30] || code[1][31] ||
                         code[2][0] || code[2][15] || !code[2][21] || code[2][22] || !code[2][23] || !code[2][24] || !code[2][25] || !code[2][26] || !code[2][27] || !code[2][28] || !code[2][29] || !code[2][30] || code[2][31] )
                   && (!(apollo->IsMetric()) || (code[3][0] || !code[3][13] || !code[3][14] || code[3][15] || !code[3][16] || !code[3][17] || !code[3][18] || !code[3][19] || !code[3][20] || !code[3][21] ||
                                                   !code[3][22] || !code[3][23] || !code[3][24] || !code[3][25] || !code[3][26] || !code[3][27] || !code[3][28] || !code[3][30] || code[3][31]));
}

QString FrameTime() {
  iTime launch = apollo->LaunchDate();
    int year = launch.Year(),
        month = launch.Month(),
        days = launch.Day(),
        hours = launch.Hour(),
        minutes = launch.Minute();
    double seconds = launch.Second();
    for (int i=0; i<5; i++) {
      if (i<4) days += (int)pow(2.0, i)*code[0][9-i];
      else days += 10*(int)pow(2.0, i-4)*code[0][8-i];
    }

    for (int i=0; i<6; i++) {
      if (i<4) hours += (int)pow(2.0, i)*code[0][20-i];
      else hours += 10*(int)pow(2.0, i-4)*code[0][18-i];
    }

    for (int i=0; i<7; i++) {
      if (i<4) minutes += (int)pow(2.0, i)*code[0][30-i];
      else minutes += 10*(int)pow(2.0, i-4)*code[0][29-i];
    }

    for (int i=0; i<7; i++) {
      if (i<4) seconds += (int)pow(2.0, i)*code[1][10-i];
      else seconds += 10*(int)pow(2.0, i-4)*code[1][9-i];
    }

    // And the milliseconds
    for (int i=0; i<12; i++) {
      if (i<4) seconds += 0.001* pow(2.0, i)*code[1][27-i];
      else if (i<8) seconds += 0.01*pow(2.0, i-4)*code[1][27-i];
      else seconds += 0.1*pow(2.0, i-8)*code[1][27-i];
    }

    // Perform checks to make sure the time is correct
    //   (i.e. convert 60 seconds to a minute, etc.)
    if (seconds > 60) {
      seconds -= 60;
      minutes += 1;
    }
    if (minutes >= 60) {
      minutes -= 60;
      hours += 1;
    }
    if ( hours >= 24) {
      hours -= 24;
      days +=1;
    }
    // This last check will only affect Apollo 15 which launched in July (31 days) and landed in August
    // Both Apollo 16 and Apollo 17 launched and landed in the same month
    if (days > 31){
      days -= 31;
      month += 1;
    }

    QString sTime = QString::number(year) + "-";
    if (month < 10) sTime += "0";
    sTime += QString::number(month)+ "-";
    if (days <10) sTime += "0";
    sTime += QString::number(days) + "T";
    if (hours <10) sTime += "0";
    sTime += QString::number(hours) + ":";
    if (minutes <10) sTime += "0";
    sTime += QString::number(minutes) + ":";
    if (seconds <10) sTime += "0";
    sTime += QString::number(seconds);

    return sTime;
}

int Altitude() {
  int altitude = 0;
  for (int i=0; i<18; i++) {
    if (i<5) altitude += (int)pow(2.0, i)*code[2][20-i];
    else altitude += (int)pow(2.0, i)*code[2][19-i];
  }
  return altitude;
}

double ShutterInterval() {
  int shutterInterval = 0;
  for (int i=0; i<10; i++) {
    shutterInterval += (int)pow(2.0, i)*code[3][12-i];
  }
  return 0.1*shutterInterval;
}

QString FMC() {
  if (code[3][29]) return "True";
  return "False";
}
