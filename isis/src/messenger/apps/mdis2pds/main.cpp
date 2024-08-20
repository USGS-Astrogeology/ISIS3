/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

#include <cmath>
#include <fstream>
#include <sstream>
#include <string>

#include "BufferManager.h"
#include "Cube.h"
#include "FileName.h"
#include "Histogram.h"
#include "IException.h"
#include "IString.h"
#include "LineManager.h"
#include "OriginalLabel.h"
#include "ProcessExportPds.h"
#include "Progress.h"
#include "PvlToPvlTranslationManager.h"
#include "UserInterface.h"

using namespace std;
using namespace Isis;

inline QString quote(const QString &value, const char qChar='"') {
  if (value.isEmpty()) return (value);
  if (value[0] == qChar) return (value);
  return (QString(qChar + value + qChar));
}

inline double setRound(double value, const int precision) {
  double scale = pow(10.0, precision);
  value = rint(value * scale) / scale;
  return (value);
}

inline void validateUnit(PvlKeyword &key, const QString &kunit) {
  PvlKeyword temp = key;
  key.clear();
  for (int i = 0; i < temp.size(); i++) {
    try {
      //  If this works, check unit, otherwise an exception is thrown
      std::stod(temp[i]);
      QString unit = QString::fromStdString(temp.unit(i));
      if (unit.isEmpty()) unit = kunit;
      key.addValue(temp[i], unit.toStdString());
    }
    catch (...) {
      key.addValue(temp[i]);
    }
  }
  return;
}

inline void fixUnit(PvlObject &obj, const QString &key, const QString &unit) {
  if (obj.hasKeyword(key.toStdString(), PvlObject::Traverse)) {
    validateUnit(obj.findKeyword(key.toStdString(), PvlObject::Traverse), unit);
  }
  return;
}

inline void fixQuotes(PvlContainer &kcont, const QString &value="N/A") {
  PvlContainer::PvlKeywordIterator kiter;
  for (kiter = kcont.begin(); kiter != kcont.end(); ++kiter) {
    for (int nv = 0; nv < kiter->size(); nv++) {
      if (QString::fromStdString((*kiter)[nv]) == value)(*kiter)[nv] = quote(QString::fromStdString((*kiter)[nv])).toStdString();
    }
  }
}

inline void fixLabels(PvlObject &obj) {
  // Current object-owned keywords
  fixQuotes(obj);

  // Fix all nested objects
  PvlObject::PvlObjectIterator o;
  for (o = obj.beginObject(); o != obj.endObject(); ++o) {
    fixLabels(*o);
  }

  // Fix local groups
  PvlObject::PvlGroupIterator g;
  for (g = obj.beginGroup(); g != obj.endGroup(); ++g) {
    fixQuotes(*g);
  }
  return;
}


