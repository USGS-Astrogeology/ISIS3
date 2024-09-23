/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "ProcessByLine.h"
#include "SpecialPixel.h"
#include "iTime.h"
#include "IException.h"
#include "TextFile.h"
#include "LineManager.h"
#include "Brick.h"
#include "Table.h"
#include "UserInterface.h"
#include "Camera.h"
#include "NaifStatus.h"

#include "ctxcal.h"

using namespace std;
using namespace Isis;

namespace Isis {
    // Working functions and parameters
    static void Calibrate(Buffer &in, Buffer &out);

    static Brick *flat;
    static vector <double> dcA;
    static vector <double> dcB;
    static vector <double> dc;
    static double exposure;     // Exposure duration
    static int sum;             // Summing mode
    static int firstSamp;       // First sample
    static double iof;          // conversion from counts/ms to IOF

    void ctxcal(UserInterface &ui) {
      Cube icube(ui.GetCubeName("FROM").toStdString());
      ctxcal(&icube, ui);
    }

    void ctxcal(Cube *icube, UserInterface &ui) {
      // We will be processing by line
      ProcessByLine p;

      Isis::Pvl lab(icube->fileName().toStdString());
      Isis::PvlGroup &inst =
          lab.findGroup("Instrument", Pvl::Traverse);

      QString instId = QString::fromStdString(inst["InstrumentId"]);
      if(instId != "CTX") {
          std::string msg = "This is not a CTX image.  Ctxcal requires a CTX image.";
          throw IException(IException::User, msg, _FILEINFO_);
      }

      p.SetInputCube(icube, OneBand);

      Cube flatFile;
      if(ui.WasEntered("FLATFILE")) {
          flatFile.open(ui.GetCubeName("FLATFILE"));
      }
      else {
          FileName flat;
          if (ui.GetBoolean("MONTHLYFLAT")) {
              FileName outputFileName(icube->fileName().toStdString());
              QString outputFileBase = QString::fromStdString(outputFileName.baseName());

              QStringRef month(&outputFileBase, 0, 3);
              flat = FileName("$mro/calibration/ctxFlatFiles/" + month.toString().toStdString() + ".flat.cub");
              if (!flat.fileExists()) {
                std::string msg = "Could not find flat file [" + flat.expanded() + "]. "
                              "Either the data area is not set or a month is missing.";
                throw IException(IException::Unknown, msg, _FILEINFO_);
              }
          }
          else {
              flat = FileName("$mro/calibration/ctxFlat_????.cub").highestVersion();
          }
          flatFile.open(QString::fromStdString(flat.expanded()));
      }
      flat = new Brick(5000, 1, 1, flatFile.pixelType());
      flat->SetBasePosition(1, 1, 1);
      flatFile.read(*flat);

      // If it is already calibrated then complain
      if(icube->hasGroup("Radiometry")) {
          std::string msg = "The CTX image [" + icube->fileName().toStdString() + "] has already "
                      "been radiometrically calibrated";
          throw IException(IException::User, msg, _FILEINFO_);
      }

      // Get label parameters we will need for calibration equation
      iTime startTime(QString::fromStdString(inst["StartTime"]));
      double etStart = startTime.Et();

      //  Read exposure and convert to milliseconds
      exposure = inst["LineExposureDuration"];
      //exposure *= 1000.;

      sum = inst["SpatialSumming"];
      //  If firstSamp > 0, adjust by 38 to account for prefix pixels.
      firstSamp = inst["SampleFirstPixel"];
      if(firstSamp > 0) firstSamp -= 38;

      //  Read dark current info, if no dc exit?
      Table dcTable = icube->readTable("Ctx Prefix Dark Pixels");
      //  TODO::  make sure dc records match cube nlines.

      //  If summing mode = 1 , average odd & even dc pixels separately for
      //  a & b channels.
      //  If summing mode != 1, average all dc pixels and use for both


      for(int rec = 0; rec < dcTable.Records(); rec++) {
          vector<int> darks = dcTable[rec]["DarkPixels"];

          bool aChannel = true;
          double dcASum = 0.;
          double dcBSum = 0.;
          int dcACount = 0;
          int dcBCount = 0;

          double dcSum = 0;
          int dcCount = 0;

          for(int i = 0; i < (int)darks.size(); i++) {

          if(sum == 1) {
              if(aChannel == true) {
              dcASum += (double)darks.at(i);
              dcACount++;
              }
              else {
              dcBSum += (double)darks.at(i);
              dcBCount++;
              }
              aChannel = !aChannel;
          }
          else if(sum > 1) {
              dcSum += (double)darks.at(i);
              dcCount ++;
          }
          }
          if(sum == 1) {
          dcA.push_back(dcASum / (double)dcACount);
          dcB.push_back(dcBSum / (double)dcBCount);
          }
          else {
          dc.push_back(dcSum / (double)dcCount);
          }
      }

      // See if the user wants counts/ms or i/f
      //    iof = conversion factor from counts/ms to i/f
      bool convertIOF = ui.GetBoolean("IOF");
      if(convertIOF) {
        double dist1 = 1;
        try {
          Camera *cam;
          cam = icube->camera();
          cam->setTime(startTime);
          dist1 = cam->sunToBodyDist();
        }
        catch(IException &e) {
          // Get the distance between Mars and the Sun at the given time in
          // Astronomical Units (AU)
          QString bspKernel = p.MissionData("base", "/kernels/spk/de???.bsp", true);
          NaifStatus::CheckErrors();
          furnsh_c(bspKernel.toLatin1().data());
          NaifStatus::CheckErrors();
          QString satKernel = p.MissionData("base", "/kernels/spk/mar???.bsp", true);
          furnsh_c(satKernel.toLatin1().data());
          NaifStatus::CheckErrors();
          QString pckKernel = p.MissionData("base", "/kernels/pck/pck?????.tpc", true);
          furnsh_c(pckKernel.toLatin1().data());
          NaifStatus::CheckErrors();
          double sunpos[6], lt;

          spkezr_c("sun", etStart, "iau_mars", "LT+S", "mars", sunpos, &lt);
          NaifStatus::CheckErrors();

          dist1 = vnorm_c(sunpos);

          NaifStatus::CheckErrors();
          unload_c(bspKernel.toLatin1().data());
          unload_c(satKernel.toLatin1().data());
          unload_c(pckKernel.toLatin1().data());
          NaifStatus::CheckErrors();
        }

        double dist = 2.07E8;
        double w0 = 3660.5;
        double w1 = w0 * ((dist * dist) / (dist1 * dist1));
        if(exposure *w1 == 0.0) {
          std::string msg = icube->fileName().toStdString() + ": exposure or w1 has value of 0.0 ";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        iof = 1.0 / (exposure * w1);
      }
      else {
        iof = 1.0;
      }

      // Setup the output cube
      Cube *ocube = p.SetOutputCubeStretch("TO", &ui);

      // Add the radiometry group
      PvlGroup calgrp("Radiometry");

      calgrp += PvlKeyword("FlatFile", flatFile.fileName().toStdString());
      calgrp += PvlKeyword("iof", toString(iof));


      ocube->putGroup(calgrp);

      // Start the line-by-line calibration sequence
      p.StartProcess(Calibrate);
      p.EndProcess();
    }


    // Line processing routine
    static void Calibrate(Buffer &in, Buffer &out) {
    //  TODO::  Check for valid dc & flat

    double dark = 0.;
    if(sum != 1) {
        dark = dc.at(in.Line() - 1);
    }

    bool aChannel = true;
    // Loop and apply calibration
    for(int i = 0; i < in.size(); i++) {

        if(sum == 1) {
        if(aChannel == true) {
            dark = dcA.at(in.Line() - 1);
        }
        else {
            dark = dcB.at(in.Line() - 1);
        }
        aChannel = !aChannel;
        }

        //
        // Handle good pixels
        if(IsValidPixel(in[i])) {
        double flatPix;
        // Find correct flat correction.  If summing = 2, average correct
        // two flat pixels together.
        if(sum == 1) {
            flatPix = (*flat)[i+firstSamp];
        }
        else {
            flatPix = ((*flat)[i*2+firstSamp] + (*flat)[i*2+firstSamp+1]) / 2.;
        }

        if(iof == 1) {
            // compute r in counts/ms
            out[i] = (in[i] - dark) / (exposure * flatPix);

            // double r = (in[i] - dark) / (flatPix * exposure);

        }
        else {
            out[i] = ((in[i] - dark) / flatPix) * iof;
        }

        }

        // Handle special pixels
        else {
        out[i] = in[i];
        }
    }

    }
}