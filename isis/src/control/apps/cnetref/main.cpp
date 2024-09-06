/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#define GUIHELPERS // should always be the first command

#include "Isis.h"

#include "CnetRefByEmission.h"
#include "CnetRefByIncidence.h"
#include "CnetRefByResolution.h"
#include "ControlNet.h"
#include "ControlNetStatistics.h"
#include "GuiEditFile.h"
#include "InterestOperatorFactory.h"
#include "InterestOperator.h"
#include "IException.h"
#include "IString.h"
#include "Progress.h"
#include "Pvl.h"


using namespace std;
using namespace Isis;

void ViewDefFile();
void EditDefFile();

map <QString, void *> GuiHelpers() {
  map <QString, void *> helper;
  helper ["View"] = (void *)ViewDefFile;
  helper ["Edit"] = (void *)EditDefFile;
  return helper;
}

ResolutionType GetResolutionType(QString psType);

void IsisMain() {

  try {
    UserInterface &ui = Application::GetUserInterface();
    QString sSerialNumFile = ui.GetFileName("FROMLIST");

    // get the Criteria option
    QString sCriteria = ui.GetString("CRITERIA");

    // Check format of Pvl DefFile
    bool bDefFile = false;
    Pvl *pvlDefFile = 0;
    Pvl pvlTemplate, pvlResults;
    if (ui.WasEntered("DEFFILE")) {
      bDefFile = true;
      pvlDefFile = new Pvl(ui.GetFileName("DEFFILE").toStdString());

      // Log the DefFile
      Application::Log(pvlDefFile->group(0));

      if (pvlDefFile->group(0).hasKeyword("PixelsFromEdge") && pvlDefFile->group(0).hasKeyword("MetersFromEdge")) {
        std::string message = "DefFile Error : Cannot have both \"PixelsFromEdge\" && \"MetersFromEdge\"" ;
        throw IException(IException::User, message, _FILEINFO_);
      }

      Pvl pvlTemplate, pvlResults;
      if (sCriteria == "INTEREST") {
        pvlTemplate = Pvl("$ISISROOT/appdata/templates/cnetref/cnetref_operator.def");
      }
      else {
        pvlTemplate = Pvl("$ISISROOT/appdata/templates/cnetref/cnetref_nooperator.def");
      }
      pvlTemplate.validatePvl(*pvlDefFile, pvlResults);
      if (pvlResults.groups() > 0 || pvlResults.keywords() > 0) {
        Application::Log(pvlResults.group(0));
        QString sErrMsg = "Invalid Deffile\n";
        throw IException(IException::User, sErrMsg, _FILEINFO_);
      }
    }

    // Get the original control net internalized
    Progress progress;
    ControlNet cNet(ui.GetFileName("CNET"), &progress);

    if (ui.WasEntered("NETWORKID")) {
      cNet.SetNetworkId(ui.GetString("NETWORKID"));
    }

    cNet.SetUserName(Isis::Application::UserName());

    if (ui.WasEntered("DESCRIPTION")) {
      cNet.SetDescription(ui.GetString("DESCRIPTION"));
    }

    // Get the output Log file
    bool bLogFile = false;
    QString sLogFile;
    if (ui.WasEntered("LOG")) {
      sLogFile = ui.GetFileName("LOG");
      bLogFile = true;
    }

    // get the Criteria option
    ControlNetValidMeasure *cnetValidMeas = NULL;

    // Process Reference by Emission Angle
    if (sCriteria == "EMISSION") {
      cnetValidMeas = new CnetRefByEmission(pvlDefFile, sSerialNumFile);
      cnetValidMeas->FindCnetRef(cNet);
    }

    // Process Reference by Incidence Angle
    else if (sCriteria == "INCIDENCE") {
      cnetValidMeas = new CnetRefByIncidence(pvlDefFile, sSerialNumFile);
      cnetValidMeas->FindCnetRef(cNet);
    }

    // Process Reference by Resolution
    else if (sCriteria == "RESOLUTION") {
      QString sType = ui.GetString("TYPE");
      double dResValue = 0;
      double dMinRes = 0, dMaxRes = 0;
      if (sType == "NEAREST") {
        dResValue = ui.GetDouble("RESVALUE");
        if (dResValue < 0) {
          std::string message = "Invalid Nearest Resolution Value";
          throw IException(IException::User, message, _FILEINFO_);
        }
      }
      else if (sType == "RANGE") {
        dMinRes = ui.GetDouble("MINRES");
        dMaxRes = ui.GetDouble("MAXRES");
        if (dMinRes < 0 || dMaxRes < 0 || dMinRes > dMaxRes) {
          std::string message = "Invalid Resolution Range";
          throw IException(IException::User, message, _FILEINFO_);
        }
      }
      cnetValidMeas = new CnetRefByResolution(pvlDefFile, sSerialNumFile, GetResolutionType(sType), dResValue, dMinRes, dMaxRes);
      cnetValidMeas->FindCnetRef(cNet);
    }

    // Process Reference by Interest
    else if (sCriteria == "INTEREST") {
      if (!bDefFile) {
        std::string msg = "Interest Option must have a DefFile";
        throw IException(IException::User, msg, _FILEINFO_);
      }
      QString sOverlapListFile = "";
      if (ui.WasEntered("LIMIT")) {
        if (ui.GetBoolean("LIMIT")) {
          sOverlapListFile = FileName(ui.GetFileName("OVERLAPLIST")).expanded();
        }
      }

      // Get the InterestOperator set up
      InterestOperator *interestOp = InterestOperatorFactory::Create(*pvlDefFile);
      interestOp->Operate(cNet, sSerialNumFile, sOverlapListFile);

      // Write to print.prt and screen interest details
      // add operator to print.prt
      PvlGroup opGroup = interestOp->Operator();
      Application::Log(opGroup);
      if (bLogFile) {
        Pvl pvlLog = interestOp->GetLogPvl();
        pvlLog += opGroup;
        pvlLog.write(sLogFile.toStdString());
      }
      Application::Log(interestOp->GetStdOptions());
      Application::Log(interestOp->GetStatistics());
    }

    // Write the new control network out
    cNet.Write(ui.GetFileName("ONET"));

    // Get Control Net Stats Summary
    PvlGroup statsGrp;
    ControlNetStatistics cnetStats(&cNet);
    cnetStats.GenerateControlNetStats(statsGrp);
    Application::Log(statsGrp);

    if (cnetValidMeas) {
      Pvl pvlLog = cnetValidMeas->GetLogPvl();
      if (bLogFile) {
        pvlLog.write(sLogFile.toStdString());
      }
      Application::Log(cnetValidMeas->GetStdOptions());
      Application::Log(cnetValidMeas->GetStatistics());

      // clean up
      delete cnetValidMeas;
    }

  } // REFORMAT THESE ERRORS INTO ISIS TYPES AND RETHROW
  catch (IException &e) {
    throw;
  }
  catch (geos::util::GEOSException *exc) {
    std::string message = "GEOS Exception: " + (QString)exc->what();
    delete exc;
    throw IException(IException::User, message, _FILEINFO_);
  }
  catch (std::exception const &se) {
    std::string message = "std::exception: " + (QString)se.what();
    throw IException(IException::User, message, _FILEINFO_);
  }
  catch (...) {
    std::string message = "Other Error";
    throw IException(IException::User, message, _FILEINFO_);
  }
}

