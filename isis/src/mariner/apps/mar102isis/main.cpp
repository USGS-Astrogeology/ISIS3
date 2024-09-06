/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

#include "Cube.h"
#include "FileName.h"
#include "IString.h"
#include "iTime.h"
#include "OriginalLabel.h"
#include "PixelType.h"
#include "ProcessImport.h"
#include "ProcessImportPds.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "PvlFormat.h"
#include "PvlFormatPds.h"

#include <sstream>

#include <QString>

using namespace std;
using namespace Isis;

void UpdateLabels(Cube *cube, const QString &labels);
void TranslateIsis2Labels(FileName &labelFile, Cube *oCube);
QString EbcdicToAscii(unsigned char *header);
QString DaysToDate(int days);

void IsisMain() {
  UserInterface &ui = Application::GetUserInterface();

  // Determine whether input is a raw Mariner 10 image or an Isis 2 cube
  bool isRaw = false;
  FileName inputFile = ui.GetFileName("FROM");
  Pvl label(inputFile.expanded());

  // If the PVL created from the input labels is empty, then input is raw
  if(label.groups() + label.objects() + label.keywords() == 0) {
    isRaw = true;
  }

  // Import for RAW files
  if(isRaw) {
    ProcessImport p;

    // All mariner images from both cameras share this size
    p.SetDimensions(832, 700, 1);
    p.SetFileHeaderBytes(968);
    p.SaveFileHeader();
    p.SetPixelType(UnsignedByte);
    p.SetByteOrder(Lsb);
    p.SetDataSuffixBytes(136);

    p.SetInputFile(ui.GetFileName("FROM"));
    Cube *oCube = p.SetOutputCube("TO");

    p.StartProcess();
    unsigned char *header = (unsigned char *) p.FileHeader();
    QString labels = EbcdicToAscii(header);
    UpdateLabels(oCube, labels);
    p.EndProcess();
  }
  // Import for Isis 2 cubes
  else {
    ProcessImportPds p;

    // All mariner images from both cameras share this size
    p.SetDimensions(832, 700, 1);
    p.SetPixelType(UnsignedByte);
    p.SetByteOrder(Lsb);
    p.SetDataSuffixBytes(136);

    p.SetPdsFile(inputFile.expanded(), "", label);
    Cube *oCube = p.SetOutputCube("TO");

    TranslateIsis2Labels(inputFile, oCube);
    p.StartProcess();
    p.EndProcess();
  }
}

