/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "ProcessExportPds4.h"

#include <cmath>
#include <iostream>
#include <sstream>

#include <QDomDocument>
#include <QMap>
#include <QRegularExpression>
#include <QString>

#include "Application.h"
#include "FileName.h"
#include "IException.h"
#include "Projection.h"
#include "ProjectionFactory.h"
#include "Pvl.h"
#include "PvlToXmlTranslationManager.h"

using namespace std;

namespace Isis {


  /**
   * Default Constructor - Set to default the data members
   *
   */
  ProcessExportPds4::ProcessExportPds4() {

    m_lid = "";
    m_versionId = "";
    m_title = "";

    m_imageType = StandardImage;

    qSetGlobalQHashSeed(0); // hash seed to force consistent output

    m_domDoc = new QDomDocument("");

    // base xml file
    // <?xml version="1.0" encoding="UTF-8"?>
    QString xmlVersion = "version=\"1.0\" encoding=\"utf-8\"";
    QDomProcessingInstruction xmlHeader =
        m_domDoc->createProcessingInstruction("xml", xmlVersion);
    m_domDoc->appendChild(xmlHeader);

    // base pds4 schema location
    m_schemaLocation = "http://pds.nasa.gov/pds4/pds/v1 http://pds.nasa.gov/pds4/pds/v1/PDS4_PDS_1B00.xsd";

    QString xmlModel;
    xmlModel += "href=\"http://pds.nasa.gov/pds4/pds/v1/PDS4_PDS_1B00.sch\" ";
    xmlModel += "schematypens=\"http://purl.oclc.org/dsdl/schematron\"";
    QDomProcessingInstruction header =
        m_domDoc->createProcessingInstruction("xml-model", xmlModel);
    m_domDoc->appendChild(header);

  }


  /**
   * Destructor
   *
   */
  ProcessExportPds4::~ProcessExportPds4() {
    delete m_domDoc;
    m_domDoc = NULL;
  }


  /**
   * Create a standard PDS4 image label from the input cube.
   *
   * @return @b QDomDocument The output PDS4 label.
   */
  QDomDocument &ProcessExportPds4::StandardPds4Label() {
    CreateImageLabel();
    translateUnits(*m_domDoc);
    return *m_domDoc;
  }


  /**
   * Create a standard PDS4 image label from the input cube.
   *
   * @return @b QDomDocument The output PDS4 label.
   */
  void ProcessExportPds4::setImageType(ImageType imageType) {
    m_imageType = imageType;
  }


