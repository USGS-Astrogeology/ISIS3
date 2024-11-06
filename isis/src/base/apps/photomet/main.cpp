#define GUIHELPERS
#include "Isis.h"

#include "photomet.h"

#include "Application.h"
#include "Pvl.h"
#include "PvlGroup.h"

#include <map>
#include <sstream>
#include <QString>

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

void IsisMain() {
    UserInterface &ui = Application::GetUserInterface();
    Pvl appLog;
    photomet(ui, &appLog);
}

// Helper function to print the input pvl file to session log
void PrintPvl() {
  UserInterface &ui = Application::GetUserInterface();

  // Write file out to log
  QString inFile(ui.GetFileName("FROMPVL"));
  Pvl inPvl;
  inPvl.read(ui.GetFileName("FROMPVL"));
  QString OQString = "***** Output of [" + inFile + "] *****";
  Application::GuiLog(OQString);
  Application::GuiLog(inPvl);
}

// Helper function to load the input pvl file into the GUI
void LoadPvl() {
  std::stringstream os;
  UserInterface &ui = Application::GetUserInterface();
  QString inFile(ui.GetFileName("FROMPVL"));
  Pvl inPvl;
  inPvl.read(inFile);
  QString phtName = ui.GetAsString("PHTNAME");
  phtName = phtName.toUpper();
  QString atmName = ui.GetAsString("ATMNAME");
  atmName = atmName.toUpper();
  QString nrmName = ui.GetAsString("NORMNAME");
  nrmName = nrmName.toUpper();

  QString phtVal;
  if (inPvl.hasObject("PhotometricModel")) {
      PvlObject phtObj = inPvl.findObject("PhotometricModel");
      if (!phtObj.hasGroup("Algorithm")) {
      QString message = "The input PVL does not contain a valid photometric model so you must specify one ";
      message += "- the [Algorithm] group is missing in your [PhotometricModel]";
      throw IException(IException::User, message, _FILEINFO_);
      }
      else {
      PvlObject::PvlGroupIterator phtGrp = phtObj.beginGroup();
      bool wasFound = false;
      if (phtGrp->hasKeyword("PHTNAME")) {
          phtVal = (QString)phtGrp->findKeyword("PHTNAME");
      } else if (phtGrp->hasKeyword("NAME")) {
          phtVal = (QString)phtGrp->findKeyword("NAME");
      } else {
          QString message = "The input PVL does not contain a valid photometric model so you must specify one ";
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
              phtVal = (QString)phtGrp->findKeyword("PHTNAME");
              } else if (phtGrp->hasKeyword("NAME")) {
              phtVal = (QString)phtGrp->findKeyword("NAME");
              } else {
              QString message = "The input PVL does not contain a valid photometric model so you must specify one ";
              message += "- the [Phtname] keyword is missing in your [Algorithm] group";
              throw IException(IException::User, message, _FILEINFO_);
              }
              phtVal = phtVal.toUpper();
              if (phtName == phtVal) {
              wasFound = true;
              break;
              }
          }
          phtGrp++;
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
              double theta = phtGrp->findKeyword("THETA");
              os.str("");
              os << theta;
              ui.PutAsString("THETA", os.str().c_str());
          }
          if (phtGrp->hasKeyword("WH")) {
              double wh = phtGrp->findKeyword("WH");
              os.str("");
              os << wh;
              ui.PutAsString("WH", os.str().c_str());
          }
          if (phtGrp->hasKeyword("HH")) {
              double hh = phtGrp->findKeyword("HH");
              os.str("");
              os << hh;
              ui.PutAsString("HH", os.str().c_str());
          }
          if (phtGrp->hasKeyword("B0")) {
              double b0 = phtGrp->findKeyword("B0");
              os.str("");
              os << b0;
              ui.PutAsString("B0", os.str().c_str());
          }
          if (phtGrp->hasKeyword("ZEROB0STANDARD")) {
              QString zerob0 = (QString)phtGrp->findKeyword("ZEROB0STANDARD");
              QString izerob0 = zerob0;
              izerob0 = izerob0.toUpper();
              if (izerob0 == "TRUE") {
              ui.PutString("ZEROB0STANDARD", "TRUE");
              } else if (izerob0 == "FALSE") {
              ui.PutString("ZEROB0STANDARD", "FALSE");
              } else {
              QString message = "The ZEROB0STANDARD value is invalid - must be set to TRUE or FALSE";
              throw IException(IException::User, message, _FILEINFO_);
              }
          }
          if (phtVal == "HAPKEHEN") {
              if (phtGrp->hasKeyword("HG1")) {
              double hg1 = phtGrp->findKeyword("HG1");
              os.str("");
              os << hg1;
              ui.PutAsString("HG1", os.str().c_str());
              }
              if (phtGrp->hasKeyword("HG2")) {
              double hg2 = phtGrp->findKeyword("HG2");
              os.str("");
              os << hg2;
              ui.PutAsString("HG2", os.str().c_str());
              }
          }
          if (phtVal == "HAPKELEG") {
              if (phtGrp->hasKeyword("BH")) {
              double bh = phtGrp->findKeyword("BH");
              os.str("");
              os << bh;
              ui.PutAsString("BH", os.str().c_str());
              }
              if (phtGrp->hasKeyword("CH")) {
              double ch = phtGrp->findKeyword("CH");
              os.str("");
              os << ch;
              ui.PutAsString("CH", os.str().c_str());
              }
          }
          } else if (phtVal == "LUNARLAMBERTEMPIRICAL" || phtVal == "MINNAERTEMPIRICAL") {
          if (phtGrp->hasKeyword("PHASELIST")) {
              QString phaselist = (QString)phtGrp->findKeyword("PHASELIST");
              ui.PutAsString("PHASELIST", phaselist);
          }
          if (phtGrp->hasKeyword("PHASECURVELIST")) {
              QString phasecurvelist = (QString)phtGrp->findKeyword("PHASECURVELIST");
              ui.PutAsString("PHASECURVELIST", phasecurvelist);
          }
          if (phtVal == "LUNARLAMBERTEMPIRICAL") {
              if (phtGrp->hasKeyword("LLIST")) {
              QString llist = (QString)phtGrp->findKeyword("LLIST");
              ui.PutAsString("LLIST", llist);
              }
          }
          if (phtVal == "MINNAERTEMPIRICAL") {
              if (phtGrp->hasKeyword("KLIST")) {
              QString klist = (QString)phtGrp->findKeyword("KLIST");
              ui.PutAsString("KLIST", klist);
              }
          }
          } else if (phtVal == "LUNARLAMBERT") {
          if (phtGrp->hasKeyword("L")) {
              double l = phtGrp->findKeyword("L");
              os.str("");
              os << l;
              ui.PutAsString("L", os.str().c_str());
          }
          } else if (phtVal == "MINNAERT") {
          if (phtGrp->hasKeyword("K")) {
              double k = phtGrp->findKeyword("K");
              os.str("");
              os << k;
              ui.PutAsString("K", os.str().c_str());
          }
          } else if (phtVal != "LAMBERT" && phtVal != "LOMMELSEELIGER" &&
                  phtVal != "LUNARLAMBERTMCEWEN") {
          QString message = "Unsupported photometric model [" + phtVal + "].";
          throw IException(IException::User, message, _FILEINFO_);
          }
          ui.PutAsString("PHTNAME", phtVal);
      }
      }
  }

  QString nrmVal;
  if (inPvl.hasObject("NormalizationModel")) {
      PvlObject nrmObj = inPvl.findObject("NormalizationModel");
      if (!nrmObj.hasGroup("Algorithm")) {
      QString message = "The input PVL does not contain a valid normalization model so you must specify one ";
      message += "- the [Algorithm] group is missing in your [NormalizationModel]";
      throw IException(IException::User, message, _FILEINFO_);
      }
      else {
      PvlObject::PvlGroupIterator nrmGrp = nrmObj.beginGroup();
      bool wasFound = false;
      if (nrmGrp->hasKeyword("NORMNAME")) {
          nrmVal = (QString)nrmGrp->findKeyword("NORMNAME");
      } else if (nrmGrp->hasKeyword("NAME")) {
          nrmVal = (QString)nrmGrp->findKeyword("NAME");
      } else {
          QString message = "The input PVL does not contain a valid normalization model so you must specify one ";
          message += "- the [Normname] keyword is missing in your [Algorithm] group";
          throw IException(IException::User, message, _FILEINFO_);
      }
      nrmVal = nrmVal.toUpper();
      if (nrmName == nrmVal || nrmName == "NONE" || nrmName == "FROMPVL") {
          wasFound = true;
      }
      if (!wasFound) {
          while (nrmGrp != nrmObj.endGroup()) {
          if (nrmGrp->hasKeyword("NORMNAME") || nrmGrp->hasKeyword("NAME")) {
              if (nrmGrp->hasKeyword("NORMNAME")) {
              nrmVal = (QString)nrmGrp->findKeyword("NORMNAME");
              } else if (nrmGrp->hasKeyword("NAME")) {
              nrmVal = (QString)nrmGrp->findKeyword("NAME");
              } else {
              QString message = "The input PVL does not contain a valid normalization model so you must specify one ";
              message += "- the [Normname] keyword is missing in your [Algorithm] group";
              throw IException(IException::User, message, _FILEINFO_);
              }
              nrmVal = nrmVal.toUpper();
              if (nrmName == nrmVal) {
              wasFound = true;
              break;
              }
          }
          nrmGrp++;
          }
      }
      if (wasFound) {
          if (nrmVal != "ALBEDOATM" && nrmVal != "SHADEATM" && nrmVal != "TOPOATM") {
          ui.Clear("ATMNAME");
          }
          ui.Clear("NORMNAME");
          ui.Clear("INCREF");
          ui.Clear("INCMAT");
          ui.Clear("THRESH");
          ui.Clear("ALBEDO");
          ui.Clear("D");
          ui.Clear("E");
          ui.Clear("F");
          ui.Clear("G2");
          ui.Clear("XMUL");
          ui.Clear("WL");
          ui.Clear("H");
          ui.Clear("BSH1");
          ui.Clear("XB1");
          ui.Clear("XB2");
          if (nrmVal != "MOONALBEDO") {
          if (nrmVal == "ALBEDO" || nrmVal == "MIXED") {
              if (nrmGrp->hasKeyword("INCREF")) {
              double incref = nrmGrp->findKeyword("INCREF");
              os.str("");
              os << incref;
              ui.PutAsString("INCREF", os.str().c_str());
              }
              if (nrmGrp->hasKeyword("INCMAT") && nrmVal == "MIXED") {
              double incmat = nrmGrp->findKeyword("INCMAT");
              os.str("");
              os << incmat;
              ui.PutAsString("INCMAT", os.str().c_str());
              }
              if (nrmGrp->hasKeyword("THRESH")) {
              double thresh = nrmGrp->findKeyword("THRESH");
              os.str("");
              os << thresh;
              ui.PutAsString("THRESH", os.str().c_str());
              }
              if (nrmGrp->hasKeyword("ALBEDO")) {
              double albedo = nrmGrp->findKeyword("ALBEDO");
              os.str("");
              os << albedo;
              ui.PutAsString("ALBEDO", os.str().c_str());
              }
          } else if (nrmVal == "SHADE") {
              if (nrmGrp->hasKeyword("INCREF")) {
              double incref = nrmGrp->findKeyword("INCREF");
              os.str("");
              os << incref;
              ui.PutAsString("INCREF", os.str().c_str());
              }
              if (nrmGrp->hasKeyword("ALBEDO")) {
              double albedo = nrmGrp->findKeyword("ALBEDO");
              os.str("");
              os << albedo;
              ui.PutAsString("ALBEDO", os.str().c_str());
              }
          } else if (nrmVal == "TOPO") {
              if (nrmGrp->hasKeyword("INCREF")) {
              double incref = nrmGrp->findKeyword("INCREF");
              os.str("");
              os << incref;
              ui.PutAsString("INCREF", os.str().c_str());
              }
              if (nrmGrp->hasKeyword("ALBEDO")) {
              double albedo = nrmGrp->findKeyword("ALBEDO");
              os.str("");
              os << albedo;
              ui.PutAsString("ALBEDO", os.str().c_str());
              }
              if (nrmGrp->hasKeyword("THRESH")) {
              double thresh = nrmGrp->findKeyword("THRESH");
              os.str("");
              os << thresh;
              ui.PutAsString("THRESH", os.str().c_str());
              }
          } else if (nrmVal == "ALBEDOATM") {
              if (nrmGrp->hasKeyword("INCREF")) {
              double incref = nrmGrp->findKeyword("INCREF");
              os.str("");
              os << incref;
              ui.PutAsString("INCREF", os.str().c_str());
              }
          } else if (nrmVal == "SHADEATM") {
              if (nrmGrp->hasKeyword("INCREF")) {
              double incref = nrmGrp->findKeyword("INCREF");
              os.str("");
              os << incref;
              ui.PutAsString("INCREF", os.str().c_str());
              }
              if (nrmGrp->hasKeyword("ALBEDO")) {
              double albedo = nrmGrp->findKeyword("ALBEDO");
              os.str("");
              os << albedo;
              ui.PutAsString("ALBEDO", os.str().c_str());
              }
          } else if (nrmVal == "TOPOATM") {
              if (nrmGrp->hasKeyword("INCREF")) {
              double incref = nrmGrp->findKeyword("INCREF");
              os.str("");
              os << incref;
              ui.PutAsString("INCREF", os.str().c_str());
              }
              if (nrmGrp->hasKeyword("ALBEDO")) {
              double albedo = nrmGrp->findKeyword("ALBEDO");
              os.str("");
              os << albedo;
              ui.PutAsString("ALBEDO", os.str().c_str());
              }
          } else {
              QString message = "Unsupported normalization model [" + nrmVal + "].";
              throw IException(IException::User, message, _FILEINFO_);
          }
          } else {
          if (nrmGrp->hasKeyword("D")) {
              double d = nrmGrp->findKeyword("D");
              os.str("");
              os << d;
              ui.PutAsString("D", os.str().c_str());
          }
          if (nrmGrp->hasKeyword("E")) {
              double e = nrmGrp->findKeyword("E");
              os.str("");
              os << e;
              ui.PutAsString("E", os.str().c_str());
          }
          if (nrmGrp->hasKeyword("F")) {
              double f = nrmGrp->findKeyword("F");
              os.str("");
              os << f;
              ui.PutAsString("F", os.str().c_str());
          }
          if (nrmGrp->hasKeyword("G2")) {
              double g2 = nrmGrp->findKeyword("G2");
              os.str("");
              os << g2;
              ui.PutAsString("G2", os.str().c_str());
          }
          if (nrmGrp->hasKeyword("XMUL")) {
              double xmul = nrmGrp->findKeyword("XMUL");
              os.str("");
              os << xmul;
              ui.PutAsString("XMUL", os.str().c_str());
          }
          if (nrmGrp->hasKeyword("WL")) {
              double wl = nrmGrp->findKeyword("WL");
              os.str("");
              os << wl;
              ui.PutAsString("WL", os.str().c_str());
          }
          if (nrmGrp->hasKeyword("H")) {
              double h = nrmGrp->findKeyword("H");
              os.str("");
              os << h;
              ui.PutAsString("H", os.str().c_str());
          }
          if (nrmGrp->hasKeyword("BSH1")) {
              double bsh1 = nrmGrp->findKeyword("BSH1");
              os.str("");
              os << bsh1;
              ui.PutAsString("BSH1", os.str().c_str());
          }
          if (nrmGrp->hasKeyword("XB1")) {
              double xb1 = nrmGrp->findKeyword("XB1");
              os.str("");
              os << xb1;
              ui.PutAsString("XB1", os.str().c_str());
          }
          if (nrmGrp->hasKeyword("XB2")) {
              double xb2 = nrmGrp->findKeyword("XB2");
              os.str("");
              os << xb2;
              ui.PutAsString("XB2", os.str().c_str());
          }
          }
          ui.PutAsString("NORMNAME", nrmVal);
      }
      }
  }

  if (nrmName == "NONE" || nrmName == "FROMPVL") {
      if (nrmVal != "ALBEDOATM" && nrmVal != "SHADEATM" && nrmVal != "TOPOATM") {
      return;
      }
  }
  else if (nrmName != "ALBEDOATM" && nrmName != "SHADEATM" && nrmName != "TOPOATM") {
      return;
  }
  QString atmVal;
  if (inPvl.hasObject("AtmosphericModel")) {
      PvlObject atmObj = inPvl.findObject("AtmosphericModel");
      if (!atmObj.hasGroup("Algorithm")) {
      QString message = "The input PVL does not contain a valid atmospheric model so you must specify one ";
      message += "- the [Algorithm] group is missing in your [AtmosphericModel]";
      throw IException(IException::User, message, _FILEINFO_);
      }
      else {
      PvlObject::PvlGroupIterator atmGrp = atmObj.beginGroup();
      bool wasFound = false;
      if (atmGrp->hasKeyword("ATMNAME")) {
          atmVal = (QString)atmGrp->findKeyword("ATMNAME");
      } else if (atmGrp->hasKeyword("NAME")) {
          atmVal = (QString)atmGrp->findKeyword("NAME");
      } else {
          QString message = "The input PVL does not contain a valid atmospheric model so you must specify one ";
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
              atmVal = (QString)atmGrp->findKeyword("ATMNAME");
              } else if (atmGrp->hasKeyword("NAME")) {
              atmVal = (QString)atmGrp->findKeyword("NAME");
              } else {
              QString message = "The input PVL does not contain a valid atmospheric model so you must specify one ";
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
          ui.Clear("NULNEG");
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
              QString nulneg = (QString)atmGrp->findKeyword("NULNEG");
              QString inulneg = nulneg;
              inulneg = inulneg.toUpper();
              if (inulneg == "YES") {
              ui.PutString("NULNEG", "YES");
              } else if (inulneg == "NO") {
              ui.PutString("NULNEG", "NO");
              } else {
              QString message = "The NULNEG value is invalid - must be set to YES or NO";
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
          QString message = "Unsupported atmospheric model [" + atmVal + "].";
          throw IException(IException::User, message, _FILEINFO_);
          }
          ui.PutAsString("ATMNAME", atmVal);
      }
      }
  }
}