/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "photomet.h"

#include <QDebug>
#include <QString>
#include <map>
#include <sstream>

#include "Angle.h"
#include "Camera.h"
#include "Cube.h"
#include "IException.h"
#include "Photometry.h"
#include "ProcessByLine.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "SpecialPixel.h"

using namespace std;

namespace Isis {

void photomet(UserInterface &ui, Pvl *appLog) {
  Cube icube;
  CubeAttributeInput inAtt = ui.GetInputAttribute("FROM");
  if (inAtt.bands().size() != 0) {
    icube.setVirtualBands(inAtt.bands());
  }
  icube.open(ui.GetCubeName("FROM"));
  photomet(&icube, ui, appLog);
}

void photomet(Cube *icube, UserInterface &ui, Pvl *appLog) {
  Camera *cam;
  Photometry *pho;
  QString angleSource;
  bool useBackplane = false;
  double maxema;
  double maxinc;
  bool usedem;
  double centerPhase;
  double centerIncidence;
  double centerEmission;
  bool usePhasefile = false;
  bool useIncidencefile = false;
  bool useEmissionfile = false;
  double phaseAngle;
  double incidenceAngle;
  double emissionAngle;

  // We will be processing by line
  ProcessByLine p;

  // get QString of parameter changes to make
  QString changePar = (QString)ui.GetString("CHNGPAR");
  changePar = changePar.toUpper();
  (void)
      changePar.simplified();  // cast to void to silence unused result warning
  changePar.replace(" =", "=");
  changePar.replace("= ", "=");
  changePar.remove('"');
  bool useChangePar = true;
  if (changePar == "NONE" || changePar == "") {
    useChangePar = false;
  }
  QMap<QString, QString> parMap;
  if (useChangePar) {
    QStringList parList = changePar.split(" ");
    for (int i = 0; i < parList.size(); i++) {
      QString parPair = parList.at(i);
      parPair = parPair.toUpper();
      QStringList parvalList = parPair.split("=");
      if (parvalList.size() != 2) {
        QString message =
            "The value you entered for CHNGPAR is invalid. You must enter "
            "pairs of ";
        message +=
            "data that are formatted as parname=value and each pair is "
            "separated by spaces.";
        throw IException(IException::User, message, _FILEINFO_);
      }
      parMap[parvalList.at(0)] = parvalList.at(1);
    }
  }

  Pvl toNormPvl;
  PvlGroup normLog("NormalizationModelParametersUsed");
  QString normName = ui.GetAsString("NORMNAME");
  normName = normName.toUpper();
  bool wasFound = false;
  if (ui.WasEntered("FROMPVL")) {
    QString normVal;
    Pvl fromNormPvl;
    PvlObject fromNormObj;
    PvlGroup fromNormGrp;
    QString input = ui.GetFileName("FROMPVL");
    fromNormPvl.read(input);
    if (fromNormPvl.hasObject("NormalizationModel")) {
      fromNormObj = fromNormPvl.findObject("NormalizationModel");
      if (fromNormObj.hasGroup("Algorithm")) {
        PvlObject::PvlGroupIterator fromNormGrp = fromNormObj.beginGroup();
        if (fromNormGrp->hasKeyword("NORMNAME")) {
          normVal = (QString)fromNormGrp->findKeyword("NORMNAME");
        } else if (fromNormGrp->hasKeyword("NAME")) {
          normVal = (QString)fromNormGrp->findKeyword("NAME");
        } else {
          normVal = "NONE";
        }
        normVal = normVal.toUpper();
        if (normName == normVal && normVal != "NONE") {
          wasFound = true;
        }
        if ((normName == "NONE" || normName == "FROMPVL") &&
            normVal != "NONE" && !wasFound) {
          normName = normVal;
          wasFound = true;
        }
        if (!wasFound) {
          while (fromNormGrp != fromNormObj.endGroup()) {
            if (fromNormGrp->hasKeyword("NORMNAME") ||
                fromNormGrp->hasKeyword("NAME")) {
              if (fromNormGrp->hasKeyword("NORMNAME")) {
                normVal = (QString)fromNormGrp->findKeyword("NORMNAME");
              } else if (fromNormGrp->hasKeyword("NAME")) {
                normVal = (QString)fromNormGrp->findKeyword("NAME");
              } else {
                normVal = "NONE";
              }
              normVal = normVal.toUpper();
              if (normName == normVal && normVal != "NONE") {
                wasFound = true;
                break;
              }
              if ((normName == "NONE" || normName == "FROMPVL") &&
                  normVal != "NONE" && !wasFound) {
                normName = normVal;
                wasFound = true;
                break;
              }
            }
            fromNormGrp++;
          }
        }
      }
    }
    // Check to make sure that a normalization model was specified
    if (normName == "NONE" || normName == "FROMPVL") {
      QString message =
          "A Normalization model must be specified before running this "
          "program. ";
      message +=
          "You need to provide a Normalization model through an input PVL "
          "(FROMPVL) or ";
      message +=
          "you need to specify a Normalization model through the program "
          "interface.";
      throw IException(IException::User, message, _FILEINFO_);
    }
    if (wasFound) {
      toNormPvl.addObject(fromNormObj);
    } else {
      toNormPvl.addObject(PvlObject("NormalizationModel"));
      toNormPvl.findObject("NormalizationModel")
          .addGroup(PvlGroup("Algorithm"));
      toNormPvl.findObject("NormalizationModel")
          .findGroup("Algorithm")
          .addKeyword(PvlKeyword("NORMNAME", normName), Pvl::Replace);
    }
  } else {
    // Check to make sure that a normalization model was specified
    if (normName == "NONE" || normName == "FROMPVL") {
      QString message =
          "A Normalization model must be specified before running this "
          "program. ";
      message +=
          "You need to provide a Normalization model through an input PVL "
          "(FROMPVL) or ";
      message +=
          "you need to specify a Normalization model through the program "
          "interface.";
      throw IException(IException::User, message, _FILEINFO_);
    }
    toNormPvl.addObject(PvlObject("NormalizationModel"));
    toNormPvl.findObject("NormalizationModel").addGroup(PvlGroup("Algorithm"));
    toNormPvl.findObject("NormalizationModel")
        .findGroup("Algorithm")
        .addKeyword(PvlKeyword("NORMNAME", normName), Pvl::Replace);
  }
  normLog += PvlKeyword("NORMNAME", normName);

  if (normName == "ALBEDO" || normName == "MIXED") {
    if (parMap.contains("INCREF")) {
      toNormPvl.findObject("NormalizationModel")
          .findGroup("Algorithm")
          .addKeyword(
              PvlKeyword("INCREF", toString(toDouble(parMap["INCREF"]))),
              Pvl::Replace);
    } else if (ui.WasEntered("INCREF")) {
      QString keyval = ui.GetString("INCREF");
      double incref = toDouble(keyval);
      toNormPvl.findObject("NormalizationModel")
          .findGroup("Algorithm")
          .addKeyword(PvlKeyword("INCREF", toString(incref)), Pvl::Replace);
    } else {
      if (!toNormPvl.findObject("NormalizationModel")
               .findGroup("Algorithm")
               .hasKeyword("INCREF")) {
        QString message =
            "The " + normName +
            " Normalization model requires a value for the INCREF parameter.";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }
    normLog += toNormPvl.findObject("NormalizationModel")
                   .findGroup("Algorithm")
                   .findKeyword("INCREF");
    if (normName == "MIXED") {
      if (parMap.contains("INCMAT")) {
        toNormPvl.findObject("NormalizationModel")
            .findGroup("Algorithm")
            .addKeyword(
                PvlKeyword("INCMAT", toString(toDouble(parMap["INCMAT"]))),
                Pvl::Replace);
      } else if (ui.WasEntered("INCMAT")) {
        QString keyval = ui.GetString("INCMAT");
        double incmat = toDouble(keyval);
        toNormPvl.findObject("NormalizationModel")
            .findGroup("Algorithm")
            .addKeyword(PvlKeyword("INCMAT", toString(incmat)), Pvl::Replace);
      } else {
        if (!toNormPvl.findObject("NormalizationModel")
                 .findGroup("Algorithm")
                 .hasKeyword("INCMAT")) {
          QString message =
              "The " + normName +
              " Normalization model requires a value for the INCMAT parameter.";
          message += "The normal range for INCMAT is: 0 <= INCMAT < 90";
          throw IException(IException::User, message, _FILEINFO_);
        }
      }
      normLog += toNormPvl.findObject("NormalizationModel")
                     .findGroup("Algorithm")
                     .findKeyword("INCMAT");
    }
    if (parMap.contains("THRESH")) {
      toNormPvl.findObject("NormalizationModel")
          .findGroup("Algorithm")
          .addKeyword(
              PvlKeyword("THRESH", toString(toDouble(parMap["THRESH"]))),
              Pvl::Replace);
    } else if (ui.WasEntered("THRESH")) {
      QString keyval = ui.GetString("THRESH");
      double thresh = toDouble(keyval);
      toNormPvl.findObject("NormalizationModel")
          .findGroup("Algorithm")
          .addKeyword(PvlKeyword("THRESH", toString(thresh)), Pvl::Replace);
    } else {
      if (!toNormPvl.findObject("NormalizationModel")
               .findGroup("Algorithm")
               .hasKeyword("THRESH")) {
        QString message =
            "The " + normName +
            " Normalization model requires a value for the THRESH parameter.";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }
    normLog += toNormPvl.findObject("NormalizationModel")
                   .findGroup("Algorithm")
                   .findKeyword("THRESH");
    if (parMap.contains("ALBEDO")) {
      toNormPvl.findObject("NormalizationModel")
          .findGroup("Algorithm")
          .addKeyword(
              PvlKeyword("ALBEDO", toString(toDouble(parMap["ALBEDO"]))),
              Pvl::Replace);
    } else if (ui.WasEntered("ALBEDO")) {
      QString keyval = ui.GetString("ALBEDO");
      double albedo = toDouble(keyval);
      toNormPvl.findObject("NormalizationModel")
          .findGroup("Algorithm")
          .addKeyword(PvlKeyword("ALBEDO", toString(albedo)), Pvl::Replace);
    } else {
      if (!toNormPvl.findObject("NormalizationModel")
               .findGroup("Algorithm")
               .hasKeyword("ALBEDO")) {
        QString message =
            "The " + normName +
            " Normalization model requires a value for the ALBEDO parameter.";
        message += "The ALBEDO parameter has no limited range";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }
    normLog += toNormPvl.findObject("NormalizationModel")
                   .findGroup("Algorithm")
                   .findKeyword("ALBEDO");
  } else if (normName == "MOONALBEDO") {
    if (parMap.contains("D")) {
      toNormPvl.findObject("NormalizationModel")
          .findGroup("Algorithm")
          .addKeyword(PvlKeyword("D", toString(toDouble(parMap["D"]))),
                      Pvl::Replace);
    } else if (ui.WasEntered("D")) {
      QString keyval = ui.GetString("D");
      double d = toDouble(keyval);
      toNormPvl.findObject("NormalizationModel")
          .findGroup("Algorithm")
          .addKeyword(PvlKeyword("D", toString(d)), Pvl::Replace);
    } else {
      if (!toNormPvl.findObject("NormalizationModel")
               .findGroup("Algorithm")
               .hasKeyword("D")) {
        QString message =
            "The " + normName +
            " Normalization model requires a value for the D parameter.";
        message += "The D parameter has no limited range";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }
    normLog += toNormPvl.findObject("NormalizationModel")
                   .findGroup("Algorithm")
                   .findKeyword("D");
    if (parMap.contains("E")) {
      toNormPvl.findObject("NormalizationModel")
          .findGroup("Algorithm")
          .addKeyword(PvlKeyword("E", toString(toDouble(parMap["E"]))),
                      Pvl::Replace);
    } else if (ui.WasEntered("E")) {
      QString keyval = ui.GetString("E");
      double e = toDouble(keyval);
      toNormPvl.findObject("NormalizationModel")
          .findGroup("Algorithm")
          .addKeyword(PvlKeyword("E", toString(e)), Pvl::Replace);
    } else {
      if (!toNormPvl.findObject("NormalizationModel")
               .findGroup("Algorithm")
               .hasKeyword("E")) {
        QString message =
            "The " + normName +
            " Normalization model requires a value for the E parameter.";
        message += "The E parameter has no limited range";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }
    normLog += toNormPvl.findObject("NormalizationModel")
                   .findGroup("Algorithm")
                   .findKeyword("E");
    if (parMap.contains("F")) {
      toNormPvl.findObject("NormalizationModel")
          .findGroup("Algorithm")
          .addKeyword(PvlKeyword("F", toString(toDouble(parMap["F"]))),
                      Pvl::Replace);
    } else if (ui.WasEntered("F")) {
      QString keyval = ui.GetString("F");
      double f = toDouble(keyval);
      toNormPvl.findObject("NormalizationModel")
          .findGroup("Algorithm")
          .addKeyword(PvlKeyword("F", toString(f)), Pvl::Replace);
    } else {
      if (!toNormPvl.findObject("NormalizationModel")
               .findGroup("Algorithm")
               .hasKeyword("F")) {
        QString message =
            "The " + normName +
            " Normalization model requires a value for the F parameter.";
        message += "The F parameter has no limited range";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }
    normLog += toNormPvl.findObject("NormalizationModel")
                   .findGroup("Algorithm")
                   .findKeyword("F");
    if (parMap.contains("G2")) {
      toNormPvl.findObject("NormalizationModel")
          .findGroup("Algorithm")
          .addKeyword(PvlKeyword("G2", toString(toDouble(parMap["G2"]))),
                      Pvl::Replace);
    } else if (ui.WasEntered("G2")) {
      QString keyval = ui.GetString("G2");
      double g2 = toDouble(keyval);
      toNormPvl.findObject("NormalizationModel")
          .findGroup("Algorithm")
          .addKeyword(PvlKeyword("G2", toString(g2)), Pvl::Replace);
    } else {
      if (!toNormPvl.findObject("NormalizationModel")
               .findGroup("Algorithm")
               .hasKeyword("G2")) {
        QString message =
            "The " + normName +
            " Normalization model requires a value for the G2 parameter.";
        message += "The G2 parameter has no limited range";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }
    normLog += toNormPvl.findObject("NormalizationModel")
                   .findGroup("Algorithm")
                   .findKeyword("G2");
    if (parMap.contains("XMUL")) {
      toNormPvl.findObject("NormalizationModel")
          .findGroup("Algorithm")
          .addKeyword(PvlKeyword("XMUL", toString(toDouble(parMap["XMUL"]))),
                      Pvl::Replace);
    } else if (ui.WasEntered("XMUL")) {
      QString keyval = ui.GetString("XMUL");
      double xmul = toDouble(keyval);
      toNormPvl.findObject("NormalizationModel")
          .findGroup("Algorithm")
          .addKeyword(PvlKeyword("XMUL", toString(xmul)), Pvl::Replace);
    } else {
      if (!toNormPvl.findObject("NormalizationModel")
               .findGroup("Algorithm")
               .hasKeyword("XMUL")) {
        QString message =
            "The " + normName +
            " Normalization model requires a value for the XMUL parameter.";
        message += "The XMUL parameter has no range limit";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }
    normLog += toNormPvl.findObject("NormalizationModel")
                   .findGroup("Algorithm")
                   .findKeyword("XMUL");
    if (parMap.contains("WL")) {
      toNormPvl.findObject("NormalizationModel")
          .findGroup("Algorithm")
          .addKeyword(PvlKeyword("WL", toString(toDouble(parMap["WL"]))),
                      Pvl::Replace);
    } else if (ui.WasEntered("WL")) {
      QString keyval = ui.GetString("WL");
      double wl = toDouble(keyval);
      toNormPvl.findObject("NormalizationModel")
          .findGroup("Algorithm")
          .addKeyword(PvlKeyword("WL", toString(wl)), Pvl::Replace);
    } else {
      if (!toNormPvl.findObject("NormalizationModel")
               .findGroup("Algorithm")
               .hasKeyword("WL")) {
        QString message =
            "The " + normName +
            " Normalization model requires a value for the WL parameter.";
        message += "The WL parameter has no range limit";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }
    normLog += toNormPvl.findObject("NormalizationModel")
                   .findGroup("Algorithm")
                   .findKeyword("WL");
    if (parMap.contains("H")) {
      toNormPvl.findObject("NormalizationModel")
          .findGroup("Algorithm")
          .addKeyword(PvlKeyword("H", toString(toDouble(parMap["H"]))),
                      Pvl::Replace);
    } else if (ui.WasEntered("H")) {
      QString keyval = ui.GetString("H");
      double h = toDouble(keyval);
      toNormPvl.findObject("NormalizationModel")
          .findGroup("Algorithm")
          .addKeyword(PvlKeyword("H", toString(h)), Pvl::Replace);
    } else {
      if (!toNormPvl.findObject("NormalizationModel")
               .findGroup("Algorithm")
               .hasKeyword("H")) {
        QString message =
            "The " + normName +
            " Normalization model requires a value for the H parameter.";
        message += "The H parameter has no limited range";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }
    normLog += toNormPvl.findObject("NormalizationModel")
                   .findGroup("Algorithm")
                   .findKeyword("H");
    if (parMap.contains("BSH1")) {
      toNormPvl.findObject("NormalizationModel")
          .findGroup("Algorithm")
          .addKeyword(PvlKeyword("BSH1", toString(toDouble(parMap["BSH1"]))),
                      Pvl::Replace);
    } else if (ui.WasEntered("BSH1")) {
      QString keyval = ui.GetString("BSH1");
      double bsh1 = toDouble(keyval);
      toNormPvl.findObject("NormalizationModel")
          .findGroup("Algorithm")
          .addKeyword(PvlKeyword("BSH1", toString(bsh1)), Pvl::Replace);
    } else {
      if (!toNormPvl.findObject("NormalizationModel")
               .findGroup("Algorithm")
               .hasKeyword("BSH1")) {
        QString message =
            "The " + normName +
            " Normalization model requires a value for the BSH1 parameter.";
        message += "The normal range for BSH1 is: 0 <= BSH1";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }
    normLog += toNormPvl.findObject("NormalizationModel")
                   .findGroup("Algorithm")
                   .findKeyword("BSH1");
    if (parMap.contains("XB1")) {
      toNormPvl.findObject("NormalizationModel")
          .findGroup("Algorithm")
          .addKeyword(PvlKeyword("XB1", toString(toDouble(parMap["XB1"]))),
                      Pvl::Replace);
    } else if (ui.WasEntered("XB1")) {
      QString keyval = ui.GetString("XB1");
      double xb1 = toDouble(keyval);
      toNormPvl.findObject("NormalizationModel")
          .findGroup("Algorithm")
          .addKeyword(PvlKeyword("XB1", toString(xb1)), Pvl::Replace);
    } else {
      if (!toNormPvl.findObject("NormalizationModel")
               .findGroup("Algorithm")
               .hasKeyword("XB1")) {
        QString message =
            "The " + normName +
            " Normalization model requires a value for the XB1 parameter.";
        message += "The XB1 parameter has no range limit";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }
    normLog += toNormPvl.findObject("NormalizationModel")
                   .findGroup("Algorithm")
                   .findKeyword("XB1");
    if (parMap.contains("XB2")) {
      toNormPvl.findObject("NormalizationModel")
          .findGroup("Algorithm")
          .addKeyword(PvlKeyword("XB2", toString(toDouble(parMap["XB2"]))),
                      Pvl::Replace);
    } else if (ui.WasEntered("XB2")) {
      QString keyval = ui.GetString("XB2");
      double xb2 = toDouble(keyval);
      toNormPvl.findObject("NormalizationModel")
          .findGroup("Algorithm")
          .addKeyword(PvlKeyword("XB2", toString(xb2)), Pvl::Replace);
    } else {
      if (!toNormPvl.findObject("NormalizationModel")
               .findGroup("Algorithm")
               .hasKeyword("XB2")) {
        QString message =
            "The " + normName +
            " Normalization model requires a value for the XB2 parameter.";
        message += "The XB2 parameter has no range limit";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }
    normLog += toNormPvl.findObject("NormalizationModel")
                   .findGroup("Algorithm")
                   .findKeyword("XB2");
  } else if (normName == "SHADE") {
    if (parMap.contains("INCREF")) {
      toNormPvl.findObject("NormalizationModel")
          .findGroup("Algorithm")
          .addKeyword(
              PvlKeyword("INCREF", toString(toDouble(parMap["INCREF"]))),
              Pvl::Replace);
    } else if (ui.WasEntered("INCREF")) {
      QString keyval = ui.GetString("INCREF");
      double incref = toDouble(keyval);
      toNormPvl.findObject("NormalizationModel")
          .findGroup("Algorithm")
          .addKeyword(PvlKeyword("INCREF", toString(incref)), Pvl::Replace);
    } else {
      if (!toNormPvl.findObject("NormalizationModel")
               .findGroup("Algorithm")
               .hasKeyword("INCREF")) {
        QString message =
            "The " + normName +
            " Normalization model requires a value for the INCREF parameter.";
        message += "The normal range for INCREF is: 0 <= INCREF < 90";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }
    normLog += toNormPvl.findObject("NormalizationModel")
                   .findGroup("Algorithm")
                   .findKeyword("INCREF");
    if (parMap.contains("ALBEDO")) {
      toNormPvl.findObject("NormalizationModel")
          .findGroup("Algorithm")
          .addKeyword(
              PvlKeyword("ALBEDO", toString(toDouble(parMap["ALBEDO"]))),
              Pvl::Replace);
    } else if (ui.WasEntered("ALBEDO")) {
      QString keyval = ui.GetString("ALBEDO");
      double albedo = toDouble(keyval);
      toNormPvl.findObject("NormalizationModel")
          .findGroup("Algorithm")
          .addKeyword(PvlKeyword("ALBEDO", toString(albedo)), Pvl::Replace);
    } else {
      if (!toNormPvl.findObject("NormalizationModel")
               .findGroup("Algorithm")
               .hasKeyword("ALBEDO")) {
        QString message =
            "The " + normName +
            " Normalization model requires a value for the ALBEDO parameter.";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }
    normLog += toNormPvl.findObject("NormalizationModel")
                   .findGroup("Algorithm")
                   .findKeyword("ALBEDO");
  } else if (normName == "TOPO") {
    if (parMap.contains("INCREF")) {
      toNormPvl.findObject("NormalizationModel")
          .findGroup("Algorithm")
          .addKeyword(
              PvlKeyword("INCREF", toString(toDouble(parMap["INCREF"]))),
              Pvl::Replace);
    } else if (ui.WasEntered("INCREF")) {
      QString keyval = ui.GetString("INCREF");
      double incref = toDouble(keyval);
      toNormPvl.findObject("NormalizationModel")
          .findGroup("Algorithm")
          .addKeyword(PvlKeyword("INCREF", toString(incref)), Pvl::Replace);
    } else {
      if (!toNormPvl.findObject("NormalizationModel")
               .findGroup("Algorithm")
               .hasKeyword("INCREF")) {
        QString message =
            "The " + normName +
            " Normalization model requires a value for the INCREF parameter.";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }
    normLog += toNormPvl.findObject("NormalizationModel")
                   .findGroup("Algorithm")
                   .findKeyword("INCREF");
    if (parMap.contains("THRESH")) {
      toNormPvl.findObject("NormalizationModel")
          .findGroup("Algorithm")
          .addKeyword(
              PvlKeyword("THRESH", toString(toDouble(parMap["THRESH"]))),
              Pvl::Replace);
    } else if (ui.WasEntered("THRESH")) {
      QString keyval = ui.GetString("THRESH");
      double thresh = toDouble(keyval);
      toNormPvl.findObject("NormalizationModel")
          .findGroup("Algorithm")
          .addKeyword(PvlKeyword("THRESH", toString(thresh)), Pvl::Replace);
    } else {
      if (!toNormPvl.findObject("NormalizationModel")
               .findGroup("Algorithm")
               .hasKeyword("THRESH")) {
        QString message =
            "The " + normName +
            " Normalization model requires a value for the THRESH parameter.";
        message += "The THRESH parameter has no range limit";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }
    normLog += toNormPvl.findObject("NormalizationModel")
                   .findGroup("Algorithm")
                   .findKeyword("THRESH");
    if (parMap.contains("ALBEDO")) {
      toNormPvl.findObject("NormalizationModel")
          .findGroup("Algorithm")
          .addKeyword(
              PvlKeyword("ALBEDO", toString(toDouble(parMap["ALBEDO"]))),
              Pvl::Replace);
    } else if (ui.WasEntered("ALBEDO")) {
      QString keyval = ui.GetString("ALBEDO");
      double albedo = toDouble(keyval);
      toNormPvl.findObject("NormalizationModel")
          .findGroup("Algorithm")
          .addKeyword(PvlKeyword("ALBEDO", toString(albedo)), Pvl::Replace);
    } else {
      if (!toNormPvl.findObject("NormalizationModel")
               .findGroup("Algorithm")
               .hasKeyword("ALBEDO")) {
        QString message =
            "The " + normName +
            " Normalization model requires a value for the ALBEDO parameter.";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }
    normLog += toNormPvl.findObject("NormalizationModel")
                   .findGroup("Algorithm")
                   .findKeyword("ALBEDO");
  } else if (normName == "ALBEDOATM") {
    if (parMap.contains("INCREF")) {
      toNormPvl.findObject("NormalizationModel")
          .findGroup("Algorithm")
          .addKeyword(
              PvlKeyword("INCREF", toString(toDouble(parMap["INCREF"]))),
              Pvl::Replace);
    } else if (ui.WasEntered("INCREF")) {
      QString keyval = ui.GetString("INCREF");
      double incref = toDouble(keyval);
      toNormPvl.findObject("NormalizationModel")
          .findGroup("Algorithm")
          .addKeyword(PvlKeyword("INCREF", toString(incref)), Pvl::Replace);
    } else {
      if (!toNormPvl.findObject("NormalizationModel")
               .findGroup("Algorithm")
               .hasKeyword("INCREF")) {
        QString message =
            "The " + normName +
            " Normalization model requires a value for the INCREF parameter.";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }
    normLog += toNormPvl.findObject("NormalizationModel")
                   .findGroup("Algorithm")
                   .findKeyword("INCREF");
  } else if (normName == "SHADEATM") {
    if (parMap.contains("INCREF")) {
      toNormPvl.findObject("NormalizationModel")
          .findGroup("Algorithm")
          .addKeyword(
              PvlKeyword("INCREF", toString(toDouble(parMap["INCREF"]))),
              Pvl::Replace);
    } else if (ui.WasEntered("INCREF")) {
      QString keyval = ui.GetString("INCREF");
      double incref = toDouble(keyval);
      toNormPvl.findObject("NormalizationModel")
          .findGroup("Algorithm")
          .addKeyword(PvlKeyword("INCREF", toString(incref)), Pvl::Replace);
    } else {
      if (!toNormPvl.findObject("NormalizationModel")
               .findGroup("Algorithm")
               .hasKeyword("INCREF")) {
        QString message =
            "The " + normName +
            " Normalization model requires a value for the INCREF parameter.";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }
    normLog += toNormPvl.findObject("NormalizationModel")
                   .findGroup("Algorithm")
                   .findKeyword("INCREF");
    if (parMap.contains("ALBEDO")) {
      toNormPvl.findObject("NormalizationModel")
          .findGroup("Algorithm")
          .addKeyword(
              PvlKeyword("ALBEDO", toString(toDouble(parMap["ALBEDO"]))),
              Pvl::Replace);
    } else if (ui.WasEntered("ALBEDO")) {
      QString keyval = ui.GetString("ALBEDO");
      double albedo = toDouble(keyval);
      toNormPvl.findObject("NormalizationModel")
          .findGroup("Algorithm")
          .addKeyword(PvlKeyword("ALBEDO", toString(albedo)), Pvl::Replace);
    } else {
      if (!toNormPvl.findObject("NormalizationModel")
               .findGroup("Algorithm")
               .hasKeyword("ALBEDO")) {
        QString message =
            "The " + normName +
            " Normalization model requires a value for the ALBEDO parameter.";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }
    normLog += toNormPvl.findObject("NormalizationModel")
                   .findGroup("Algorithm")
                   .findKeyword("ALBEDO");
  } else if (normName == "TOPOATM") {
    if (parMap.contains("INCREF")) {
      toNormPvl.findObject("NormalizationModel")
          .findGroup("Algorithm")
          .addKeyword(
              PvlKeyword("INCREF", toString(toDouble(parMap["INCREF"]))),
              Pvl::Replace);
    } else if (ui.WasEntered("INCREF")) {
      QString keyval = ui.GetString("INCREF");
      double incref = toDouble(keyval);
      toNormPvl.findObject("NormalizationModel")
          .findGroup("Algorithm")
          .addKeyword(PvlKeyword("INCREF", toString(incref)), Pvl::Replace);
    } else {
      if (!toNormPvl.findObject("NormalizationModel")
               .findGroup("Algorithm")
               .hasKeyword("INCREF")) {
        QString message =
            "The " + normName +
            " Normalization model requires a value for the INCREF parameter.";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }
    normLog += toNormPvl.findObject("NormalizationModel")
                   .findGroup("Algorithm")
                   .findKeyword("INCREF");
    if (parMap.contains("ALBEDO")) {
      toNormPvl.findObject("NormalizationModel")
          .findGroup("Algorithm")
          .addKeyword(
              PvlKeyword("ALBEDO", toString(toDouble(parMap["ALBEDO"]))),
              Pvl::Replace);
    } else if (ui.WasEntered("ALBEDO")) {
      QString keyval = ui.GetString("ALBEDO");
      double albedo = toDouble(keyval);
      toNormPvl.findObject("NormalizationModel")
          .findGroup("Algorithm")
          .addKeyword(PvlKeyword("ALBEDO", toString(albedo)), Pvl::Replace);
    } else {
      if (!toNormPvl.findObject("NormalizationModel")
               .findGroup("Algorithm")
               .hasKeyword("ALBEDO")) {
        QString message =
            "The " + normName +
            " Normalization model requires a value for the ALBEDO parameter.";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }
    normLog += toNormPvl.findObject("NormalizationModel")
                   .findGroup("Algorithm")
                   .findKeyword("ALBEDO");
  }
  appLog->addLogGroup(normLog);

  Pvl toAtmPvl;
  PvlGroup atmLog("AtmosphericModelParametersUsed");
  QString atmName = ui.GetAsString("ATMNAME");
  atmName = atmName.toUpper();
  // Check to make sure that an atmospheric model was specified (if the
  // normalization model requires it)
  if (normName == "ALBEDOATM" || normName == "SHADEATM" ||
      normName == "TOPOATM") {
    wasFound = false;
    if (ui.WasEntered("FROMPVL")) {
      QString atmVal;
      Pvl fromAtmPvl;
      PvlObject fromAtmObj;
      PvlGroup fromAtmGrp;
      QString input = ui.GetFileName("FROMPVL");
      fromAtmPvl.read(input);
      if (fromAtmPvl.hasObject("AtmosphericModel")) {
        fromAtmObj = fromAtmPvl.findObject("AtmosphericModel");
        if (fromAtmObj.hasGroup("Algorithm")) {
          PvlObject::PvlGroupIterator fromAtmGrp = fromAtmObj.beginGroup();
          if (fromAtmGrp->hasKeyword("ATMNAME")) {
            atmVal = (QString)fromAtmGrp->findKeyword("ATMNAME");
          } else if (fromAtmGrp->hasKeyword("NAME")) {
            atmVal = (QString)fromAtmGrp->findKeyword("NAME");
          } else {
            atmVal = "NONE";
          }
          atmVal = atmVal.toUpper();
          if (atmName == atmVal && atmVal != "NONE") {
            wasFound = true;
          }
          if ((atmName == "NONE" || atmName == "FROMPVL") && atmVal != "NONE" &&
              !wasFound) {
            atmName = atmVal;
            wasFound = true;
          }
          if (!wasFound) {
            while (fromAtmGrp != fromAtmObj.endGroup()) {
              if (fromAtmGrp->hasKeyword("ATMNAME") ||
                  fromAtmGrp->hasKeyword("NAME")) {
                if (fromAtmGrp->hasKeyword("ATMNAME")) {
                  atmVal = (QString)fromAtmGrp->findKeyword("ATMNAME");
                } else if (fromAtmGrp->hasKeyword("NAME")) {
                  atmVal = (QString)fromAtmGrp->findKeyword("NAME");
                } else {
                  atmVal = "NONE";
                }
                atmVal = atmVal.toUpper();
                if (atmName == atmVal && atmVal != "NONE") {
                  wasFound = true;
                  break;
                }
                if ((atmName == "NONE" || atmName == "FROMPVL") &&
                    atmVal != "NONE" && !wasFound) {
                  atmName = atmVal;
                  wasFound = true;
                  break;
                }
              }
              fromAtmGrp++;
            }
          }
        }
      }
      if (atmName == "NONE" || atmName == "FROMPVL") {
        QString message =
            "An Atmospheric model must be specified when doing normalization "
            "with atmosphere.";
        message +=
            "You need to provide an Atmospheric model through an input PVL "
            "(FROMPVL) or ";
        message +=
            "you need to specify an Atmospheric model through the program "
            "interface.";
        throw IException(IException::User, message, _FILEINFO_);
      }
      if (wasFound) {
        toAtmPvl.addObject(fromAtmObj);
      } else {
        toAtmPvl.addObject(PvlObject("AtmosphericModel"));
        toAtmPvl.findObject("AtmosphericModel").addGroup(PvlGroup("Algorithm"));
        toAtmPvl.findObject("AtmosphericModel")
            .findGroup("Algorithm")
            .addKeyword(PvlKeyword("ATMNAME", atmName), Pvl::Replace);
      }
    } else {
      if (atmName == "NONE" || atmName == "FROMPVL") {
        QString message =
            "An Atmospheric model must be specified when doing normalization "
            "with atmosphere.";
        message +=
            "You need to provide an Atmospheric model through an input PVL "
            "(FROMPVL) or ";
        message +=
            "you need to specify an Atmospheric model through the program "
            "interface.";
        throw IException(IException::User, message, _FILEINFO_);
      }
      toAtmPvl.addObject(PvlObject("AtmosphericModel"));
      toAtmPvl.findObject("AtmosphericModel").addGroup(PvlGroup("Algorithm"));
      toAtmPvl.findObject("AtmosphericModel")
          .findGroup("Algorithm")
          .addKeyword(PvlKeyword("ATMNAME", atmName), Pvl::Replace);
    }
    atmLog += PvlKeyword("ATMNAME", atmName);

    if (atmName == "ANISOTROPIC1" || atmName == "ANISOTROPIC2" ||
        atmName == "HAPKEATM1" || atmName == "HAPKEATM2" ||
        atmName == "ISOTROPIC1" || atmName == "ISOTROPIC2") {
      if (parMap.contains("HNORM")) {
        toAtmPvl.findObject("AtmosphericModel")
            .findGroup("Algorithm")
            .addKeyword(
                PvlKeyword("HNORM", toString(toDouble(parMap["HNORM"]))),
                Pvl::Replace);
      } else if (ui.WasEntered("HNORM")) {
        QString keyval = ui.GetString("HNORM");
        double hnorm = toDouble(keyval);
        toAtmPvl.findObject("AtmosphericModel")
            .findGroup("Algorithm")
            .addKeyword(PvlKeyword("HNORM", toString(hnorm)), Pvl::Replace);
      } else {
        if (!toAtmPvl.findObject("AtmosphericModel")
                 .findGroup("Algorithm")
                 .hasKeyword("HNORM")) {
          QString message =
              "The " + atmName +
              " Atmospheric model requires a value for the HNORM parameter.";
          message += "The normal range for HNORM is: 0 <= HNORM";
          throw IException(IException::User, message, _FILEINFO_);
        }
      }
      atmLog += toAtmPvl.findObject("AtmosphericModel")
                    .findGroup("Algorithm")
                    .findKeyword("HNORM");
      if (parMap.contains("TAU")) {
        toAtmPvl.findObject("AtmosphericModel")
            .findGroup("Algorithm")
            .addKeyword(PvlKeyword("TAU", toString(toDouble(parMap["TAU"]))),
                        Pvl::Replace);
      } else if (ui.WasEntered("TAU")) {
        QString keyval = ui.GetString("TAU");
        double tau = toDouble(keyval);
        toAtmPvl.findObject("AtmosphericModel")
            .findGroup("Algorithm")
            .addKeyword(PvlKeyword("TAU", toString(tau)), Pvl::Replace);
      } else {
        if (!toAtmPvl.findObject("AtmosphericModel")
                 .findGroup("Algorithm")
                 .hasKeyword("TAU")) {
          QString message =
              "The " + atmName +
              " Atmospheric model requires a value for the TAU parameter.";
          message += "The normal range for TAU is: 0 <= TAU";
          throw IException(IException::User, message, _FILEINFO_);
        }
      }
      atmLog += toAtmPvl.findObject("AtmosphericModel")
                    .findGroup("Algorithm")
                    .findKeyword("TAU");
      if (parMap.contains("TAUREF")) {
        toAtmPvl.findObject("AtmosphericModel")
            .findGroup("Algorithm")
            .addKeyword(
                PvlKeyword("TAUREF", toString(toDouble(parMap["TAUREF"]))),
                Pvl::Replace);
      } else if (ui.WasEntered("TAUREF")) {
        QString keyval = ui.GetString("TAUREF");
        double tauref = toDouble(keyval);
        toAtmPvl.findObject("AtmosphericModel")
            .findGroup("Algorithm")
            .addKeyword(PvlKeyword("TAUREF", toString(tauref)), Pvl::Replace);
      } else {
        if (!toAtmPvl.findObject("AtmosphericModel")
                 .findGroup("Algorithm")
                 .hasKeyword("TAUREF")) {
          QString message =
              "The " + atmName +
              " Atmospheric model requires a value for the TAUREF parameter.";
          message += "The normal range for TAUREF is: 0 <= TAUREF";
          throw IException(IException::User, message, _FILEINFO_);
        }
      }
      atmLog += toAtmPvl.findObject("AtmosphericModel")
                    .findGroup("Algorithm")
                    .findKeyword("TAUREF");
      if (parMap.contains("WHA")) {
        toAtmPvl.findObject("AtmosphericModel")
            .findGroup("Algorithm")
            .addKeyword(PvlKeyword("WHA", toString(toDouble(parMap["WHA"]))),
                        Pvl::Replace);
      } else if (ui.WasEntered("WHA")) {
        QString keyval = ui.GetString("WHA");
        double wha = toDouble(keyval);
        toAtmPvl.findObject("AtmosphericModel")
            .findGroup("Algorithm")
            .addKeyword(PvlKeyword("WHA", toString(wha)), Pvl::Replace);
      } else {
        if (!toAtmPvl.findObject("AtmosphericModel")
                 .findGroup("Algorithm")
                 .hasKeyword("WHA")) {
          QString message =
              "The " + atmName +
              " Atmospheric model requires a value for the WHA parameter.";
          message += "The normal range for WHA is: 0 < WHA < 1";
          throw IException(IException::User, message, _FILEINFO_);
        }
      }
      atmLog += toAtmPvl.findObject("AtmosphericModel")
                    .findGroup("Algorithm")
                    .findKeyword("WHA");
      if (parMap.contains("NULNEG")) {
        if (parMap["NULNEG"].toStdString() == "YES") {
          toAtmPvl.findObject("AtmosphericModel")
              .findGroup("Algorithm")
              .addKeyword(PvlKeyword("NULNEG", "YES"), Pvl::Replace);
        } else if (parMap["NULNEG"].toStdString() == "NO") {
          toAtmPvl.findObject("AtmosphericModel")
              .findGroup("Algorithm")
              .addKeyword(PvlKeyword("NULNEG", "NO"), Pvl::Replace);
        } else {
          QString message =
              "The " + atmName +
              " Atmospheric model requires a value for the NULNEG parameter.";
          message += "The valid values for NULNEG are: YES, NO";
          throw IException(IException::User, message, _FILEINFO_);
        }
      } else if (!toAtmPvl.findObject("AtmosphericModel")
                      .findGroup("Algorithm")
                      .hasKeyword("NULNEG")) {
        if (ui.GetString("NULNEG") == "YES") {
          toAtmPvl.findObject("AtmosphericModel")
              .findGroup("Algorithm")
              .addKeyword(PvlKeyword("NULNEG", "YES"), Pvl::Replace);
        } else if (ui.GetString("NULNEG") == "NO") {
          toAtmPvl.findObject("AtmosphericModel")
              .findGroup("Algorithm")
              .addKeyword(PvlKeyword("NULNEG", "NO"), Pvl::Replace);
        } else {
          QString message =
              "The " + atmName +
              " Atmospheric model requires a value for the NULNEG parameter.";
          message += "The valid values for NULNEG are: YES, NO";
          throw IException(IException::User, message, _FILEINFO_);
        }
      }
      atmLog += toAtmPvl.findObject("AtmosphericModel")
                    .findGroup("Algorithm")
                    .findKeyword("NULNEG");
    }

    if (atmName == "ANISOTROPIC1" || atmName == "ANISOTROPIC2") {
      if (parMap.contains("BHA")) {
        toAtmPvl.findObject("AtmosphericModel")
            .findGroup("Algorithm")
            .addKeyword(PvlKeyword("BHA", toString(toDouble(parMap["BHA"]))),
                        Pvl::Replace);
      } else if (ui.WasEntered("BHA")) {
        QString keyval = ui.GetString("BHA");
        double bha = toDouble(keyval);
        toAtmPvl.findObject("AtmosphericModel")
            .findGroup("Algorithm")
            .addKeyword(PvlKeyword("BHA", toString(bha)), Pvl::Replace);
      } else {
        if (!toAtmPvl.findObject("AtmosphericModel")
                 .findGroup("Algorithm")
                 .hasKeyword("BHA")) {
          QString message =
              "The " + atmName +
              " Atmospheric model requires a value for the BHA parameter.";
          message += "The normal range for BHA is: -1 <= BHA <= 1";
          throw IException(IException::User, message, _FILEINFO_);
        }
      }
      atmLog += toAtmPvl.findObject("AtmosphericModel")
                    .findGroup("Algorithm")
                    .findKeyword("BHA");
    }
    if (atmName == "HAPKEATM1" || atmName == "HAPKEATM2") {
      if (parMap.contains("HGA")) {
        toAtmPvl.findObject("AtmosphericModel")
            .findGroup("Algorithm")
            .addKeyword(PvlKeyword("HGA", toString(toDouble(parMap["HGA"]))),
                        Pvl::Replace);
      } else if (ui.WasEntered("HGA")) {
        QString keyval = ui.GetString("HGA");
        double hga = toDouble(keyval);
        toAtmPvl.findObject("AtmosphericModel")
            .findGroup("Algorithm")
            .addKeyword(PvlKeyword("HGA", toString(hga)), Pvl::Replace);
      } else {
        if (!toAtmPvl.findObject("AtmosphericModel")
                 .findGroup("Algorithm")
                 .hasKeyword("HGA")) {
          QString message =
              "The " + atmName +
              " Atmospheric model requires a value for the HGA parameter.";
          message += "The normal range for HGA is: -1 < HGA < 1";
          throw IException(IException::User, message, _FILEINFO_);
        }
      }
      atmLog += toAtmPvl.findObject("AtmosphericModel")
                    .findGroup("Algorithm")
                    .findKeyword("HGA");
    }
  }
  appLog->addLogGroup(atmLog);

  Pvl toPhtPvl;
  PvlGroup phtLog("PhotometricModelParametersUsed");
  QString phtName = ui.GetAsString("PHTNAME");
  phtName = phtName.toUpper();
  wasFound = false;
  if (ui.WasEntered("FROMPVL")) {
    QString phtVal;
    Pvl fromPhtPvl;
    PvlObject fromPhtObj;
    PvlGroup fromPhtGrp;
    QString input = ui.GetFileName("FROMPVL");
    fromPhtPvl.read(input);
    if (fromPhtPvl.hasObject("PhotometricModel")) {
      fromPhtObj = fromPhtPvl.findObject("PhotometricModel");
      if (fromPhtObj.hasGroup("Algorithm")) {
        PvlObject::PvlGroupIterator fromPhtGrp = fromPhtObj.beginGroup();
        if (fromPhtGrp->hasKeyword("PHTNAME")) {
          phtVal = (QString)fromPhtGrp->findKeyword("PHTNAME");
        } else if (fromPhtGrp->hasKeyword("NAME")) {
          phtVal = (QString)fromPhtGrp->findKeyword("NAME");
        } else {
          phtVal = "NONE";
        }
        phtVal = phtVal.toUpper();
        if (phtName == phtVal && phtVal != "NONE") {
          wasFound = true;
        }
        if ((phtName == "NONE" || phtName == "FROMPVL") && phtVal != "NONE" &&
            !wasFound) {
          phtName = phtVal;
          wasFound = true;
        }
        if (!wasFound) {
          while (fromPhtGrp != fromPhtObj.endGroup()) {
            if (fromPhtGrp->hasKeyword("PHTNAME") ||
                fromPhtGrp->hasKeyword("NAME")) {
              if (fromPhtGrp->hasKeyword("PHTNAME")) {
                phtVal = (QString)fromPhtGrp->findKeyword("PHTNAME");
              } else if (fromPhtGrp->hasKeyword("NAME")) {
                phtVal = (QString)fromPhtGrp->findKeyword("NAME");
              } else {
                phtVal = "NONE";
              }
              phtVal = phtVal.toUpper();
              if (phtName == phtVal && phtVal != "NONE") {
                wasFound = true;
                break;
              }
              if ((phtName == "NONE" || phtName == "FROMPVL") &&
                  phtVal != "NONE" && !wasFound) {
                phtName = phtVal;
                wasFound = true;
                break;
              }
            }
            fromPhtGrp++;
          }
        }
      }
    }
    // Check to make sure that a photometric model was specified
    if (phtName == "NONE" || phtName == "FROMPVL") {
      QString message =
          "A Photometric model must be specified before running this program.";
      message +=
          "You need to provide a Photometric model through an input PVL "
          "(FROMPVL) or ";
      message +=
          "you need to specify a Photometric model through the program "
          "interface.";
      throw IException(IException::User, message, _FILEINFO_);
    }
    if (wasFound) {
      toPhtPvl.addObject(fromPhtObj);
    } else {
      toPhtPvl.addObject(PvlObject("PhotometricModel"));
      toPhtPvl.findObject("PhotometricModel").addGroup(PvlGroup("Algorithm"));
      toPhtPvl.findObject("PhotometricModel")
          .findGroup("Algorithm")
          .addKeyword(PvlKeyword("PHTNAME", phtName), Pvl::Replace);
    }
  } else {
    // Check to make sure that a photometric model was specified
    if (phtName == "NONE" || phtName == "FROMPVL") {
      QString message =
          "A Photometric model must be specified before running this program.";
      message +=
          "You need to provide a Photometric model through an input PVL "
          "(FROMPVL) or ";
      message +=
          "you need to specify a Photometric model through the program "
          "interface.";
      throw IException(IException::User, message, _FILEINFO_);
    }
    toPhtPvl.addObject(PvlObject("PhotometricModel"));
    toPhtPvl.findObject("PhotometricModel").addGroup(PvlGroup("Algorithm"));
    toPhtPvl.findObject("PhotometricModel")
        .findGroup("Algorithm")
        .addKeyword(PvlKeyword("PHTNAME", phtName), Pvl::Replace);
  }
  phtLog += PvlKeyword("PHTNAME", phtName);

  if (phtName == "HAPKEHEN" || phtName == "HAPKELEG") {
    if (parMap.contains("THETA")) {
      toPhtPvl.findObject("PhotometricModel")
          .findGroup("Algorithm")
          .addKeyword(PvlKeyword("THETA", toString(toDouble(parMap["THETA"]))),
                      Pvl::Replace);
    } else if (ui.WasEntered("THETA")) {
      QString keyval = ui.GetString("THETA");
      double theta = toDouble(keyval);
      toPhtPvl.findObject("PhotometricModel")
          .findGroup("Algorithm")
          .addKeyword(PvlKeyword("THETA", toString(theta)), Pvl::Replace);
    } else {
      if (!toPhtPvl.findObject("PhotometricModel")
               .findGroup("Algorithm")
               .hasKeyword("THETA")) {
        QString message =
            "The " + phtName +
            " Photometric model requires a value for the THETA parameter.";
        message += "The normal range for THETA is: 0 <= THETA <= 90";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }
    phtLog += toPhtPvl.findObject("PhotometricModel")
                  .findGroup("Algorithm")
                  .findKeyword("THETA");
    if (parMap.contains("WH")) {
      toPhtPvl.findObject("PhotometricModel")
          .findGroup("Algorithm")
          .addKeyword(PvlKeyword("WH", toString(toDouble(parMap["WH"]))),
                      Pvl::Replace);
    } else if (ui.WasEntered("WH")) {
      QString keyval = ui.GetString("WH");
      double wh = toDouble(keyval);
      toPhtPvl.findObject("PhotometricModel")
          .findGroup("Algorithm")
          .addKeyword(PvlKeyword("WH", toString(wh)), Pvl::Replace);
    } else {
      if (!toPhtPvl.findObject("PhotometricModel")
               .findGroup("Algorithm")
               .hasKeyword("WH")) {
        QString message =
            "The " + phtName +
            " Photometric model requires a value for the WH parameter.";
        message += "The normal range for WH is: 0 < WH <= 1";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }
    phtLog += toPhtPvl.findObject("PhotometricModel")
                  .findGroup("Algorithm")
                  .findKeyword("WH");
    if (parMap.contains("HH")) {
      toPhtPvl.findObject("PhotometricModel")
          .findGroup("Algorithm")
          .addKeyword(PvlKeyword("HH", toString(toDouble(parMap["HH"]))),
                      Pvl::Replace);
    } else if (ui.WasEntered("HH")) {
      QString keyval = ui.GetString("HH");
      double hh = toDouble(keyval);
      toPhtPvl.findObject("PhotometricModel")
          .findGroup("Algorithm")
          .addKeyword(PvlKeyword("HH", toString(hh)), Pvl::Replace);
    } else {
      if (!toPhtPvl.findObject("PhotometricModel")
               .findGroup("Algorithm")
               .hasKeyword("HH")) {
        QString message =
            "The " + phtName +
            " Photometric model requires a value for the HH parameter.";
        message += "The normal range for HH is: 0 <= HH";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }
    phtLog += toPhtPvl.findObject("PhotometricModel")
                  .findGroup("Algorithm")
                  .findKeyword("HH");
    if (parMap.contains("B0")) {
      toPhtPvl.findObject("PhotometricModel")
          .findGroup("Algorithm")
          .addKeyword(PvlKeyword("B0", toString(toDouble(parMap["B0"]))),
                      Pvl::Replace);
    } else if (ui.WasEntered("B0")) {
      QString keyval = ui.GetString("B0");
      double b0 = toDouble(keyval);
      toPhtPvl.findObject("PhotometricModel")
          .findGroup("Algorithm")
          .addKeyword(PvlKeyword("B0", toString(b0)), Pvl::Replace);
    } else {
      if (!toPhtPvl.findObject("PhotometricModel")
               .findGroup("Algorithm")
               .hasKeyword("B0")) {
        QString message =
            "The " + phtName +
            " Photometric model requires a value for the B0 parameter.";
        message += "The normal range for B0 is: 0 <= B0";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }
    phtLog += toPhtPvl.findObject("PhotometricModel")
                  .findGroup("Algorithm")
                  .findKeyword("B0");
    if (parMap.contains("ZEROB0STANDARD")) {
      if (parMap["ZEROB0STANDARD"].toStdString() == "TRUE") {
        toPhtPvl.findObject("PhotometricModel")
            .findGroup("Algorithm")
            .addKeyword(PvlKeyword("ZEROB0STANDARD", "TRUE"), Pvl::Replace);
      } else if (parMap["ZEROB0STANDARD"].toStdString() == "FALSE") {
        toPhtPvl.findObject("PhotometricModel")
            .findGroup("Algorithm")
            .addKeyword(PvlKeyword("ZEROB0STANDARD", "FALSE"), Pvl::Replace);
      } else {
        QString message = "The " + phtName +
                          " Photometric model requires a value for the "
                          "ZEROB0STANDARD parameter.";
        message += "The valid values for ZEROB0STANDARD are: TRUE, FALSE";
        throw IException(IException::User, message, _FILEINFO_);
      }
    } else if (ui.GetString("ZEROB0STANDARD") != "READFROMPVL") {
      if (ui.GetString("ZEROB0STANDARD") == "TRUE") {
        toPhtPvl.findObject("PhotometricModel")
            .findGroup("Algorithm")
            .addKeyword(PvlKeyword("ZEROB0STANDARD", "TRUE"), Pvl::Replace);
      } else if (ui.GetString("ZEROB0STANDARD") == "FALSE") {
        toPhtPvl.findObject("PhotometricModel")
            .findGroup("Algorithm")
            .addKeyword(PvlKeyword("ZEROB0STANDARD", "FALSE"), Pvl::Replace);
      }
    } else if (!toPhtPvl.findObject("PhotometricModel")
                    .findGroup("Algorithm")
                    .hasKeyword("ZEROB0STANDARD")) {
      toPhtPvl.findObject("PhotometricModel")
          .findGroup("Algorithm")
          .addKeyword(PvlKeyword("ZEROB0STANDARD", "TRUE"), Pvl::Replace);
    }
    QString zerob0 = (QString)toPhtPvl.findObject("PhotometricModel")
                         .findGroup("Algorithm")
                         .findKeyword("ZEROB0STANDARD");
    QString izerob0 = zerob0;
    izerob0 = izerob0.toUpper();
    if (izerob0 != "TRUE" && izerob0 != "FALSE") {
      QString message = "The " + phtName +
                        " Photometric model requires a value for the "
                        "ZEROB0STANDARD parameter.";
      message += "The valid values for ZEROB0STANDARD are: TRUE, FALSE";
      throw IException(IException::User, message, _FILEINFO_);
    }
    phtLog += toPhtPvl.findObject("PhotometricModel")
                  .findGroup("Algorithm")
                  .findKeyword("ZEROB0STANDARD");
    if (phtName == "HAPKEHEN") {
      if (parMap.contains("HG1")) {
        toPhtPvl.findObject("PhotometricModel")
            .findGroup("Algorithm")
            .addKeyword(PvlKeyword("HG1", toString(toDouble(parMap["HG1"]))),
                        Pvl::Replace);
      } else if (ui.WasEntered("HG1")) {
        QString keyval = ui.GetString("HG1");
        double hg1 = toDouble(keyval);
        toPhtPvl.findObject("PhotometricModel")
            .findGroup("Algorithm")
            .addKeyword(PvlKeyword("HG1", toString(hg1)), Pvl::Replace);
      } else {
        if (!toPhtPvl.findObject("PhotometricModel")
                 .findGroup("Algorithm")
                 .hasKeyword("HG1")) {
          QString message =
              "The " + phtName +
              " Photometric model requires a value for the HG1 parameter.";
          message += "The normal range for HG1 is: -1 < HG1 < 1";
          throw IException(IException::User, message, _FILEINFO_);
        }
      }
      phtLog += toPhtPvl.findObject("PhotometricModel")
                    .findGroup("Algorithm")
                    .findKeyword("HG1");
      if (parMap.contains("HG2")) {
        toPhtPvl.findObject("PhotometricModel")
            .findGroup("Algorithm")
            .addKeyword(PvlKeyword("HG2", toString(toDouble(parMap["HG2"]))),
                        Pvl::Replace);
      } else if (ui.WasEntered("HG2")) {
        QString keyval = ui.GetString("HG2");
        double hg2 = toDouble(keyval);
        toPhtPvl.findObject("PhotometricModel")
            .findGroup("Algorithm")
            .addKeyword(PvlKeyword("HG2", toString(hg2)), Pvl::Replace);
      } else {
        if (!toPhtPvl.findObject("PhotometricModel")
                 .findGroup("Algorithm")
                 .hasKeyword("HG2")) {
          QString message =
              "The " + phtName +
              " Photometric model requires a value for the HG2 parameter.";
          message += "The normal range for HG2 is: 0 <= HG2 <= 1";
          throw IException(IException::User, message, _FILEINFO_);
        }
      }
      phtLog += toPhtPvl.findObject("PhotometricModel")
                    .findGroup("Algorithm")
                    .findKeyword("HG2");
    } else {
      if (parMap.contains("BH")) {
        toPhtPvl.findObject("PhotometricModel")
            .findGroup("Algorithm")
            .addKeyword(PvlKeyword("BH", toString(toDouble(parMap["BH"]))),
                        Pvl::Replace);
      } else if (ui.WasEntered("BH")) {
        QString keyval = ui.GetString("BH");
        double bh = toDouble(keyval);
        toPhtPvl.findObject("PhotometricModel")
            .findGroup("Algorithm")
            .addKeyword(PvlKeyword("BH", toString(bh)), Pvl::Replace);
      } else {
        if (!toPhtPvl.findObject("PhotometricModel")
                 .findGroup("Algorithm")
                 .hasKeyword("BH")) {
          QString message =
              "The " + phtName +
              " Photometric model requires a value for the BH parameter.";
          message += "The normal range for BH is: -1 <= BH <= 1";
          throw IException(IException::User, message, _FILEINFO_);
        }
      }
      phtLog += toPhtPvl.findObject("PhotometricModel")
                    .findGroup("Algorithm")
                    .findKeyword("BH");
      if (parMap.contains("CH")) {
        toPhtPvl.findObject("PhotometricModel")
            .findGroup("Algorithm")
            .addKeyword(PvlKeyword("CH", toString(toDouble(parMap["CH"]))),
                        Pvl::Replace);
      } else if (ui.WasEntered("CH")) {
        QString keyval = ui.GetString("CH");
        double ch = toDouble(keyval);
        toPhtPvl.findObject("PhotometricModel")
            .findGroup("Algorithm")
            .addKeyword(PvlKeyword("CH", toString(ch)), Pvl::Replace);
      } else {
        if (!toPhtPvl.findObject("PhotometricModel")
                 .findGroup("Algorithm")
                 .hasKeyword("CH")) {
          QString message =
              "The " + phtName +
              " Photometric model requires a value for the CH parameter.";
          message += "The normal range for CH is: -1 <= CH <= 1";
          throw IException(IException::User, message, _FILEINFO_);
        }
      }
      phtLog += toPhtPvl.findObject("PhotometricModel")
                    .findGroup("Algorithm")
                    .findKeyword("CH");
    }
  } else if (phtName == "LUNARLAMBERTEMPIRICAL" ||
             phtName == "MINNAERTEMPIRICAL") {
    if (parMap.contains("PHASELIST")) {
      toPhtPvl.findObject("PhotometricModel")
          .findGroup("Algorithm")
          .addKeyword(PvlKeyword("PHASELIST", parMap["PHASELIST"]),
                      Pvl::Replace);
    } else if (ui.WasEntered("PHASELIST")) {
      QString keyval = ui.GetString("PHASELIST");
      toPhtPvl.findObject("PhotometricModel")
          .findGroup("Algorithm")
          .addKeyword(PvlKeyword("PHASELIST", keyval), Pvl::Replace);
    } else {
      if (!toPhtPvl.findObject("PhotometricModel")
               .findGroup("Algorithm")
               .hasKeyword("PHASELIST")) {
        QString message =
            "The " + phtName +
            " Photometric model requires a value for the PHASELIST parameter.";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }
    phtLog += toPhtPvl.findObject("PhotometricModel")
                  .findGroup("Algorithm")
                  .findKeyword("PHASELIST");
    if (parMap.contains("PHASECURVELIST")) {
      toPhtPvl.findObject("PhotometricModel")
          .findGroup("Algorithm")
          .addKeyword(PvlKeyword("PHASECURVELIST", parMap["PHASECURVELIST"]),
                      Pvl::Replace);
    } else if (ui.WasEntered("PHASECURVELIST")) {
      QString keyval = ui.GetString("PHASECURVELIST");
      toPhtPvl.findObject("PhotometricModel")
          .findGroup("Algorithm")
          .addKeyword(PvlKeyword("PHASECURVELIST", keyval), Pvl::Replace);
    } else {
      if (!toPhtPvl.findObject("PhotometricModel")
               .findGroup("Algorithm")
               .hasKeyword("PHASECURVELIST")) {
        QString message = "The " + phtName +
                          " Photometric model requires a value for the "
                          "PHASECURVELIST parameter.";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }
    phtLog += toPhtPvl.findObject("PhotometricModel")
                  .findGroup("Algorithm")
                  .findKeyword("PHASECURVELIST");
    if (phtName == "LUNARLAMBERTEMPIRICAL") {
      if (parMap.contains("LLIST")) {
        toPhtPvl.findObject("PhotometricModel")
            .findGroup("Algorithm")
            .addKeyword(PvlKeyword("LLIST", parMap["LLIST"]), Pvl::Replace);
      } else if (ui.WasEntered("LLIST")) {
        QString keyval = ui.GetString("LLIST");
        toPhtPvl.findObject("PhotometricModel")
            .findGroup("Algorithm")
            .addKeyword(PvlKeyword("LLIST", keyval), Pvl::Replace);
      } else {
        if (!toPhtPvl.findObject("PhotometricModel")
                 .findGroup("Algorithm")
                 .hasKeyword("LLIST")) {
          QString message =
              "The " + phtName +
              " Photometric model requires a value for the LLIST parameter.";
          throw IException(IException::User, message, _FILEINFO_);
        }
      }
      phtLog += toPhtPvl.findObject("PhotometricModel")
                    .findGroup("Algorithm")
                    .findKeyword("LLIST");
    } else {
      if (parMap.contains("KLIST")) {
        toPhtPvl.findObject("PhotometricModel")
            .findGroup("Algorithm")
            .addKeyword(PvlKeyword("KLIST", parMap["KLIST"]), Pvl::Replace);
      } else if (ui.WasEntered("KLIST")) {
        QString keyval = ui.GetString("KLIST");
        toPhtPvl.findObject("PhotometricModel")
            .findGroup("Algorithm")
            .addKeyword(PvlKeyword("KLIST", keyval), Pvl::Replace);
      } else {
        if (!toPhtPvl.findObject("PhotometricModel")
                 .findGroup("Algorithm")
                 .hasKeyword("KLIST")) {
          QString message =
              "The " + phtName +
              " Photometric model requires a value for the KLIST parameter.";
          throw IException(IException::User, message, _FILEINFO_);
        }
      }
      phtLog += toPhtPvl.findObject("PhotometricModel")
                    .findGroup("Algorithm")
                    .findKeyword("KLIST");
    }
  } else if (phtName == "LUNARLAMBERT") {
    if (parMap.contains("L")) {
      toPhtPvl.findObject("PhotometricModel")
          .findGroup("Algorithm")
          .addKeyword(PvlKeyword("L", toString(toDouble(parMap["L"]))),
                      Pvl::Replace);
    } else if (ui.WasEntered("L")) {
      QString keyval = ui.GetString("L");
      double l = toDouble(keyval);
      toPhtPvl.findObject("PhotometricModel")
          .findGroup("Algorithm")
          .addKeyword(PvlKeyword("L", toString(l)), Pvl::Replace);
    } else {
      if (!toPhtPvl.findObject("PhotometricModel")
               .findGroup("Algorithm")
               .hasKeyword("L")) {
        QString message =
            "The " + phtName +
            " Photometric model requires a value for the L parameter.";
        message += "The L parameter has no limited range";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }
    phtLog += toPhtPvl.findObject("PhotometricModel")
                  .findGroup("Algorithm")
                  .findKeyword("L");
  } else if (phtName == "MINNAERT") {
    if (parMap.contains("K")) {
      toPhtPvl.findObject("PhotometricModel")
          .findGroup("Algorithm")
          .addKeyword(PvlKeyword("K", toString(toDouble(parMap["K"]))),
                      Pvl::Replace);
    } else if (ui.WasEntered("K")) {
      QString keyval = ui.GetString("K");
      double k = toDouble(keyval);
      toPhtPvl.findObject("PhotometricModel")
          .findGroup("Algorithm")
          .addKeyword(PvlKeyword("K", toString(k)), Pvl::Replace);
    } else {
      if (!toPhtPvl.findObject("PhotometricModel")
               .findGroup("Algorithm")
               .hasKeyword("K")) {
        QString message =
            "The " + phtName +
            " Photometric model requires a value for the K parameter.";
        message += "The normal range for K is: 0 <= K";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }
    phtLog += toPhtPvl.findObject("PhotometricModel")
                  .findGroup("Algorithm")
                  .findKeyword("K");
  }
  appLog->addLogGroup(phtLog);

  PvlObject normObj = toNormPvl.findObject("NormalizationModel");
  PvlObject phtObj = toPhtPvl.findObject("PhotometricModel");
  PvlObject atmObj;
  if (normName == "ALBEDOATM" || normName == "SHADEATM" ||
      normName == "TOPOATM") {
    atmObj = toAtmPvl.findObject("AtmosphericModel");
  }

  Pvl par;
  par.addObject(normObj);
  par.addObject(phtObj);
  if (normName == "ALBEDOATM" || normName == "SHADEATM" ||
      normName == "TOPOATM") {
    par.addObject(atmObj);
  }

  // Set value for maximum emission/incidence angles chosen by user
  maxema = ui.GetDouble("MAXEMISSION");
  maxinc = ui.GetDouble("MAXINCIDENCE");
  usedem = ui.GetBoolean("USEDEM");

  // determine how photometric angles should be calculated
  angleSource = ui.GetString("ANGLESOURCE");

  if ((normName == "TOPO" || normName == "MIXED") && angleSource == "DEM") {
    QString message = "The " + normName +
                      " Normalized model is not recommended for use with the " +
                      angleSource + " Angle Source option";
    PvlGroup warning("Warnings");
    warning.addKeyword(PvlKeyword("Warning", message));
    appLog->addLogGroup(warning);
  }
  // Get camera information if needed
  if (angleSource == "ELLIPSOID" || angleSource == "DEM" ||
      angleSource == "CENTER_FROM_IMAGE") {
    // Set up the input cube
    p.SetInputCube(icube);
    cam = icube->camera();
  } else {
    p.SetInputCube(icube);
  }

    // Create the output cube
    CubeAttributeOutput &att = ui.GetOutputAttribute("TO");
    p.SetOutputCube(ui.GetCubeName("TO"), att);

  Pvl inLabel;
  inLabel.read(ui.GetCubeName("FROM"));

  // If the source of photometric angles is the center of the image,
  // then get the angles at the center of the image.
  if (angleSource == "CENTER_FROM_IMAGE") {
    cam->SetImage(cam->Samples() / 2, cam->Lines() / 2);
    centerPhase = cam->PhaseAngle();
    centerIncidence = cam->IncidenceAngle();
    centerEmission = cam->EmissionAngle();
  } else if (angleSource == "CENTER_FROM_LABEL") {
    centerPhase = inLabel.findKeyword("PhaseAngle", Pvl::Traverse);
    centerIncidence = inLabel.findKeyword("IncidenceAngle", Pvl::Traverse);
    centerEmission = inLabel.findKeyword("EmissionAngle", Pvl::Traverse);
  } else if (angleSource == "CENTER_FROM_USER") {
    centerPhase = ui.GetDouble("PHASE_ANGLE");
    centerIncidence = ui.GetDouble("INCIDENCE_ANGLE");
    centerEmission = ui.GetDouble("EMISSION_ANGLE");
  } else if (angleSource == "BACKPLANE") {
    useBackplane = true;
    CubeAttributeInput cai;
    CubeAttributeInput phaseCai;
    CubeAttributeInput incidenceCai;
    CubeAttributeInput emissionCai;
    if (ui.WasEntered("PHASE_ANGLE_FILE")) {
      phaseCai = ui.GetInputAttribute("PHASE_ANGLE_FILE");
      p.SetInputCube(ui.GetCubeName("PHASE_ANGLE_FILE"), phaseCai);
      usePhasefile = true;
    } else {
      phaseAngle = ui.GetDouble("PHASE_ANGLE");
    }
    if (ui.WasEntered("INCIDENCE_ANGLE_FILE")) {
      incidenceCai = ui.GetInputAttribute("INCIDENCE_ANGLE_FILE");
      p.SetInputCube(ui.GetCubeName("INCIDENCE_ANGLE_FILE"), incidenceCai);
      useIncidencefile = true;
    } else {
      incidenceAngle = ui.GetDouble("INCIDENCE_ANGLE");
    }
    if (ui.WasEntered("EMISSION_ANGLE_FILE")) {
      emissionCai = ui.GetInputAttribute("EMISSION_ANGLE_FILE");
      p.SetInputCube(ui.GetCubeName("EMISSION_ANGLE_FILE"), emissionCai);
      useEmissionfile = true;
    } else {
      emissionAngle = ui.GetDouble("EMISSION_ANGLE");
    }
  }

  // Get the BandBin Center from the image
  PvlGroup pvlg = inLabel.findGroup("BandBin", Pvl::Traverse);
  double wl;
  if (pvlg.hasKeyword("Center")) {
    PvlKeyword &wavelength = pvlg.findKeyword("Center");
    wl = toDouble(wavelength[0]);
  } else {
    wl = 1.0;
  }

  // Create the photometry object and set the wavelength
  PvlGroup &algo = par.findObject("NormalizationModel")
                       .findGroup("Algorithm", Pvl::Traverse);
  if (!algo.hasKeyword("Wl")) {
    algo.addKeyword(Isis::PvlKeyword("Wl", toString(wl)));
  }
  pho = new Photometry(par);
  pho->SetPhotomWl(wl);

  auto photometWithBackplane = [&](std::vector<Isis::Buffer *> &in,
                                   std::vector<Isis::Buffer *> &out) -> void {
    Buffer &image = *in[0];
    int index = 1;
    Buffer &phasebp = *in[1];
    if (usePhasefile) {
      index = index + 1;
    }
    Buffer &incidencebp = *in[index];
    if (useIncidencefile) {
      index = index + 1;
    }
    Buffer &emissionbp = *in[index];

    Buffer &outimage = *out[0];

    double deminc = 0., demema = 0., mult = 0., base = 0.;
    double ellipsoidpha = 0., ellipsoidinc = 0., ellipsoidema = 0.;

    for (int i = 0; i < image.size(); i++) {
      // if special pixel, copy to output
      if (!IsValidPixel(image[i])) {
        outimage[i] = image[i];
      }

      // if off the target, set to null
      else if ((angleSource == "ELLIPSOID" || angleSource == "DEM" ||
                angleSource == "CENTER_FROM_IMAGE") &&
               (!cam->SetImage(image.Sample(i), image.Line(i)))) {
        outimage[i] = NULL8;
      }

      // otherwise, compute angle values
      else {
        if (usePhasefile) {
          ellipsoidpha = phasebp[i];
        } else {
          ellipsoidpha = phaseAngle;
        }
        if (useIncidencefile) {
          ellipsoidinc = incidencebp[i];
        } else {
          ellipsoidinc = incidenceAngle;
        }
        if (useEmissionfile) {
          ellipsoidema = emissionbp[i];
        } else {
          ellipsoidema = emissionAngle;
        }
        deminc = ellipsoidinc;
        demema = ellipsoidema;

        // if invalid angles, set to null
        if (!IsValidPixel(ellipsoidpha) || !IsValidPixel(ellipsoidinc) ||
            !IsValidPixel(ellipsoidema)) {
          outimage[i] = NULL8;
        } else if (deminc >= 90.0 || demema >= 90.0) {
          outimage[i] = NULL8;
        }
        // if angles greater than max allowed by user, set to null
        else if (deminc > maxinc || demema > maxema) {
          outimage[i] = NULL8;
        }
        // otherwise, do photometric correction
        else {
          pho->Compute(ellipsoidpha, ellipsoidinc, ellipsoidema, deminc, demema,
                       image[i], outimage[i], mult, base);
        }
      }
    }
  };

  auto photomet = [&](Buffer &in, Buffer &out) -> void {
    double deminc = 0., demema = 0., mult = 0., base = 0.;
    double ellipsoidpha = 0., ellipsoidinc = 0., ellipsoidema = 0.;

    for (int i = 0; i < in.size(); i++) {
      // if special pixel, copy to output
      if (!IsValidPixel(in[i])) {
        out[i] = in[i];
      }

      // if off the target, set to null
      else if ((angleSource == "ELLIPSOID" || angleSource == "DEM" ||
                angleSource == "CENTER_FROM_IMAGE") &&
               (!cam->SetImage(in.Sample(i), in.Line(i)))) {
        out[i] = NULL8;
      }

      // otherwise, compute angle values
      else {
        bool success = true;
        if (angleSource == "CENTER_FROM_IMAGE" ||
            angleSource == "CENTER_FROM_LABEL" ||
            angleSource == "CENTER_FROM_USER") {
          ellipsoidpha = centerPhase;
          ellipsoidinc = centerIncidence;
          ellipsoidema = centerEmission;
          deminc = centerIncidence;
          demema = centerEmission;
        } else {
          // calculate photometric angles
          ellipsoidpha = cam->PhaseAngle();
          ellipsoidinc = cam->IncidenceAngle();
          ellipsoidema = cam->EmissionAngle();
          if (angleSource == "DEM") {
            Angle phase, incidence, emission;
            cam->LocalPhotometricAngles(phase, incidence, emission, success);
            if (success) {
              deminc = incidence.degrees();
              demema = emission.degrees();
            }
          } else if (angleSource == "ELLIPSOID") {
            deminc = ellipsoidinc;
            demema = ellipsoidema;
          }
        }

        // if invalid angles, set to null
        if (!success) {
          out[i] = NULL8;
        }
        // otherwise, do photometric correction
        else {
          pho->Compute(ellipsoidpha, ellipsoidinc, ellipsoidema, deminc, demema,
                       in[i], out[i], mult, base);
        }
      }
    }
    // Trim
    if (!usedem) {
      cam->IgnoreElevationModel(true);
    }
    double trimInc = 0, trimEma = 0;
    // bool success = true;
    for (int i = 0; i < in.size(); i++) {
      // if off the target, set to null
      if (!cam->SetImage(in.Sample(i), in.Line(i))) {
        out[i] = NULL8;
        // success = false;
      } else {
        trimInc = cam->IncidenceAngle();
        trimEma = cam->EmissionAngle();
      }

      if (trimInc > maxinc || trimEma > maxema) {
        out[i] = NULL8;
      }
    }
    cam->IgnoreElevationModel(false);
  };

  // Start the processing
  if (useBackplane) {
    p.ProcessCubes(photometWithBackplane, false);
  } else {
    p.ProcessCube(photomet, false);
  }
  p.EndProcess();
}
}  // namespace Isis