  /**
   * Creates a PDS4 label. The image label will be
   * stored internally in the class.
   *
   * This method has a similar function to
   * ProcessExportPds::CreateImageLabel. However, it will create
   * images of object type Array_3D_Image, Array_2D_Image, or
   * Array_3D_Spectrum.
   */
  void ProcessExportPds4::CreateImageLabel() {
    if (InputCubes.size() == 0) {
      std::string msg("Must set an input cube before creating a PDS4 label.");
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    if (m_domDoc->documentElement().isNull()) {
      QDomElement root = m_domDoc->createElement("Product_Observational");
      root.setAttribute("xmlns", "http://pds.nasa.gov/pds4/pds/v1");
      root.setAttribute("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");
      root.setAttribute("xsi:schemaLocation",
                        "http://pds.nasa.gov/pds4/pds/v1 http://pds.nasa.gov/pds4/pds/v1");
      m_domDoc->appendChild(root);
    }

    try {
      // <Product_Observational>
      //   <Identification_Area>
      identificationArea();
    }
    catch (IException &e) {
      std::string msg = "Unable to translate and export identification information.";
      throw IException(e, IException::Programmer, msg, _FILEINFO_);
    }
    try {
      // <Product_Observational>
      //   <Observation_Area>
      standardInstrument();
    }
    catch (IException &e) {
      std::string msg = "Unable to translate and export instrument information.";
      throw IException(e, IException::Programmer, msg, _FILEINFO_);
    }
    try {
      // <Product_Observational>
      //   <Observation_Area>
      //     <Discipline_Area>
      //       <disp:Display_Settings>
      displaySettings();
    }
    catch (IException &e) {
      std::string msg = "Unable to translate and export display settings.";
      throw IException(e, IException::Programmer, msg, _FILEINFO_);
    }

    try {
      // <Product_Observational>
      //   <Observation_Area>
      //     <Discipline_Area>
      //       <sp:Spectral_Characteristics> OR <img:Imaging>
      standardBandBin();
    }
    catch (IException &e) {
      std::string msg = "Unable to translate and export spectral information.";
      throw IException(e, IException::Programmer, msg, _FILEINFO_);
    }

    try {
      // <Product_Observational>
      //   <Observation_Area>
      //     <Discipline_Area>
      //       <card:Cartography>
      StandardAllMapping();
    }
    catch (IException &e) {
      std::string msg = "Unable to translate and export mapping group.";
      throw IException(e, IException::Programmer, msg, _FILEINFO_);
    }
    try {
      // <Product_Observational>
      //   <File_Area_Observational>
      fileAreaObservational();
    }
    catch (IException &e) {
      std::string msg = "Unable to translate and export standard image information.";
      throw IException(e, IException::Programmer, msg, _FILEINFO_);
    }
  }


  /**
   * This method translates the information from the ISIS
   * Instrument group to the PDS4 labels.
   */
  void ProcessExportPds4::standardInstrument() {
    Pvl *inputLabel = InputCubes[0]->label();
    FileName translationFileName;

    if (inputLabel->findObject("IsisCube").hasGroup("Instrument")) {

      // Translate the Instrument group
      translationFileName = "$ISISROOT/appdata/translations/pds4ExportInstrument.trn";
      PvlToXmlTranslationManager instXlator(*inputLabel, QString::fromStdString(translationFileName.expanded()));
      instXlator.Auto(*m_domDoc);

      // If instrument and spacecraft values were translated, create the combined name
      QDomElement obsAreaNode = m_domDoc->documentElement().firstChildElement("Observation_Area");

      if ( !obsAreaNode.isNull() ) {

        // fix start/stop times, if needed
        QDomElement timeNode = obsAreaNode.firstChildElement("Time_Coordinates");
        if (!timeNode.isNull()) {
          QDomElement startTime = timeNode.firstChildElement("start_date_time");
          if (startTime.text() == "") {
            startTime.setAttribute("xsi:nil", "true");
          }
          else {
            QString timeValue = startTime.text();
            PvlToXmlTranslationManager::resetElementValue(startTime, timeValue + "Z");
          }
          QDomElement stopTime  = timeNode.firstChildElement("stop_date_time");
          if (stopTime.text() == "") {
            stopTime.setAttribute("xsi:nil", "true");
          }
          else {
            QString timeValue = stopTime.text();
            PvlToXmlTranslationManager::resetElementValue(stopTime, timeValue + "Z");
          }
        }

        QDomElement obsSysNode = obsAreaNode.firstChildElement("Observing_System");
        if ( !obsSysNode.isNull() ) {
          QString instrumentName;
          QString spacecraftName;
          QDomElement obsSysCompNode = obsSysNode.firstChildElement("Observing_System_Component");
          while ( !obsSysCompNode.isNull()) {
            QDomElement compTypeNode = obsSysCompNode.firstChildElement("type");
            if ( compTypeNode.text().compare("Spacecraft") == 0 ) {
              QString componentName = obsSysCompNode.firstChildElement("name").text();
              if (QString::compare(componentName, "TBD", Qt::CaseInsensitive) != 0) {
                spacecraftName = componentName;
              }
            }
            else if ( compTypeNode.text().compare("Instrument") == 0 ) {
              QString componentName = obsSysCompNode.firstChildElement("name").text();
              if (QString::compare(componentName, "TBD", Qt::CaseInsensitive) != 0) {
                instrumentName = componentName;
              }
            }
            obsSysCompNode = obsSysCompNode.nextSiblingElement("Observing_System_Component");
          }
          QDomElement combinedNode = m_domDoc->createElement("name");
          QString combinedValue = "TBD";
          if ( !instrumentName.isEmpty() && !spacecraftName.isEmpty() ) {
            combinedValue = spacecraftName + " " + instrumentName;
          }
          combinedNode.appendChild( m_domDoc->createTextNode(combinedValue) );
          obsSysNode.insertBefore( combinedNode, obsSysNode.firstChild() );
        }
      }

      // Translate the Target name
      translationFileName = "$ISISROOT/appdata/translations/pds4ExportTargetFromInstrument.trn";
      PvlToXmlTranslationManager targXlator(*inputLabel, QString::fromStdString(translationFileName.expanded())
);
      targXlator.Auto(*m_domDoc);

      // Move target to just below Observing_System.
      QDomElement targetIdNode = obsAreaNode.firstChildElement("Target_Identification");
      obsAreaNode.insertAfter(targetIdNode, obsAreaNode.firstChildElement("Observing_System"));
    }
    else if (inputLabel->findObject("IsisCube").hasGroup("Mapping")) {

      translationFileName = "$ISISROOT/appdata/translations/pds4ExportTargetFromMapping.trn";
      PvlToXmlTranslationManager targXlator(*inputLabel, QString::fromStdString(translationFileName.expanded())
);
      targXlator.Auto(*m_domDoc);
    }
    else {
      throw IException(IException::Unknown, "Unable to find a target in input cube.", _FILEINFO_);
    }
  }


 /**
   * This method reorders the existing m_domDoc to follow PDS4 standards and fixes time formatting
   * if needed.
   */
  void ProcessExportPds4::reorder() {
      QDomElement obsAreaNode = m_domDoc->documentElement().firstChildElement("Observation_Area");
      if ( !obsAreaNode.isNull() ) {

        // fix times
        QDomElement timeNode = obsAreaNode.firstChildElement("Time_Coordinates");
        if (!timeNode.isNull()) {
          QDomElement startTime = timeNode.firstChildElement("start_date_time");
          if (startTime.text() == "") {
            startTime.setAttribute("xsi:nil", "true");
          }
          else {
            QString timeValue = startTime.text();
            if (!timeValue.contains("Z")) {
              PvlToXmlTranslationManager::resetElementValue(startTime, timeValue + "Z");
            }
          }

          QDomElement stopTime  = timeNode.firstChildElement("stop_date_time");
          if (stopTime.text() == "") {
            stopTime.setAttribute("xsi:nil", "true");
          }
          else {
            QString timeValue = stopTime.text();
            if (!timeValue.contains("Z")) {
              PvlToXmlTranslationManager::resetElementValue(stopTime, timeValue + "Z");
            }
          }
          QStringList xmlPath;
          xmlPath << "Product_Observational"
            << "Observation_Area"
            << "Discipline_Area"
            << "geom:Geometry"
            << "geom:Geometry_Orbiter"
            << "geom:geometry_reference_time_utc";

          QDomElement baseElement = m_domDoc->documentElement();
          QDomElement geomRefTime = getElement(xmlPath, baseElement);
          if (geomRefTime.text() == "") {
            geomRefTime.setAttribute("xsi:nil", "true");
          }
          else {
            QString timeValue = geomRefTime.text();
            PvlToXmlTranslationManager::resetElementValue(geomRefTime, timeValue + "Z");
          }
        xmlPath.clear();
          xmlPath << "Product_Observational"
            << "Observation_Area"
            << "Discipline_Area"
            << "geom:Geometry"
            << "geom:Image_Display_Geometry"
            << "geom:Object_Orientation_North_East"
            << "geom:east_azimuth";
          QDomElement eastAzimuth = getElement(xmlPath, baseElement);
          if (eastAzimuth.text() != "") {
            PvlToXmlTranslationManager::resetElementValue(eastAzimuth, eastAzimuth.text(), "deg");
          }
        }

        QDomElement investigationAreaNode = obsAreaNode.firstChildElement("Investigation_Area");
        obsAreaNode.insertAfter(investigationAreaNode, obsAreaNode.firstChildElement("Time_Coordinates"));

        QDomElement obsSystemNode = obsAreaNode.firstChildElement("Observing_System");
        obsAreaNode.insertAfter(obsSystemNode, obsAreaNode.firstChildElement("Investigation_Area"));

        QDomElement targetIdNode = obsAreaNode.firstChildElement("Target_Identification");
        obsAreaNode.insertAfter(targetIdNode, obsAreaNode.firstChildElement("Observing_System"));

        QDomElement missionAreaNode = obsAreaNode.firstChildElement("Mission_Area");
        obsAreaNode.insertAfter(missionAreaNode, obsAreaNode.firstChildElement("Target_Identification"));

        QDomElement disciplineAreaNode = obsAreaNode.firstChildElement("Discipline_Area");
        obsAreaNode.insertAfter(disciplineAreaNode, obsAreaNode.firstChildElement("Mission_Area"));
      }

      QDomElement identificationAreaNode = m_domDoc->documentElement().firstChildElement("Identification_Area");
      if ( !identificationAreaNode.isNull() ) {
        QDomElement aliasListNode = identificationAreaNode.firstChildElement("Alias_List");
        identificationAreaNode.insertAfter(aliasListNode, identificationAreaNode.firstChildElement("product_class"));
      }

      // Put Reference list in correct place:
      QDomElement referenceListNode = m_domDoc->documentElement().firstChildElement("Reference_List");
      if ( !referenceListNode.isNull() && !identificationAreaNode.isNull() ) {
         m_domDoc->documentElement().insertAfter(referenceListNode, obsAreaNode);
      }

      QDomElement fileAreaObservationalNode = m_domDoc->documentElement().firstChildElement("File_Area_Observational");
      QDomElement array2DImageNode = fileAreaObservationalNode.firstChildElement("Array_2D_Image");
      if ( !array2DImageNode.isNull() ) {
        QDomElement descriptionNode = array2DImageNode.firstChildElement("description");
        array2DImageNode.insertAfter(descriptionNode, array2DImageNode.firstChildElement("axis_index_order"));
      }
  }

  /**
   * Allows mission specific programs to set logical_identifier
   * required for PDS4 labels. This value is added to the xml file
   * by the identificationArea() method.
   *
   * The input value will be converted to all-lowercase if not already
   * in line with PDS4 requirements.
   *
   * The input string should be colon separated string with 6
   * identifiers:
   *
   * <ol>
   *   <li> urn </li>
   *   <li> space_agency (ususally nasa) </li>
   *   <li> archiving_organization (usually pds) </li>
   *   <li> bundle_id </li>
   *   <li> collection_id </li>
   *   <li> product_id </li>
   * </ol>
   *
   * Example:
   * urn:esa:psa:em16_tgo_frd:data_raw:frd_raw_sc_d_20150625T133700-20150625T135700
   *
   * @author 2018-05-21 Jeannie Backer
   *
   * @param lid The logical identifier value required for PDS4
   *            compliant labels.
   */
  void ProcessExportPds4::setLogicalId(QString lid) {
    m_lid = lid.toLower();
  }


  /**
   * Allows mission specific programs to set version_id
   * required for PDS4 labels. This value is added to the xml file
   * by the identificationArea() method.
   *
   * The input string should be colon separated string with 6
   * identifiers:
   *
   * @author 2019-03-01 Kristin Berry
   *
   * @param versiondId The version_id value required for PDS4
   *            compliant labels.
   */
   void ProcessExportPds4::setVersionId(QString versionId) {
    m_versionId = versionId;
  }


  /**
   * Allows mission specific programs to set the title
   * required for PDS4 labels. This value is added to the xml file
   * by the identificationArea() method.
   *
   * @author 2019-03-01 Kristin Berry
   *
   * @param title The title value required for PDS4
   *            compliant labels.
   */
   void ProcessExportPds4::setTitle(QString title) {
    m_title = title;
  }


 /**
   * Allows mission specific programs to use specified
   * versions of dictionaries.
   *
   * @author 2018-05-21 Jeannie Backer
   *
   * @param schema The string of schema to be set.
   */
  void ProcessExportPds4::setSchemaLocation(QString schema) {
    m_schemaLocation = schema;
  }


  /**
   * This method writes the identification information to the PDS4
   * labels.
   */
  void ProcessExportPds4::identificationArea() {
    Pvl *inputLabel = InputCubes[0]->label();
    FileName translationFileName;
    translationFileName = "$ISISROOT/appdata/translations/pds4ExportIdentificationArea.trn";
    PvlToXmlTranslationManager xlator(*inputLabel, QString::fromStdString(translationFileName.expanded())
);
    xlator.Auto(*m_domDoc);

    if (m_lid.isEmpty()) {
      m_lid = "urn:nasa:pds:TBD:TBD:TBD";
    }

    QDomElement identificationElement;
    QStringList identificationPath;
    identificationPath.append("Product_Observational");
    identificationPath.append("Identification_Area");
    try {
      identificationElement = getElement(identificationPath);
      if( identificationElement.isNull() ) {
        throw IException(IException::Unknown, "", _FILEINFO_);
      }
    }
    catch(IException &e) {
      std::string msg = "Could not find Identification_Area element "
                    "to add modification history under.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    QDomElement lidElement = identificationElement.firstChildElement("logical_identifier");
    PvlToXmlTranslationManager::resetElementValue(lidElement, m_lid);

    if (m_versionId != "") {
      QDomElement versionElement = identificationElement.firstChildElement("version_id");
      PvlToXmlTranslationManager::resetElementValue(versionElement, m_versionId);
    }

    if (m_title != "") {
      QDomElement titleElement = identificationElement.firstChildElement("title");
      PvlToXmlTranslationManager::resetElementValue(titleElement, m_title);
    }

    // Get export history and add <Modification_History> element.
    // These regular expressions match the pipe followed by the date from
    // the Application::Version() return value.
    QRegularExpression versionRegex(" \\| \\d{4}\\-\\d{2}\\-\\d{2}");
    QString historyDescription = "Created PDS4 output product from ISIS cube with the "
                                 + QString::fromStdString(FileName(Application::Name().toStdString()).baseName())
                                 + " application from ISIS version "
                                 + Application::Version().remove(versionRegex) + ".";
    // This regular expression matches the time from the Application::DateTime return value.
    QRegularExpression dateRegex("T\\d{2}:\\d{2}:\\d{2}");
    QString historyDate = Application::DateTime().remove(dateRegex);
    addHistory(historyDescription, historyDate);
  }


  /**
   * This method writes the display direction information to
   * the PDS4 labels.
   */
  void ProcessExportPds4::displaySettings() {
    // Add header info
    addSchema("PDS4_DISP_1B00.sch",
              "PDS4_DISP_1B00.xsd",
              "xmlns:disp",
              "http://pds.nasa.gov/pds4/disp/v1");

    Pvl *inputLabel = InputCubes[0]->label();
    FileName translationFileName;
    translationFileName = "$ISISROOT/appdata/translations/pds4ExportDisplaySettings.trn";
    PvlToXmlTranslationManager xlator(*inputLabel, QString::fromStdString(translationFileName.expanded())
);
    xlator.Auto(*m_domDoc);
  }


 /**
  * Export bandbin group to sp:Spectral Characteristics
  *
  */
  void ProcessExportPds4::standardBandBin() {
    Pvl *inputLabel = InputCubes[0]->label();
    if ( !inputLabel->findObject("IsisCube").hasGroup("BandBin") ) return;
    // Add header info
    addSchema("PDS4_IMG_1A10_1510.sch",
              "PDS4_IMG_1A10_1510.xsd",
              "xmlns:img",
              "http://pds.nasa.gov/pds4/img/v1");

    // Get the input Isis cube label and find the BandBin group if it has one
    if (m_imageType == StandardImage) {
      translateBandBinImage(*inputLabel);
    }
    else {
      // Add header info
      addSchema("PDS4_SP_1100.sch",
                "PDS4_SP_1100.xsd",
                "xmlns:sp",
                "http://pds.nasa.gov/pds4/sp/v1");
      if (m_imageType == UniformlySampledSpectrum) {
        translateBandBinSpectrumUniform(*inputLabel);
      }
      else if (m_imageType == BinSetSpectrum) {
        translateBandBinSpectrumBinSet(*inputLabel);
      }
    }
  }


  /**
   * Export BandBin group for 2D or 3D Image format.
   */
  void ProcessExportPds4::translateBandBinImage(Pvl &inputLabel) {
    QString translationFile = "$ISISROOT/appdata/translations/";
    translationFile += "pds4ExportBandBinImage.trn";
    FileName translationFileName(translationFile.toStdString());
    PvlToXmlTranslationManager xlator(inputLabel, QString::fromStdString(translationFileName.expanded())
);
    xlator.Auto(*m_domDoc);
  }


  /**
   * Export BandBin group for uniformly spaced 3D Spectral data format.
   */
  void ProcessExportPds4::translateBandBinSpectrumUniform(Pvl &inputLabel) {
    QString translationFile = "$ISISROOT/appdata/translations/";
    translationFile += "pds4ExportBandBinSpectrumUniform.trn";
    FileName translationFileName(translationFile.toStdString());
    PvlToXmlTranslationManager xlator(inputLabel, QString::fromStdString(translationFileName.expanded())
);
    xlator.Auto(*m_domDoc);

    PvlGroup bandBinGroup = inputLabel.findObject("IsisCube").findGroup("BandBin");
    // fix multi-valued bandbin info
    QStringList xmlPath;
    xmlPath << "Product_Observational"
            << "Observation_Area"
            << "Discipline_Area"
            << "sp:Spectral_Characteristics";
    QDomElement baseElement = m_domDoc->documentElement();
    QDomElement spectralCharElement = getElement(xmlPath, baseElement);

    // Axis_Bin_Set for variable bin widths
    // required - bin_sequence_number, center_value, bin_width
    // optional - detector_number, grating_position, original_bin_number, scaling_factor, value_offset, Filter
    // ... see schema for more...
    PvlKeyword center;
    if (bandBinGroup.hasKeyword("Center")) {
      center = bandBinGroup["Center"];
    }
    else if (bandBinGroup.hasKeyword("FilterCenter")) {
      center = bandBinGroup["FilterCenter"];
    }
    else {
      std::string msg = "Unable to translate BandBin info for BinSetSpectrum. "
                    "Translation for PDS4 required value [center_value] not found.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    PvlKeyword width;
    if (bandBinGroup.hasKeyword("Width")) {
      width = bandBinGroup["Width"];
    }
    else if (bandBinGroup.hasKeyword("FilterWidth")) {
      width = bandBinGroup["FilterWidth"];
    }
    else {
      std::string msg = "Unable to translate BandBin info for BinSetSpectrum. "
                    "Translation for PDS4 required value [bin_width] not found.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    QString units = QString::fromStdString(center.unit());

    if (!QString::fromStdString(width.unit()).isEmpty() ) {
      if (units.isEmpty()) {
        units = QString::fromStdString(width.unit());
      }
      if (units.compare(QString::fromStdString(width.unit()), Qt::CaseInsensitive) != 0) {
        std::string msg = "Unable to translate BandBin info for BinSetSpectrum. "
                      "Unknown or unmatching units for [center_value] and [bin_width].";
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }
    }

    PvlKeyword originalBand;
    if (bandBinGroup.hasKeyword("OriginalBand")) {
      originalBand = bandBinGroup["OriginalBand"];
    }
    PvlKeyword name;
    if (bandBinGroup.hasKeyword("Name")) {
      name = bandBinGroup["Name"];
    }
    else if (bandBinGroup.hasKeyword("FilterName")) {
      name = bandBinGroup["FilterName"];
    }
    else if (bandBinGroup.hasKeyword("FilterId")) {
      name = bandBinGroup["FilterId"];
    }
    PvlKeyword number;
    if (bandBinGroup.hasKeyword("Number")) {
      number = bandBinGroup["Number"];
    }
    else if (bandBinGroup.hasKeyword("FilterNumber")) {
      number = bandBinGroup["FilterNumber"];
    }

    QDomElement axisBinSetElement = spectralCharElement.firstChildElement("sp:Axis_Bin_Set");
    if (axisBinSetElement.isNull()) {
      axisBinSetElement = m_domDoc->createElement("sp:Axis_Bin_Set");
      spectralCharElement.appendChild(axisBinSetElement);
    }
    int bands = (int)inputLabel.findObject("IsisCube")
                                .findObject("Core")
                                .findGroup("Dimensions")
                                .findKeyword("Bands");

    for (int i = 0; i < bands; i++) {

      QDomElement bin = m_domDoc->createElement("sp:Bin");
      axisBinSetElement.appendChild(bin);

      QDomElement binSequenceNumber = m_domDoc->createElement("sp:bin_sequence_number");
      PvlToXmlTranslationManager::setElementValue(binSequenceNumber, QString::fromStdString(toString(i+1)));
      bin.appendChild(binSequenceNumber);


      QDomElement centerValue = m_domDoc->createElement("sp:center_value");
      PvlToXmlTranslationManager::setElementValue(centerValue, QString::fromStdString(center[i]), units);
      bin.appendChild(centerValue);

      QDomElement binWidth = m_domDoc->createElement("sp:bin_width");
      if (width.size() == bands) {
        PvlToXmlTranslationManager::setElementValue(binWidth, QString::fromStdString(width[i]), units);
      }
      else {
        PvlToXmlTranslationManager::setElementValue(binWidth, QString::fromStdString(width[0]), units);
      }
      bin.appendChild(binWidth);

      QDomElement originalBinNumber = m_domDoc->createElement("sp:original_bin_number");
      if (originalBand.size() > 0) {
        PvlToXmlTranslationManager::setElementValue(originalBinNumber, QString::fromStdString(originalBand[i]));
        bin.appendChild(originalBinNumber);
      }

      if (name.size() > 0 || number.size() > 0) {
        QDomElement filter = m_domDoc->createElement("sp:Filter");
        bin.appendChild(filter);
        if (name.size() > 0) {
          QDomElement filterName = m_domDoc->createElement("sp:filter_name");
          PvlToXmlTranslationManager::setElementValue(filterName, QString::fromStdString(name[i]));
          filter.appendChild(filterName);
        }
        if (number.size() > 0) {
          QDomElement filterNumber= m_domDoc->createElement("sp:filter_number");
          PvlToXmlTranslationManager::setElementValue(filterNumber, QString::fromStdString(number[i]));
          filter.appendChild(filterNumber);
        }
      }
    }

  }


  /**
   * Export BandBin group for non-uniformly spaced 3D Spectral data format.
   */
  void ProcessExportPds4::translateBandBinSpectrumBinSet(Pvl &inputLabel) {
    QString translationFile = "$ISISROOT/appdata/translations/";
    translationFile += "pds4ExportBandBinSpectrumBinSet.trn";
    FileName translationFileName(translationFile.toStdString());
    PvlToXmlTranslationManager xlator(inputLabel, QString::fromStdString(translationFileName.expanded())
);
    xlator.Auto(*m_domDoc);

    PvlGroup bandBinGroup = inputLabel.findObject("IsisCube").findGroup("BandBin");
    // fix multi-valued bandbin info
    QStringList xmlPath;
    xmlPath << "Product_Observational"
            << "Observation_Area"
            << "Discipline_Area"
            << "sp:Spectral_Characteristics";
    QDomElement baseElement = m_domDoc->documentElement();
    QDomElement spectralCharElement = getElement(xmlPath, baseElement);

    // Axis_Uniformly_Sampled
    // required - sampling_parameter_type (frequency, wavelength, wavenumber)
    //            sampling_interval (units Hz, Angstrom, cm**-1, respectively)
    //            bin_width  (units Hz, Angstrom, cm**-1, respectively)
    //            first_center_value  (units Hz, Angstrom, cm**-1, respectively)
    //            last_center_value  (units Hz, Angstrom, cm**-1, respectively)
    //            Local_Internal_Reference
    //            Local_Internal_Reference:local_reference_type = spectral_characteristics_to_array_axis
    //            Local_Internal_Reference:local_identifier_reference,
    //                1. At least one Axis_Array:axis_name must match the
    //                   value of the local_identifier_reference in the
    //                   Axis_Uniformly_Sampled.
    //                   Set Axis_Uniformly_Sampled:local_identifier_reference = Axis_Array:axis_name = Band
    //                2. At least one Array_3D_Spectrum:local_identifier must match
    //                   the value of the local_identifier_reference in the
    //                   Spectral_Characteristics.
    //                   Set Spectral_Characteristics:local_identifier_reference = Array_3D_Spectrum:local_identifier = Spectral_Array_Object
    //            Local_Internal_Reference:local_reference_type = spectral_characteristics_to_array_axis
    PvlKeyword center("Center");
    if (bandBinGroup.hasKeyword("FilterCenter")) {
      center = bandBinGroup["FilterCenter"];
    }
    else if (bandBinGroup.hasKeyword("Center")) {
      center = bandBinGroup["Center"];
    }
    else {
      std::string msg = "Unable to translate BandBin info for UniformlySpacedSpectrum. "
                    "Translation for PDS4 required value [last_center_value] not found.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    QString lastCenter = QString::fromStdString(center[center.size() - 1]);

    QDomElement axisBinSetElement = spectralCharElement.firstChildElement("sp:Axis_Uniformly_Sampled");
    if (axisBinSetElement.isNull()) {
      axisBinSetElement = m_domDoc->createElement("sp:Axis_Uniformly_Sampled");
      spectralCharElement.appendChild(axisBinSetElement);
    }

    QDomElement lastCenterElement = m_domDoc->createElement("sp:last_center_value");
    PvlToXmlTranslationManager::setElementValue(lastCenterElement, lastCenter);
    spectralCharElement.appendChild(lastCenterElement);

  }

 /**
  * Sets the description string which describes the pixel vales in
  * File_Area_Observational
  *
  * @param description Description of pixel values to use.
  */
  void ProcessExportPds4::setPixelDescription(QString description) {
    m_pixelDescription = description;
  }

  /**
   * Create and internalize an image output label from the input
   * image. This method has a similar function to
   * ProcessExportPds::StandardImageImage.
   */
  void ProcessExportPds4::fileAreaObservational() {
    Pvl *inputLabel = InputCubes[0]->label();
    QString imageObject = "";

    QString translationFile = "$ISISROOT/appdata/translations/pds4Export";
    if (m_imageType == StandardImage) {
      int bands = (int)inputLabel->findObject("IsisCube")
                                  .findObject("Core")
                                  .findGroup("Dimensions")
                                  .findKeyword("Bands");
      if (bands > 1) {
        imageObject = "Array_3D_Image";
      }
      else {
        imageObject = "Array_2D_Image";
      }
      translationFile += QString(imageObject).remove('_');
    }
    else {
      imageObject = "Array_3D_Spectrum";
      translationFile += QString(imageObject).remove('_');
      if (m_imageType == UniformlySampledSpectrum) {
        translationFile += "Uniform";
      }
      else if (m_imageType == BinSetSpectrum) {
        translationFile += "BinSet";
      }
    }
    translationFile += ".trn";
    FileName translationFileName(translationFile.toStdString());

    PvlToXmlTranslationManager xlator(*inputLabel, QString::fromStdString(translationFileName.expanded())
);
    xlator.Auto(*m_domDoc);

    QDomElement rootElement = m_domDoc->documentElement();
    QDomElement fileAreaObservationalElement =
                    rootElement.firstChildElement("File_Area_Observational");

    // Calculate the core base/mult for the output cube
    double base = 0.0;
    double multiplier = 1.0;
    double outputMin, outputMax;

    double inputMin = (p_inputMinimum.size()) ? p_inputMinimum[0] : 0.0;
    double inputMax = (p_inputMaximum.size()) ? p_inputMaximum[0] : 0.0;

    for(unsigned int i = 0; i < p_inputMinimum.size(); i ++) {
      inputMin = std::min(inputMin, p_inputMinimum[i]);
      inputMax = std::max(inputMax, p_inputMaximum[i]);
    }

    outputMin = p_outputMinimum;
    outputMax = p_outputMaximum;

    if(p_inputMinimum.size() && ( p_pixelType == Isis::UnsignedByte ||
                                  p_pixelType == Isis::SignedWord   ||
                                  p_pixelType == Isis::UnsignedWord ) ) {
      multiplier = (inputMax - inputMin) / (outputMax - outputMin);
      base = inputMin - multiplier * outputMin;
    }

    if (!fileAreaObservationalElement.isNull()) {
      QDomElement arrayImageElement =
                      fileAreaObservationalElement.firstChildElement(imageObject);
      if (!arrayImageElement.isNull()) {

        // reorder axis elements.
        // Translation order:  elements, axis_name, sequence_number
        // Correct order:      axis_name, elements, sequence_number
        QDomElement axisArrayElement = arrayImageElement.firstChildElement("Axis_Array");
        while( !axisArrayElement.isNull() ) {
          QDomElement axisNameElement = axisArrayElement.firstChildElement("axis_name");
          axisArrayElement.insertBefore(axisNameElement,
                                        axisArrayElement.firstChildElement("elements"));
          axisArrayElement = axisArrayElement.nextSiblingElement("Axis_Array");
        }

        QDomElement elementArrayElement = m_domDoc->createElement("Element_Array");
        arrayImageElement.insertBefore(elementArrayElement,
                                         arrayImageElement.firstChildElement("Axis_Array"));

        QDomElement dataTypeElement = m_domDoc->createElement("data_type");
        PvlToXmlTranslationManager::setElementValue(dataTypeElement,
                                                    PDS4PixelType(p_pixelType, p_endianType));
        elementArrayElement.appendChild(dataTypeElement);

        QDomElement scalingFactorElement = m_domDoc->createElement("scaling_factor");
        PvlToXmlTranslationManager::setElementValue(scalingFactorElement,
                                                    QString::fromStdString(Isis::toString(multiplier)));
        elementArrayElement.appendChild(scalingFactorElement);

        QDomElement offsetElement = m_domDoc->createElement("value_offset");
        PvlToXmlTranslationManager::setElementValue(offsetElement,
                                                    QString::fromStdString(Isis::toString(base)));
        elementArrayElement.appendChild(offsetElement);
      }

      // Add the Special_Constants class to define ISIS special pixel values if pixel type is
      QDomElement specialConstantElement = m_domDoc->createElement("Special_Constants");
      arrayImageElement.insertAfter(specialConstantElement,
                                    arrayImageElement.lastChildElement("Axis_Array"));

      switch (p_pixelType) {
        case Real:
          { QDomElement nullElement = m_domDoc->createElement("missing_constant");
          PvlToXmlTranslationManager::setElementValue(nullElement, QString::fromStdString(toString(NULL4, 18)));
          specialConstantElement.appendChild(nullElement);

          QDomElement highInstrumentSatElement = m_domDoc->createElement("high_instrument_saturation");
          PvlToXmlTranslationManager::setElementValue(highInstrumentSatElement, QString::fromStdString(toString(HIGH_INSTR_SAT4, 18)));
          specialConstantElement.appendChild(highInstrumentSatElement);

          QDomElement highRepresentationSatElement = m_domDoc->createElement("high_representation_saturation");
          PvlToXmlTranslationManager::setElementValue(highRepresentationSatElement, QString::fromStdString(toString(HIGH_REPR_SAT4, 18)));
          specialConstantElement.appendChild(highRepresentationSatElement);

          QDomElement lowInstrumentSatElement = m_domDoc->createElement("low_instrument_saturation");
          PvlToXmlTranslationManager::setElementValue(lowInstrumentSatElement, QString::fromStdString(toString(LOW_INSTR_SAT4, 18)));
          specialConstantElement.appendChild(lowInstrumentSatElement);

          QDomElement lowRepresentationSatElement = m_domDoc->createElement("low_representation_saturation");
          PvlToXmlTranslationManager::setElementValue(lowRepresentationSatElement, QString::fromStdString(toString(LOW_REPR_SAT4, 18)));
          specialConstantElement.appendChild(lowRepresentationSatElement);
          break;}

        case UnsignedByte:
          { QDomElement nullElement = m_domDoc->createElement("missing_constant");
          PvlToXmlTranslationManager::setElementValue(nullElement, QString::fromStdString(toString(NULL1, 18)));
          specialConstantElement.appendChild(nullElement);

          QDomElement highInstrumentSatElement = m_domDoc->createElement("high_instrument_saturation");
          PvlToXmlTranslationManager::setElementValue(highInstrumentSatElement, QString::fromStdString(toString(HIGH_INSTR_SAT1, 18)));
          specialConstantElement.appendChild(highInstrumentSatElement);

          QDomElement highRepresentationSatElement = m_domDoc->createElement("high_representation_saturation");
          PvlToXmlTranslationManager::setElementValue(highRepresentationSatElement, QString::fromStdString(toString(HIGH_REPR_SAT1, 18)));
          specialConstantElement.appendChild(highRepresentationSatElement);

          QDomElement lowInstrumentSatElement = m_domDoc->createElement("low_instrument_saturation");
          PvlToXmlTranslationManager::setElementValue(lowInstrumentSatElement, QString::fromStdString(toString(LOW_INSTR_SAT1, 18)));
          specialConstantElement.appendChild(lowInstrumentSatElement);

          QDomElement lowRepresentationSatElement = m_domDoc->createElement("low_representation_saturation");
          PvlToXmlTranslationManager::setElementValue(lowRepresentationSatElement, QString::fromStdString(toString(LOW_REPR_SAT1, 18)));
          specialConstantElement.appendChild(lowRepresentationSatElement);
          break; }

        case SignedWord:
          { QDomElement nullElement = m_domDoc->createElement("missing_constant");
          PvlToXmlTranslationManager::setElementValue(nullElement, QString::fromStdString(toString(NULL2, 18)));
          specialConstantElement.appendChild(nullElement);

          QDomElement highInstrumentSatElement = m_domDoc->createElement("high_instrument_saturation");
          PvlToXmlTranslationManager::setElementValue(highInstrumentSatElement, QString::fromStdString(toString(HIGH_INSTR_SAT2, 18)));
          specialConstantElement.appendChild(highInstrumentSatElement);

          QDomElement highRepresentationSatElement = m_domDoc->createElement("high_representation_saturation");
          PvlToXmlTranslationManager::setElementValue(highRepresentationSatElement, QString::fromStdString(toString(HIGH_REPR_SAT2, 18)));
          specialConstantElement.appendChild(highRepresentationSatElement);

          QDomElement lowInstrumentSatElement = m_domDoc->createElement("low_instrument_saturation");
          PvlToXmlTranslationManager::setElementValue(lowInstrumentSatElement, QString::fromStdString(toString(LOW_INSTR_SAT2, 18)));
          specialConstantElement.appendChild(lowInstrumentSatElement);

          QDomElement lowRepresentationSatElement = m_domDoc->createElement("low_representation_saturation");
          PvlToXmlTranslationManager::setElementValue(lowRepresentationSatElement, QString::fromStdString(toString(LOW_REPR_SAT2, 18)));
          specialConstantElement.appendChild(lowRepresentationSatElement);
          break; }

        case UnsignedWord:
          { QDomElement nullElement = m_domDoc->createElement("missing_constant");
          PvlToXmlTranslationManager::setElementValue(nullElement, QString::fromStdString(toString(NULLU2, 18)));
          specialConstantElement.appendChild(nullElement);

          QDomElement highInstrumentSatElement = m_domDoc->createElement("high_instrument_saturation");
          PvlToXmlTranslationManager::setElementValue(highInstrumentSatElement, QString::fromStdString(toString(HIGH_INSTR_SATU2, 18)));
          specialConstantElement.appendChild(highInstrumentSatElement);

          QDomElement highRepresentationSatElement = m_domDoc->createElement("high_representation_saturation");
          PvlToXmlTranslationManager::setElementValue(highRepresentationSatElement, QString::fromStdString(toString(HIGH_REPR_SATU2, 18)));
          specialConstantElement.appendChild(highRepresentationSatElement);

          QDomElement lowInstrumentSatElement = m_domDoc->createElement("low_instrument_saturation");
          PvlToXmlTranslationManager::setElementValue(lowInstrumentSatElement, QString::fromStdString(toString(LOW_INSTR_SATU2, 18)));
          specialConstantElement.appendChild(lowInstrumentSatElement);

          QDomElement lowRepresentationSatElement = m_domDoc->createElement("low_representation_saturation");
          PvlToXmlTranslationManager::setElementValue(lowRepresentationSatElement, QString::fromStdString(toString(LOW_REPR_SATU2, 18)));
          specialConstantElement.appendChild(lowRepresentationSatElement);
          break; }

        case None:
          break;

        default:
          break;
      }



      if (!m_pixelDescription.isEmpty()) {
        QDomElement descriptionElement = m_domDoc->createElement("description");
        PvlToXmlTranslationManager::setElementValue(descriptionElement,
                                                    m_pixelDescription);
        arrayImageElement.insertAfter(descriptionElement, arrayImageElement.lastChildElement());
      }
    }
  }


  /**
   * Adds necessary information to the xml header for a pds4 class for schema which lack
   * schematron files (.sch)
   *
   * @param xsd Schema filename without path
   * @param xmlns The xml namespace used
   * @param xmlnsURI Full URL to the xml namespace URI. Also used as the location of the sch and xsd
   */
  void ProcessExportPds4::addSchema(QString xsd, QString xmlns, QString xmlnsURI) {
    // Add xmlns
    QDomElement root = m_domDoc->documentElement();
    root.setAttribute(xmlns, xmlnsURI);

    // Add to xsi:schemaLocation
    m_schemaLocation += " ";
    m_schemaLocation += xmlnsURI;
    m_schemaLocation += " ";
    m_schemaLocation += xmlnsURI;
    m_schemaLocation += "/";
    m_schemaLocation += xsd;
    root.setAttribute("xsi:schemaLocation", m_schemaLocation);
  }


  /**
   * Adds necessary information to the xml header for a pds4 class.
   *
   * @param sch Schematron filename without path
   * @param xsd Schema filename without path
   * @param xmlns The xml namespace used
   * @param xmlnsURI Full URL to the xml namespace URI. Also used as the location of the sch and xsd
   */
  void ProcessExportPds4::addSchema(QString sch, QString xsd, QString xmlns, QString xmlnsURI) {
    // Add xml-model
    QString xmlModel;
    xmlModel += "href=\"";
    xmlModel +=  xmlnsURI;
    xmlModel +=  "/";
    xmlModel +=  sch;
    xmlModel += "\" schematypens=\"http://purl.oclc.org/dsdl/schematron\"";
    QDomProcessingInstruction header =
        m_domDoc->createProcessingInstruction("xml-model", xmlModel);
    m_domDoc->insertAfter(header, m_domDoc->firstChild());

    // Add xmlns
    addSchema(xsd, xmlns, xmlnsURI);
  }


  /**
   * Write the XML label to the supplied stream.
   *
   * @param[out] os file stream to which the XML label will be written.
   */
  void ProcessExportPds4::OutputLabel(std::ofstream &os) {
    os << m_domDoc->toString().toStdString() << endl;
  }


  /**
   * This method fills the image data of the PDS4 file using the parent class
   * ProcessExport::StartProcess.
   *
   * @param[out] fout Output file stream to be filled with the PDS4 data.
   *
   */
  void ProcessExportPds4::StartProcess(std::ofstream &fout) {
    ProcessExport::StartProcess(fout);
  }


  /**
   * Return the internalized PDS4 label. If no label is internalized yet, an
   * empty label will be returned.
   *
   * @return @b QDomDocument The PDS4 Xml label
   */
  QDomDocument &ProcessExportPds4::GetLabel() {
    if (m_domDoc->documentElement().isNull()) {
      QDomElement root = m_domDoc->createElement("Product_Observational");
      root.setAttribute("xmlns", "http://pds.nasa.gov/pds4/pds/v1");
      root.setAttribute("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");
      root.setAttribute("xsi:schemaLocation",
                        "http://pds.nasa.gov/pds4/pds/v1 http://pds.nasa.gov/pds4/pds/v1");
      m_domDoc->appendChild(root);
    }
    return *m_domDoc;
  }


  /**
   * This method write out the labels and image data to the specified output file.
   * Creates an IMG and XML file.
   *
   * @param outFile QString of the name of the output image file. Will create an XML
   *        and an IMG file with the output file name.
   *
   */
  void ProcessExportPds4::WritePds4(QString outFile) {

    FileName outputFile(outFile.toStdString());

    // Name for output label
    QString path(QString::fromStdString(outputFile.originalPath()));
    QString name(QString::fromStdString(outputFile.baseName()));
    QString labelName = path + "/" + name + ".xml";

    // Name for output image
    QString imageName = QString::fromStdString(outputFile.expanded());

    // If input file ends in .xml, the user entered a label name for the output file, not an
    // image name with a unique file extension.
    if (QString::compare(QString::fromStdString(outputFile.extension()), "xml", Qt::CaseInsensitive) == 0) {
      imageName = path + "/" + name + ".img";
    }

    QDomElement rootElement = m_domDoc->documentElement();
    QDomElement fileAreaObservationalElement =
                    rootElement.firstChildElement("File_Area_Observational");

    QDomElement fileElement = m_domDoc->createElement("File");
    fileAreaObservationalElement.insertBefore(fileElement,
                                              fileAreaObservationalElement.firstChildElement());

    QDomElement fileNameElement = m_domDoc->createElement("file_name");
    PvlToXmlTranslationManager::setElementValue(fileNameElement, QString::fromStdString(outputFile.name()));
    fileElement.appendChild(fileNameElement);

//    QDomElement creationElement = m_domDoc->createElement("creation_date_time");
//    PvlToXmlTranslationManager::setElementValue(creationElement, );
//    fileElement.appendChild(creationElement);

    ofstream outputLabel(labelName.toLatin1().data());
    OutputLabel(outputLabel);
    outputLabel.close();

    ofstream outputImageFile(imageName.toLatin1().data());
    StartProcess(outputImageFile);
    outputImageFile.close();

    EndProcess();
  }


  /**
   * Create the standard keywords for the IMAGE_MAP_PROJECTION group in a PDS
   * label
   *
   *
   * @throws IException::User "Unable to export projection [" + projName + "] to PDS4 product. " +
                              "This projection is not supported in ISIS."
   */
  void ProcessExportPds4::StandardAllMapping() {
    // Cartography
    // Get the input Isis cube label and find the Mapping group if it has one
    Pvl *inputLabel = InputCubes[0]->label();
    if(inputLabel->hasObject("IsisCube") &&
        !(inputLabel->findObject("IsisCube").hasGroup("Mapping"))) return;
    PvlGroup &inputMapping = inputLabel->findGroup("Mapping", Pvl::Traverse);

    addSchema("PDS4_CART_1900.sch",
              "PDS4_CART_1900.xsd",
              "xmlns:cart",
              "http://pds.nasa.gov/pds4/cart/v1");

    // Translate the projection specific keywords for a PDS IMAGE_MAP_PROJECTION
    Projection *proj = ProjectionFactory::Create(*inputLabel);
    QString projName = proj->Name();
    try {
      PvlToXmlTranslationManager xlatorSpecProj(*inputLabel,
                                                "$ISISROOT/appdata/translations/pds4Export" + projName + ".trn");
      xlatorSpecProj.Auto(*m_domDoc);
    }
    catch (IException &e) {
      std::string msg = "Unable to export projection [" + projName.toStdString() + "] to PDS4 product. " +
                     "This projection is not supported in ISIS3.";
      throw IException(e, IException::User, msg, _FILEINFO_);
    }

    // convert units.
    QStringList xmlPath;
    xmlPath << "Product_Observational"
            << "Observation_Area"
            << "Discipline_Area"
            << "cart:Cartography"
            << "cart:Map_Projection"
            << "cart:Spatial_Reference_Information"
            << "cart:Horizontal_Coordinate_System_Definition"
            << "cart:Geodetic_Model";

    QDomElement baseElement = m_domDoc->documentElement();
    QDomElement geodeticModelElement = getElement(xmlPath, baseElement);
    QDomElement semiMajorRadElement = geodeticModelElement.firstChildElement("cart:semi_major_radius");
    if (!semiMajorRadElement.isNull()) {

      QString units = semiMajorRadElement.attribute("unit");
      if( units.compare("km", Qt::CaseInsensitive) != 0 && units.compare("kilometers", Qt::CaseInsensitive) != 0) {

        //if no units, assume in meters
        double dValue = toDouble(semiMajorRadElement.text().toStdString());
        dValue /= 1000.0;
        PvlToXmlTranslationManager::resetElementValue(semiMajorRadElement, QString::fromStdString(toString(dValue)), "km");
      }
    }

    QDomElement semiMinorRadElement = geodeticModelElement.firstChildElement("cart:semi_minor_radius");
    if (!semiMinorRadElement.isNull()) {

      QString units = semiMinorRadElement.attribute("unit");
      if( units.compare("km", Qt::CaseInsensitive) != 0 && units.compare("kilometers", Qt::CaseInsensitive) != 0) {
        // If no units, assume in meters
        double dValue = toDouble(semiMinorRadElement.text().toStdString());
        dValue /= 1000.0;
        PvlToXmlTranslationManager::resetElementValue(semiMinorRadElement, QString::fromStdString(toString(dValue)), "km");
      }
    }

    QDomElement polarRadElement = geodeticModelElement.firstChildElement("cart:polar_radius");
    if (!polarRadElement.isNull()) {
      QString units = polarRadElement.attribute("unit");
      if( units.compare("km", Qt::CaseInsensitive) != 0 && units.compare("kilometers", Qt::CaseInsensitive) != 0) {
        // If no units, assume in meters
        double dValue = toDouble(polarRadElement.text().toStdString());
        dValue /= 1000.0;
        PvlToXmlTranslationManager::resetElementValue(polarRadElement, QString::fromStdString(toString(dValue)), "km");
      }
    }

    PvlKeyword &isisLonDir = inputMapping.findKeyword("LongitudeDirection");
    QString lonDir = QString::fromStdString(isisLonDir[0]);
    lonDir = lonDir.toUpper();

    // Add Lat/Lon range
    double maxLon, minLon, maxLat, minLat;
    InputCubes[0]->latLonRange(minLat, maxLat, minLon, maxLon);

    xmlPath.clear();
    xmlPath << "Product_Observational"
            << "Observation_Area"
            << "Discipline_Area"
            << "cart:Cartography"
            << "cart:Spatial_Domain"
            << "cart:Bounding_Coordinates";
    QDomElement boundingCoordElement = getElement(xmlPath, baseElement);
    QDomElement eastElement = boundingCoordElement.firstChildElement("cart:east_bounding_coordinate");
    QDomElement westElement = boundingCoordElement.firstChildElement("cart:west_bounding_coordinate");
    QDomElement northElement = boundingCoordElement.firstChildElement("cart:north_bounding_coordinate");
    QDomElement southElement = boundingCoordElement.firstChildElement("cart:south_bounding_coordinate");

    // Translation files currently handles Positive West case where east = min, west = max
    // so if positive east, swap min/max
    if(QString::compare(lonDir, "PositiveEast", Qt::CaseInsensitive) == 0) {
      // west min, east max
      PvlToXmlTranslationManager::resetElementValue(eastElement, QString::fromStdString(toString(maxLon)), "deg");
      PvlToXmlTranslationManager::resetElementValue(westElement, QString::fromStdString(toString(minLon)), "deg");
    }
    else {
      PvlToXmlTranslationManager::resetElementValue(eastElement, QString::fromStdString(toString(minLon)), "deg");
      PvlToXmlTranslationManager::resetElementValue(westElement, QString::fromStdString(toString(maxLon)), "deg");
    }

    PvlToXmlTranslationManager::resetElementValue(northElement, QString::fromStdString(toString(maxLat)), "deg");
    PvlToXmlTranslationManager::resetElementValue(southElement, QString::fromStdString(toString(minLat)), "deg");

    // longitude_of_central_meridian and latitude_of_projection_origin need to be converted to floats.
    xmlPath.clear();
    xmlPath << "Product_Observational"
            << "Observation_Area"
            << "Discipline_Area"
            << "cart:Cartography"
            << "cart:Spatial_Reference_Information"
            << "cart:Horizontal_Coordinate_System_Definition"
            << "cart:Planar"
            << "cart:Map_Projection";

    // The following is necessary because the full xmlPath differs depending on the projection used.
    QDomElement projectionElement = getElement(xmlPath, baseElement);
    QDomElement tempElement = projectionElement.firstChildElement();
    QDomElement nameElement = tempElement.nextSiblingElement();

    QDomElement longitudeElement = nameElement.firstChildElement("cart:longitude_of_central_meridian");
    QDomElement originElement = nameElement.firstChildElement("cart:latitude_of_projection_origin");

    double longitudeElementValue = longitudeElement.text().toDouble();
    double originElementValue = originElement.text().toDouble();

    // Only update the ouput formatting if there are no digits after the decimal point.
    if (!longitudeElement.text().contains('.')) {
      QString toset1 = QString::fromStdString(toString(longitudeElementValue, 1));
      PvlToXmlTranslationManager::resetElementValue(longitudeElement, toset1, "deg");
    }

    if (!originElement.text().contains('.')) {
      QString toset2 = QString::fromStdString(toString(originElementValue, 1));
      PvlToXmlTranslationManager::resetElementValue(originElement, toset2, "deg");
    }
  }


/**
  * Convenience method to get an element given a path and its parent.
  *
  * @param xmlPath The XML path to the element to retrieve,
  *                starting at the parent element. Note: The
  *                first element of this path must be the same as
  *                the parent element passed in, unless the
  *                parent element is NULL. If the parent element
  *                passed in is NULL, then we assume that the
  *                parent is the root and a full path has been
  *                given.
  * @param parent The parent QDomElement of the given path. Defaults to
  *               the root element of the document.
  *
  * @return QDomElement
  */
  QDomElement ProcessExportPds4::getElement(QStringList xmlPath, QDomElement parent) {
    QDomElement baseElement = parent;
    if (baseElement.isNull()) {
      baseElement = m_domDoc->documentElement();
    }
    if (baseElement.isNull()) {
      std::string msg = "Unable to get element from empty XML document.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    QString parentName = xmlPath[0];
    if (parentName != baseElement.tagName()) {
      std::string msg = "The tag name of the parent element passed in "
                    "must be the first value in the given XML path.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    for (int i = 1; i < xmlPath.size(); i++) {
      QString elementName = xmlPath[i];
      QDomElement nextElement = baseElement.firstChildElement(elementName);
      baseElement = nextElement;
    }
    return baseElement;
  }


  /**
   * Helper function for converting ISIS pixel type and byte order to a PDS4 data_type value.
   *
   * @param pixelType The ISIS pixel type of the data
   * @param endianType The byte order of the data
   *
   * @return @b QString The PDS4 data_type value for the given pixel type and byte order.
   */
  QString ProcessExportPds4::PDS4PixelType(PixelType pixelType, ByteOrder endianType) {
    QString pds4Type("UNK");
    if(p_pixelType == Isis::UnsignedByte) {
      pds4Type = "UnsignedByte";
    }
    else if((p_pixelType == Isis::UnsignedWord) && (p_endianType == Isis::Msb)) {
      pds4Type = "UnsignedMSB2";
    }
    else if((p_pixelType == Isis::UnsignedWord) && (p_endianType == Isis::Lsb)) {
      pds4Type = "UnsignedLSB2";
    }
    else if((p_pixelType == Isis::SignedWord) && (p_endianType == Isis::Msb)) {
      pds4Type = "SignedMSB2";
    }
    else if((p_pixelType == Isis::SignedWord) && (p_endianType == Isis::Lsb)) {
      pds4Type = "SignedLSB2";
    }
    else if((p_pixelType == Isis::Real) && (p_endianType == Isis::Msb)) {
      pds4Type = "IEEE754MSBSingle";
    }
    else if((p_pixelType == Isis::Real) && (p_endianType == Isis::Lsb)) {
      pds4Type = "IEEE754LSBSingle";
    }
    else {
      std::string msg = "Unsupported PDS pixel type or sample size";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    return pds4Type;
  }


  /**
   * Add a modification history instance by adding a Modification_Detail entry
   * to the Modification_History element. If there are no existing entries,
   * this will create a Modification_History element also.
   *
   * @param description The description of the modification.
   * @param date The date of the modification. Expected format is "YYYY-MM-DD". Defaults to "tbd".
   * @param version The product version. Expected format is "m.n". Defaults to "tbd".
   */
  void ProcessExportPds4::addHistory(QString description, QString date, QString version) {
    // Check that at least the "Identification_Area" element exists.
    QDomElement identificationElement;
    QStringList identificationPath;
    identificationPath.append("Product_Observational");
    identificationPath.append("Identification_Area");
    try {
      identificationElement = getElement(identificationPath);
      if( identificationElement.isNull() ) {
        throw IException(IException::Unknown, "", _FILEINFO_);
      }
    }
    catch(IException &e) {
      std::string msg = "Could not find Identification_Area element "
                    "to add modification history under.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    // Check if the "Modification_History" element exists yet.
    // If not, create one.
    QDomElement historyElement = identificationElement.firstChildElement("Modification_History");
    if ( historyElement.isNull() ) {
      historyElement = m_domDoc->createElement("Modification_History");
      identificationElement.insertAfter( historyElement,
                                         identificationElement.lastChildElement() );
    }

    // Create the "Modification_Detail" element and add it to the end of the
    // "Modification_History" element.
    QDomElement detailElement = m_domDoc->createElement("Modification_Detail");

    QDomElement modDateElement = m_domDoc->createElement("modification_date");
    PvlToXmlTranslationManager::setElementValue(modDateElement, date);
    detailElement.appendChild(modDateElement);

    QDomElement versionIdElement = m_domDoc->createElement("version_id");
    PvlToXmlTranslationManager::setElementValue(versionIdElement, version);
    detailElement.appendChild(versionIdElement);

    QDomElement descriptionElement = m_domDoc->createElement("description");
    PvlToXmlTranslationManager::setElementValue(descriptionElement, description);
    detailElement.appendChild(descriptionElement);

    historyElement.insertAfter( detailElement,
                                historyElement.lastChildElement() );
  }


  /**
   * This function will go through an XML document and attempt to convert all
   * "units" attributes to the appropriate PDS4 units format.
   *
   * This method uses a pvl config file to determine what the proper PDS4
   * format is and what potential input formats are. The file is converted to
   * a map which is then used to convert all of the input units. See
   * $ISISROOT/appdata/translations/pds4ExportUnits.pvl for more information on this file.
   *
   * This method is automatically called in StandardPds4Label(), but may need
   * to be called again if the label is changed afterwards.
   *
   * @param[in,out] label A reference to the label that the units will be
   *                      translated in.
   * @param transMapFile The path to the config file that will be used to
   *                     determine unit translations.
   */
  void ProcessExportPds4::translateUnits(QDomDocument &label, QString transMapFile) {
    Pvl configPvl;
    try {
      configPvl.read(transMapFile.toStdString());
    }
    catch(IException &e) {
      std::string msg = "Failed to read unit translation config file [" + transMapFile.toStdString() + "].";
      throw IException(e, IException::Io, msg, _FILEINFO_);
    }

    QMap<QString, QString> transMap;
    try {
      transMap = createUnitMap(configPvl);
    }
    catch(IException &e) {
      std::string msg = "Failed to load unit translation config file [" + transMapFile.toStdString() + "].";
      throw IException(e, IException::Unknown, msg, _FILEINFO_);
    }

    // Now that the map is filled, recursively search through the XML document
    // for units and translate them.
    try {
      translateChildUnits( label.documentElement(), transMap );
    }
    catch(IException &e) {
      std::string msg = "Failed to translate units with config file [" + transMapFile.toStdString() + "].";
      throw IException(e, IException::Unknown, msg, _FILEINFO_);
    }
  }


  /**
   * Helper function for creating the unit translation map from a PVL object.
   *
   * @param configPvl The config PVL that defines the map.
   *
   * @return @b QMap<QString,QString> The map that converts lower case ISIS
   *                                  units to PDS4 units.
   *
   * @see ProcessExportPds4::translateUnits
   */
  QMap<QString, QString> ProcessExportPds4::createUnitMap(Pvl configPvl) {
    QMap<QString, QString> transMap;
    for (int i = 0; i < configPvl.objects(); i++) {
      PvlObject unitObject = configPvl.object(i);
      for (int j = 0; j < unitObject.groups(); j++) {
        PvlGroup unitGroup = unitObject.group(j);
        if (!unitGroup.hasKeyword("PDS4_Unit")) {
          std::string msg = "No PDS4 standard specified for for [" + unitGroup.name() + "]";
          throw IException(IException::Programmer, msg, _FILEINFO_);
        }
        PvlKeyword pds4Key = unitGroup["PDS4_Unit"];
        // Add the PDS4 format for when the format is already correct.
        // This also handles case issues such as KM instead of km.
        transMap.insert(QString::fromStdString(pds4Key[0]).toLower(), QString::fromStdString(pds4Key[0]));

        // If there are ISIS versions with different formats then add those.
        if (unitGroup.hasKeyword("ISIS_Units")) {
          PvlKeyword isisKey = unitGroup["ISIS_Units"];
          for (int k = 0; k < isisKey.size() ; k++) {
            transMap.insert(QString::fromStdString(isisKey[k]).toLower(), QString::fromStdString(pds4Key[0]));
          }
        }
      }
    }
    return transMap;
  }


  /**
   * Recursive method that will translate the "unit" attribute of any child
   * elements of a given element. Returns void if the given element has no
   * children.
   *
   * @param parent The element whose children's units will be translated. This
   *               method will be recursively called on all child elements.
   * @param transMap The translation map with lowercase ISIS units as keys and
   *                 PDS4 units as values.
   *
   * @see ProcessExportPds4::translateUnits
   */
  void ProcessExportPds4::translateChildUnits(QDomElement parent, QMap<QString, QString> transMap) {
    QDomElement childElement = parent.firstChildElement();

    while( !childElement.isNull() ) {
      if ( childElement.hasAttribute("unit") ) {
        QString originalUnit = childElement.attribute("unit");
        if ( transMap.contains( originalUnit.toLower() ) ) {
          childElement.setAttribute("unit", transMap.value( originalUnit.toLower() ) );
        }
        else {
          std::string msg = "Could not translate unit [" + originalUnit.toStdString() + "] to PDS4 format.";
          throw IException(IException::Unknown, msg, _FILEINFO_);
        }
      }
      translateChildUnits(childElement, transMap);
      childElement = childElement.nextSiblingElement();
    }

    // Base case: If there are no more children end return
    return;
  }


} // End of Isis namespace
