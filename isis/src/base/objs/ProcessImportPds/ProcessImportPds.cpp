/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "ProcessImportPds.h"

#include <QString>

#include <iostream>
#include <QString>
#include <sstream>

#include "IException.h"
#include "ImportPdsTable.h"
#include "IString.h"
#include "LineManager.h"
#include "OriginalLabel.h"
#include "PixelType.h"
#include "Preference.h"
#include "Projection.h"
#include "TProjection.h"
#include "Pvl.h"
#include "PvlKeyword.h"
#include "PvlGroup.h"
#include "PvlObject.h"
#include "PvlToPvlTranslationManager.h"
#include "SpecialPixel.h"
#include "Table.h"
#include "UserInterface.h"

using namespace std;
namespace Isis {

  /**
  * Constructor.
  */
  ProcessImportPds::ProcessImportPds() {
    p_keepOriginalLabel = true;
    p_encodingType = NONE;
    p_jp2File.clear();
    p_projectionOffsetChange = false;
    p_projectionOffsetGroup.setName("ProjectionOffsets");

    // Set up a translater for PDS file of type IMAGE
    p_transDir = "$ISISROOT/appdata";
  }


  ProcessImportPds::~ProcessImportPds() {
  }


  /**
  * @deprecated. Please use Finalize.
  */
  void ProcessImportPds::EndProcess() {
    ProcessImportPds::Finalize();
  }


  /**
   * Extract all possible PDS projection parameters from the PDS label
   *
   * @param pdsXlater
   *
   * @throws Isis::iException::Message
   *
   * @history 2007-04-12 Tracie Sucharski - Modified the projection translation
   *                            tables to include additional versions of
   *                            Longitude direction, latitude type and if
   *                            the min or max longitude values is greater than
   *                            180, change longitude domain to 360.
   *                            Read projection mults/offsets from def file
   *                            so lat/lon values are correct.
   * @history 2007-07-12 Stuart Sides- Modified to handle units
   *                            of meters on the pixel
   *                            resolution
   *
   * @history 2008-06-06 Tracie Sucharski, Added LineProjectionOffset2 for
   *                             Magellan images.
   * @history 2008-06-09 Tracie Sucharski, Added MinimumLongitude2 and
   *                             MaximumLongitude2 for Magellan images.
   *
   */
  void ProcessImportPds::ExtractPdsProjection(Isis::PvlToPvlTranslationManager &pdsXlater) {

    QString str;

    if (pdsXlater.InputHasKeyword("ProjectionName")) {
      p_projection = pdsXlater.Translate("ProjectionName");
    }
    else {
      std::string message = "No projection name in labels";
      throw IException(IException::Unknown, message, _FILEINFO_);
    }

    if (pdsXlater.InputHasKeyword("TargetName")) {
      p_targetName = pdsXlater.Translate("TargetName");
    }
    else {
      std::string message = "No target name in labels";
      throw IException(IException::Unknown, message, _FILEINFO_);
    }

    if (pdsXlater.InputHasKeyword("EquatorialRadius")) {
      str = pdsXlater.Translate("EquatorialRadius");
      p_equatorialRadius = toDouble(str) * 1000.0;
    }
    else {
      std::string message = "No equatorial radius name in labels";
      throw IException(IException::User, message, _FILEINFO_);
    }

    if (pdsXlater.InputHasKeyword("PolarRadius")) {
      str = pdsXlater.Translate("PolarRadius");
      p_polarRadius = toDouble(str) * 1000.0;
    }
    else {
      std::string message = "No polar radius in labels";
      throw IException(IException::User, message, _FILEINFO_);
    }

    if (pdsXlater.InputHasKeyword("LongitudeDirection")) {
      p_longitudeDirection = pdsXlater.Translate("LongitudeDirection");
    }
    else {
      p_longitudeDirection = pdsXlater.Translate("LongitudeDirection2");
    }

    if (p_polarRadius == p_equatorialRadius) {
      p_latitudeType = "Planetocentric";
    }
    else if (pdsXlater.InputHasKeyword("LatitudeType2")) {
      p_latitudeType = pdsXlater.Translate("LatitudeType2");
    }
    else {
      p_latitudeType = pdsXlater.Translate("LatitudeType");
    }

    if (pdsXlater.InputHasKeyword("MinimumLatitude")) {
      str = pdsXlater.Translate("MinimumLatitude");
      try {
        p_minimumLatitude = toDouble(str);
      }
      catch(IException &e) {
        p_minimumLatitude = Isis::NULL8;
      }
    }
    else {
      p_minimumLatitude = Isis::NULL8;
    }

    if (pdsXlater.InputHasKeyword("MaximumLatitude")) {
      str = pdsXlater.Translate("MaximumLatitude");
      try {
        p_maximumLatitude = toDouble(str);
      }
      catch(IException &e) {
        p_maximumLatitude = Isis::NULL8;
      }
    }
    else {
      p_maximumLatitude = Isis::NULL8;
    }

    // This variable represents if the longitudes were read in as
    //   positive west
    bool positiveWest = false;
    if (pdsXlater.InputHasKeyword("MinimumLongitude")) {
      str = pdsXlater.Translate("MinimumLongitude");
      try {
        positiveWest = true;
        p_minimumLongitude = toDouble(str);
      }
      catch(IException &e) {
        p_minimumLongitude = Isis::NULL8;
      }
    }
    else if (pdsXlater.InputHasKeyword("MinimumLongitude2")) {
      str = pdsXlater.Translate("MinimumLongitude2");
      try {
        p_minimumLongitude = toDouble(str);
      }
      catch(IException &e) {
        p_minimumLongitude = Isis::NULL8;
      }
    }
    else {
      p_minimumLongitude = Isis::NULL8;
    }

    if (pdsXlater.InputHasKeyword("MaximumLongitude")) {
      str = pdsXlater.Translate("MaximumLongitude");
      try {
        positiveWest = true;
        p_maximumLongitude = toDouble(str);
      }
      catch(IException &e) {
        p_maximumLongitude = Isis::NULL8;
      }
    }
    else if (pdsXlater.InputHasKeyword("MaximumLongitude2")) {
      str = pdsXlater.Translate("MaximumLongitude2");
      try {
        p_maximumLongitude = toDouble(str);
      }
      catch(IException &e) {
        p_maximumLongitude = Isis::NULL8;
      }
    }
    else {
      p_maximumLongitude = Isis::NULL8;
    }

    str = pdsXlater.Translate("LongitudeDomain");
    p_longitudeDomain = toInt(str);

    /**
     * The input file does not have a longitude domain.
     * We need to figure it out!
     *
     * The current process is two-step. First, we use the
     * longitude direction to swap into what should be the
     * proper order of min,max longitude. Then, if the values
     * are still misordered, we have a 180 domain projection.
     * Try converting the minimum to 180 domain, which hopefully
     * results in ordering the min,max properly. Only do this to
     * the minimum because if they are out of order, then it must
     * be something like 330-30 which needs to be -30-30.
     *
     * pdsImageProjection.trn assumes EasternMost is the MINIMUM,
     * which is PositiveWest. For a PositiveEast image this
     * swap should occur. On a PositiveWest image this swap should not
     * occur.
     */
    if (positiveWest && (p_longitudeDirection.compare("PositiveEast") == 0)) {
      double tmp = p_minimumLongitude;
      p_minimumLongitude = p_maximumLongitude;
      p_maximumLongitude = tmp;
    }

    if (p_minimumLongitude > p_maximumLongitude) {
      // Force the change to 180
      p_longitudeDomain = 180;
      p_minimumLongitude = Isis::TProjection::To180Domain(p_minimumLongitude);
    }

    //  If either the minimumLongitude or maximumLongitude are < 0, change
    //  longitude Domain to 180.
    if (p_minimumLongitude < 0 || p_maximumLongitude < 0) {
      p_longitudeDomain = 180;
    }

    str = pdsXlater.Translate("PixelResolution");
    p_pixelResolution = toDouble(str);
    str = QString::fromStdString(pdsXlater.InputKeyword("PixelResolution").unit()).toUpper();
    // Assume KM/PIXEL if the unit doesn't exist or is not METERS/PIXEL
    if ((str != "METERS/PIXEL") && (str != "M/PIXEL") && (str != "M/PIX")) {
      p_pixelResolution *= 1000.0;
    }

    str = pdsXlater.Translate("Scale");
    p_scaleFactor = toDouble(str);

    try {
      str = pdsXlater.Translate("Rotation");
      p_rotation = toDouble(str);
    }
    catch(IException &) {
      // assume no rotation if the value isn't a number
      p_rotation = 0.0;
    }

    //  Look for projection offsets/mults to convert between line/samp and x/y
    double xoff, yoff, xmult, ymult;
    GetProjectionOffsetMults(xoff, yoff, xmult, ymult);

    if (pdsXlater.InputHasKeyword("LineProjectionOffset")) {
      str = pdsXlater.Translate("LineProjectionOffset");
    }
    else {
      str = pdsXlater.Translate("LineProjectionOffset2");
    }
    p_lineProjectionOffset = toDouble(str);
    p_upperLeftY = ymult * (p_lineProjectionOffset + yoff) * p_pixelResolution;

    if (pdsXlater.InputHasKeyword("SampleProjectionOffset")) {
      str = pdsXlater.Translate("SampleProjectionOffset");
    }
    else {
      str = pdsXlater.Translate("SampleProjectionOffset2");
    }
    p_sampleProjectionOffset = toDouble(str);
    p_upperLeftX = xmult * (p_sampleProjectionOffset + xoff) * p_pixelResolution;

    p_projectionOffsetGroup.addKeyword(PvlKeyword("xOffset", std::to_string(xoff)));
    p_projectionOffsetGroup.addKeyword(PvlKeyword("yOffset", std::to_string(yoff)));
    p_projectionOffsetGroup.addKeyword(PvlKeyword("xMultiplier", std::to_string(xmult)));
    p_projectionOffsetGroup.addKeyword(PvlKeyword("yMultiplier", std::to_string(ymult)));

  }


