#define GUIHELPERS

#include "Isis.h"
#include <sstream>
#include "PvlGroup.h"
#include "UserInterface.h"
#include "IString.h"
#include <QMessageBox>

using namespace std;
using namespace Isis;

void PrintPvl();
void LoadPvl();

map <QString, void *> GuiHelpers() {
  map <QString, void *> helper;
  helper ["PrintPvl"] = (void *) PrintPvl;
  helper ["LoadPvl"] = (void *) LoadPvl;
  return helper;
}

//functions in the code
void addPhoModel(Pvl &pvl, Pvl &outPvl);
void addAtmosModel(Pvl &pvl, Pvl &outPvl);

// Helper function to print the input pvl file to session log
void PrintPvl() {
  UserInterface &ui = Application::GetUserInterface();

  // Write file out to log
  QString inFile(ui.GetFileName("FROMPVL"));
  Pvl inPvl;
  inPvl.read(ui.GetFileName("FROMPVL").toStdString());
  QString OQString = "***** Output of [" + inFile + "] *****";
  Application::GuiLog(OQString);
  Application::GuiLog(inPvl);
}

// Load the values from the input PVL to a QString to be displayed
// onto the UI.
void LoadKeyValue(const PvlKeyword & key, QString & val){
  int size = key.size();
  val = "";
  for (int i=0; i<size; i++) {
    if (i > 0) {
      val += ", ";
    }
    val += QString::fromStdString(key[i]);
  }
}

// Data from the UI is output to a PVL
// Converts the QString into double value
void OutputKeyValue(PvlKeyword & key, QString val){
  key.clear();
  int found = val.indexOf(",");
  while(found != -1) {
    key += toString((val.mid(0, found)).toDouble());
    val = val.mid(found+1);
    found = val.indexOf(",");
  }
  key += toString((val).toDouble());
}