void IsisMain() {
  const QString mdis2pdsProgram = "mdis2pds";
  const QString mdis2pdsVersion = "1.1";
  const QString mdis2pdsRevision = "$Revision: 6715 $";
  const QString mdis2pdsRuntime = Application::DateTime();

  UserInterface &ui = Application::GetUserInterface();
  FileName input(ui.GetCubeName("FROM"));
  FileName output = ui.GetFileName("TO");
  output = output.addExtension("IMG");

  // Set up the export
  ProcessExportPds processPds;
  Cube *incube = processPds.SetInputCube("FROM");

  Histogram *hist = incube->histogram(0);
  double minmin = 0.0;
  double maxmax = 0.0;
  // if the type of calibration is automatic and
  //     1) the given min percent is valid, then set minmin to the percent obtained from this min
  //     2) the given min percent is not valid, then set minmin to the histogram's min
  //     3) the given max percent is valid, then set maxmax to the percent obtained from this max
  //     4) the given max percent is not valid, then set maxmax to the histogram's max
  // Otherwise, (calibration is manual)
  //     set minmin to the given min value
  //     set maxmax to the given max value
  if (ui.GetString("TYPE").compare("AUTOMATIC") == 0) {
    minmin = (ui.GetDouble("MINPER") <= 0.000001) ?
             hist->Minimum() :
             hist->Percent(ui.GetDouble("MINPER"));

    maxmax = (ui.GetDouble("MAXPER") >= 99.999999) ?
             hist->Maximum() :
             hist->Percent(ui.GetDouble("MAXPER"));
  }
  else {
    minmin = ui.GetDouble("MIN");
    maxmax = ui.GetDouble("MAX");
  }

  processPds.SetOutputEndian(Isis::Msb);
  processPds.SetExportType(ProcessExportPds::Fixed);
  processPds.SetInputRange(minmin, maxmax);

  // Set the output pixel type and the special pixel values
  QString dataSetID = "MESS-E/V/H-MDIS-";
  int nbits = ui.GetInteger("BITS");
  if (nbits == 8) {
    processPds.SetOutputType(UnsignedByte);
    processPds.SetOutputRange(VALID_MIN1, VALID_MAX1);
    processPds.SetOutputNull(NULL1);
    processPds.SetOutputLis(LOW_INSTR_SAT1);
    processPds.SetOutputLrs(LOW_REPR_SAT1);
    processPds.SetOutputHis(HIGH_INSTR_SAT1);
    processPds.SetOutputHrs(HIGH_REPR_SAT1);
    dataSetID += "1";
  }
  else if (nbits == 16) {
    processPds.SetOutputType(UnsignedWord);
    processPds.SetOutputRange(VALID_MINU2, VALID_MAXU2);
    processPds.SetOutputNull(NULLU2);
    processPds.SetOutputLis(LOW_INSTR_SATU2);
    processPds.SetOutputLrs(LOW_REPR_SATU2);
    processPds.SetOutputHis(HIGH_INSTR_SATU2);
    processPds.SetOutputHrs(HIGH_REPR_SATU2);
    dataSetID += "2";
  }
  else if (nbits == 32) {
    processPds.SetOutputType(Real);
    processPds.SetOutputRange(minmin, maxmax);
    processPds.SetOutputNull(NULL4);
    processPds.SetOutputLrs(LOW_REPR_SAT4);
    processPds.SetOutputLis(LOW_INSTR_SAT4);
    processPds.SetOutputHrs(HIGH_REPR_SAT4);
    processPds.SetOutputHis(HIGH_INSTR_SAT4);
    dataSetID += "4";
  }
  else if (nbits < 16) { // 8 < nbits < 16
                         // NOTE: xml restricts BITS to between 8 and 32, inclusive
    processPds.SetOutputType(UnsignedWord);
    processPds.SetOutputRange(3.0, pow(2.0, (double)(nbits)) - 3.0);
    processPds.SetOutputNull(0);
    processPds.SetOutputLrs(1);
    processPds.SetOutputLis(2);
    processPds.SetOutputHis(pow(2.0, (double)(nbits)) - 2.0);
    processPds.SetOutputHrs(pow(2.0, (double)(nbits)) - 1.0);
    dataSetID += "0";
  }
  else { // 16 < nbits < 32
    QString msg = "[" + toString(nbits) + "] is not a supported bit length.";
    throw IException(IException::User, msg, _FILEINFO_);
  }
  dataSetID += "-CDR-CALDATA-V1.0";

  Progress p;
  p.SetText("Modifying Keywords");
  p.SetMaximumSteps(7);
  p.CheckStatus();

  // Get the PDS label from the process
  Pvl &pdsLabel = processPds.StandardPdsLabel(ProcessExportPds::Image);

  // Translate the keywords from the original EDR PDS label that go in
  // this RDR PDS label
  OriginalLabel origLabel = incube->readOriginalLabel();
  Pvl pvlOrigLabel;
  PvlObject origLabelObj = origLabel.ReturnLabels();
  origLabelObj.setName("OriginalLabelObject");
  pvlOrigLabel.addObject(origLabelObj);

  p.CheckStatus();

  // Translates the ISIS labels along with the original EDR labels
  pvlOrigLabel.addObject(*(incube->label()));
  PvlToPvlTranslationManager labels(pvlOrigLabel,
                               "$ISISROOT/appdata/translations/MessengerMdisCdrLabel.trn");
  labels.Auto(pdsLabel);

  p.CheckStatus();

  // The translation file automatically adds this keyword from
  // "EmpiricalCorrectionFactor" using a default of "N/A" if not found. This
  // keyword was previously named "ContaminationEventFactor"
  // So here we replace the "N/A" with the value of the keyword
  // "ContaminationEventFactor", if it exists.
  PvlKeyword &empiricalCorrection(pdsLabel.findKeyword("MESS:EC_FACTOR"));
  if (empiricalCorrection[0] == "N/A") {
    if (incube->label()->hasKeyword("ContaminationEventFactor", Pvl::Traverse)) {
      empiricalCorrection.setValue(incube->label()->findKeyword("ContaminationEventFactor",
                                                                Pvl::Traverse)[0]);
    }
  }

  p.CheckStatus();

  // Creates keywords from the input's hist above
  PvlKeyword minDn("MINIMUM", std::to_string(setRound(hist->Minimum(), 16)));
  PvlKeyword maxDn("MAXIMUM", std::to_string(setRound(hist->Maximum(), 16)));
  PvlKeyword meanDn("MEAN", std::to_string(setRound(hist->Average(), 16)));
  PvlKeyword stddev("STANDARD_DEVIATION", std::to_string(setRound(hist->StandardDeviation(), 16)));

  PvlKeyword saturated("SATURATED_PIXEL_COUNT", std::to_string(hist->HisPixels()));

  PvlObject &imageObj = pdsLabel.findObject("IMAGE");

  minDn.addComment("/*** IMAGE STATISTICS ***/");
  imageObj.addKeyword(minDn);
  imageObj.addKeyword(maxDn);
  imageObj.addKeyword(meanDn);
  imageObj.addKeyword(stddev);
  saturated.addComment("/*** PIXEL COUNTS ***/");
  imageObj.addKeyword(saturated);
  // This line is commented out since mdisCDRLabel.trn automatically creates a
  // DARK_STRIP_MEAN keyword with default = N/A if it is not found in the input cube
  // if (imageObj.hasKeyword("DARK_STRIP_MEAN")) {
  PvlKeyword &darkStripMean = imageObj.findKeyword("DARK_STRIP_MEAN");

  try {
    darkStripMean[0] = std::to_string(setRound(std::stod(darkStripMean[0]), 16));
  }
  catch (IException &) {
    // If we fail to convert this keyword to a number, then preserve
    // its existing value
  }
  //}

  p.CheckStatus();

  // Fixes bad keywords
  // data set id
  PvlKeyword &dataSetIdKeyword = pdsLabel.findKeyword("DATA_SET_ID",
                                                      Pvl::Traverse);
  dataSetIdKeyword.setValue(dataSetID.toStdString());

  // product id
  PvlKeyword &productIdKeyword = pdsLabel.findKeyword("PRODUCT_ID",
                                                      Pvl::Traverse);
  if (productIdKeyword[0] == "N/A") {
    productIdKeyword.setValue(output.baseName().toStdString());
  }

  // product creation time
  PvlKeyword &productCreationTimeKeyword = pdsLabel.findKeyword("PRODUCT_CREATION_TIME",
                                                                Pvl::Traverse);
  productCreationTimeKeyword.setValue(mdis2pdsRuntime.toStdString());

  // software name
  PvlKeyword &softwareNameKeyword = pdsLabel.findKeyword("SOFTWARE_NAME",
                                                         Pvl::Traverse);
  if (softwareNameKeyword[0] == "N/A") {
    softwareNameKeyword.setValue(mdis2pdsProgram.toStdString());
  }

  // software version id
  PvlKeyword &softwareVersionIdKeyword = pdsLabel.findKeyword("SOFTWARE_VERSION_ID",
                                                              Pvl::Traverse);
  if (softwareVersionIdKeyword[0] == "N/A") {
    softwareVersionIdKeyword.setValue(quote(mdis2pdsVersion).toStdString());
  }
  else {
    softwareVersionIdKeyword.setValue(softwareVersionIdKeyword[0]);
  }

  // filter number
  PvlKeyword &filterNumberKeyword = pdsLabel.findKeyword("FILTER_NUMBER",
                                                         Pvl::Traverse);
  if ((filterNumberKeyword.size() > 0)) {
    filterNumberKeyword.setValue(quote(QString::fromStdString(filterNumberKeyword[0])).toStdString());
  }


  // Add quotes
  // data quality id
  PvlKeyword &dataQualityIdKeyword = pdsLabel.findKeyword("DATA_QUALITY_ID",
                                                          Pvl::Traverse);
  dataQualityIdKeyword.setValue(quote(QString::fromStdString(dataQualityIdKeyword)).toStdString());

  // sequence name
  PvlKeyword &sequenceNameKeyword = pdsLabel.findKeyword("SEQUENCE_NAME",
                                                         Pvl::Traverse);
  sequenceNameKeyword.setValue(quote(QString::fromStdString(sequenceNameKeyword)).toStdString());

  // spacecraft clock start count
  PvlKeyword &startCountKeyword = pdsLabel.findKeyword("SPACECRAFT_CLOCK_START_COUNT",
                                                       Pvl::Traverse);
  startCountKeyword.setValue(quote(QString::fromStdString(startCountKeyword)).toStdString());

  // spacecraft clock stop count
  PvlKeyword &stopCountKeyword = pdsLabel.findKeyword("SPACECRAFT_CLOCK_STOP_COUNT",
                                                      Pvl::Traverse);
  stopCountKeyword.setValue(quote(QString::fromStdString(stopCountKeyword)).toStdString());

  // site id
  PvlKeyword &siteIdKeyword = pdsLabel.findKeyword("SITE_ID",
                                                   Pvl::Traverse);
  siteIdKeyword.setValue(quote(QString::fromStdString(siteIdKeyword)).toStdString());

  // source product id
  PvlKeyword &sourceProductIdKeyword = pdsLabel.findKeyword("SOURCE_PRODUCT_ID",
                                                            Pvl::Traverse);
  for (int i = 0; i < sourceProductIdKeyword.size(); i++) {
    sourceProductIdKeyword[i] = quote(QString::fromStdString(sourceProductIdKeyword[i])).toStdString();
  }
  //  Enforce parentheses for scalars
  if (sourceProductIdKeyword.size() == 1) {
    sourceProductIdKeyword.setValue('(' + sourceProductIdKeyword[0] + ')');
  }

  // Removes keywords
  PvlObject &imageObject(pdsLabel.findObject("IMAGE"));
  imageObject.deleteKeyword("FILTER_NAME");
  imageObject.deleteKeyword("CENTER_FILTER_WAVELENGTH");
  imageObject.deleteKeyword("BANDWIDTH");

  p.CheckStatus();

  //  Fix all the hosed units upon ingest.  They are illformed.
  fixUnit(pdsLabel, "RETICLE_POINT_RA", "DEG");
  fixUnit(pdsLabel, "RETICLE_POINT_DECLINATION", "DEG");
  fixUnit(pdsLabel, "RETICLE_POINT_LATITUDE", "DEG");
  fixUnit(pdsLabel, "RETICLE_POINT_LONGITUDE", "DEG");

  //  Now address nested keywords in SUBFRAME groups
  for (int i = 1; i <= 5; i++) {
    QString n(toString(i));
    QString group = "SUBFRAME" + n + "_PARAMETERS";
    if (pdsLabel.hasGroup(group.toStdString())) {
      PvlGroup &grp = pdsLabel.findGroup(group.toStdString());
      validateUnit(grp.findKeyword("RETICLE_POINT_LATITUDE"), "DEG");
      validateUnit(grp.findKeyword("RETICLE_POINT_LONGITUDE"), "DEG");
    }
  }

  p.CheckStatus();

//  Finally, fix keywords by Quoting missing N/A values
  fixLabels(pdsLabel);
  p.CheckStatus();

  // All done...write result.
  pdsLabel.setFormatTemplate("$ISISROOT/appdata/translations/mdisPdsCdr.def");
  QString ofile(output.expanded());
  ofstream outstream(ofile.toLatin1().data());
  processPds.OutputLabel(outstream);

  processPds.StartProcess(outstream);
  outstream.close();
  processPds.EndProcess();
}