// Converts labels into standard pvl format and adds necessary
// information not included in original labels
void UpdateLabels(Cube *cube, const QString &labels) {
  // First, we parse out as much valid information as possible from the
  // original labels
  QString key;
  int keyPosition;
  int consumeChars;

  // Image number
  key = "FDS=";
  keyPosition = labels.indexOf(key);
  consumeChars = labels.indexOf("IM-1") - keyPosition - key.length();
  QString fds(labels.mid(keyPosition + key.length(), consumeChars));
  fds = fds.trimmed();

  // Year the image was taken
  key = "YR=";
  keyPosition = labels.indexOf(key);
  consumeChars = labels.indexOf("DAY") - keyPosition - key.length();
  QString yr(labels.mid(keyPosition + key.length(), consumeChars));
  yr = yr.trimmed();

  // Day the image was taken
  key = "DAY=";
  keyPosition = labels.indexOf(key);
  consumeChars = labels.indexOf("GMT") - keyPosition - key.length();
  QString day(labels.mid(keyPosition + key.length(), consumeChars));
  day = day.trimmed();

  // Greenwich Mean Time
  key = "GMT=";
  keyPosition = labels.indexOf(key);
  consumeChars = labels.indexOf("CCAMERA") - keyPosition - key.length();
  QString gmt(labels.mid(keyPosition + key.length(), consumeChars));
  gmt = gmt.trimmed();

  // Which of the two cameras took the image
  key = "CCAMERA=";
  keyPosition = labels.indexOf(key);
  consumeChars = labels.indexOf("FILTER") - keyPosition - key.length();
  QString ccamera(labels.mid(keyPosition + key.length(), consumeChars));
  ccamera = ccamera.trimmed();

  // Filter number
  key = "FILTER=";
  keyPosition = labels.indexOf(key);
  consumeChars = labels.indexOf("(") - keyPosition - key.length();
  QString filterNum(labels.mid(keyPosition + key.length(), consumeChars));
  filterNum = filterNum.trimmed();

  // Filter name
  consumeChars = labels.indexOf(")") - keyPosition - key.length() - consumeChars;
  QString filterName(labels.mid(labels.indexOf("(") + 1, consumeChars));
  filterName = filterName.trimmed();

  // Center wavelength
  int fnum = toInt(filterNum);
  double filterCenter = 0.;
  switch(fnum) {
    case 0:
      filterCenter = .575;
      break;
    case 2:
      filterCenter = .475;
      break;
    case 3:
      filterCenter = .360;
      break;
    case 4:
      filterCenter = .511;
      break;
    case 5:
      filterCenter = .487;
      break;
    case 6:
      filterCenter = .355;
      break;
    default:
      break;
  }

  // Exposure duration
  key = "EXPOSURE=";
  keyPosition = labels.indexOf(key);
  consumeChars = labels.indexOf("MSEC") - keyPosition - key.length();
  QString exposure(labels.mid(keyPosition + key.length(), consumeChars));
  exposure = exposure.trimmed();

  // Create the instrument group
  PvlGroup inst("Instrument");
  inst += PvlKeyword("SpacecraftName", "Mariner_10");
  inst += PvlKeyword("InstrumentId", "M10_VIDICON_" + ccamera.toStdString());

  // Get the date
  int days = toInt(day);
  QString date = DaysToDate(days);

  // Get the time
  QString time = gmt;
  time = time.replace("/", ":");

  // Construct the Start Time in yyyy-mm-ddThh:mm:ss format
  QString fullTime = date + "T" + time + ".000";
  iTime startTime(fullTime);

  // Create the archive group
  PvlGroup archive("Archive");

  int year = toInt(yr);
  year += 1900;
  QString fullGMT = toString(year) + ":" + day + ":" + time;
  archive += PvlKeyword("GMT", fullGMT.toStdString());
  archive += PvlKeyword("ImageNumber", fds.toStdString());

  // Create the band bin group
  PvlGroup bandBin("BandBin");
  QString filter = filterName;
  filter = filter.replace(")", "");
  bandBin += PvlKeyword("FilterName", filter.toStdString());
  QString number = filterNum;
  bandBin += PvlKeyword("FilterNumber", number.toStdString());
  bandBin += PvlKeyword("OriginalBand", "1");
  QString center = toString(filterCenter);
  bandBin += PvlKeyword("Center", center.toStdString());
  bandBin.findKeyword("Center").setUnits("micrometers");

  // Dates taken from ASU Mariner website - http://ser.ses.asu.edu/M10/TXT/encounters.html and
  // from Nasa website - http://www.jpl.nasa.gov/missions/missiondetails.cfm?mission_Mariner10
  // under fast facts. Mariner encountered the Moon, Venus, and Mercury three times.
  // Date used for all is two days before date of first encounter on website. Information
  // is important for nominal reseaus used and for Keyword encounter
  QString target = "";
  if(startTime < iTime("1974-2-3T12:00:00")) {
    target = "$mariner10/reseaus/mar10MoonNominal.pvl";
    inst += PvlKeyword("TargetName", "Moon");
    archive += PvlKeyword("Encounter", "Moon");
  }
  //Disagreement on the below date between ASU website and NASA website, used earlier of the two - ASU
  else if(startTime < iTime("1974-3-22T12:00:00")) {
    target = "$mariner10/reseaus/mar10VenusNominal.pvl";
    inst += PvlKeyword("TargetName", "Venus");
    archive += PvlKeyword("Encounter", "Venus");
  }
  else if(startTime < iTime("1974-9-19T12:00:00")) {
    target = "$mariner10/reseaus/mar10Merc1Nominal.pvl";
    inst += PvlKeyword("TargetName", "Mercury");
    archive += PvlKeyword("Encounter", "Mercury_1");
  }
  // No specific date on ASU website, used NASA date
  else if(startTime < iTime("1975-3-14T12:00:00")) {
    target = "$mariner10/reseaus/mar10Merc2Nominal.pvl";
    inst += PvlKeyword("TargetName", "Mercury");
    archive += PvlKeyword("Encounter", "Mercury_2");
  }
  else {
    target = "$mariner10/reseaus/mar10Merc3Nominal.pvl";
    inst += PvlKeyword("TargetName", "Mercury");
    archive += PvlKeyword("Encounter", "Mercury_3");
  }

  // Place start time and exposure duration in intrument group
  inst += PvlKeyword("StartTime", fullTime.toStdString());
  inst += PvlKeyword("ExposureDuration", exposure.toStdString(), "milliseconds");

  // Open nominal positions pvl named by QString encounter
  Pvl nomRx(target.toStdString());

  // Allocate all keywords within reseaus groups well as the group its self
  PvlGroup rx("Reseaus");
  PvlKeyword line("Line");
  PvlKeyword sample("Sample");
  PvlKeyword type("Type");
  PvlKeyword valid("Valid");
  PvlKeyword templ("Template");
  PvlKeyword status("Status");

  // All cubes will stay this way until indexOfrx is run on them
  status = "Nominal";

  // Kernels group
  PvlGroup kernels("Kernels");
  PvlKeyword naif("NaifFrameCode");

  // Camera dependent information
  QString camera = "";
  if(QString("M10_VIDICON_A") == QString::fromStdString(inst["InstrumentId"][0])) {
    templ = "$mariner10/reseaus/mar10a.template.cub";
    naif += "-76110";
    camera = "M10_VIDICON_A_RESEAUS";
  }
  else {
    templ = "$mariner10/reseaus/mar10b.template.cub";
    naif += "-76120";
    camera = "M10_VIDICON_B_RESEAUS";
  }

  // Add naif frame code
  kernels += naif;

  // Find the correct PvlKeyword corresponding to the camera for nominal positions
  PvlKeyword resnom = nomRx[camera.toStdString()];

  // This loop goes through the PvlKeyword resnom which contains data
  // in the format: line, sample, type, on each line. There are 111 reseaus for
  // both cameras. To put data away correctly, it must go through a total 333 items,
  // all in one PvlKeyword.
  int i = 0;
  while(i < 333) {
    line += resnom[i];
    i++;
    sample += resnom[i];
    i++;
    type += resnom[i];
    i++;
    valid += "0";
  }

  // Add all the PvlKeywords to the PvlGroup Reseaus
  rx += line;
  rx += sample;
  rx += type;
  rx += valid;
  rx += templ;
  rx += status;

  // Get the labels and add the updated labels to them
  Pvl *cubeLabels = cube->label();
  cubeLabels->findObject("IsisCube").addGroup(inst);
  cubeLabels->findObject("IsisCube").addGroup(archive);
  cubeLabels->findObject("IsisCube").addGroup(bandBin);
  cubeLabels->findObject("IsisCube").addGroup(kernels);
  cubeLabels->findObject("IsisCube").addGroup(rx);

  PvlObject original("OriginalLabel");
  original += PvlKeyword("Label", labels.toStdString());
  Pvl olabel;
  olabel.addObject(original);
  OriginalLabel ol(olabel);
  cube->write(ol);
}