// Helper function to load the input pvl file into the GUI
void LoadPvl() {
  std::stringstream os;
  QString keyVal;
  UserInterface &ui = Application::GetUserInterface();
  QString inFile(ui.GetFileName("FROMPVL"));
  Pvl inPvl;
  inPvl.read(inFile.toStdString());
  QString phtName = ui.GetAsString("PHTNAME");
  phtName = phtName.toUpper();
  QString atmName = ui.GetAsString("ATMNAME");
  atmName = atmName.toUpper();

  QString phtVal;
  if (inPvl.hasObject("PhotometricModel")) {
    PvlObject phtObj = inPvl.findObject("PhotometricModel");
    if (!phtObj.hasGroup("Algorithm")) {
      std::string message = "The input PVL does not contain a valid photometric model so you must specify one ";
      message += "- the [Algorithm] group is missing in your [PhotometricModel]";
      throw IException(IException::User, message, _FILEINFO_);
    }
    else {
      PvlObject::PvlGroupIterator phtGrp = phtObj.beginGroup();
      bool wasFound = false;
      if (phtGrp->hasKeyword("PHTNAME")) {
        phtVal = QString::fromStdString(phtGrp->findKeyword("PHTNAME"));
      } else if (phtGrp->hasKeyword("NAME")) {
        phtVal = QString::fromStdString(phtGrp->findKeyword("NAME"));
      } else {
        std::string message = "The input PVL does not contain a valid photometric model so you must specify one ";
        message += "- the [Phtname] keyword is missing in your [Algorithm] group";
        throw IException(IException::User, message, _FILEINFO_);
      }
      phtVal = phtVal.toUpper();
      if (phtName == phtVal || phtName == "NONE" || phtName == "FROMPVL") {
        wasFound = true;
      }
      if (!wasFound) {
        while (phtGrp != phtObj.endGroup()) {
          if (phtGrp->hasKeyword("PHTNAME") || phtGrp->hasKeyword("NAME")) {
            if (phtGrp->hasKeyword("PHTNAME")) {
              phtVal = QString::fromStdString(phtGrp->findKeyword("PHTNAME"));
            } else if (phtGrp->hasKeyword("NAME")) {
              phtVal = QString::fromStdString(phtGrp->findKeyword("NAME"));
            } else {
              std::string message = "The input PVL does not contain a valid photometric model so you must specify one ";
              message += "- the [Phtname] keyword is missing in your [Algorithm] group";
              throw IException(IException::User, message, _FILEINFO_);
            }
            phtVal = phtVal.toUpper();
            if (phtName == phtVal) {
              wasFound = true;
              break;
            }
            phtGrp++;
          }
        }
      }
      if (wasFound) {
        ui.Clear("PHTNAME");
        ui.Clear("THETA");
        ui.Clear("WH");
        ui.Clear("HG1");
        ui.Clear("HG2");
        ui.Clear("HH");
        ui.Clear("B0");
        ui.Clear("ZEROB0STANDARD");
        ui.Clear("BH");
        ui.Clear("CH");
        ui.Clear("L");
        ui.Clear("K");
        ui.Clear("PHASELIST");
        ui.Clear("KLIST");
        ui.Clear("LLIST");
        ui.Clear("PHASECURVELIST");
        if (phtVal == "HAPKEHEN" || phtVal == "HAPKELEG") {
          if (phtGrp->hasKeyword("THETA")) {
            PvlKeyword thetaKey = phtGrp->findKeyword("THETA");
            LoadKeyValue(thetaKey, keyVal);
            ui.PutAsString("THETA", keyVal);
          }
          if (phtGrp->hasKeyword("WH")) {
            PvlKeyword whKey = phtGrp->findKeyword("WH");
            LoadKeyValue(whKey, keyVal);
            ui.PutAsString("WH", keyVal);
          }
          if (phtGrp->hasKeyword("HH")) {
            PvlKeyword hhKey = phtGrp->findKeyword("HH");
            LoadKeyValue(hhKey, keyVal);
            ui.PutAsString("HH", keyVal);
          }
          if (phtGrp->hasKeyword("B0")) {
            PvlKeyword b0Key = phtGrp->findKeyword("B0");
            LoadKeyValue(b0Key, keyVal);
            ui.PutAsString("B0", keyVal);
          }
          if (phtGrp->hasKeyword("ZEROB0STANDARD")) {
            QString zerob0 = QString::fromStdString(phtGrp->findKeyword("ZEROB0STANDARD"));
            QString izerob0 = zerob0;
            izerob0 = izerob0.toUpper();
            if (izerob0 == "TRUE") {
              ui.PutString("ZEROB0STANDARD", "TRUE");
            } else if (izerob0 == "FALSE") {
              ui.PutString("ZEROB0STANDARD", "FALSE");
            } else {
              std::string message = "The ZEROB0STANDARD value is invalid - must be set to TRUE or FALSE";
              throw IException(IException::User, message, _FILEINFO_);
            }
          }
          if (phtVal == "HAPKEHEN") {
            if (phtGrp->hasKeyword("HG1")) {
              PvlKeyword hg1Key = phtGrp->findKeyword("HG1");
              LoadKeyValue(hg1Key, keyVal);
              ui.PutAsString("HG1", keyVal);
            }
            if (phtGrp->hasKeyword("HG2")) {
              PvlKeyword hg2Key = phtGrp->findKeyword("HG2");
              LoadKeyValue(hg2Key, keyVal);
              ui.PutAsString("HG2", keyVal);
            }
          }
          if (phtVal == "HAPKELEG") {
            if (phtGrp->hasKeyword("BH")) {
              PvlKeyword bhKey = phtGrp->findKeyword("BH");
              LoadKeyValue(bhKey, keyVal);
              ui.PutAsString("BH", keyVal);
            }
            if (phtGrp->hasKeyword("CH")) {
              PvlKeyword chKey = phtGrp->findKeyword("CH");
              LoadKeyValue(chKey, keyVal);
              ui.PutAsString("CH", keyVal);
            }
          }
        } else if (phtVal == "MINNAERT") {
          if (phtGrp->hasKeyword("K")) {
            PvlKeyword k = phtGrp->findKeyword("K");
            LoadKeyValue(k, keyVal);
            ui.PutAsString("K", keyVal);
          }
        } else if (phtVal == "LUNARLAMBERTEMPIRICAL" || phtVal == "MINNAERTEMPIRICAL") {
          if (phtGrp->hasKeyword("PHASELIST")) {
            PvlKeyword phaselist = phtGrp->findKeyword("PHASELIST");
            LoadKeyValue(phaselist, keyVal);
            ui.PutAsString("PHASELIST", keyVal);
          }
          if (phtGrp->hasKeyword("PHASECURVELIST")) {
            PvlKeyword phasecurvelist = phtGrp->findKeyword("PHASECURVELIST");
            LoadKeyValue(phasecurvelist, keyVal);
            ui.PutAsString("PHASECURVELIST", keyVal);
          }
          if (phtVal == "LUNARLAMBERTEMPIRICAL") {
            if (phtGrp->hasKeyword("LLIST")) {
              PvlKeyword llist = phtGrp->findKeyword("LLIST");
              LoadKeyValue(llist, keyVal);
              ui.PutAsString("LLIST", keyVal);
            }
          }
          if (phtVal == "MINNAERTEMPIRICAL") {
            if (phtGrp->hasKeyword("KLIST")) {
              PvlKeyword kList = phtGrp->findKeyword("KLIST");
              LoadKeyValue(kList, keyVal);
              ui.PutAsString("KLIST", keyVal);
            }
          }
        } else if (phtVal == "LUNARLAMBERT") {
          if (phtGrp->hasKeyword("L")) {
            PvlKeyword l = phtGrp->findKeyword("L");
            LoadKeyValue(l, keyVal);
            ui.PutAsString("L", keyVal);
          }
        } else if (phtVal != "LAMBERT" && phtVal != "LOMMELSEELIGER" &&
                   phtVal != "LUNARLAMBERTMCEWEN") {
          std::string message = "Unsupported photometric model [" + phtVal.toStdString() + "].";
          throw IException(IException::User, message, _FILEINFO_);
        }
        ui.PutAsString("PHTNAME", phtVal);
      }
    }
  }

  QString atmVal;
  if (inPvl.hasObject("AtmosphericModel")) {
    PvlObject atmObj = inPvl.findObject("AtmosphericModel");
    if (!atmObj.hasGroup("Algorithm")) {
      std::string message = "The input PVL does not contain a valid atmospheric model so you must specify one ";
      message += "- the [Algorithm] group is missing in your [AtmosphericModel]";
      throw IException(IException::User, message, _FILEINFO_);
    }
    else {
      PvlObject::PvlGroupIterator atmGrp = atmObj.beginGroup();
      bool wasFound = false;
      if (atmGrp->hasKeyword("ATMNAME")) {
        atmVal = QString::fromStdString(atmGrp->findKeyword("ATMNAME"));
      } else if (atmGrp->hasKeyword("NAME")) {
        atmVal = QString::fromStdString(atmGrp->findKeyword("NAME"));
      } else {
        std::string message = "The input PVL does not contain a valid atmospheric model so you must specify one ";
        message += "- the [Atmname] keyword is missing in your [Algorithm] group";
        throw IException(IException::User, message, _FILEINFO_);
      }
      atmVal = atmVal.toUpper();
      if (atmName == atmVal || atmName == "NONE" || atmName == "FROMPVL") {
        wasFound = true;
      }
      if (!wasFound) {
        while (atmGrp != atmObj.endGroup()) {
          if (atmGrp->hasKeyword("ATMNAME") || atmGrp->hasKeyword("NAME")) {
            if (atmGrp->hasKeyword("ATMNAME")) {
              atmVal = QString::fromStdString(atmGrp->findKeyword("ATMNAME"));
            } else if (atmGrp->hasKeyword("NAME")) {
              atmVal = QString::fromStdString(atmGrp->findKeyword("NAME"));
            } else {
              std::string message = "The input PVL does not contain a valid atmospheric model so you must specify one ";
              message += "- the [Atmname] keyword is missing in your [Algorithm] group";
              throw IException(IException::User, message, _FILEINFO_);
            }
            atmVal = atmVal.toUpper();
            if (atmName == atmVal) {
              wasFound = true;
              break;
            }
          }
          atmGrp++;
        }
      }
      if (wasFound) {
        ui.Clear("ATMNAME");
        ui.Clear("HNORM");
        ui.Clear("BHA");
        ui.Clear("TAU");
        ui.Clear("TAUREF");
        ui.Clear("WHA");
        ui.Clear("HGA");
        if (atmVal == "ANISOTROPIC1" || atmVal == "ANISOTROPIC2" ||
            atmVal == "HAPKEATM1" || atmVal == "HAPKEATM2" ||
            atmVal == "ISOTROPIC1" || atmVal == "ISOTROPIC2") {
          if (atmGrp->hasKeyword("HNORM")) {
            double hnorm = atmGrp->findKeyword("HNORM");
            os.str("");
            os << hnorm;
            ui.PutAsString("HNORM", os.str().c_str());
          }
          if (atmGrp->hasKeyword("TAU")) {
            double tau = atmGrp->findKeyword("TAU");
            os.str("");
            os << tau;
            ui.PutAsString("TAU", os.str().c_str());
          }
          if (atmGrp->hasKeyword("TAUREF")) {
            double tauref = atmGrp->findKeyword("TAUREF");
            os.str("");
            os << tauref;
            ui.PutAsString("TAUREF", os.str().c_str());
          }
          if (atmGrp->hasKeyword("WHA")) {
            double wha = atmGrp->findKeyword("WHA");
            os.str("");
            os << wha;
            ui.PutAsString("WHA", os.str().c_str());
          }
          if (atmGrp->hasKeyword("NULNEG")) {
            QString nulneg = QString::fromStdString(atmGrp->findKeyword("NULNEG"));
            if (nulneg.compare("YES")) {
              ui.PutString("NULNEG", "YES");
            } else if (nulneg.compare("NO")) {
              ui.PutString("NULNEG", "NO");
            } else {
              std::string message = "The NULNEG value is invalid - must be set to YES or NO";
              throw IException(IException::User, message, _FILEINFO_);
            }
          }
        }
        if (atmVal == "ANISOTROPIC1" || atmVal == "ANISOTROPIC2") {
          if (atmGrp->hasKeyword("BHA")) {
            double bha = atmGrp->findKeyword("BHA");
            os.str("");
            os << bha;
            ui.PutAsString("BHA", os.str().c_str());
          }
        }
        if (atmVal == "HAPKEATM1" || atmVal == "HAPKEATM2") {
          if (atmGrp->hasKeyword("HGA")) {
            double hga = atmGrp->findKeyword("HGA");
            os.str("");
            os << hga;
            ui.PutAsString("HGA", os.str().c_str());
          }
        }

        if (atmVal != "ANISOTROPIC1" && atmVal != "ANISOTROPIC2" &&
            atmVal != "HAPKEATM1" && atmVal != "HAPKEATM2" &&
            atmVal != "ISOTROPIC1" && atmVal != "ISOTROPIC2") {
          std::string message = "Unsupported atmospheric model [" + atmVal.toStdString() + "].";
          throw IException(IException::User, message, _FILEINFO_);
        }
        ui.PutAsString("ATMNAME", atmVal);
      }
    }
  }
}

