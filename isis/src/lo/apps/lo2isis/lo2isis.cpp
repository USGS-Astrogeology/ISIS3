/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "lo2isis.h"
#include "ProcessImportPds.h"
#include "UserInterface.h"
#include "FileName.h"
#include "Pvl.h"
#include "IException.h"
#include "TextFile.h"
#include <QString>

using namespace std;

namespace Isis{
  void TranslateLunarLabels(FileName &labelFile, Cube *ocube);

  void lo2isis(UserInterface &ui) {
    ProcessImportPds p;
    Pvl label;
    FileName in = ui.GetCubeName("FROM").toStdString();

    //Checks if in file is rdr
    label = in.expanded();
    if(label.hasObject("IMAGE_MAP_PROJECTION")) {
      std::string msg = "[" + in.name() + "] appears to be an rdr file.";
      msg += " Use pds2isis.";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    p.SetPdsFile(QString::fromStdString(in.expanded()), "", label);
    // Segfault here
    CubeAttributeOutput &att = ui.GetOutputAttribute("TO");
    Cube *ocube = p.SetOutputCube(ui.GetCubeName("TO"), att);
    p.StartProcess();
    TranslateLunarLabels(in, ocube);
    p.EndProcess();

    return;
  }

  void TranslateLunarLabels(FileName &labelFile, Cube *ocube) {

    // Transfer the instrument group to the output cube
    std::string transDir = "$ISISROOT/appdata/translations/";
    Pvl inputLabel(labelFile.expanded());
    FileName transFile;
    FileName bandBinTransFile;

    bool hasFiducial = false;
    // Check to see if file is PDS
    if(inputLabel.hasKeyword("PDS_VERSION_ID", Pvl::None)) {
      QString pdsVersion = QString::fromStdString(inputLabel.findKeyword("PDS_VERSION_ID", Pvl::None)[0]);

      if(pdsVersion == "PDS3") {
        if(inputLabel.hasKeyword("LO:FIDUCIAL_ID", Pvl::Traverse)) {
          hasFiducial = true;
          bandBinTransFile = transDir + "LoPdsFiducialImport.trn";
        }
        else if(inputLabel.hasKeyword("LO:BORESIGHT_SAMPLE", Pvl::Traverse)) {
          bandBinTransFile = transDir + "LoPdsBoresightImport.trn";
        }
        else {
          std::string msg = "[" + labelFile.name() + "] does not contain boresight or fiducial information";
          throw IException(IException::User, msg, _FILEINFO_);
        }
      }
      else {
        std::string msg = "[" + labelFile.name() + "] contains unknown PDS version [" +
                     pdsVersion.toStdString() + "]";
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }
    // Else the input is an Isis2 cube
    else {
      if(inputLabel.hasKeyword("FIDUCIAL_ID", Pvl::Traverse)) {
        hasFiducial = true;
        bandBinTransFile = transDir + "LoIsis2FiducialImport.trn";
      }
      else if(inputLabel.hasKeyword("BORESIGHT_SAMPLE", Pvl::Traverse)) {
        bandBinTransFile = transDir + "LoIsis2BoresightImport.trn";
      }
      else {
        std::string msg = "[" + labelFile.name() + "] does not contain boresight or fiducial information";
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }

    transFile = transDir + "LoGeneralImport.trn";
    // Get the translation manager ready
    PvlToPvlTranslationManager commonlabelXlater(inputLabel, QString::fromStdString(transFile.expanded()));
    // Pvl outputLabels;
    Pvl *outputLabel = ocube->label();
    commonlabelXlater.Auto(*(outputLabel));

    PvlToPvlTranslationManager labelXlater(inputLabel, QString::fromStdString(bandBinTransFile.expanded()));
    labelXlater.Auto(*(outputLabel));

    PvlGroup &inst = outputLabel->findGroup("Instrument", Pvl::Traverse);

    //Creates FiducialCoordinateMicron with the proper units
    if(!inputLabel.hasKeyword("LO:BORESIGHT_SAMPLE", Pvl::Traverse)) {
      QString fcm = QString::fromStdString(inst.findKeyword("FiducialCoordinateMicron"));
      QString fcmUnits = fcm;
      fcmUnits.remove(QRegExp("^[0-9.]*"));
      fcm.remove(QRegExp("[a-zA-Z]*$"));
      inst.findKeyword("FiducialCoordinateMicron").setValue(fcm.toStdString(), fcmUnits.toStdString());
    }

    // High Resolution & Fiducial Medium Case
    if(hasFiducial) {
      //Add units to some keywords
      PvlKeyword fxc = inst.findKeyword("FiducialXCoordinates");
      inst.findKeyword("FiducialXCoordinates").clear();
      for(int i = 0; i < fxc.size(); i++) {
        inst.findKeyword("FiducialXCoordinates").addValue(fxc[i], "mm");
      }

      PvlKeyword fyc = inst.findKeyword("FiducialYCoordinates");
      inst.findKeyword("FiducialYCoordinates").clear();
      for(int i = 0; i < fyc.size(); i++) {
        inst.findKeyword("FiducialYCoordinates").addValue(fyc[i], "mm");
      }

      PvlKeyword fl = inst.findKeyword("FiducialLines");
      inst.findKeyword("FiducialLines").clear();
      for(int i = 0; i < fl.size(); i++) {
        inst.findKeyword("FiducialLines").addValue(fl[i], "pixels");
      }

      PvlKeyword fs = inst.findKeyword("FiducialSamples");
      inst.findKeyword("FiducialSamples").clear();
      for(int i = 0; i < fs.size(); i++) {
        inst.findKeyword("FiducialSamples").addValue(fs[i], "pixels");
      }
    }
    else if(!hasFiducial) {
      //What needs to be done if it contains Boresight info
    }

    QString instrumentID = QString::fromStdString(inst.findKeyword("InstrumentId"));
    QString spacecraftName = QString::fromStdString(inst.findKeyword("SpacecraftName"));

    //Determines the NaifFrameCode
    PvlGroup kerns("Kernels");
    QString frameCode;
    if(spacecraftName.compare("Lunar Orbiter 3") == 0) {
      frameCode = "-533";
    }
    else if(spacecraftName.compare("Lunar Orbiter 4") == 0) {
      frameCode = "-534";
    }
    else if(spacecraftName.compare("Lunar Orbiter 5") == 0) {
      frameCode = "-535";
    }

    if(instrumentID == "High Resolution Camera") {
      frameCode += "001";
    }
    else if(instrumentID == "Medium Resolution Camera") {
      frameCode += "002";
    }

    //Create subframe and frame keywords
    QString imgNumber = QString::fromStdString(inst.findKeyword("ImageNumber"));
    int subFrame = imgNumber.mid(5).toInt();

    inst.addKeyword(PvlKeyword("SubFrame", std::to_string(subFrame)));
    //ImageNumber is auto translated, and no longer needed
    inst.deleteKeyword("ImageNumber");

    kerns += PvlKeyword("NaifFrameCode", frameCode.toStdString());
    outputLabel->findObject("IsisCube").addGroup(kerns);

    return;
  }
}
