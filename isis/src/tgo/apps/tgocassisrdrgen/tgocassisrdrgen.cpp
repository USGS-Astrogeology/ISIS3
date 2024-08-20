/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "tgocassisrdrgen.h"

#include <QRegularExpression>
#include <QString>
#include <QDomDocument>

#include "Application.h"
#include "Cube.h"
#include "ExportDescription.h"
#include "FileName.h"
#include "iTime.h"
#include "Process.h"
#include "ProcessExportPds4.h"
#include "Pvl.h"
#include "PvlKeyword.h"
#include "PvlToXmlTranslationManager.h"


using namespace std;

namespace Isis {
  void tgocassisrdrgen(UserInterface &ui) {
    Cube icube;
    
    // Check if input file is indeed, a cube
    if (FileName(ui.GetCubeName("FROM")).expanded().right(3) != "cub") {
      QString msg = "Input file [" + ui.GetCubeName("FROM") +
                  "] does not appear to be a cube";
      throw  IException(IException::User, msg, _FILEINFO_);
    }
    
    icube.open(ui.GetCubeName("FROM"), "r");
    tgocassisrdrgen(&icube, ui);
  }

  void tgocassisrdrgen(Cube *icube, UserInterface &ui) {
    // Setup the process and set the input cube
    ProcessExportPds4 process;
    process.SetInputCube(icube);

    PvlObject *label= icube->label();

    PvlGroup targetGroup;
    QString logicalId = "urn:esa:psa:em16_tgo_cas:";

    if ( label->findObject("IsisCube").hasGroup("Instrument") ) {
      targetGroup = label->findObject("IsisCube").findGroup("Instrument");
      if (label->findObject("IsisCube").hasGroup("Mapping")) {
        logicalId += "data_projected:";
      }
      else {
        logicalId += "data_raw:";
      }
    }
    else if ( label->findObject("IsisCube").hasGroup("Mosaic") ) {
      targetGroup = label->findObject("IsisCube").findGroup("Mosaic");
      logicalId += "data_derived:";
    }

    // Check if the cube is able to be translated into a CaSSIS xml file
    // This could very well be unnecessary
    if (!targetGroup.findKeyword("InstrumentId").isEquivalent("CaSSIS")) {
      QString msg = "Input file [" + ui.GetCubeName("FROM") +
                  "] does not appear to be a CaSSIS RDR product. The image" +
                  "instrument is not the CaSSIS instrument";
      throw  IException(IException::User, msg, _FILEINFO_);
    }

    // Add the ProductId keyword for translation. If a product id is not specified
    // by the user, set it to the Observation Id.
    // This is added before the translation instead of adding it to the exported xml
    // because of the ease of editing pvl vs xml.
    PvlKeyword productId("ProductId");
    if ( ui.WasEntered("PRODUCTID") ) {
      productId.setValue( ui.GetString("PRODUCTID").toStdString());
    }
    else {
      // Get the observationId from the Archive Group, or the Mosaic group, if the input is a mosaic
      QString observationId;

      if (label->findObject("IsisCube").hasGroup("Mosaic")) {
        PvlGroup mosaicGroup = label->findObject("IsisCube").findGroup("Mosaic");
        observationId = QString::fromStdString(mosaicGroup.findKeyword("ObservationId")[0]);
      }
      else if(label->findObject("IsisCube").hasGroup("Archive")){
        PvlGroup archiveGroup = label->findObject("IsisCube").findGroup("Archive");
        observationId = QString::fromStdString(archiveGroup.findKeyword("ObservationId")[0]);
      }
      productId.setValue(observationId.toStdString());
    }

    targetGroup.addKeyword(productId);
    logicalId += QString::fromStdString(productId[0]);
    process.setLogicalId(logicalId);

    // For a mosaic, calculate stitched browse LID (lidvid)
    // This is as follows:
    // StartTime of the first framelet, rounded down to the nearest second
    if (label->findObject("IsisCube").hasGroup("Mosaic")) {
      QString startTime = QString::fromStdString(label->findObject("IsisCube").findGroup("Mosaic").findKeyword("StartTime"));
      startTime = iTime(startTime).UTC(0).toLower().remove("-").remove(":");

      // StopTime of the last framelet, rounded down to the nearest second + 4 seconds
      QString stopTime = QString::fromStdString(label->findObject("IsisCube").findGroup("Mosaic").findKeyword("StopTime"));
      stopTime = (iTime(iTime(stopTime).UTC(0)) + 4).UTC().toLower().remove("-").remove(":");

      // UID
      QString UID = QString::fromStdString(label->findObject("IsisCube").findGroup("Archive").findKeyword("UID"));
      // FilterName
      QString filterName = QString::fromStdString((label->findObject("IsisCube").findGroup("BandBin").
                                  findKeyword("FilterName"))).toLower();

      QString lid = QString("urn:esa:psa:em16_tgo_cas:data_calibrated:cas_cal_sc_%1-%2-%3-%4-sti").arg(startTime)
                            .arg(stopTime).arg(filterName).arg(UID);

      label->findObject("IsisCube").findGroup("Archive").addKeyword(PvlKeyword("LID", lid.toStdString()));
    }

    // Set Title
    if ( ui.WasEntered("TITLE") ) {
      process.setTitle( ui.GetString("TITLE") );
    }

    // Set Version ID
    if ( ui.WasEntered("VERSIONID") ) {
      process.setVersionId( ui.GetString("VERSIONID") );
    }

    process.setPixelDescription("Pixel values are in units of I/F (intensity/flux). I/F is defined as"
                                "the ratio of the observed radiance and the radiance of a 100% "
                                "lambertian reflector with the sun and camera orthogonal to the "
                                "observing surface.");

    // std PDS4 label
    process.StandardPds4Label();

    // Add PSA-specific schema
    process.addSchema("PDS4_PSA_1000.xsd",
                      "xmlns:psa",
                      "http://psa.esa.int/psa/v1");

    // Add CaSSIS-specific schema
    process.addSchema("PDS4_PSA_EM16_CAS_1000.xsd",
                      "xmlns:cas",
                      "http://psa.esa.int/psa/em16/cas/v1");

    // Add geometry schema for mosaics
    if (label->findObject("IsisCube").hasGroup("Mosaic")) {
      process.addSchema("PDS4_GEOM_1B00_1610.sch",
                        "PDS4_GEOM_1B00_1610.xsd",
                        "xmlns:geom",
                        "http://pds.nasa.gov/pds4/geom/v1");
    }

   /*
    * Add additional pds label data here
    */
    QDomDocument &pdsLabel = process.GetLabel();

    // The default translation for for non-mosaicked output
    QString exportTranslationFile = "$ISISROOT/appdata/translations/TgoCassisExport.trn";

    if (label->findObject("IsisCube").hasGroup("Mosaic")) {
      exportTranslationFile = "$ISISROOT/appdata/translations/TgoCassisExportMosaic.trn";
    }

    Pvl *labelPvl = icube->label();

    PvlToXmlTranslationManager cubeLab(*labelPvl, exportTranslationFile);
    cubeLab.Auto(pdsLabel);

    ProcessExportPds4::translateUnits(pdsLabel);
    process.reorder();

    // Units are automatically translated for the focal length, and there is no way at this time to
    // turn it off, but the cas:CASSIS_Data standard specifies that the focal-length output may not
    // have units, so remove the units from cas:focal length.
    QDomDocument &pdsLabelNext = process.GetLabel();
    QDomElement observationNode = pdsLabelNext.documentElement().firstChildElement("Observation_Area");
    QStringList xmlPath;
    xmlPath << "Observation_Area"
            << "Mission_Area"
            << "cas:CASSIS_Data"
            << "cas:telescope_information"
            << "cas:focal_length";
    QDomElement focalLengthNode = process.getElement(xmlPath, observationNode);

    if (focalLengthNode.hasAttribute("unit")) {
      focalLengthNode.removeAttribute("unit");
    }

    // Fix the footprint section's output structure
    QStringList xmlPath2;
    xmlPath2 << "Observation_Area"
            << "Discipline_Area"
            << "geom:Geometry"
            << "geom:Geometry_Orbiter"
            << "geom:Surface_Geometry"
            << "geom:Surface_Geometry_Specific"
            << "geom:Footprint_Vertices"
            << "geom:Pixel_Intercept";
    QDomElement pixelInterceptNode = process.getElement(xmlPath2, observationNode);

    if(pixelInterceptNode.hasChildNodes()) {
      QDomElement pixelInterceptNode2 = pixelInterceptNode.nextSiblingElement();
      QDomElement pixelInterceptNode3 = pixelInterceptNode2.nextSiblingElement();
      QDomElement pixelInterceptNode4 = pixelInterceptNode3.nextSiblingElement();

      pixelInterceptNode.insertAfter(pixelInterceptNode.firstChildElement("geom:pixel_latitude"), pixelInterceptNode.firstChildElement("geom:reference_pixel_location"));
      pixelInterceptNode2.insertAfter(pixelInterceptNode2.firstChildElement("geom:pixel_latitude"), pixelInterceptNode2.firstChildElement("geom:reference_pixel_location"));
      pixelInterceptNode3.insertAfter(pixelInterceptNode3.firstChildElement("geom:pixel_latitude"), pixelInterceptNode3.firstChildElement("geom:reference_pixel_location"));
      pixelInterceptNode4.insertAfter(pixelInterceptNode4.firstChildElement("geom:pixel_latitude"), pixelInterceptNode4.firstChildElement("geom:reference_pixel_location"));
      
      pixelInterceptNode2.setTagName("geom:Pixel_Intercept");
      pixelInterceptNode3.setTagName("geom:Pixel_Intercept");
      pixelInterceptNode4.setTagName("geom:Pixel_Intercept");
    }
    else {
    QStringList xmlPathFootprint;
    xmlPathFootprint << "Observation_Area"
                     << "Discipline_Area"
                     << "geom:Geometry"
                     << "geom:Geometry_Orbiter"
                     << "geom:Surface_Geometry"
                     << "geom:Surface_Geometry_Specific"
                     << "geom:Footprint_Vertices";
      QDomElement footprintNode = process.getElement(xmlPathFootprint, observationNode);
      footprintNode.parentNode().removeChild(footprintNode);
    }

    if(label->findObject("IsisCube").hasGroup("Archive")) {
      PvlGroup archiveGroup = label->findObject("IsisCube").findGroup("Archive");

      if (archiveGroup.hasKeyword("Browse")) {
        PvlKeyword browseKeyword = archiveGroup.findKeyword("Browse");

        QDomElement referenceListNode = pdsLabelNext.documentElement().firstChildElement("Reference_List");
        QDomElement browseNode = pdsLabel.createElement("Browse");

        for (int i = 0; i < browseKeyword.size(); i++) {
          QString browseFile = QString::fromStdString(browseKeyword[i]);
          QStringList browseComponentList = browseFile.split(QLatin1Char('_'));

          QDomElement browseItem = pdsLabel.createElement("browse_record");
          browseItem.setAttribute("record_num", i);
          browseItem.setAttribute("filterType", browseComponentList[4]);

          QDomText browseText = pdsLabel.createTextNode(browseFile);
          browseItem.appendChild(browseText);
          browseNode.appendChild(browseItem);
        }
        referenceListNode.appendChild(browseNode);
      }
    }

    QString outFile = ui.GetFileName("TO");

    process.WritePds4(outFile);
  }
}