void IsisMain() {
  // Get the output file name from the GUI and write the pvl
  // to the file. If no extension is given, '.pvl' will be used.
  UserInterface &ui = Application::GetUserInterface();
  FileName out = ui.GetFileName("TOPVL").toStdString();
  QString output = ui.GetFileName("TOPVL");
  if(out.extension() == "") {
    output += ".pvl";
  }

  //The PVL to be written out
  Pvl p;
  Pvl op;

  if (ui.WasEntered("FROMPVL")) {
    QString input = ui.GetFileName("FROMPVL");
    p.read(input.toStdString());
  }

  //Check to make sure that a model was specified
  if (ui.GetAsString("PHTNAME") == "NONE" && ui.GetAsString("ATMNAME") == "NONE") {
    std::string message = "A photometric model or an atmospheric model must be specified before running this program";
    throw IException(IException::User, message, _FILEINFO_);
  }

  //Add the different models to the PVL
  if (ui.GetAsString("PHTNAME") != "NONE") {
    addPhoModel(p, op);
  }

  if (ui.GetAsString("ATMNAME") != "NONE") {
    addAtmosModel(p, op);
  }

  op.write(output.toStdString());
}

//Function to add photometric model to the PVL
void addPhoModel(Pvl &pvl, Pvl &outPvl) {
  UserInterface &ui = Application::GetUserInterface();

  bool wasFound = false;
  QString phtName = ui.GetAsString("PHTNAME");
  phtName = phtName.toUpper();
  QString phtVal;
  PvlObject pvlObj;
  PvlGroup pvlGrp;
  if (pvl.hasObject("PhotometricModel")) {
    pvlObj = pvl.findObject("PhotometricModel");
    if (pvlObj.hasGroup("Algorithm")) {
      PvlObject::PvlGroupIterator pvlGrp = pvlObj.beginGroup();
      if (pvlGrp->hasKeyword("PHTNAME")) {
        phtVal = QString::fromStdString(pvlGrp->findKeyword("PHTNAME"));
      } else if (pvlGrp->hasKeyword("NAME")) {
        phtVal = QString::fromStdString(pvlGrp->findKeyword("NAME"));
      } else {
        phtVal = "NONE";
      }
      phtVal = phtVal.toUpper();
      if (phtName == phtVal) {
        wasFound = true;
      }
      if (!wasFound) {
        while (pvlGrp != pvlObj.endGroup()) {
          if (pvlGrp->hasKeyword("PHTNAME") || pvlGrp->hasKeyword("NAME")) {
            if (pvlGrp->hasKeyword("PHTNAME")) {
              phtVal = QString::fromStdString(pvlGrp->findKeyword("PHTNAME"));
            } else if (pvlGrp->hasKeyword("NAME")) {
              phtVal = QString::fromStdString(pvlGrp->findKeyword("NAME"));
            } else {
              phtVal = "NONE";
            }
            phtVal = phtVal.toUpper();
            if (phtName == phtVal) {
              wasFound = true;
              break;
            }
          }
          pvlGrp++;
        }
      }
    }
    if (wasFound) {
      outPvl.addObject(pvlObj);
    } else {
      outPvl.addObject(PvlObject("PhotometricModel"));
      outPvl.findObject("PhotometricModel").addGroup(PvlGroup("Algorithm"));
      outPvl.findObject("PhotometricModel").findGroup("Algorithm").
             addKeyword(PvlKeyword("PHTNAME",phtName.toStdString()),Pvl::Replace);
    }
  } else {
    outPvl.addObject(PvlObject("PhotometricModel"));
    outPvl.findObject("PhotometricModel").addGroup(PvlGroup("Algorithm"));
    outPvl.findObject("PhotometricModel").findGroup("Algorithm").
           addKeyword(PvlKeyword("PHTNAME",phtName.toStdString()),Pvl::Replace);
  }

  //Get the photometric model and any parameters specific to that
  //model and write it to the algorithm group

  //Hapke Photometric Models
  if(phtName == "HAPKEHEN" || phtName == "HAPKELEG") {
    if (ui.WasEntered("THETA")) {
      PvlKeyword thetaKey("THETA");
      OutputKeyValue(thetaKey, ui.GetString("THETA"));
      outPvl.findObject("PhotometricModel").findGroup("Algorithm").
             addKeyword(thetaKey,Pvl::Replace);
    } else {
      if (!outPvl.findObject("PhotometricModel").findGroup("Algorithm").
                  hasKeyword("THETA")) {
        std::string message = "The " + phtName.toStdString() + " Photometric model requires a value for the THETA parameter.";
        message += "The normal range for THETA is: 0 <= THETA <= 90";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }
    if (ui.WasEntered("WH")) {
      PvlKeyword whKey("WH");
      OutputKeyValue(whKey, ui.GetString("WH"));
      outPvl.findObject("PhotometricModel").findGroup("Algorithm").
             addKeyword(whKey, Pvl::Replace);
    } else {
      if (!outPvl.findObject("PhotometricModel").findGroup("Algorithm").
                  hasKeyword("WH")) {
        std::string message = "The " + phtName.toStdString() + " Photometric model requires a value for the WH parameter.";
        message += "The normal range for WH is: 0 < WH <= 1";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }
    if (ui.WasEntered("HH")) {
      PvlKeyword hhKey("HH");
      OutputKeyValue(hhKey, ui.GetString("HH"));
      outPvl.findObject("PhotometricModel").findGroup("Algorithm").
             addKeyword(hhKey, Pvl::Replace);
    } else {
      if (!outPvl.findObject("PhotometricModel").findGroup("Algorithm").
                  hasKeyword("HH")) {
        std::string message = "The " + phtName.toStdString() + " Photometric model requires a value for the HH parameter.";
        message += "The normal range for HH is: 0 <= HH";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }
    if (ui.WasEntered("B0")) {
      PvlKeyword b0Key("B0");
      OutputKeyValue(b0Key, ui.GetString("B0"));
      outPvl.findObject("PhotometricModel").findGroup("Algorithm").
             addKeyword(b0Key, Pvl::Replace);
    } else {
      if (!outPvl.findObject("PhotometricModel").findGroup("Algorithm").
                  hasKeyword("B0")) {
        std::string message = "The " + phtName.toStdString() + " Photometric model requires a value for the B0 parameter.";
        message += "The normal range for B0 is: 0 <= B0";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }
    if (ui.GetString("ZEROB0STANDARD") != "READFROMPVL") {
      if (ui.GetString("ZEROB0STANDARD") == "TRUE") {
        outPvl.findObject("PhotometricModel").findGroup("Algorithm").
               addKeyword(PvlKeyword("ZEROB0STANDARD","TRUE"),Pvl::Replace);
      } else if (ui.GetString("ZEROB0STANDARD") == "FALSE") {
        outPvl.findObject("PhotometricModel").findGroup("Algorithm").
               addKeyword(PvlKeyword("ZEROB0STANDARD","FALSE"),Pvl::Replace);
      }
    } else if (!outPvl.findObject("PhotometricModel").findGroup("Algorithm").
               hasKeyword("ZEROB0STANDARD")) {
      if (ui.IsInteractive()) { 
        QMessageBox msgbox;
        QString message = "You requested that the ZEROB0STANDARD value come from the input PVL file, but there is not one, so the ";
        message += "ZEROB0STANDARD parameter is being set to TRUE.";
        msgbox.setText(message);
        msgbox.exec();
      }
      outPvl.findObject("PhotometricModel").findGroup("Algorithm").
             addKeyword(PvlKeyword("ZEROB0STANDARD","TRUE"),Pvl::Replace);
    }
    if (phtName == "HAPKEHEN") {
      if (ui.WasEntered("HG1")) {
        PvlKeyword hg1Key("HG1");
        OutputKeyValue(hg1Key, ui.GetString("HG1"));
        outPvl.findObject("PhotometricModel").findGroup("Algorithm").
               addKeyword(hg1Key, Pvl::Replace);
      } else {
        if (!outPvl.findObject("PhotometricModel").findGroup("Algorithm").
                    hasKeyword("HG1")) {
          std::string message = "The " + phtName.toStdString() + " Photometric model requires a value for the HG1 parameter.";
          message += "The normal range for HG1 is: -1 < HG1 < 1";
          throw IException(IException::User, message, _FILEINFO_);
        }
      }
      if (ui.WasEntered("HG2")) {
        PvlKeyword hg2Key("HG2");
        OutputKeyValue(hg2Key, ui.GetString("HG2"));
        outPvl.findObject("PhotometricModel").findGroup("Algorithm").
               addKeyword(hg2Key, Pvl::Replace);
      } else {
        if (!outPvl.findObject("PhotometricModel").findGroup("Algorithm").
                    hasKeyword("HG2")) {
          std::string message = "The " + phtName.toStdString() + " Photometric model requires a value for the HG2 parameter.";
          message += "The normal range for HG2 is: 0 <= HG2 <= 1";
          throw IException(IException::User, message, _FILEINFO_);
        }
      }
    } else {
      if (ui.WasEntered("BH")) {
        PvlKeyword bhKey("BH");
        OutputKeyValue(bhKey, ui.GetString("BH"));
        outPvl.findObject("PhotometricModel").findGroup("Algorithm").
               addKeyword(bhKey, Pvl::Replace);
      } else {
        if (!outPvl.findObject("PhotometricModel").findGroup("Algorithm").
                    hasKeyword("BH")) {
          std::string message = "The " + phtName.toStdString() + " Photometric model requires a value for the BH parameter.";
          message += "The normal range for BH is: -1 <= BH <= 1";
          throw IException(IException::User, message, _FILEINFO_);
        }
      }
      if (ui.WasEntered("CH")) {
        PvlKeyword chKey("CH");
        OutputKeyValue(chKey, ui.GetString("CH"));
        outPvl.findObject("PhotometricModel").findGroup("Algorithm").
               addKeyword(chKey, Pvl::Replace);
      } else {
        if (!outPvl.findObject("PhotometricModel").findGroup("Algorithm").
                    hasKeyword("CH")) {
          std::string message = "The " + phtName.toStdString() + " Photometric model requires a value for the CH parameter.";
          message += "The normal range for CH is: -1 <= CH <= 1";
          throw IException(IException::User, message, _FILEINFO_);
        }
      }
    }

  }
  //Lunar Lambert Empirical and Minnaert Empirical Photometric Models
  else if (phtName == "LUNARLAMBERTEMPIRICAL" || phtName == "MINNAERTEMPIRICAL") {
    int phaselistsize = 0;
    int phasecurvelistsize = 0;
    int llistsize = 0;
    int klistsize = 0;
    if (ui.WasEntered("PHASELIST")) {
      PvlKeyword phaseListKey("PHASELIST");
      OutputKeyValue(phaseListKey, ui.GetString("PHASELIST"));
      outPvl.findObject("PhotometricModel").findGroup("Algorithm").
             addKeyword(phaseListKey, Pvl::Replace);
      phaselistsize = phaseListKey.size();
    } else {
      if (!outPvl.findObject("PhotometricModel").findGroup("Algorithm").
                    hasKeyword("PHASELIST")) {
        std::string message = "The " + phtName.toStdString() + " Photometric model requires a value for the PHASELIST parameter.";
        throw IException(IException::User, message, _FILEINFO_);
      }
      phaselistsize = outPvl.findObject("PhotometricModel").findGroup("Algorithm").
                      findKeyword("PHASELIST").size();
    }
    if (ui.WasEntered("PHASECURVELIST")) {
      PvlKeyword phCurveListKey("PHASECURVELIST");
      OutputKeyValue(phCurveListKey, ui.GetString("PHASECURVELIST"));
      outPvl.findObject("PhotometricModel").findGroup("Algorithm").
             addKeyword(phCurveListKey, Pvl::Replace);
      phasecurvelistsize = phCurveListKey.size();
    } else {
      if (!outPvl.findObject("PhotometricModel").findGroup("Algorithm").
                  hasKeyword("PHASECURVELIST")) {
        std::string message = "The " + phtName.toStdString() + " Photometric model requires a value for the PHASECURVELIST parameter.";
        throw IException(IException::User, message, _FILEINFO_);
      }
      phasecurvelistsize = outPvl.findObject("PhotometricModel").findGroup("Algorithm").
                           findKeyword("PHASECURVELIST").size();
    }
    if (phtName == "LUNARLAMBERTEMPIRICAL") {
      if (ui.WasEntered("LLIST")) {
        PvlKeyword lListKey("LLIST");
        OutputKeyValue(lListKey, ui.GetString("LLIST"));
        outPvl.findObject("PhotometricModel").findGroup("Algorithm").
               addKeyword(lListKey, Pvl::Replace);
        llistsize = lListKey.size();
      } else {
        if (!outPvl.findObject("PhotometricModel").findGroup("Algorithm").
                    hasKeyword("LLIST")) {
          std::string message = "The " + phtName.toStdString() + " Photometric model requires a value for the LLIST parameter.";
          throw IException(IException::User, message, _FILEINFO_);
        }
        llistsize = outPvl.findObject("PhotometricModel").findGroup("Algorithm").
                    findKeyword("LLIST").size();
      }
      if (llistsize != phaselistsize || llistsize != phasecurvelistsize) {
        std::string message = "The " + phtName.toStdString() + " Photometric model requires that the LLIST, ";
        message += "PHASELIST, and PHASECURVELIST  parameters all have the same number of ";
        message += "entries.";
        throw IException(IException::User, message, _FILEINFO_);
      }
    } else {
      if (ui.WasEntered("KLIST")) {
        PvlKeyword kListKey("KLIST");
        OutputKeyValue(kListKey, ui.GetString("KLIST"));
        outPvl.findObject("PhotometricModel").findGroup("Algorithm").
               addKeyword(kListKey, Pvl::Replace);
        klistsize = kListKey.size();
      } else {
        if (!outPvl.findObject("PhotometricModel").findGroup("Algorithm").
                    hasKeyword("KLIST")) {
          std::string message = "The " + phtName.toStdString() + " Photometric model requires a value for the KLIST parameter.";
          throw IException(IException::User, message, _FILEINFO_);
        }
        klistsize = outPvl.findObject("PhotometricModel").findGroup("Algorithm").
                    findKeyword("KLIST").size();
      }
      if (klistsize != phaselistsize || klistsize != phasecurvelistsize) {
        std::string message = "The " + phtName.toStdString() + " Photometric model requires that the KLIST, ";
        message += "PHASELIST, and PHASECURVELIST  parameters all have the same number of ";
        message += "entries.";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }
  }
  //Lunar Lambert Photometric Model
  else if (phtName == "LUNARLAMBERT") {
    if (ui.WasEntered("L")) {
      PvlKeyword lKey("L");
      OutputKeyValue(lKey, ui.GetString("L"));
      outPvl.findObject("PhotometricModel").findGroup("Algorithm").
             addKeyword(lKey,Pvl::Replace);
    } else {
      if (!outPvl.findObject("PhotometricModel").findGroup("Algorithm").
                  hasKeyword("L")) {
        std::string message = "The " + phtName.toStdString() + " Photometric model requires a value for the L parameter.";
        message += "The L parameter has no limited range";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }
  }
  //Minnaert Photometric Model
  else if (phtName == "MINNAERT") {
    if (ui.WasEntered("K")) {
      PvlKeyword kKey("K");
      OutputKeyValue(kKey, ui.GetString("K"));
      outPvl.findObject("PhotometricModel").findGroup("Algorithm").
             addKeyword(kKey, Pvl::Replace);
    } else {
      if (!outPvl.findObject("PhotometricModel").findGroup("Algorithm").
                  hasKeyword("K")) {
        std::string message = "The " + phtName.toStdString() + " Photometric model requires a value for the K parameter.";
        message += "The normal range for K is: 0 <= K";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }
  }
}

//Function to add atmospheric model to the PVL
void addAtmosModel(Pvl &pvl, Pvl &outPvl) {
  UserInterface &ui = Application::GetUserInterface();

  bool wasFound = false;
  QString atmName = ui.GetAsString("ATMNAME");
  atmName = atmName.toUpper();
  QString atmVal;
  PvlObject pvlObj;
  PvlGroup pvlGrp;
  if (pvl.hasObject("AtmosphericModel")) {
    pvlObj = pvl.findObject("AtmosphericModel");
    if (pvlObj.hasGroup("Algorithm")) {
      PvlObject::PvlGroupIterator pvlGrp = pvlObj.beginGroup();
      if (pvlGrp->hasKeyword("ATMNAME")) {
        atmVal = QString::fromStdString(pvlGrp->findKeyword("ATMNAME"));
      } else if (pvlGrp->hasKeyword("NAME")) {
        atmVal = QString::fromStdString(pvlGrp->findKeyword("NAME"));
      } else {
        atmVal = "NONE";
      }
      atmVal = atmVal.toUpper();
      if (atmName == atmVal) {
        wasFound = true;
      }
      if (!wasFound) {
        while (pvlGrp != pvlObj.endGroup()) {
          if (pvlGrp->hasKeyword("ATMNAME") || pvlGrp->hasKeyword("NAME")) {
            if (pvlGrp->hasKeyword("ATMNAME")) {
              atmVal =  QString::fromStdString(pvlGrp->findKeyword("ATMNAME"));
            } else if (pvlGrp->hasKeyword("NAME")) {
              atmVal = QString::fromStdString(pvlGrp->findKeyword("NAME"));
            } else {
              atmVal = "NONE";
            }
            atmVal = atmVal.toUpper();
            if (atmName == atmVal) {
              wasFound = true;
              break;
            }
          }
          pvlGrp++;
        }
      }
    }
    if (wasFound) {
      outPvl.addObject(pvlObj);
    } else {
      outPvl.addObject(PvlObject("AtmosphericModel"));
      outPvl.findObject("AtmosphericModel").addGroup(PvlGroup("Algorithm"));
      outPvl.findObject("AtmosphericModel").findGroup("Algorithm").
             addKeyword(PvlKeyword("ATMNAME",atmName.toStdString()),Pvl::Replace);
    }
  } else {
    outPvl.addObject(PvlObject("AtmosphericModel"));
    outPvl.findObject("AtmosphericModel").addGroup(PvlGroup("Algorithm"));
    outPvl.findObject("AtmosphericModel").findGroup("Algorithm").
           addKeyword(PvlKeyword("ATMNAME",atmName.toStdString()),Pvl::Replace);
  }

  //Get the atmospheric model and any parameters specific to that
  //model and write it to the algorithm group

  if (atmName == "ANISOTROPIC1" || atmName == "ANISOTROPIC2" ||
      atmName == "HAPKEATM1" || atmName == "HAPKEATM2" ||
      atmName == "ISOTROPIC1" || atmName == "ISOTROPIC2") {
    if (ui.WasEntered("HNORM")) {
       PvlKeyword hnormKey("HNORM");
       OutputKeyValue(hnormKey, ui.GetString("HNORM"));
       outPvl.findObject("AtmosphericModel").findGroup("Algorithm").
             addKeyword(hnormKey, Pvl::Replace);
    } else {
      if (!outPvl.findObject("AtmosphericModel").findGroup("Algorithm").
                  hasKeyword("HNORM")) {
        std::string message = "The " + atmName.toStdString() + " Atmospheric model requires a value for the HNORM parameter.";
        message += "The normal range for HNORM is: 0 <= HNORM";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }
    if (ui.WasEntered("TAU")) {
      PvlKeyword tauKey("TAU");
      OutputKeyValue(tauKey, ui.GetString("TAU"));
      outPvl.findObject("AtmosphericModel").findGroup("Algorithm").
             addKeyword(tauKey, Pvl::Replace);
    } else {
      if (!outPvl.findObject("AtmosphericModel").findGroup("Algorithm").
                  hasKeyword("TAU")) {
        std::string message = "The " + atmName.toStdString() + " Atmospheric model requires a value for the TAU parameter.";
        message += "The normal range for TAU is: 0 <= TAU";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }
    if (ui.WasEntered("TAUREF")) {
      PvlKeyword taurefKey("TAUREF");
      OutputKeyValue(taurefKey, ui.GetString("TAUREF"));
      outPvl.findObject("AtmosphericModel").findGroup("Algorithm").
             addKeyword(taurefKey,Pvl::Replace);
    } else {
      if (!outPvl.findObject("AtmosphericModel").findGroup("Algorithm").
                  hasKeyword("TAUREF")) {
        std::string message = "The " + atmName.toStdString() + " Atmospheric model requires a value for the TAUREF parameter.";
        message += "The normal range for TAUREF is: 0 <= TAUREF";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }
    if (ui.WasEntered("WHA")) {
      PvlKeyword whaKey("WHA");
      OutputKeyValue(whaKey, ui.GetString("WHA"));
      outPvl.findObject("AtmosphericModel").findGroup("Algorithm").
             addKeyword(whaKey, Pvl::Replace);
    } else {
      if (!outPvl.findObject("AtmosphericModel").findGroup("Algorithm").
                  hasKeyword("WHA")) {
        std::string message = "The " + atmName.toStdString() + " Atmospheric model requires a value for the WHA parameter.";
        message += "The normal range for WHA is: 0 < WHA < 1";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }
    if (ui.GetString("NULNEG") != "READFROMPVL") {
      if (ui.GetString("NULNEG") == "YES") {
        outPvl.findObject("AtmosphericModel").findGroup("Algorithm").
               addKeyword(PvlKeyword("NULNEG","YES"),Pvl::Replace);
      } else if (ui.GetString("NULNEG") == "NO") {
        outPvl.findObject("AtmosphericModel").findGroup("Algorithm").
               addKeyword(PvlKeyword("NULNEG","NO"),Pvl::Replace);
    } else if (!outPvl.findObject("AtmosphericModel").findGroup("Algorithm").
                  hasKeyword("NULNEG")) {
        std::string message = "The " + atmName.toStdString() + " Atmospheric model requires a value for the NULNEG parameter.";
        message += "The valid values for NULNEG are: YES, NO";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }
  }
  if (atmName == "ANISOTROPIC1" || atmName == "ANISOTROPIC2") {
    if (ui.WasEntered("BHA")) {
      PvlKeyword bhaKey("BHA");
      OutputKeyValue(bhaKey, ui.GetString("BHA"));
      outPvl.findObject("AtmosphericModel").findGroup("Algorithm").
             addKeyword(bhaKey, Pvl::Replace);
    } else {
      if (!outPvl.findObject("AtmosphericModel").findGroup("Algorithm").
                  hasKeyword("BHA")) {
        std::string message = "The " + atmName.toStdString() + " Atmospheric model requires a value for the BHA parameter.";
        message += "The normal range for BHA is: -1 <= BHA <= 1";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }
  }
  if (atmName == "HAPKEATM1" || atmName == "HAPKEATM2") {
    if (ui.WasEntered("HGA")) {
      PvlKeyword hgaKey("HGA");
      OutputKeyValue(hgaKey, ui.GetString("HGA"));
      outPvl.findObject("AtmosphericModel").findGroup("Algorithm").
             addKeyword(hgaKey, Pvl::Replace);
    } else {
      if (!outPvl.findObject("AtmosphericModel").findGroup("Algorithm").
                  hasKeyword("HGA")) {
        std::string message = "The " + atmName.toStdString() + " Atmospheric model requires a value for the HGA parameter.";
        message += "The normal range for HGA is: -1 < HGA < 1";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }
  }
}