/**
 * Return the enumerated ResolutionType for a given QString
 *
 * @author Sharmila Prasad (6/4/2010)
 *
 * @param psType
 *
 * @return ResolutionType
 */
ResolutionType GetResolutionType(QString psType) {
  if (psType == "LOW")
    return Low;
  else if (psType == "HIGH")
    return High;
  else if (psType == "MEAN")
    return Mean;
  else if (psType == "NEAREST")
    return Nearest;
  else if (psType == "RANGE")
    return Range;

  return High;
}

/**
 * Helper function to print out template to log.
 *
 * @author Sharmila Prasad (5/24/2011)
 */
void ViewDefFile() {
  UserInterface &ui = Application::GetUserInterface();

  // Get template PVL
  Pvl defFile;
  defFile.read(ui.GetFileName("DEFFILE").toStdString());

  // Write deffile file out to the log
  Isis::Application::GuiLog(defFile);
}

/**
 * Helper function to be able to edit the Deffile.
 * Opens an editor to edit the file.
 *
 * @author Sharmila Prasad (5/23/2011)
 */
void EditDefFile(void) {
  UserInterface &ui = Application::GetUserInterface();
  QString sDefFile = ui.GetAsString("DEFFILE");

  GuiEditFile::EditFile(ui, sDefFile);
}