  /**
  * End the processing sequence and cleans up by closing cubes,
  * freeing memory, etc. Adds the OriginalLabel data to the end of
  * the cube file, unless OmitOriginalLabel() has been called.
  */
  void ProcessImportPds::Finalize() {
    if (p_keepOriginalLabel) {
      OriginalLabel ol(p_pdsLabel);
      for (unsigned int i = 0; i < OutputCubes.size(); i++) {
        OutputCubes[i]->write(ol);
      }
    }
    Process::Finalize();
  }


  /**
   * Return whether the projection offsets have changed.
   */
  bool ProcessImportPds::GetProjectionOffsetChange() {
    return p_projectionOffsetChange;
  }


  /**
   * Return the projection offsets. This will allow an app to log offset data.
   */
  PvlGroup ProcessImportPds::GetProjectionOffsetGroup() {
    return p_projectionOffsetGroup;
  }


  /**
   * Read mults and offsets from a def file in order to calculate the upper
   * left x/y.
   *
   * @param[out] xoff    (double &) x offset
   * @param[out] yoff    (double &) y offset
   * @param[out] xmult   (double &) x multiplicative factor
   * @param[out] ymult   (double &) y multiplicative factor
   *
   * @history 2007-04-12 Tracie Sucharski - New Method
   *
   */
  void ProcessImportPds::GetProjectionOffsetMults(double &xoff, double &yoff,
      double &xmult, double &ymult) {

    xmult = -1.0;
    ymult = 1.0;
    xoff = 0.5;
    yoff = 0.5;

    //  Open projectionOffsetMults file
    Isis::Pvl p(p_transDir.toStdString() + "/" + "translations/pdsProjectionLineSampToXY.def");

    Isis::PvlObject &projDef = p.findObject("ProjectionOffsetMults",
                                            Pvl::Traverse);

    for(int g = 0; g < projDef.groups(); g++) {
      QString key = QString::fromStdString(projDef.group(g)["Keyword"]);
      if (p_pdsLabel.hasKeyword(key.toStdString())) {
        QString value = QString::fromStdString(p_pdsLabel[key.toStdString()]);
        QString pattern = QString::fromStdString(projDef.group(g)["Pattern"]);
        //  If value contains pattern, then set the mults to what is in translation file and
        //  update p_pdfChange
        if (value.contains(pattern)) {
          xmult = projDef.group(g)["xMult"];
          ymult = projDef.group(g)["yMult"];
          xoff = projDef.group(g)["xOff"];
          yoff = projDef.group(g)["yOff"];
          p_projectionOffsetChange = true;
          return;
        }
      }
    }
  }


  /**
  * Identify the source of this file PDS or ISIS2.
  *
  * @param inputLabel  The label from the input file.
  *
  */
  void ProcessImportPds::IdentifySource(Isis::Pvl &inputLabel) {

    // Create a temporary Isis::PvlToPvlTranslationManager so we can find out what
    // type of input file we have
    stringstream trnsStrm;
    trnsStrm << "Group = PdsFile" << endl;
    trnsStrm << "  InputPosition = ROOT" << endl;
    trnsStrm << "  InputKey = PDS_VERSION_ID" << endl;
    trnsStrm << "EndGroup" << endl;
    trnsStrm << "Group = Isis2File" << endl;
    trnsStrm << "  InputPosition = ROOT" << endl;
    trnsStrm << "  InputKey = CCSD3ZF0000100000001NJPL3IF0PDS200000001" << endl;
    trnsStrm << "EndGroup" << endl;
    trnsStrm << "END";

    Isis::PvlToPvlTranslationManager sourceXlater(inputLabel, trnsStrm);

    if (sourceXlater.InputHasKeyword("PdsFile")) {
      p_source = PDS;
    }
    else if (sourceXlater.InputHasKeyword("Isis2File")) {
      p_source = ISIS2;
    }
    else {
      p_source = NOSOURCE;
    }

  }


  /**
   * This method will import the PDS table with the given name into an Isis
   * Table object. The table will be added to the cube file in the call to
   * StartProcess().
   *
   * @param pdsTableName Name of the PDS table object to be imported.
   */
  Table &ProcessImportPds::ImportTable(QString pdsTableName) {
    // No table file given, let ImportPdsTable find it.
    ImportPdsTable pdsTable(p_labelFile, "", pdsTableName);
    // reformat the table name. If the name ends with the word "Table", remove
    // it. (So, for example, INSTRUMENT_POINTING_TABLE gets formatted to
    // InstrumentPointingTable and then to InstrumentPointing)
    QString isisTableName = pdsTable.getFormattedName(pdsTableName);
    int found = isisTableName.lastIndexOf("Table");
    if (found == isisTableName.length() - 5) {
      isisTableName.remove(found, 5);
    }

    Table isisTable = pdsTable.importTable(isisTableName);
    p_tables.push_back(isisTable);

    return p_tables.at(p_tables.size() - 1);
  }


  /**
   * Return true if ISIS2 cube, else return false
   *
   *  @return (bool) returns true if pds file is an Isis2 file
   *
   *  @history 2007-04-12 Tracie Sucharski - New method
   *
   */
  bool ProcessImportPds::IsIsis2() {

    if (p_source == ISIS2) {

      return true;
    }
    else {
      return false;
    }

  }


