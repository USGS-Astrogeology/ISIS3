/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"
#include "Pipeline.h"

using namespace Isis;

void ProcessVis(bool isRdr);
void ProcessIr();

void IsisMain() {
  UserInterface &ui = Application::GetUserInterface();

  if(!ui.GetBoolean("INGESTION") && !ui.GetBoolean("MAPPING")) {
    std::string msg = "You must pick one of [INGESTION,MAPPING]";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  if(ui.WasEntered("PCK")) {
    FileName pckPreference = ui.GetFileName("PCK").toStdString();
    if(!pckPreference.fileExists()){
      std::string msg = "Please provide a valid PCK preference file.";
      throw IException(IException::User, msg, _FILEINFO_);
    }
  }

  if(ui.WasEntered("CK")) {
    FileName ckPreference = ui.GetFileName("CK").toStdString();
    if(!ckPreference.fileExists()){
      std::string msg = "Please provide a valid CK preference file.";
      throw IException(IException::User, msg, _FILEINFO_);
    }
  }

  if(ui.WasEntered("SPK")) {
    FileName spkPreference = ui.GetFileName("SPK").toStdString();
    if(!spkPreference.fileExists()){
      std::string msg = "Please provide a valid SPK preference file.";
      throw IException(IException::User, msg, _FILEINFO_);
    }
  }
  if(ui.WasEntered("MAP")) {
    FileName mapPreference = ui.GetFileName("MAP").toStdString();
    if(!mapPreference.fileExists()){
      std::string msg = "Please provide a valid MAP preference file.";
      throw IException(IException::User, msg, _FILEINFO_);
    }
  }
  if(ui.WasEntered("MODEL")) {
    FileName modelPreference = ui.GetCubeName("MODEL").toStdString();
    if(!modelPreference.fileExists()){
      std::string msg = "Please provide a valid MODEL preference file.";
      throw IException(IException::User, msg, _FILEINFO_);
    }
  }
  if(ui.GetBoolean("INGESTION")) {
    Pvl labels(ui.GetFileName("FROM").toStdString());

    if(QString::fromStdString(labels["DETECTOR_ID"][0]) == "VIS") {
      if((QString::fromStdString(labels["DATA_SET_ID"])).contains("RDR")) {
        ProcessVis(true);
      }
      else {
        ProcessVis(false);
      }
    }
    else {
      ProcessIr();
    }
  }
  else {
    ProcessIr();
  }
}

void ProcessVis(bool isRdr) {
  UserInterface &ui = Application::GetUserInterface();
  Pipeline p("thmproc");

  p.SetInputFile("FROM", "BANDS");
  p.SetOutputFile("TO");
  // Set continue to false so that we know if something fails
  p.SetContinue(false);
  p.KeepTemporaryFiles(!ui.GetBoolean("REMOVE"));

  p.AddToPipeline("thm2isis");
  p.Application("thm2isis").SetInputParameter("FROM", false);
  p.Application("thm2isis").SetOutputParameter("TO", "raw");
  p.Application("thm2isis").AddBranch("even", PipelineApplication::ConstantStrings);
  p.Application("thm2isis").AddBranch("odd", PipelineApplication::ConstantStrings);

  p.AddToPipeline("spiceinit");
  p.Application("spiceinit").SetInputParameter("FROM", false);
  p.Application("spiceinit").AddParameter("PCK", "PCK");
  p.Application("spiceinit").AddParameter("CK", "CK");
  p.Application("spiceinit").AddParameter("SPK", "SPK");
  p.Application("spiceinit").AddParameter("SHAPE", "SHAPE");
  p.Application("spiceinit").AddParameter("MODEL", "MODEL");
  p.Application("spiceinit").AddParameter("CKNADIR", "CKNADIR");

  p.AddToPipeline("thmvisflat");
  p.Application("thmvisflat").SetInputParameter("FROM", true);
  p.Application("thmvisflat").SetOutputParameter("TO", "flat");

  p.AddToPipeline("thmvistrim");
  p.Application("thmvistrim").SetInputParameter("FROM", true);
  p.Application("thmvistrim").SetOutputParameter("TO", "cal");

  if(!ui.GetBoolean("VISCLEANUP")) {
    p.Application("thmvisflat").Disable();
    p.Application("thmvistrim").Disable();
  }

  if(isRdr) {
    p.Application("thmvisflat").Disable();
  }

  p.AddToPipeline("cam2map");
  p.Application("cam2map").SetInputParameter("FROM", true);
  p.Application("cam2map").SetOutputParameter("TO", "lev2");

  p.Application("cam2map").AddParameter("even", "MAP", "MAP");
  p.Application("cam2map").AddParameter("even", "PIXRES", "RESOLUTION");

  if(ui.WasEntered("PIXRES")) {
    p.Application("cam2map").AddConstParameter("even", "PIXRES", "MPP");
  }

  p.Application("cam2map").AddParameter("odd", "MAP", PipelineApplication::LastOutput);
  p.Application("cam2map").AddConstParameter("odd", "PIXRES", "MAP");
  p.Application("cam2map").AddConstParameter("odd", "DEFAULTRANGE", "MAP");

  p.AddToPipeline("automos");
  p.Application("automos").SetInputParameter("FROMLIST", PipelineApplication::LastAppOutputList, false);
  p.Application("automos").SetOutputParameter("MOSAIC", "mos");

  p.SetFirstApplication("thm2isis");

  if(ui.GetBoolean("MAPPING")) {
    p.SetLastApplication("automos");
  }
  else {
    p.SetLastApplication("thmvistrim");
  }

  p.Run();
}

void ProcessIr() {
  UserInterface &ui = Application::GetUserInterface();
  Pipeline p;

  p.SetInputFile("FROM", "BANDS");
  p.SetOutputFile("TO");

  p.KeepTemporaryFiles(!ui.GetBoolean("REMOVE"));
  // Set continue to false so that we know if something fails
  p.SetContinue(false);

  p.AddToPipeline("thm2isis");
  p.Application("thm2isis").SetInputParameter("FROM", false);
  p.Application("thm2isis").SetOutputParameter("TO", "raw");

  p.AddToPipeline("spiceinit");
  p.Application("spiceinit").SetInputParameter("FROM", false);
  p.Application("spiceinit").AddParameter("PCK", "PCK");
  p.Application("spiceinit").AddParameter("CK", "CK");
  p.Application("spiceinit").AddParameter("SPK", "SPK");
  p.Application("spiceinit").AddParameter("SHAPE", "SHAPE");
  p.Application("spiceinit").AddParameter("MODEL", "MODEL");
  p.Application("spiceinit").AddParameter("CKNADIR", "CKNADIR");

  p.AddToPipeline("cam2map");
  p.Application("cam2map").SetInputParameter("FROM", true);
  p.Application("cam2map").SetOutputParameter("TO", "lev2");
  p.Application("cam2map").AddParameter("MAP", "MAP");
  p.Application("cam2map").AddParameter("PIXRES", "RESOLUTION");

  if(ui.WasEntered("PIXRES")) {
    p.Application("cam2map").AddConstParameter("PIXRES", "MPP");
  }

  if(ui.GetBoolean("INGESTION")) {
    p.SetFirstApplication("thm2isis");
  }
  else {
    p.SetFirstApplication("spiceinit");
  }

  if(ui.GetBoolean("MAPPING")) {
    p.SetLastApplication("cam2map");
  }
  else {
    p.SetLastApplication("spiceinit");
  }

  p.Run();
}