// Translate Isis 2 labels into Isis labels.
void TranslateIsis2Labels(FileName &labelFile, Cube *oCube) {
  // Transfer the instrument group to the output cube
  QString transDir = "$ISISROOT/appdata/translations/";
  Pvl inputLabel(labelFile.expanded());
  FileName transFile;

  transFile = transDir + "Mariner10isis2.trn";

  // Get the translation manager ready
  PvlToPvlTranslationManager translation(inputLabel, transFile.expanded());
  Pvl *outputLabel = oCube->label();
  translation.Auto(*(outputLabel));

  //Instrument group
  PvlGroup &inst = outputLabel->findGroup("Instrument", Pvl::Traverse);

  PvlKeyword &instrumentId = inst.findKeyword("InstrumentId");
  instrumentId.setValue("M10_VIDICON_" + instrumentId[0]);

  PvlKeyword &targetName = inst.findKeyword("TargetName");
  QString targetTail(QString::fromStdString(targetName[0]).mid(1));
  targetTail = targetTail.toLower();
  targetName.setValue((QString::fromStdString(targetName[0]).at(0) + targetTail).toStdString());

  PvlKeyword &startTime = inst.findKeyword("StartTime");
  startTime.setValue(QString::fromStdString(startTime[0]).mid(0, QString::fromStdString(startTime[0]).size() - 1).toStdString());

  PvlGroup &archive = outputLabel->findGroup("Archive", Pvl::Traverse);
  PvlKeyword &imgNo = archive.findKeyword("ImageNumber");
  QString ino = QString::fromStdString(imgNo[0]);
  ino = ino.trimmed();
  imgNo.setValue(ino.toStdString());

  iTime time(QString::fromStdString(startTime[0]));
  if(time < iTime("1974-2-3T12:00:00")) {
    archive += PvlKeyword("Encounter", "Moon");
  }
  else if(time < iTime("1974-3-22T12:00:00")) {
    archive += PvlKeyword("Encounter", "Venus");
  }
  else if(time < iTime("1974-9-19T12:00:00")) {
    archive += PvlKeyword("Encounter", "Mercury_1");
  }
  else if(time < iTime("1975-3-14T12:00:00")) {
    archive += PvlKeyword("Encounter", "Mercury_2");
  }
  else {
    archive += PvlKeyword("Encounter", "Mercury_3");
  }

  inst.findKeyword("ExposureDuration").setUnits("milliseconds");

  PvlGroup &bBin = outputLabel->findGroup("BandBin", Pvl::Traverse);
  QString filter = QString::fromStdString(inputLabel.findObject("QUBE")["FILTER_NAME"]);
  if(filter != "F") {
    //Band Bin group
    bBin.findKeyword("Center").setUnits("micrometers");
  }

  // Kernels group
  PvlGroup &kernels = outputLabel->findGroup("Kernels", Pvl::Traverse);
  PvlGroup &reseaus = outputLabel->findGroup("Reseaus", Pvl::Traverse);
  PvlKeyword &templ = reseaus.findKeyword("Template");
  PvlKeyword &valid = reseaus.findKeyword("Valid");

  for(int i = 0; i < valid.size(); i++) {
    valid[i] = (QString::fromStdString(valid[i]).mid(0, 1)).toStdString();
  }

  // Camera dependent information
  QString camera = "";
  if(QString("M10_VIDICON_A") == QString::fromStdString(inst["InstrumentId"][0])) {
    templ = "$mariner10/reseaus/mar10a.template.cub";
    kernels.findKeyword("NaifFrameCode").setValue("-76110");
    camera = "M10_VIDICON_A_RESEAUS";
  }
  else {
    templ = "$mariner10/reseaus/mar10b.template.cub";
    kernels.findKeyword("NaifFrameCode").setValue("-76120");
    camera = "M10_VIDICON_B_RESEAUS";
  }
}