  /**
   * Prevents the Original Label blob from being written out to
   * the end of the cube.
   */
  void ProcessImportPds::OmitOriginalLabel() {
    p_keepOriginalLabel = false;
  }


/**
   * Handles the DataFilePointer keyword, aka ^QUBE or ^IMAGE.
   * There are two side effects of this method, those are
   * SetInputFile and SetFileHeaderBytes, both are called during this method.
   * Will not do SetInputFile if calcOffsetOnly is true
   */
  void ProcessImportPds::ProcessDataFilePointer(Isis::PvlToPvlTranslationManager & pdsXlater,
        const bool & calcOffsetOnly) {

    const PvlKeyword & dataFilePointer = pdsXlater.InputKeyword("DataFilePointer");

    QString dataFileName;
    QString units;
    QString str;
    int offset = -1;

    // If only size 1, we either have a file name or an offset
    // Either way, when we're done with these two ifs, variables offset and
    // dataFileName will be set.
    if (dataFilePointer.size() == 1) {
      try {
        str = pdsXlater.Translate("DataFilePointer");
        offset = toInt(str);
        units = QString::fromStdString(dataFilePointer.unit());
        // Successful? we have an offset, means current, p_labelFile
        // is the location of the data as well
        dataFileName = QString::fromStdString(FileName(p_labelFile.toStdString()).name());
      }
      catch(IException &e) {
        // Failed to parse to an int, means we have a file name
        // No offset given, so we use 1, offsets are 1 based
        offset = 1;
        units = "BYTES";
        dataFileName = str;
      }
    }
    // We must have a filename and an offset, in that order
    // Expection ("filname", <offset>)
    else if (dataFilePointer.size() == 2) {
      dataFileName = pdsXlater.Translate("DataFilePointer", 0);
      offset = IString(pdsXlater.Translate("DataFilePointer", 1)).ToInteger();
      units = QString::fromStdString(dataFilePointer.unit(1));
    }
    // Error, no value
    else if (dataFilePointer.size() == 0) {
      std::string msg = "Data file pointer ^IMAGE or ^QUBE has no value, must"
                   "have either file name or offset or both, in [" +
                   p_labelFile + "]";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }
    // Error, more than two values
    else {
      std::string msg = "Improperly formatted data file pointer keyword ^IMAGE or "
                   "^QUBE, in [" + p_labelFile + "], must contain filename "
                   " or offset or both";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }

    // Now, to handle the values we found
    // the filename first, only do so if calcOffsetOnly is false
    if (!calcOffsetOnly) {
      Isis::FileName labelFile(p_labelFile.toStdString());

      // If dataFileName isn't empty, and does start at the root, use it
      Isis::FileName dataFile;
      if (dataFileName.size() != 0 && dataFileName.at(0) == '/')
        dataFile = FileName(dataFileName.toStdString());
      // Otherwise, use the path to it and its name
      else
        dataFile = FileName(labelFile.path() + "/" + dataFileName.toStdString());

      // If it exists, use it
      if (dataFile.fileExists()) {
        SetInputFile(QString::fromStdString(dataFile.expanded()));
      }
      // Retry with downcased name, if still no luck, fail
      else {
        QString tmp = QString::fromStdString(dataFile.expanded());
        dataFileName = dataFileName.toLower();
        dataFile = FileName(labelFile.path() + "/" + dataFileName.toStdString());
        if (dataFile.fileExists()) {
          SetInputFile(QString::fromStdString(dataFile.expanded()));
        }
        else {
          std::string msg = "Unable to find input file [" + tmp.toStdString() + "] or [" +
                       dataFile.expanded() + "]";
          throw IException(IException::Io, msg, _FILEINFO_);
        }
      }
    }

    // Now, to handle the offset
    units = units.trimmed();
    if (units == "BYTES" || units == "B") {
      SetFileHeaderBytes(offset - 1);
    }
    else {
      QString recSize = pdsXlater.Translate("DataFileRecordBytes");
      SetFileHeaderBytes((offset - 1) * toInt(recSize));
    }
  }


/**
   * Load the PDS labels after determining what type of data file was provided.
   * This code used to be part of the SetPdsFile method, but had to be moved to
   * a separate method in order to allow calling applications to intercept the PDS
   * labels before this method loaded them. This was necessary to fix problems in
   * PDS labels such as adding missing keywords.
   *
   * @param pdsDataFile The name of the PDS data file where the actual image/cube
   *                    data is stored. This parameter can be an empty QString, in
   *                    which case the label information will be searched to find
   *                    the data file name or the data will be assumed to be
   *                    after the label information.
   *
   * @throws Isis::iException::Message
   */
  void ProcessImportPds::ProcessLabel(const QString &pdsDataFile, PdsFileType allowedTypes) {
    // Create a temporary Isis::PvlToPvlTranslationManager so we can find out what
    // type of PDS file this is (i.e., Qube or Image or SpectralQube)
    stringstream trnsStrm;
    trnsStrm << "Group = PdsTypeImage" << endl;
    trnsStrm << "  InputPosition = ROOT" << endl;
    trnsStrm << "  InputPosition = FILE" << endl;
    trnsStrm << "  InputPosition = UNCOMPRESSED_FILE" << endl;
    trnsStrm << "  InputKey = ^IMAGE" << endl;
    trnsStrm << "EndGroup" << endl;
    trnsStrm << "Group = PdsCombinedSpectrum" << endl;
    trnsStrm << "  InputPosition = ROOT" << endl;
    trnsStrm << "  InputPosition = FILE" << endl;
    trnsStrm << "  InputPosition = UNCOMPRESSED_FILE" << endl;
    trnsStrm << "  InputKey = ^COMBINED_SPECTRUM" << endl;
    trnsStrm << "EndGroup" << endl;
    trnsStrm << "Group = PdsTypeL0" << endl;
    trnsStrm << "  InputPosition = L0_FILE" << endl;
    trnsStrm << "  InputKey = ^L0_IMAGE" << endl;
    trnsStrm << "EndGroup" << endl;
    trnsStrm << "Group = PdsTypeRdn" << endl;
    trnsStrm << "  InputPosition = RDN_FILE" << endl;
    trnsStrm << "  InputKey = ^RDN_IMAGE" << endl;
    trnsStrm << "EndGroup" << endl;
    trnsStrm << "Group = PdsTypeLoc" << endl;
    trnsStrm << "  InputPosition = LOC_FILE" << endl;
    trnsStrm << "  InputKey = ^LOC_IMAGE" << endl;
    trnsStrm << "EndGroup" << endl;
    trnsStrm << "Group = PdsTypeObs" << endl;
    trnsStrm << "  InputPosition = OBS_FILE" << endl;
    trnsStrm << "  InputKey = ^OBS_IMAGE" << endl;
    trnsStrm << "EndGroup" << endl;
    trnsStrm << "Group = PdsTypeQube" << endl;
    trnsStrm << "  InputKey = ^QUBE" << endl;
    trnsStrm << "EndGroup" << endl;
    trnsStrm << "Group = PdsTypeSpectralQube" << endl;
    trnsStrm << "  InputKey = ^SPECTRAL_QUBE" << endl;
    trnsStrm << "EndGroup" << endl;
    trnsStrm << "Group = PdsEncodingType" << endl;
    trnsStrm << "  InputPosition = COMPRESSED_FILE" << endl;
    trnsStrm << "  InputKey = ENCODING_TYPE" << endl;
    trnsStrm << "  Translation = (*,*)" << endl;
    trnsStrm << "EndGroup" << endl;
    trnsStrm << "Group = PdsCompressedFile" << endl;
    trnsStrm << "  InputPosition = COMPRESSED_FILE" << endl;
    trnsStrm << "  InputKey = FILE_NAME" << endl;
    trnsStrm << "  Translation = (*,*)" << endl;
    trnsStrm << "EndGroup" << endl;
    trnsStrm << "END";


    //tjw
    //Determine if we are processing a QUBE whose
    //core data type is VAX_REAL

    try {
      PvlObject obj = p_pdsLabel.findObject("QUBE");
      PvlKeyword coreKey = obj.findKeyword("CORE_ITEM_TYPE");
      PvlKeyword suffixKey = obj.findKeyword("BAND_SUFFIX_ITEM_TYPE");
      //if ( (coreKey[0] == "VAX_REAL") && (suffixKey[0] =="VAX_REAL") )

      if (coreKey[0] == "VAX_REAL") {
        ProcessImport::SetVAXConvert(true);
      }
    }
    catch (IException &e) {
    }

    Isis::PvlToPvlTranslationManager pdsXlater(p_pdsLabel, trnsStrm);

    // Check to see if we are dealing with a JPEG2000 file
    QString str;
    if (pdsXlater.InputHasKeyword("PdsEncodingType")) {
      str = pdsXlater.Translate("PdsEncodingType");
      if (str == "JP2") {
        p_encodingType = JP2;
        str = pdsXlater.Translate("PdsCompressedFile");
        if (pdsDataFile.isEmpty()) {
          Isis::FileName lfile(p_labelFile.toStdString());
          Isis::FileName ifile(lfile.path() + "/" + str.toStdString());
          if (ifile.fileExists()) {
            p_jp2File = QString::fromStdString(ifile.expanded());
          }
          else {
            QString tmp = QString::fromStdString(ifile.expanded());
            str = str.toLower();
            ifile = lfile.path() + "/" + str.toStdString();
            if (ifile.fileExists()) {
              p_jp2File = QString::fromStdString(ifile.expanded());
            }
            else {
              std::string msg = "Unable to find input file [" + tmp.toStdString() + "] or [" +
                           ifile.expanded() + "]";
              throw IException(IException::Io, msg, _FILEINFO_);
            }
          }
        }
      }
      else {
        std::string msg = "Unsupported encoding type in [" + p_labelFile + "]";
        throw IException(IException::Io, msg, _FILEINFO_);
      }
    }


    // Call the correct label processing
    if ((allowedTypes & Image) == Image && pdsXlater.InputHasKeyword("PdsTypeImage")) {

      ProcessPdsImageLabel(pdsDataFile);
    }
    else if ((allowedTypes & Qube) == Qube && pdsXlater.InputHasKeyword("PdsTypeQube")) {

      ProcessPdsQubeLabel(pdsDataFile, "pdsQube.trn");
    }
    else if ((allowedTypes & SpectralQube) == SpectralQube &&
             pdsXlater.InputHasKeyword("PdsTypeSpectralQube")) {

      ProcessPdsQubeLabel(pdsDataFile, "pdsSpectralQube.trn");
    }
    else if ((allowedTypes & L0) == L0 && pdsXlater.InputHasKeyword("PdsTypeL0")) {

      ProcessPdsM3Label(pdsDataFile, L0);
    }
    else if ((allowedTypes & Rdn) == Rdn && pdsXlater.InputHasKeyword("PdsTypeRdn")) {

      ProcessPdsM3Label(pdsDataFile, Rdn);
    }
    else if ((allowedTypes & Loc) == Loc && pdsXlater.InputHasKeyword("PdsTypeLoc")) {

      ProcessPdsM3Label(pdsDataFile, Loc);
    }
    else if ((allowedTypes & Obs) == Obs && pdsXlater.InputHasKeyword("PdsTypeObs")) {

      ProcessPdsM3Label(pdsDataFile, Obs);
    }
    else if ((allowedTypes & CombinedSpectrum) == CombinedSpectrum &&
              pdsXlater.InputHasKeyword("PdsCombinedSpectrum")) {

      ProcessPdsCombinedSpectrumLabel(pdsDataFile);
    }
    else {
      std::string msg = "Unknown label type in [" + p_labelFile + "]. It is possible the label file "
+                    "does not describe an image product (IMAGE, CUBE, or SPECTRALCUBE).";
      throw IException(IException::Io, msg, _FILEINFO_);
    }


    // Find out if this is a PDS file or an ISIS2 file
    IdentifySource(p_pdsLabel);


    return;
  }


