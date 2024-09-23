/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

#include <cmath>
#include <fstream>
#include <sstream>

#include <QString>

#include "BufferManager.h"
#include "CollectorMap.h"
#include "Cube.h"
#include "FileName.h"
#include "Histogram.h"
#include "IException.h"
#include "Kernels.h"
#include "LineManager.h"
#include "OriginalLabel.h"
#include "ProcessExportPds.h"
#include "ProgramLauncher.h"
#include "Progress.h"
#include "PvlToPvlTranslationManager.h"
#include "UserInterface.h"

using namespace std;
using namespace Isis;


typedef CollectorMap<IString, int> BandMap;

inline QString quote(const QString &value, const char qChar = '"') {
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
      (void)IString::ToDouble(temp[i]);
      QString unit =  QString::fromStdString(temp.unit(i));
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

inline void fixQuotes(PvlContainer &kcont, const QString &value = "N/A") {
  PvlContainer::PvlKeywordIterator kiter;
  for (kiter = kcont.begin(); kiter != kcont.end(); ++kiter) {
    for (int nv = 0; nv < kiter->size(); nv++) {
      if (QString::fromStdString((*kiter)[nv]) == value) QString::fromStdString((*kiter)[nv]) = quote(QString::fromStdString((*kiter)[nv]));
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

inline void writeBand(ProcessExportPds &process, ofstream &out,
                      const QString &fname, int band)  {
  process.ClearInputCubes();
  (void) process.SetInputCube(fname, CubeAttributeInput("+" + toString(band)));
  process.StartProcess(out);
  return;
}


void IsisMain() {
  const QString mdisddrProgram = "mdisddr";
  const QString mdisddrVersion = "1.0";
  const QString mdisddrRevision = "$Revision: 6715 $";
  const QString mdisddrRuntime = Application::DateTime();
  const QString dataSetID = "MESS-E/V/H-MDIS-6-DDR-GEOMDATA-V1.0";

  UserInterface &ui = Application::GetUserInterface();
  FileName input(ui.GetCubeName("FROM").toStdString());
  QString to = "";
  bool toEntered = ui.WasEntered("TO");
  if (toEntered) {
    to = ui.GetAsString("TO");
  }

  QString opath(".");    // Set default to local directory
  if (ui.WasEntered("OPATH")) {
    opath = ui.GetString("OPATH");
  }
  else {
    ui.PutAsString("OPATH", opath);
  }

  //  Generate the image cube that phocube produces for the DDR data
  FileName phoFile = FileName::createTempFile("$temporary/" + input.baseName() + "_phocube.cub");
  QString pfile = QString::fromStdString(phoFile.expanded());
  QString parameters = "FROM=" + QString::fromStdString(input.expanded()) + " TO=" + pfile +
                      " LATITUDE=TRUE LONGITUDE=TRUE PHASE=TRUE EMISSION=TRUE INCIDENCE=TRUE";
  ProgramLauncher::RunIsisProgram("phocube", parameters);

  //  Wrap a try clause so that if anything goes wrong below, we can remove
  //  the phocube file.
  try {
    Pvl phoLabel(pfile.toStdString());
    BandMap bandmap;
    PvlKeyword bn = phoLabel.findGroup("BandBin", Pvl::Traverse)["Name"];
    for (int i = 0; i < bn.size(); i++) {
      bandmap.add(bn[i], i + 1);
    }

    // Set up the export.  Note that the attributes selects 5 bands from the
    // output of the phocube run.  It doesn't matter at this time which 5
    // bands it is, just so that we have this established so the right labels
    // and file size is created.
    ProcessExportPds processPds;
    (void) processPds.SetInputCube(pfile, CubeAttributeInput("+1-5"));

    //  Due to the nature of the phocube file, we cannot compute a histogram
    //  of the data (it includes lots of data we don't need).  So we will
    //  fix the range to the expected well defined angle ranges.
    double minmin = 0.0;
    double maxmax = 0.0;
    if (ui.GetString("TYPE").compare("AUTOMATIC") == 0) {
      minmin = -360.0;
      maxmax = 360.0;
    }
    else {
      minmin = ui.GetDouble("MIN");
      maxmax = ui.GetDouble("MAX");
    }

    processPds.SetOutputEndian(Isis::Msb);
    processPds.SetExportType(ProcessExportPds::Fixed);
    processPds.SetInputRange(minmin, maxmax);

    // Set the output pixel type and the special pixel values
    processPds.SetOutputType(Real);
    processPds.SetOutputRange(minmin, maxmax);
    processPds.SetOutputNull(NULL4);
    processPds.SetOutputLrs(LOW_REPR_SAT4);
    processPds.SetOutputLis(LOW_INSTR_SAT4);
    processPds.SetOutputHrs(HIGH_REPR_SAT4);
    processPds.SetOutputHis(HIGH_INSTR_SAT4);

    Progress p;
    p.SetText("Modifying Keywords");
    p.SetMaximumSteps(6);
    p.CheckStatus();

    // Get the PDS label from the process
    Pvl &pdsLabel = processPds.StandardPdsLabel(ProcessExportPds::Image);

    // Translate the keywords from the original EDR PDS label that go in
    // this DDR PDS label.  Note that we have to open the original (FROM)
    // cube as the phocube output goes into the specification of the
    // output PDS file (required for 5 band IMAGE object).
    Cube from;
    from.open(QString::fromStdString(input.expanded()));
    OriginalLabel origBlob = from.readOriginalLabel();
    Pvl origLabel;
    PvlObject origLabelObj = origBlob.ReturnLabels();
    origLabelObj.setName("OriginalLabelObject");
    origLabel.addObject(origLabelObj);
    p.CheckStatus();

    // Translates the ISIS labels along with the original EDR labels
    origLabel.addObject(*from.label());
    PvlToPvlTranslationManager labels(origLabel,
                                 "$ISISROOT/appdata/translations/MessengerMdisDdrLabel.trn");
    labels.Auto(pdsLabel);
    p.CheckStatus();

    //  Add any new keywords
    QString lnote = "2007-12-20, S. Murchie (JHU/APL); "
                    "2008-01-02, S. Murchie (JHU/APL); "
                    "2008-01-11, J. Ward (GEO)";
    pdsLabel += PvlKeyword("LABEL_REVISION_NOTE", lnote.toStdString());
    pdsLabel += PvlKeyword("SPACECRAFT_NAME", quote("MESSENGER").toStdString());

    // Fix bad keywords
    // data set id
    PvlKeyword &dataSetIdKeyword = pdsLabel.findKeyword("DATA_SET_ID",
                                                        Pvl::Traverse);
    dataSetIdKeyword.setValue(dataSetID.toStdString());

    // product set id
    QString prodid(QString::fromStdString(input.baseName()));
    PvlKeyword &productIdKeyword = pdsLabel.findKeyword("PRODUCT_ID",
                                                        Pvl::Traverse);
    if ((productIdKeyword.size() == 0)
        || ((productIdKeyword.size() > 0) && (productIdKeyword[0] == "N/A"))) {
      productIdKeyword.setValue(prodid.toStdString());
    }
    else {
      QString pid =  QString::fromStdString(productIdKeyword[0]);
      pid[0] = 'D';
      pid.remove(QRegExp("_.*"));
      pid.append("_DE_0");
      productIdKeyword.setValue(pid.toStdString());
      prodid = pid;
    }
    // Now we have enough to establish output file name
    if (!toEntered) to = opath + "/" + prodid;
    FileName output(to.toStdString());
    output = output.addExtension("IMG");
    if (!toEntered) ui.PutFileName("TO", QString::fromStdString(output.expanded()));

    // product creation time
    PvlKeyword &productCreationTimeKeyword = pdsLabel.findKeyword("PRODUCT_CREATION_TIME",
                                                             Pvl::Traverse);
    productCreationTimeKeyword.setValue(mdisddrRuntime.toStdString());

    // software name
    PvlKeyword &softwareNameKeyword = pdsLabel.findKeyword("SOFTWARE_NAME",
                                                           Pvl::Traverse);
    softwareNameKeyword.setValue(mdisddrProgram.toStdString());

    // software version id
    PvlKeyword &softwareVersionIdKeyword = pdsLabel.findKeyword("SOFTWARE_VERSION_ID",
                                                                Pvl::Traverse);
    softwareVersionIdKeyword.setValue(quote(mdisddrVersion).toStdString());

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

    // source product id
    //  For DDRs, the SOURCE_PRODUCT_ID is made up of SPICE kernels.  I need to go get em.
    Kernels kernels(from);
    QStringList kfiles = kernels.getKernelList();
    PvlKeyword &sourceProductIdKeyword = pdsLabel.findKeyword("SOURCE_PRODUCT_ID",
                                                              Pvl::Traverse);
    sourceProductIdKeyword.clear();
    for (int i = 0; i < kfiles.size(); i++) {
      FileName kfile(kfiles[i].toStdString());
      sourceProductIdKeyword.addValue(quote(QString::fromStdString(kfile.name())).toStdString());
    }
    //  Enforce parentheses for scalars
    if (sourceProductIdKeyword.size() == 1)
      sourceProductIdKeyword.setValue('(' + sourceProductIdKeyword[0] + ')');

    // Removes keywords
    PvlObject imageObject(pdsLabel.findObject("IMAGE"));
    if (imageObject.hasKeyword("CENTER_FILTER_WAVELENGTH")) {
      imageObject.deleteKeyword("CENTER_FILTER_WAVELENGTH");
    }
    if (imageObject.hasKeyword("BANDWIDTH")) {
      imageObject.deleteKeyword("BANDWIDTH");
    }
    if (imageObject.hasKeyword("UNIT")) {
      imageObject.deleteKeyword("UNIT");
    }
    if (imageObject.hasKeyword("DARK_STRIP_MEAN")) {
      imageObject.deleteKeyword("DARK_STRIP_MEAN");
    }
    if (imageObject.hasKeyword("OFFSET")) {
      imageObject.deleteKeyword("OFFSET");
    }
    if (imageObject.hasKeyword("SCALING_FACTOR")) {
      imageObject.deleteKeyword("SCALING_FACTOR");
    }
    if (imageObject.hasKeyword("SAMPLE_BIT_MASK")) {
      imageObject.deleteKeyword("SAMPLE_BIT_MASK");
    }

    // Add band names to image object
    PvlKeyword &bandNames = imageObject.findKeyword("FILTER_NAME");
    bandNames.setName("BAND_NAME");
    bandNames.clear();
    bandNames.addValue("Latitude, planetocentric, deg N");
    bandNames.addValue("Longitude, planetocentric, deg E");
    bandNames.addValue("Incidence angle at equipotential surface, deg");
    bandNames.addValue("Emission angle at equipotential surface, deg");
    bandNames.addValue("Phase angle at equipotential surface, deg");
    pdsLabel.deleteObject("IMAGE");
    pdsLabel.addObject(imageObject);

    p.CheckStatus();

    //  Fix all the hosed units upon ingest.  They are illformed.
    fixUnit(pdsLabel, "RETICLE_POINT_RA", "DEG");
    fixUnit(pdsLabel, "RETICLE_POINT_DECLINATION", "DEG");
    fixUnit(pdsLabel, "RETICLE_POINT_LATITUDE", "DEG");
    fixUnit(pdsLabel, "RETICLE_POINT_LONGITUDE", "DEG");

    //  Now address nested keywords in SUBFRAME groups
    for (int i = 1; i <= 5; i++) {
      QString n(QString::number(i));
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
    pdsLabel.setFormatTemplate("$ISISROOT/appdata/translations/mdisPdsDdr.def");
    QString ofile(QString::fromStdString(output.expanded()));
    ofstream outstream(ofile.toLatin1().data());
    processPds.OutputLabel(outstream);

    // Writing out the 5 bands is a bit tricky for this product.  The bands
    // must be ordered in a specific order, but phocube orders them in a
    // different order.  To make this approach work, determine the proper band
    // as ordered in the phocube output and select the desired bands one at a
    // time setting the input cube to the desired band and writing it out by
    // stream.

    //  Write latitude, longitude, incidence, emission, phase bands
    writeBand(processPds, outstream, pfile, bandmap.get("Latitude"));
    writeBand(processPds, outstream, pfile, bandmap.get("Longitude"));
    writeBand(processPds, outstream, pfile, bandmap.get("Incidence Angle"));
    writeBand(processPds, outstream, pfile, bandmap.get("Emission Angle"));
    writeBand(processPds, outstream, pfile, bandmap.get("Phase Angle"));
    outstream.close();
    processPds.EndProcess();
    remove(pfile.toLatin1().data());
  }
  catch (IException &) {
    remove(pfile.toLatin1().data());
    throw;
  }
  catch (...) {
    remove(pfile.toLatin1().data());
    throw IException(IException::Unknown, "Unexpected exception caught!",
                     _FILEINFO_);
  }

}