// FYI, mariner10 original labels are stored in ebcdic, a competitor with ascii,
// a conversion table is necessary then to get the characters over to ascii. For
// more info: http://en.wikipedia.org/wiki/Extended_Binary_Coded_Decimal_Interchange_Code
//! Converts ebsidic Mariner10 labels to ascii
QString EbcdicToAscii(unsigned char *header) {
  // Table to convert ebcdic to ascii
  unsigned char xlate[] = {
    0x00, 0x01, 0x02, 0x03, 0x9C, 0x09, 0x86, 0x7F, 0x97, 0x8D, 0x8E, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x9D, 0x85, 0x08, 0x87, 0x18, 0x19, 0x92, 0x8F, 0x1C, 0x1D, 0x1E, 0x1F,
    0x80, 0x81, 0x82, 0x83, 0x84, 0x0A, 0x17, 0x1B, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x05, 0x06, 0x07,
    0x90, 0x91, 0x16, 0x93, 0x94, 0x95, 0x96, 0x04, 0x98, 0x99, 0x9A, 0x9B, 0x14, 0x15, 0x9E, 0x1A,
    0x20, 0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xD5, 0x2E, 0x3C, 0x28, 0x2B, 0x7C,
    0x26, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 0xB0, 0xB1, 0x21, 0x24, 0x2A, 0x29, 0x3B, 0x5E,
    0x2D, 0x2F, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xE5, 0x2C, 0x25, 0x5F, 0x3E, 0x3F,
    0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF, 0xC0, 0xC1, 0xC2, 0x60, 0x3A, 0x23, 0x40, 0x27, 0x3D, 0x22,
    0xC3, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9,
    0xCA, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF, 0xD0,
    0xD1, 0x7E, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0xD2, 0xD3, 0xD4, 0x5B, 0xD6, 0xD7,
    0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF, 0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0x5D, 0xE6, 0xE7,
    0x7B, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED,
    0x7D, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50, 0x51, 0x52, 0xEE, 0xEF, 0xF0, 0xF1, 0xF2, 0xF3,
    0x5C, 0x9F, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
  };

  // Mariner 10 has 360 bytes of header information
  for(int i = 0; i < 360; i++) {
    header[i] = xlate[header[i]];
  }

  // Put in a end of QString mark and return
  header[215] = 0;
  return QString((const char *)header);
}

// Mariner 10 labels provide the number of days since the beginning of the year
// 1974 in the GMT keyword, but not always a start time.  In order to derive an
// estimated start time, with an actual date attached, a conversion must be
// performed.
QString DaysToDate(int days) {
  int currentMonth = 12;
  int currentDay = 31;
  int currentYear = 1973;
  while(days > 0) {
    // The Mariner 10 mission took place in the years 1973 through 1975,
    // none of which were Leap Years, thus February always had 28 days
    if(currentDay == 28 && currentMonth == 2) {
      currentMonth = 3;
      currentDay = 1;
    }
    else if(currentDay == 30 &&
            (currentMonth == 4 || currentMonth == 6 ||
             currentMonth == 9 || currentMonth == 11)) {
      currentMonth++;
      currentDay = 1;
    }
    else if(currentDay == 31 && currentMonth == 12) {
      currentMonth = 1;
      currentDay = 1;
      currentYear++;
    }
    else if(currentDay == 31) {
      currentMonth++;
      currentDay = 1;
    }
    else {
      currentDay++;
    }
    days--;
  }
  QString year = toString(currentYear);
  QString month = (currentMonth < 10) ? "0" + toString(currentMonth) : toString(currentMonth);
  QString day = (currentDay < 10) ? "0" + toString(currentDay) : toString(currentDay);
  return year + "-" + month + "-" + day;
}
