//************************************************************************
// See Full documentation in isis2fits.xml
//************************************************************************
#include "Isis.h"

#include <iostream>
#include <sstream>

#include "IException.h"
#include "IString.h"
#include "ProcessExport.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "UserInterface.h"

using namespace std;
using namespace Isis;

// Global variables

int g_headerBytes = 0;

QString FitsKeyword(QString keyword, bool isVal, QString value, QString unit = "");

QString WritePvl(QString fitsKey, QString group, QString key, Cube *icube, bool isString);

// Main program
void IsisMain() {

  // Create an object for exporting Isis data
  ProcessExport p;
  // Open the input cube
  Cube *icube = p.SetInputCube("FROM");

  // Conform to the Big-Endian format for FITS
  if (IsLsb()) p.SetOutputEndian(Isis::Msb);

  // Generate the name of the fits file and open it
  UserInterface &ui = Application::GetUserInterface();

  // specify the bits per pixel
  QString bitpix;
  if (ui.GetString("BITTYPE") == "8BIT") bitpix = "8";
  else if (ui.GetString("BITTYPE") == "16BIT") bitpix = "16";
  else if (ui.GetString("BITTYPE") == "32BIT") bitpix = "-32";
  else {
    QString msg = "Pixel type of [" + ui.GetString("BITTYPE") + "] is unsupported";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  //  Determine bit size and calculate number of bytes to write
  //  for each line.
  if (bitpix == "8") p.SetOutputType(Isis::UnsignedByte);
  if (bitpix == "16") p.SetOutputType(Isis::SignedWord);
  if (bitpix == "-32") p.SetOutputType(Isis::Real);

  // determine core base and multiplier, set up the stretch
  PvlGroup pix = icube->label()->findObject("IsisCube").findObject("Core").findGroup("Pixels");
  double scale = std::stod(pix["Multiplier"][0]);
  double base = std::stod(pix["Base"][0]);

  if (ui.GetString("STRETCH") != "NONE" && bitpix != "-32") {
    if (ui.GetString("STRETCH") == "LINEAR") {
      p.SetInputRange();
    }
    else if (ui.GetString("STRETCH") == "MANUAL") {
      p.SetInputRange(ui.GetDouble("MINIMUM"), ui.GetDouble("MAXIMUM"));
    }

    // create a proper scale so pixels look like 32bit data.
    scale = ((p.GetInputMaximum() - p.GetInputMinimum()) *
             (p.GetOutputMaximum() - p.GetOutputMinimum()));

    // round off after 14 decimals to avoid system architecture differences
    scale = ((floor(scale * 1e14)) / 1e14);

    // create a proper zero point so pixels look like 32bit data.
    base = -1.0 * (scale * p.GetOutputMinimum()) + p.GetInputMinimum();
    // round off after 14 decimals to avoid system architecture differences
    base = ((floor(base * 1e14)) / 1e14);
  }


  // Write the minimal fits header   
  QString header;

  // specify that this file conforms to simple fits standard
  header += FitsKeyword("SIMPLE", true, "T");


  // specify the bits per pixel
  header += FitsKeyword("BITPIX", true, bitpix);

  // specify the number of data axes (2: samples by lines)
  int axes = 2;
  if (icube->bandCount() > 1) {
    axes = 3;
  }

  header += FitsKeyword("NAXIS", true, toString(axes));

  // specify the limit on data axis 1 (number of samples)
  header += FitsKeyword("NAXIS1", true, toString(icube->sampleCount()));

  // specify the limit on data axis 2 (number of lines)
  header += FitsKeyword("NAXIS2", true, toString(icube->lineCount()));

  if (axes == 3) {
    header += FitsKeyword("NAXIS3", true, toString(icube->bandCount()));
  }

  header += FitsKeyword("BZERO", true,  toString(base));

  header += FitsKeyword("BSCALE", true, toString(scale));

  // Sky and All cases
  if (ui.GetString("INFO") == "SKY" || ui.GetString("INFO") == "ALL") {

    //tjw:  IString msg changed to QString
    QString msg = "cube has not been skymapped";
    PvlGroup map;

    if (icube->hasGroup("mapping")) {
      map = icube->group("mapping");
      msg = QString::fromStdString(map["targetname"]);
    }
    // If we have sky we want it
    if (msg == "Sky") {
      double midRa = 0, midDec = 0;

      midRa = ((double)map["MaximumLongitude"] + (double)map["MinimumLongitude"]) / 2;

      midDec = ((double)map["MaximumLatitude"] + (double)map["MinimumLatitude"]) / 2;

      header += FitsKeyword("OBJCTRA", true, toString(midRa));

      // Specify the Declination
      header += FitsKeyword("OBJCTDEC", true, toString(midDec));

    }

    //tjw:  All strings in this block  placed inside QString constructors
    if (ui.GetString("INFO") == "ALL") {
      header += WritePvl(QString("INSTRUME"), QString("Instrument"),
                         QString("InstrumentId"), icube, true);
      header += WritePvl(QString("OBSERVER"), QString("Instrument"),
                         QString("SpacecraftName"), icube, true);
      header += WritePvl(QString("OBJECT  "), QString("Instrument"),
                         QString("TargetName"), icube, true);
      // StartTime is sometimes middle of the exposure and somtimes beginning,
      // so StopTime can't be calculated off of exposure reliably.
      header += WritePvl(QString("DATE-OBS"), QString("Instrument"),
                         QString("StartTime"), icube, true);
      // Some cameras don't have StopTime
      if (icube->hasGroup("Instrument")) {
        PvlGroup inst = icube->group("Instrument");
        if (inst.hasKeyword("StopTime")) {
          header += WritePvl(QString("TIME_END"), QString("Instrument"),
                             QString("StopTime"), icube, true);
        }
        if (inst.hasKeyword("ExposureDuration")) {
          header += WritePvl(QString("EXPTIME"), QString("Instrument"),
                             QString("ExposureDuration"), icube, false);
        }
      }
    }
    // If we were set on SKY and Sky doesn't exist
    else if (msg != "Sky") {
      throw IException(IException::User, msg, _FILEINFO_);
    }
  }

  // signal the end of the header
  header += FitsKeyword("END", false, "");

  // fill the rest of the fits header with space so to conform with the fits header
  // size of 2880 bytes
  for (int i = header.length() % 2880 ; i < 2880 ; i++) header += " ";

  // open the cube for writing
  QString to = ui.GetFileName("TO", "fits");
  ofstream fout;
  fout.open(to.toLatin1().data(), ios::out | ios::binary);
  if (!fout.is_open()) {
    std::string msg = "Cannot open fits output file";
    throw IException(IException::Programmer, msg, _FILEINFO_);
  }

  fout.seekp(0);
  fout.write(header.toLatin1().data(), header.length());
  // write the raw cube data
  p.StartProcess(fout);

  // Finish off data area to a number n % 2880 == 0 is true
  // 2880 is the size of the data blocks
  int count = 2880 - (fout.tellp() % 2880);
  for (int i = 0; i < count; i++) {
    // Write nul characters as needed. ascii 0, hex 00...
    fout.write("\0", 1);
  }
  fout.close();
  p.EndProcess();
}

QString FitsKeyword(QString key, bool isValue, QString value, QString unit) {
  // pad the keyword with space
  for (int i = key.length() ; i < 8 ; i++)
    key += " ";

  // add the value indicator (or lack thereof)
  if (isValue)
    key += "= ";
  else
    key += "  ";

  // right-justify the value
  if (value.length() < 70) {
    // pad the left part of the value with space
    for (int i =  value.length() ; i < 20 ; i++)
      key += " ";

    // add the actual value
    key += value;
    if (isValue) {
      key += " / ";
      if (unit != "") {
        key += "[" + unit + "]";
      }
    }

    // finish the line by padding the rest of the space
    for (int i =  key.length() ; i < 80 ; i++)
      key += " ";
  }

  // record the total length of the header
  g_headerBytes += key.length();
  return key;
}

//tjw:  IString key -> QString key
QString WritePvl(QString fitsKey, QString group, QString key, Cube *icube, bool isString) {
  if (icube->hasGroup(group)) {
    PvlGroup theGroup = icube->group(group);
    QString name = QString::fromStdString(theGroup[key.toStdString()]);
    if (isString) {
      name = "'" + name + "'";
    }
    QString unit = QString::fromStdString(theGroup[key.toStdString()].unit());
    return FitsKeyword(fitsKey, true, name, unit);
  }
  return NULL;
}