  /**
   * Process the PDS label of type CombinedSpectrum.
   *
   * @param pdsDataFile The name of the PDS data file where the actual image/cube
   *                    data is stored. This parameter can be an empty QString, in
   *                    which case the label information will be searched to find
   *                    the data file name or the data will be assumed to be
   *                    after the label information.
   *
   * @throws Isis::iException::Message
   */
  void ProcessImportPds::ProcessPdsCombinedSpectrumLabel(const QString &pdsDataFile) {
    Isis::FileName transFile(p_transDir.toStdString() + "/translations/pdsCombinedSpectrum.trn");
    Isis::PvlToPvlTranslationManager pdsXlater(p_pdsLabel, QString::fromStdString(transFile.expanded()));

    QString str;

    str = pdsXlater.Translate("CoreLinePrefixBytes");
    SetDataPrefixBytes(toInt(str));

    str = pdsXlater.Translate("CoreLineSuffixBytes");
    SetDataSuffixBytes(toInt(str));

    ProcessPixelBitandType(pdsXlater);

    str = pdsXlater.Translate("CoreByteOrder");
    SetByteOrder(Isis::ByteOrderEnumeration(str));

    str = pdsXlater.Translate("CoreSamples");
    int ns = toInt(str);
    str = pdsXlater.Translate("CoreLines");
    int nl = toInt(str);
    str = pdsXlater.Translate("CoreBands");
    int nb = toInt(str);

    SetDimensions(ns, nl, nb);

    //-----------------------------------------------------------------
    // Find the data filename it may be the same as the label file
    // OR the label file may contain a pointer to the data
    //-----------------------------------------------------------------

    // Use the name supplied by the application if it is there
    if (pdsDataFile.length() > 0) {
      ProcessDataFilePointer(pdsXlater, false);
    }
    // If the data is in JPEG 2000 format, then use the name of the file
    // from the label
    else if (p_jp2File.length() > 0) {
      SetInputFile(p_jp2File);
      ProcessDataFilePointer(pdsXlater, true);
    }
    // Use the "^IMAGE or ^QUBE" label to get the filename for the image data
    // Get the path portion from user entered label file spec
    else {
      // Handle filename and offset
      ProcessDataFilePointer(pdsXlater, false);
    }

    //------------------------------------------------------------
    // Find the image data base and multiplier
    //------------------------------------------------------------
    str = pdsXlater.Translate("CoreBase");
    SetBase(toDouble(str));
    str = pdsXlater.Translate("CoreMultiplier");
    SetMultiplier(toDouble(str));

    // Find the organization of the image data
    str = pdsXlater.Translate("CoreOrganization");

    if (p_encodingType == JP2) {
      SetOrganization(ProcessImport::JP2);
    }
    else if (str == "BSQ") {
      SetOrganization(ProcessImport::BSQ);
    }
    else if (str == "BIP") {
      SetOrganization(ProcessImport::BIP);
    }
    else if (str == "BIL") {
      SetOrganization(ProcessImport::BIL);
    }
    else {
      std::string msg = "Unsupported axis order [" + str + "]";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }


  /**
   * Process the PDS label of type QUBE or SPECTRALQUBE.
   *
   * @param pdsDataFile The name of the PDS data file where the actual image/cube
   *                    data is stored. This parameter can be an empty QString, in
   *                    which case the label information will be searched to find
   *                    the data file name or the data will be assumed to be
   *                    after the label information.
   *
   * @param transFile
   *
   * @throws Isis::iException::Message
   *
   * @history 2010-12-09 Sharmila Prasad - Set default offset to be 1 for detatched label
   *                                       and offset not set
   */
  void ProcessImportPds::ProcessPdsQubeLabel(const QString &pdsDataFile,
      const QString &transFile) {


    Isis::FileName tFile(p_transDir.toStdString() + "/translations/" + transFile.toStdString());

    Isis::PvlToPvlTranslationManager pdsXlater(p_pdsLabel, QString::fromStdString(tFile.expanded()));

    QString str;


    // Find the organization of the image data
    // Save off which axis the samples, lines and bands are on
    int linePos = 0;
    int samplePos = 0;
    int bandPos = 0;
    int val = pdsXlater.InputKeyword("CoreOrganization").size();
    QString tmp = "";
    for(int i = 0; i < val; i++) {
      str = pdsXlater.Translate("CoreOrganization", i);
      tmp += str;
      if (str == "SAMPLE") {
        samplePos = i;
      }
      else if (str == "LINE") {
        linePos = i;
      }
      else if (str == "BAND") {
        bandPos = i;
      }
      else {
        std::string message = "Unknown file axis name [" + str + "]";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }

    if (p_encodingType == JP2) {
      SetOrganization(ProcessImport::JP2);
    }
    else if (tmp == "SAMPLELINEBAND") {
      SetOrganization(ProcessImport::BSQ);
    }
    else if (tmp == "BANDSAMPLELINE") {
      SetOrganization(ProcessImport::BIP);
    }
    else if (tmp == "SAMPLEBANDLINE") {
      SetOrganization(ProcessImport::BIL);
    }
    else {
      PvlKeyword pdsCoreOrg = p_pdsLabel.findKeyword(pdsXlater.
          InputKeywordName("CoreOrganization").toStdString(), Pvl::Traverse);

      stringstream pdsCoreOrgStream;
      pdsCoreOrgStream << pdsCoreOrg;

      std::string msg = "Unsupported axis order [" + QString(pdsCoreOrgStream.str().c_str()) + "]";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }


    // Set the number of byte preceding the second dimension (left side plane)
    // There are no capabilities in a PDS QUBE for this
    SetDataPrefixBytes(0);

    // Set the number of bytes following the second dimension (right side plane)
    str = pdsXlater.Translate("SuffixItemSize");
    int suffix = toInt(str);
    str = pdsXlater.Translate("AxisSuffixCount", 0);
    suffix *= toInt(str);
    SetDataSuffixBytes(suffix);

    str = pdsXlater.Translate("SuffixItemSize");

    // Only set DataTrailerBytes if we haven't already set it elsewhere. (It's inialized to 0.)
    if (DataTrailerBytes() == 0) {
      int trailer = toInt(str);
      str = pdsXlater.Translate("AxisSuffixCount", 1);
      trailer *= toInt(str);
      str = pdsXlater.Translate("CoreSamples", samplePos);
      trailer *= toInt(str);
      trailer += suffix;
      SetDataTrailerBytes(trailer);
    }

    // Save the Data Trailer if it exists
    if (DataTrailerBytes() != 0) {
      SaveDataTrailer();
    }

    ProcessPixelBitandType(pdsXlater);

    // Set the byte order

    //tjw:
    str = pdsXlater.Translate("CoreByteOrder");

    SetByteOrder(Isis::ByteOrderEnumeration(str));

    //if(str == "LSB" || str == "MSB")
    //    SetByteOrder(Isis::ByteOrderEnumeration(str));
    //else {
    //    std::string msg = "Unrecognized byte order ["+str+"]";
    //    throw IException(IException::Programmer,msg,_FILEINFO_);
    //}



    // Set the number of samples, lines and bands
    str = pdsXlater.Translate("CoreSamples", samplePos);
    int ns = toInt(str);
    str = pdsXlater.Translate("CoreLines", linePos);
    int nl = toInt(str);
    str = pdsXlater.Translate("CoreBands", bandPos);
    int nb = toInt(str);
    SetDimensions(ns, nl, nb);


    // Set any special pixels values, qube, so use true
    ProcessSpecialPixels(pdsXlater, true);

    //---------------------------------------------------------------
    // Find the data filename, it may be the same as the label file
    // Or the label file may contain a pointer to the data
    //---------------------------------------------------------------

    // Use the name supplied by the application if it is there
    if (pdsDataFile.length() > 0) {
      SetInputFile(pdsDataFile);
      ProcessDataFilePointer(pdsXlater, true);
    }
    // If the data is in JPEG 2000 format, then use the name of the file
    // from the label
    else if (p_jp2File.length() > 0) {
      SetInputFile(p_jp2File);
      ProcessDataFilePointer(pdsXlater, true);
    }
    else {
      // Handle filename and offset
      ProcessDataFilePointer(pdsXlater, false);
    }


    //------------------------------------------------------------
    // Find the image data base and multiplier
    //------------------------------------------------------------
    // First see if there are base and multiplier in the band bin group
    if ((pdsXlater.InputHasKeyword("BandBase")) &&
        (pdsXlater.InputHasKeyword("BandMultiplier"))) {
      vector<double> bases;
      vector<double> mults;
      for(int i = 0; i < pdsXlater.InputKeyword("BandBase").size(); i++) {
        str = pdsXlater.Translate("BandBase", i);
        bases.push_back(toDouble(str));
        str = pdsXlater.Translate("BandMultiplier", i);
        mults.push_back(toDouble(str));
      }
      SetBase(bases);
      SetMultiplier(mults);
    }
    else {
      str = pdsXlater.Translate("CoreBase");
      SetBase(toDouble(str));
      str = pdsXlater.Translate("CoreMultiplier");
      SetMultiplier(toDouble(str));
    }
  }


  /**
   * Process the PDS label of type IMAGE.
   *
   * @param pdsDataFile The name of the PDS data file where the actual image/cube
   *                    data is stored. This parameter can be an empty QString, in
   *                    which case the label information will be searched to find
   *                    the data file name or the data will be assumed to be
   *                    after the label information.
   *
   * @throws Isis::iException::Message
   */
  void ProcessImportPds::ProcessPdsImageLabel(const QString &pdsDataFile) {
    Isis::FileName transFile(p_transDir.toStdString() + "/translations/pdsImage.trn");
    Isis::PvlToPvlTranslationManager pdsXlater(p_pdsLabel, QString::fromStdString(transFile.expanded()));

    QString str;

    str = pdsXlater.Translate("CoreLinePrefixBytes");
    SetDataPrefixBytes(toInt(str));

    str = pdsXlater.Translate("CoreLineSuffixBytes");
    SetDataSuffixBytes(toInt(str));

    ProcessPixelBitandType(pdsXlater);

    str = pdsXlater.Translate("CoreByteOrder");
    SetByteOrder(Isis::ByteOrderEnumeration(str));

    str = pdsXlater.Translate("CoreSamples");
    int ns = toInt(str);
    str = pdsXlater.Translate("CoreLines");
    int nl = toInt(str);
    str = pdsXlater.Translate("CoreBands");
    int nb = toInt(str);

    SetDimensions(ns, nl, nb);

    // Set any special pixel values, not qube, so use false
    ProcessSpecialPixels(pdsXlater, false);

    //-----------------------------------------------------------------
    // Find the data filename it may be the same as the label file
    // OR the label file may contain a pointer to the data
    //-----------------------------------------------------------------

    // Use the name supplied by the application if it is there
    if (pdsDataFile.length() > 0) {
      SetInputFile(pdsDataFile);
      ProcessDataFilePointer(pdsXlater, true);
    }
    // If the data is in JPEG 2000 format, then use the name of the file
    // from the label
    else if (p_jp2File.length() > 0) {
      SetInputFile(p_jp2File);
      ProcessDataFilePointer(pdsXlater, true);
    }
    // Use the "^IMAGE or ^QUBE" label to get the filename for the image data
    // Get the path portion from user entered label file spec
    else {
      // Handle filename and offset
      ProcessDataFilePointer(pdsXlater, false);
    }

    //------------------------------------------------------------
    // Find the image data base and multiplier
    //------------------------------------------------------------
    str = pdsXlater.Translate("CoreBase");
    SetBase(toDouble(str));
    str = pdsXlater.Translate("CoreMultiplier");
    SetMultiplier(toDouble(str));

    // Find the organization of the image data
    str = pdsXlater.Translate("CoreOrganization");

    if (p_encodingType == JP2) {
      SetOrganization(ProcessImport::JP2);
    }
    else if (str == "BSQ") {
      SetOrganization(ProcessImport::BSQ);
    }
    else if (str == "BIP") {
      SetOrganization(ProcessImport::BIP);
    }
    else if (str == "BIL") {
      SetOrganization(ProcessImport::BIL);
    }
    else {
      std::string msg = "Unsupported axis order [" + str + "]";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }


  /**
   * Handles PixelType and BitsPerPixel
   * Calls SetPixelType with the correct values
   */
  void ProcessImportPds::ProcessPixelBitandType(Isis::PvlToPvlTranslationManager & pdsXlater) {
    QString str;
    str = pdsXlater.Translate("CoreBitsPerPixel");
    int bitsPerPixel = toInt(str);
    str = pdsXlater.Translate("CorePixelType");

    if ((str == "Real") && (bitsPerPixel == 64)) {

      SetPixelType(Isis::Double);
    }
    else if ((str == "Real") && (bitsPerPixel == 32)) {
      SetPixelType(Isis::Real);
    }
    else if ((str == "Integer") && (bitsPerPixel == 8)) {
      SetPixelType(Isis::UnsignedByte);
    }
    else if ((str == "Integer") && (bitsPerPixel == 16)) {
      SetPixelType(Isis::SignedWord);
    }
    else if ((str == "Integer") && (bitsPerPixel == 32)) {
      SetPixelType(Isis::SignedInteger);
    }
    else if ((str == "Natural") && (bitsPerPixel == 8)) {
      SetPixelType(Isis::UnsignedByte);
    }
    else if ((str == "Natural") && (bitsPerPixel == 16)) {
      SetPixelType(Isis::UnsignedWord);
    }
    else if ((str == "Natural") && (bitsPerPixel == 16)) {
      SetPixelType(Isis::SignedWord);
    }
    else if ((str == "Natural") && (bitsPerPixel == 32)) {
      SetPixelType(Isis::UnsignedInteger);
    }

    else {
      std::string msg = "Invalid PixelType and BitsPerPixel combination [" + str.toStdString() +
                   ", " + std::to_string(bitsPerPixel) + "]";
      throw IException(IException::Io, msg, _FILEINFO_);
    }
  }


  /**
   * Process Chandrayaan M3 PDS label
   *
   * @param pdsDataFile The name of the PDS data file where the actual image/cube
   *                    data is stored. This parameter can be an empty QString, in
   *                    which case the label information will be searched to find
   *                    the data file name or the data will be assumed to be
   *                    after the label information.
   *
   * @throws Isis::iException::Message
   */
  void ProcessImportPds::ProcessPdsM3Label(const QString &pdsDataFile, PdsFileType fileType) {
    Isis::FileName transFile;
    if (fileType == L0) {
      transFile = p_transDir.toStdString() + "/translations/pdsL0.trn";
    }
    else if (fileType == Rdn) {
      transFile = p_transDir.toStdString() + "/translations/pdsRdn.trn";
    }
    else if (fileType == Loc) {
      transFile = p_transDir.toStdString() + "/translations/pdsLoc.trn";
    }
    else if (fileType == Obs) {
      transFile = p_transDir.toStdString() + "/translations/pdsObs.trn";
    }
    else {
      throw IException(IException::Programmer, "ProcessImportPds::ProcessPdsM3Label can only be "
                       "called with file type of L0, Rdn, Loc or Obs.", _FILEINFO_);
    }
    Isis::PvlToPvlTranslationManager pdsXlater(p_pdsLabel, QString::fromStdString(transFile.expanded()));

    QString str;

    //  L1B (Rdn) products do not have an prefix or suffix data.  L0 have 1280 bytes at the
    //  beginning of each line of the BIL formatted file.
    str = pdsXlater.Translate("CoreLinePrefixBytes");
    SetDataHeaderBytes(toInt(str));

    str = pdsXlater.Translate("CoreLineSuffixBytes");
    SetDataSuffixBytes(toInt(str));

    ProcessPixelBitandType(pdsXlater);

    str = pdsXlater.Translate("CoreByteOrder");
    SetByteOrder(Isis::ByteOrderEnumeration(str));

    str = pdsXlater.Translate("CoreSamples");
    int ns = toInt(str);
    str = pdsXlater.Translate("CoreLines");
    int nl = toInt(str);
    str = pdsXlater.Translate("CoreBands");
    int nb = toInt(str);
    SetDimensions(ns, nl, nb);

    // Set any special pixel values, not qube, so use false
    ProcessSpecialPixels(pdsXlater, false);

    //-----------------------------------------------------------------
    // Find the data filename it may be the same as the label file
    // OR the label file may contain a pointer to the data
    //-----------------------------------------------------------------

    // Use the name supplied by the application if it is there
    if (pdsDataFile.length() > 0) {
      SetInputFile(pdsDataFile);
      ProcessDataFilePointer(pdsXlater, true);
    }
    // If the data is in JPEG 2000 format, then use the name of the file
    // from the label
    else if (p_jp2File.length() > 0) {
      SetInputFile(p_jp2File);
      ProcessDataFilePointer(pdsXlater, true);
    }
    // Use the "^IMAGE or ^QUBE" label to get the filename for the image data
    // Get the path portion from user entered label file spec
    else {
      // Handle filename and offset
      ProcessDataFilePointer(pdsXlater, false);
    }

    //------------------------------------------------------------
    // Find the image data base and multiplier
    //------------------------------------------------------------
    str = pdsXlater.Translate("CoreBase");
    SetBase(toDouble(str));
    str = pdsXlater.Translate("CoreMultiplier");
    SetMultiplier(toDouble(str));

    // Find the organization of the image data
    str = pdsXlater.Translate("CoreOrganization");

    if (p_encodingType == JP2) {
      SetOrganization(ProcessImport::JP2);
    }
    else if (str == "BSQ") {
      SetOrganization(ProcessImport::BSQ);
    }
    else if (str == "BIP") {
      SetOrganization(ProcessImport::BIP);
    }
    else if (str == "BIL") {
      SetOrganization(ProcessImport::BIL);
    }
    else {
      std::string msg = "Unsupported axis order [" + str + "]";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }


  /**
   * Handles all special pixel setting, ultimately, calls SetSpecialValues.
   */
  void ProcessImportPds::ProcessSpecialPixels(Isis::PvlToPvlTranslationManager & pdsXlater, const bool & isQube) {
    QString str;
    // Set any special pixel values
    double pdsNull = Isis::NULL8;
    if (pdsXlater.InputHasKeyword("CoreNull")) {
      str = pdsXlater.Translate("CoreNull");
      if (str != "NULL") {
        pdsNull = toDouble(str);
      }
    }
    else if (!isQube && pdsXlater.InputHasKeyword("CoreNull2")) {
      str = pdsXlater.Translate("CoreNull2");
      if (str != "NULL") {
        pdsNull = toDouble(str);
      }
    }

    double pdsLrs = Isis::Lrs;
    if (pdsXlater.InputHasKeyword("CoreLrs")) {
      str = pdsXlater.Translate("CoreLrs");
      if (str != "NULL") {
        pdsLrs = toDouble(str);
      }
    }
    else if (!isQube && pdsXlater.InputHasKeyword("CoreLrs2")) {
      str = pdsXlater.Translate("CoreLrs2");
      if (str != "NULL") {
        pdsLrs = toDouble(str);
      }
    }

    double pdsLis = Isis::Lis;
    if (pdsXlater.InputHasKeyword("CoreLis")) {
      str = pdsXlater.Translate("CoreLis");
      if (str != "NULL") {
        pdsLis = toDouble(str);
      }
    }
    else if (!isQube && pdsXlater.InputHasKeyword("CoreLis2")) {
      str = pdsXlater.Translate("CoreLis2");
      if (str != "NULL") {
        pdsLis = toDouble(str);
      }
    }

    double pdsHrs = Isis::Hrs;
    if (pdsXlater.InputHasKeyword("CoreHrs")) {
      str = pdsXlater.Translate("CoreHrs");
      if (str != "NULL") {
        pdsHrs = toDouble(str);
      }
    }
    else if (!isQube && pdsXlater.InputHasKeyword("CoreHrs2")) {
      str = pdsXlater.Translate("CoreHrs2");
      if (str != "NULL") {
        pdsHrs = toDouble(str);
      }
    }

    double pdsHis = Isis::His;
    if (pdsXlater.InputHasKeyword("CoreHis")) {
      str = pdsXlater.Translate("CoreHis");
      if (str != "NULL") {
        pdsHis = toDouble(str);
      }
    }
    else if (!isQube && pdsXlater.InputHasKeyword("CoreHis2")) {
      str = pdsXlater.Translate("CoreHis2");
      if (str != "NULL") {
        pdsHis = toDouble(str);
      }
    }

    SetSpecialValues(pdsNull, pdsLrs, pdsLis, pdsHrs, pdsHis);
  }


  /**
   * Set the input label PVL and data file and initialize a Pvl with the PDS labels.
   * This method was written to allow the calling application to intercept the PDS
   * labels and make any modifications to them necessary (such as adding missing
   * keywords like TARGET_NAME) before this class loads them. See the kaguyatc2isis
   * program for an example.
   *
   * @param pdsLabelPvl The PVL containing the PDS label.
   *
   * @param pdsDataFile The name of the PDS data file where the actual image/cube
   *                    data is stored. This parameter cannot be empty.
   *
   * @throws Isis::iException::Message
   */
  void ProcessImportPds::SetPdsFile(const Isis::Pvl &pdsLabelPvl,
                                    const QString &pdsDataFile,
                                    PdsFileType allowedTypes) {

    // Save the label and file for future use
    p_pdsLabel = pdsLabelPvl;
    p_labelFile = pdsDataFile;
    ProcessLabel(pdsDataFile, allowedTypes);
  }


  /**
   * Set the input label file, data file and initialize a Pvl with the PDS labels.
   *
   * @param pdsLabelFile The name of the PDS label file.This must be the file
   *                     where the label is. It can be an attached or detached
   *                     label.
   *
   * @param pdsDataFile The name of the PDS data file where the actual image/cube
   *                    data is stored. This parameter can be an empty QString, in
   *                    which case the label information will be searched to find
   *                    the data file name or the data will be assumed to be
   *                    after the label information.
   *
   * @param pdsLabel  The label from the input PDS/Isis2 file
   *
   * @throws Isis::iException::Message
   */
  void ProcessImportPds::SetPdsFile(const QString &pdsLabelFile,
                                    const QString &pdsDataFile,
                                    Isis::Pvl &pdsLabel,
                                    PdsFileType allowedTypes) {

    // Internalize the PDS label in the PVL that was passed in
    try {
      pdsLabel.read(pdsLabelFile.toStdString());
    }
    catch (IException &e) {
      throw IException(e, IException::User,
                       QObject::tr("This image does not contain a pds label.  You will need an "
                                   "image with a PDS label or a detached PDS label for this "
                                   "image."), _FILEINFO_);
    }

    // Save the label and file for future use
    p_pdsLabel = pdsLabel;
    p_labelFile = pdsLabelFile;
    ProcessLabel(pdsDataFile, allowedTypes);
  }


  /**
   * This method will write the cube and table data to the output cube.
   */
  void ProcessImportPds::StartProcess() {
    ProcessImport::StartProcess();
    for (unsigned int i = 0; i < p_tables.size(); i++) {
      OutputCubes[0]->write(p_tables[i]);
    }
    return;
  }


  /**
   * Process the input file and send data to a method for specialized processing. The method is
   * expected to write the data after it has processed it if necessary.
   *
   * @param funct Method that accepts Isis::Buffer as an input
   *              parameter, processes the image, and has no
   *              return value.
   * @throws Isis::iException::Message "File is not a supported
   *             organization."
   */
  void ProcessImportPds::StartProcess(void funct(Isis::Buffer &out)) {
    ProcessImport::StartProcess(funct);
    return;
  }



  /**
   * Fill as many of the Isis BandBin labels as possible
   *
   * @param lab The label where the translated Isis2 keywords will
   *            be placed
   */
  void ProcessImportPds::TranslateIsis2BandBin(Isis::Pvl &lab) {
    // Set up a translater for Isis2 labels
    QString transDir = "$ISISROOT/appdata";

    Isis::FileName transFile(transDir.toStdString() + "/" + "translations/isis2bandbin.trn");
    Isis::PvlToPvlTranslationManager isis2Xlater(p_pdsLabel, QString::fromStdString(transFile.expanded()));

    // Add all the Isis2 keywords that can be translated to the requested label
    isis2Xlater.Auto(lab);
  }


  /**
   * Fill as many of the Isis instrument labels as possible
   *
   * @param lab The label where the tramslated Isis2 keywords will
   *            be placed
   */
  void ProcessImportPds::TranslateIsis2Instrument(Isis::Pvl &lab) {
    // Set up a translater for Isis2 labels
    QString transDir = "$ISISROOT/appdata";

    Isis::FileName transFile(transDir.toStdString() + "/" + "translations/isis2instrument.trn");
    Isis::PvlToPvlTranslationManager isis2Xlater(p_pdsLabel, QString::fromStdString(transFile.expanded()));

    // Add all the Isis2 keywords that can be translated to the requested label
    isis2Xlater.Auto(lab);

    //Check StartTime for appended 'z' (Zulu time) and remove
    Isis::PvlGroup &inst = lab.findGroup("Instrument");

    if (inst.hasKeyword("StartTime")) {
      Isis::PvlKeyword &stkey = inst["StartTime"];
      QString stime = QString::fromStdString(stkey[0]);
      stime = stime.remove(QRegExp("[Zz]$"));
      stkey = stime.toStdString();
    }
  }


  /**
   * Translate as many of the ISIS2 labels as possible
   *
   * @param lab The label where the translated Isis2 keywords will
   *            be placed
   *
   */
  void ProcessImportPds::TranslateIsis2Labels(Isis::Pvl &lab) {
    TranslateIsis2BandBin(lab);
    TranslateIsis2Instrument(lab);
  }


  /**
   * Fill as many of the Isis BandBin labels as possible
   *
   * @param lab The lable where the translated PDS keywords will
   *            be placed
   */
  void ProcessImportPds::TranslatePdsArchive(Isis::Pvl &lab) {
    // Set up a translater for PDS labels
    Isis::FileName transFile(p_transDir.toStdString() + "/" + "translations/pdsImageArchive.trn");
    Isis::PvlToPvlTranslationManager isis2Xlater(p_pdsLabel, QString::fromStdString(transFile.expanded()));

    // Add all the Isis2 keywords that can be translated to the requested label
    isis2Xlater.Auto(lab);
  }


  /**
   * Fill as many of the Isis BandBin labels as possible
   *
   * @param lab The lable where the translated PDS keywords will
   *            be placed
   */
  void ProcessImportPds::TranslatePdsBandBin(Isis::Pvl &lab) {
    // Set up a translater for PDS labels
    Isis::FileName transFile(p_transDir.toStdString() + "/" + "translations/pdsImageBandBin.trn");
    Isis::PvlToPvlTranslationManager isis2Xlater(p_pdsLabel, QString::fromStdString(transFile.expanded()));

    // Add all the Isis2 keywords that can be translated to the requested label
    isis2Xlater.Auto(lab);
  }


  /**
   * Translate as many of the PDS labels as possible
   *
   * @param lab The label where the translated Isis2 keywords will
   *            be placed
   *
   */
  void ProcessImportPds::TranslatePdsLabels(Isis::Pvl &lab) {
    TranslatePdsBandBin(lab);
    TranslatePdsArchive(lab);
  }


  /**
   * Fills the passed in label with the projection information from the PDS label
   * file. The application must write add the projection parameters to the output
   * cube if desired.
   *
   * @param lab The label where the projection parameters will be placed.
   *
   * @history 2008-06-06 Tracie Sucharski - Added new InputGroup for
   *                              PdsProjectionTypeImage to handle Magellan.
   *
   */
  void ProcessImportPds::TranslatePdsProjection(Isis::Pvl &lab) {

    // Create a temporary Isis::PvlToPvlTranslationManager so we can find out what
    // type of projection labels exist
    stringstream trnsStrm;
    trnsStrm << "Group = PdsProjectionTypeImage" << endl;
    trnsStrm << "  InputPosition = IMAGE_MAP_PROJECTION" << endl;
    trnsStrm << "  InputPosition = IMAGE_MAP_PROJECTION_CATALOG" << endl;
    trnsStrm << "  InputKey = MAP_PROJECTION_TYPE" << endl;
    trnsStrm << "EndGroup" << endl;
    trnsStrm << "Group = PdsProjectionTypeQube" << endl;
    trnsStrm << "  InputPosition = (QUBE,IMAGE_MAP_PROJECTION)" << endl;
    trnsStrm << "  InputKey = MAP_PROJECTION_TYPE" << endl;
    trnsStrm << "EndGroup" << endl;
    trnsStrm << "Group = PdsProjectionTypeSpectralQube" << endl;
    trnsStrm << "  InputPosition = (SPECTRAL_QUBE,IMAGE_MAP_PROJECTION)" << endl;
    trnsStrm << "  InputKey = MAP_PROJECTION_TYPE" << endl;
    trnsStrm << "EndGroup" << endl;
    trnsStrm << "END";

    Isis::PvlToPvlTranslationManager projType(p_pdsLabel, trnsStrm);

    // Set up the correct projection translation table for this label
    QString transDir = "$ISISROOT/appdata";

    Isis::FileName transFile;
    if (projType.InputHasKeyword("PdsProjectionTypeImage")) {
      transFile = transDir.toStdString() + "/" + "translations/pdsImageProjection.trn";
    }
    else if (projType.InputHasKeyword("PdsProjectionTypeQube")) {
      transFile = transDir.toStdString() + "/" + "translations/pdsQubeProjection.trn";
    }
    else if (projType.InputHasKeyword("PdsProjectionTypeSpectralQube")) {
      transFile = transDir.toStdString() + "/" + "translations/pdsSpectralQubeProjection.trn";
    }
    else {
      return;
    }

    Isis::PvlToPvlTranslationManager pdsXlater(p_pdsLabel, QString::fromStdString(transFile.expanded()));

    ExtractPdsProjection(pdsXlater);

    Isis::PvlGroup mapGroup("Mapping");
    mapGroup += Isis::PvlKeyword("ProjectionName", p_projection.toStdString());
    mapGroup += Isis::PvlKeyword("TargetName", p_targetName.toStdString());
    mapGroup += Isis::PvlKeyword("EquatorialRadius", std::to_string(p_equatorialRadius), "meters");
    mapGroup += Isis::PvlKeyword("PolarRadius", std::to_string(p_polarRadius), "meters");
    mapGroup += Isis::PvlKeyword("LongitudeDirection", p_longitudeDirection.toStdString());
    mapGroup += Isis::PvlKeyword("LongitudeDomain", std::to_string(p_longitudeDomain));
    mapGroup += Isis::PvlKeyword("LatitudeType", p_latitudeType.toStdString());
    if (p_minimumLatitude != Isis::NULL8) {
      mapGroup += Isis::PvlKeyword("MinimumLatitude", std::to_string(p_minimumLatitude));
    }
    if (p_maximumLatitude != Isis::NULL8) {
      mapGroup += Isis::PvlKeyword("MaximumLatitude", std::to_string(p_maximumLatitude));
    }
    if (p_minimumLongitude != Isis::NULL8) {
      mapGroup += Isis::PvlKeyword("MinimumLongitude", std::to_string(p_minimumLongitude));
    }
    if (p_maximumLongitude != Isis::NULL8) {
      mapGroup += Isis::PvlKeyword("MaximumLongitude", std::to_string(p_maximumLongitude));
    }

    // if both longitudes exist, verify they are ordered correctly
    if (p_minimumLongitude != Isis::NULL8 && p_maximumLongitude != Isis::NULL8) {
      if (p_maximumLongitude <= p_minimumLongitude) {
        if (p_longitudeDomain == 180) {
          mapGroup["MinimumLongitude"] = std::to_string(-180);
          mapGroup["MaximumLongitude"] = std::to_string(180);
        }
        else {
          mapGroup["MinimumLongitude"] = std::to_string(0);
          mapGroup["MaximumLongitude"] = std::to_string(360);
        }
      }
    }

    mapGroup += Isis::PvlKeyword("PixelResolution", std::to_string(p_pixelResolution), "meters/pixel");
    mapGroup += Isis::PvlKeyword("Scale", std::to_string(p_scaleFactor), "pixels/degree");
    mapGroup += Isis::PvlKeyword("UpperLeftCornerX", std::to_string(p_upperLeftX), "meters");
    mapGroup += Isis::PvlKeyword("UpperLeftCornerY", std::to_string(p_upperLeftY), "meters");
    if (p_rotation != 0.0) {
      mapGroup += Isis::PvlKeyword("Rotation", std::to_string(p_rotation));
    }

    // To handle new projections without the need to modify source code
    // we will construct a filename from the projection.  The filename will
    // contain the projection specific translations from PDS to ISIS for each
    // projection

    QString projSpecificFileName = "$ISISROOT/appdata/translations/pdsImport";
    projSpecificFileName += p_projection + ".trn";
    Isis::PvlToPvlTranslationManager specificXlater(p_pdsLabel, projSpecificFileName);

    lab.addGroup(mapGroup);
    specificXlater.Auto(lab);

    if (lab.findGroup("Mapping").hasKeyword("CenterLongitude")) {
      PvlKeyword &centerLon = lab.findGroup("Mapping")["CenterLongitude"];
      if (p_longitudeDomain == 180)
        centerLon = std::to_string(TProjection::To180Domain((double)centerLon));
      else
        centerLon = std::to_string(TProjection::To360Domain((double)centerLon));
    }

    if (lab.findGroup("Mapping").hasKeyword("PoleLongitude")) {
      PvlKeyword &poleLon = lab.findGroup("Mapping")["PoleLongitude"];
      if (p_longitudeDomain == 180)
        poleLon = std::to_string(TProjection::To180Domain((double)poleLon));
      else
        poleLon = std::to_string(TProjection::To360Domain((double)poleLon));
    }

    OutputCubes[0]->putGroup(lab.findGroup("Mapping"));
  }
}
