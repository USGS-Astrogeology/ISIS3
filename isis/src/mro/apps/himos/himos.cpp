/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "himos.h"

#include "Camera.h"
#include "Cube.h"
#include "FileList.h"
#include "IException.h"
#include "Pvl.h"
#include "IString.h"
#include "Longitude.h"
#include "OriginalLabel.h"
#include "Process.h"
#include "ProgramLauncher.h"
#include "TProjection.h"

#include <vector>

using namespace std;

namespace Isis {
  //functions in the code
  static void CompareLabels(Pvl &match, Pvl &comp);


  void himos(UserInterface &ui) {

    // Get the list of cubes to mosaic
    FileList flist(ui.GetFileName("FROMLIST"));


    vector<Cube *> clist;
    try {
      if(flist.size() < 1) {
        std::string msg = "the list file [" + ui.GetFileName("FROMLIST") +
                      "does not contain any data";
        throw IException(IException::User, msg, _FILEINFO_);
      }

      // open all the cube and place in vector clist
      for(int i = 0; i < flist.size(); i++) {
        Cube *c = new Cube();
        clist.push_back(c);
        c->open(flist[i].toString());
      }

      // run the compair function here.  This will conpair the
      // labels of the first cube to the labels of each following cube.
      PvlKeyword sourceProductId("SourceProductId");
      QString ProdId;
      for(int i = 0; i < (int)clist.size(); i++) {
        Pvl *pmatch = clist[0]->label();
        Pvl *pcomp = clist[i]->label();
        CompareLabels(*pmatch, *pcomp);
        PvlGroup g = pcomp->findGroup("Instrument", Pvl::Traverse);
        if(g.hasKeyword("StitchedProductIds")) {
          PvlKeyword k = g["StitchedProductIds"];
          for(int j = 0; j < (int)k.size(); j++) {
            sourceProductId += g["stitchedProductIds"][j];
          }
        }
        ProdId = QString::fromStdString(pmatch->findGroup("Archive", Pvl::Traverse)["ObservationId"]);
        QString bandname = QString::fromStdString(pmatch->findGroup("BandBin", Pvl::Traverse)["Name"]);
        bandname = bandname.toUpper();
        ProdId = ProdId + "_" + bandname;
      }
      bool runXY = true;

      //calculate the min and max lon
      double minLat = DBL_MAX;
      double maxLat = -DBL_MAX;
      double minLon = DBL_MAX;
      double maxLon = -DBL_MAX;
      double avgLat;
      double avgLon;
      for(int i = 0; i < (int)clist.size(); i++) {
        TProjection *proj = (TProjection *) clist[i]->projection();
        if(proj->MinimumLatitude() < minLat) minLat = proj->MinimumLatitude();
        if(proj->MaximumLatitude() > maxLat) maxLat = proj->MaximumLatitude();
        if(proj->MinimumLongitude() < minLon) minLon = proj->MinimumLongitude();
        if(proj->MaximumLongitude() > maxLon) maxLon = proj->MaximumLongitude();
      }
      avgLat = (minLat + maxLat) / 2;
      avgLon = (minLon + maxLon) / 2;
      TProjection *proj = (TProjection *) clist[0]->projection();
      proj->SetGround(avgLat, avgLon);
      avgLat = proj->UniversalLatitude();
      avgLon = proj->UniversalLongitude();

      // Use camera class to get Inc., emi., phase, and other values
      double Cemiss;
      double Cphase;
      double Cincid;
      double ClocalSolTime;
      double CsolarLong;
      double CsunAzimuth;
      double CnorthAzimuth;
      for(int i = 0; i < (int)clist.size(); i++) {
        Camera *cam = clist[i]->camera();
        if(cam->SetUniversalGround(avgLat, avgLon)) {
          Cemiss = cam->EmissionAngle();
          Cphase = cam->PhaseAngle();
          Cincid = cam->IncidenceAngle();
          ClocalSolTime = cam->LocalSolarTime();
          CsolarLong = cam->solarLongitude().degrees();
          CsunAzimuth = cam->SunAzimuth();
          CnorthAzimuth = cam->NorthAzimuth();
          runXY = false;
          break;
        }
      }

      //The code within the if runXY was added in 10/07 to find an intersect with
      //pole images that would fail when using projection set universal ground.
      // This is run if no intersect is found when using lat and lon in
      // projection space.
      if(runXY) {
        double startX = DBL_MAX;
        double endX = DBL_MIN;
        double startY = DBL_MAX;
        double endY =  DBL_MIN;
        for(int i = 0; i < (int)clist.size(); i++) {
          TProjection *proj = (TProjection *) clist[i]->projection();
          proj->SetWorld(0.5, 0.5);
          if(i == 0) {
            startX = proj->XCoord();
            endY = proj->YCoord();
          }
          else {
            if(proj->XCoord() < startX) startX =  proj->XCoord();
            if(proj->YCoord() > endY) endY = proj->YCoord();
          }
          Pvl *p = clist[i]->label();
          double nlines = p->findGroup("Dimensions", Pvl::Traverse)["Lines"];
          double nsamps = p->findGroup("Dimensions", Pvl::Traverse)["Samples"];

          proj->SetWorld((nsamps + 0.5), (nlines + 0.5));
          if(i == 0) {
            endX = proj->XCoord();
            startY = proj->YCoord();
          }
          else {
            if(proj->XCoord() > endX) endX =  proj->XCoord();
            if(proj->YCoord() < startY) startY = proj->YCoord();
          }
        }

        double avgX = (startX + endX) / 2;
        double avgY = (startY + endY) / 2;
        double sample = proj->ToWorldX(avgX);
        double line = proj->ToWorldY(avgY);

        for(int i = 0; i < (int)clist.size(); i++) {
          Camera *cam = clist[i]->camera();
          if(cam->SetImage(sample, line)) {
            Cemiss = cam->EmissionAngle();
            Cphase = cam->PhaseAngle();
            Cincid = cam->IncidenceAngle();
            ClocalSolTime = cam->LocalSolarTime();
            CsolarLong = cam->solarLongitude().degrees();
            CsunAzimuth = cam->SunAzimuth();
            CnorthAzimuth = cam->NorthAzimuth();
            runXY = false;
            break;
          }
        }
      }
      if(runXY) {
        std::string msg = "Camera did not intersect images to gather stats";
        throw IException(IException::User, msg, _FILEINFO_);
      }

      // get the min and max SCLK values ( do this with QString comp.)
      // get the value from the original label blob
      QString startClock;
      QString stopClock;
      QString startTime;
      QString stopTime;
      for(int i = 0; i < (int)clist.size(); i++) {
        OriginalLabel origLab = clist[i]->readOriginalLabel();
        PvlGroup timegrp = origLab.ReturnLabels().findGroup("TIME_PARAMETERS", Pvl::Traverse);
        if(i == 0) {
          startClock = QString::fromStdString(timegrp["SpacecraftClockStartCount"]);
          stopClock = QString::fromStdString(timegrp["SpacecraftClockStopCount"]);
          startTime = QString::fromStdString(timegrp["StartTime"]);
          stopTime = QString::fromStdString(timegrp["StopTime"]);
        }
        else {
          QString testStartTime = QString::fromStdString(timegrp["StartTime"]);
          QString testStopTime = QString::fromStdString(timegrp["StopTime"]);
          if(testStartTime < startTime) {
            startTime = testStartTime;
            startClock = QString::fromStdString(timegrp["SpacecraftClockStartCount"]);
          }
          if(testStopTime > stopTime) {
            stopTime = testStopTime;
            stopClock = QString::fromStdString(timegrp["spacecraftClockStopCount"]);
          }
        }
      }

      //  Concatenate all TDI's and summing and specialProcessingFlat into one keyword
      PvlKeyword cpmmTdiFlag("cpmmTdiFlag");
      PvlKeyword cpmmSummingFlag("cpmmSummingFlag");
      PvlKeyword specialProcessingFlag("SpecialProcessingFlag");
      for(int i = 0; i < 14; i++) {
        cpmmTdiFlag += "";
        cpmmSummingFlag += "";
        specialProcessingFlag += "";
      }

      for(int i = 0; i < (int)clist.size(); i++) {
        Pvl *clab = clist[i]->label();
        PvlGroup cInst = clab->findGroup("Instrument", Pvl::Traverse);
        OriginalLabel cOrgLab = clist[i]->readOriginalLabel();
        PvlGroup cGrp = cOrgLab.ReturnLabels().findGroup("INSTRUMENT_SETTING_PARAMETERS", Pvl::Traverse);
        cpmmTdiFlag[(int)cInst["CpmmNumber"]] = (std::string)cGrp["MRO:TDI"];
        cpmmSummingFlag[(int)cInst["CpmmNumber"]] = (std::string)cGrp["MRO:BINNING"];

        if(cInst.hasKeyword("Special_Processing_Flag")) {
          specialProcessingFlag[cInst["CpmmNumber"]] = (std::string)cInst["Special_Processing_Flag"];
        }
        else {
          // there may not be the keyword Special_Processing_Flag if no
          //keyword then set the output to NOMINAL
          specialProcessingFlag[cInst["CpmmNumber"]] = "NOMINAL";
        }
      }

      // Get the blob of original labels from first image in list
      OriginalLabel org = clist[0]->readOriginalLabel();

      //close all cubes
      for(int i = 0; i < (int)clist.size(); i++) {
        clist[i]->close();
        delete clist[i];
      }
      clist.clear();

      // automos step
      QString list = ui.GetFileName("FROMLIST");
      QString toMosaic = ui.GetCubeName("TO");
      QString MosaicPriority = ui.GetString("PRIORITY");

      QString parameters = "FROMLIST=" + list + " MOSAIC=" + toMosaic + " PRIORITY=" + MosaicPriority;
      ProgramLauncher::RunIsisProgram("automos", parameters);

      // write out new information to new group mosaic
      PvlGroup mos("Mosaic");
      mos += PvlKeyword("ProductId ", ProdId.toStdString());
      mos += PvlKeyword(sourceProductId);
      mos += PvlKeyword("StartTime ", startTime.toStdString());
      mos += PvlKeyword("SpacecraftClockStartCount ", startClock.toStdString());
      mos += PvlKeyword("StopTime ", stopTime.toStdString());
      mos += PvlKeyword("SpacecraftClockStopCount ", stopClock.toStdString());
      mos += PvlKeyword("IncidenceAngle ", std::to_string(Cincid), "DEG");
      mos += PvlKeyword("EmissionAngle ", std::to_string(Cemiss), "DEG");
      mos += PvlKeyword("PhaseAngle ", std::to_string(Cphase), "DEG");
      mos += PvlKeyword("LocalTime ", std::to_string(ClocalSolTime), "LOCALDAY/24");
      mos += PvlKeyword("SolarLongitude ", std::to_string(CsolarLong), "DEG");
      mos += PvlKeyword("SubSolarAzimuth ", std::to_string(CsunAzimuth), "DEG");
      mos += PvlKeyword("NorthAzimuth ", std::to_string(CnorthAzimuth), "DEG");
      mos += cpmmTdiFlag;
      mos += cpmmSummingFlag;
      mos += specialProcessingFlag;

      Cube mosCube;
      mosCube.open(ui.GetCubeName("TO"), "rw");
      PvlObject &lab = mosCube.label()->findObject("IsisCube");
      lab.addGroup(mos);
      //add orginal label blob to the output cube
      mosCube.write(org);
      mosCube.close();

    }
    catch(IException &e) {
      for(int i = 0; i < (int)clist.size(); i++) {
        clist[i]->close();
        delete clist[i];
      }
      std::cout << e.what() << '\n';
      std::string msg = "The mosaic [" + ui.GetCubeName("TO") + "] was NOT created";
      throw IException(IException::User, msg, _FILEINFO_);
    }
  } // end of isis main

  //Function to compare label - CompareLabels
  void CompareLabels(Pvl &pmatch, Pvl &pcomp) {
    // test of the ObservationId
    PvlGroup matchgrp = pmatch.findGroup("Archive", Pvl::Traverse);
    PvlGroup compgrp = pcomp.findGroup("Archive", Pvl::Traverse);
    QString obsMatch = QString::fromStdString(matchgrp["ObservationId"]);
    QString obsComp = QString::fromStdString(compgrp["ObservationId"]);

    if(obsMatch != obsComp) {
      std::string msg = "Images not from the same observation";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    // Test of the BandBin filter name
    PvlGroup bmatchgrp = pmatch.findGroup("BandBin", Pvl::Traverse);
    PvlGroup bcompgrp = pcomp.findGroup("BandBin", Pvl::Traverse);
    QString bandMatch = QString::fromStdString(bmatchgrp["Name"]);
    QString bandComp = QString::fromStdString(bcompgrp["Name"]);

    if(bandMatch != bandComp) {
      std::string msg = "Images not the same filter";
      throw IException(IException::User, msg, _FILEINFO_);
    }
  }
}
