/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

#include <iomanip>

#include "Brick.h"
#include "ControlNet.h"
#include "ControlPoint.h"
#include "Cube.h"
#include "FileName.h"
#include "IException.h"
#include "Interpolator.h"
#include "Latitude.h"
#include "Longitude.h"
#include "Portal.h"
#include "SpecialPixel.h"
#include "SurfacePoint.h"
#include "UniversalGroundMap.h"

using namespace std;
using namespace Isis;

enum GetLatLon { Adjusted, Apriori };

void IsisMain() {
  UserInterface &ui = Application::GetUserInterface();
  ControlNet cnet(ui.GetFileName("CNET"));

  // Get input DEM cube and get ground map for it
  QString demFile = ui.GetCubeName("MODEL");
  Cube demCube;
  demCube.open(demFile);
  UniversalGroundMap *ugm = NULL;
  try {
    ugm = new UniversalGroundMap(demCube);
  }
  catch (IException &e) {
    IString msg = "Cannot initalize UniversalGroundMap for DEM cube [" +
                   demFile.toStdString() + "]";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  //  Use bilinear interpolation to read radius from DEM
  //   Use bilinear interpolation from dem
  Interpolator *interp = new Interpolator(Interpolator::BiLinearType);;

  //   Buffer used to read from the model
  Portal *portal = new Portal(interp->Samples(), interp->Lines(),
                              demCube.pixelType(),
                              interp->HotSample(), interp->HotLine());;


  GetLatLon newRadiiSource;
  IString getLatLon = IString(ui.GetAsString("GETLATLON").toStdString()).UpCase();
  if (getLatLon == "ADJUSTED") {
    newRadiiSource = Adjusted;
  }
  else if (getLatLon == "APRIORI") {
    newRadiiSource = Apriori;
  }
  else {
    std::string msg = "The value for parameter GETLATLON [";
    msg += ui.GetAsString("GETLATLON").toStdString() + "] must be provided.";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  int numSuccesses = 0;
  int numFailures = 0;
  int numConstrainedFixed = 0;
  int numLocked = 0;
  QString failedIDs = "";

  for (int i = 0; i < cnet.GetNumPoints() ; i++) {
    ControlPoint *cp = cnet.GetPoint(i);

    if (cp->GetType() != ControlPoint::Free) {
      numConstrainedFixed++;

      if (cp->IsEditLocked()) {
        numLocked++;
        continue;
      }
      // Create Brick on samp, line to get the dn value of the pixel
      SurfacePoint surfacePt;
      if (newRadiiSource == Adjusted)
        surfacePt = cp->GetAdjustedSurfacePoint();
      else if (newRadiiSource == Apriori)
        surfacePt = cp->GetAprioriSurfacePoint();
      bool success = surfacePt.Valid();

      if (success) {
        success = ugm->SetUniversalGround(surfacePt.GetLatitude().degrees(),
                                          surfacePt.GetLongitude().degrees());
      }

      double radius = 0.;
      if (success) {
        portal->SetPosition(ugm->Sample(), ugm->Line(), 1);
        demCube.read(*portal);
        radius = interp->Interpolate(ugm->Sample(), ugm->Line(),
                                     portal->DoubleBuffer());
        success = !IsSpecial(radius);
      }

      // if we are unable to calculate a valid Radius value for a point,
      // we will ignore this point and keep track of it
      if (!success) {
        numFailures++;
        if (numFailures > 1) {
          failedIDs = failedIDs + ", ";
        }
        failedIDs = failedIDs + cp->GetId();
        cp->SetIgnored(true);
      }
      // otherwise, we will replace the computed radius value to the output control net
      else {
        numSuccesses++;
        surfacePt.ResetLocalRadius(Distance(radius, Distance::Meters));
        if (newRadiiSource == Adjusted) {
          cp->SetAdjustedSurfacePoint(surfacePt);
        }
        else if (newRadiiSource == Apriori) {
          cp->SetAprioriSurfacePoint(surfacePt);
          // Update radius source and radius source file
          cp->SetAprioriRadiusSource(ControlPoint::RadiusSource::DEM);
          cp->SetAprioriRadiusSourceFile(demFile);
        }
      }
    }
  }

  delete ugm;
  ugm = NULL;
  delete interp;
  interp = NULL;
  delete portal;
  portal = NULL;

  if (numSuccesses == 0) {
    if (numConstrainedFixed == 0) {
      std::string msg = "There were no Fixed or Constrained points in this network."
          "  No radii were replaced.";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    else {
      std::string msg = "No valid radii can be calculated. Verify that the DEM [" +
                       ui.GetAsString("MODEL").toStdString() + "] is valid.";
      throw IException(IException::User, msg, _FILEINFO_);
    }
  }

  cnet.Write(ui.GetFileName("ONET"));

  // Write results to Logs
  // Summary group is created with the counts of successes and failures
  PvlGroup summaryGroup = PvlGroup("Summary");
  summaryGroup.addKeyword(PvlKeyword("Successes", std::to_string(numSuccesses)));
  summaryGroup.addKeyword(PvlKeyword("Failures", std::to_string(numFailures)));
  summaryGroup.addKeyword(PvlKeyword("NumberFixedConstrainedPoints",
                                      std::to_string(numConstrainedFixed)));
  summaryGroup.addKeyword(PvlKeyword("NumberEditLockedPoints", std::to_string(numLocked)));

  bool errorlog;
  FileName errorlogFile;
  // if a filename was entered, use it to create the log
  if (ui.WasEntered("ERRORS")) {
    errorlog = true;
    errorlogFile = ui.GetFileName("ERRORS").toStdString();
  }
  // if no filename was entered, but there were some failures,
  // create an error log named "failures" in the current directory
  else if (numFailures > 0) {
    errorlog = true;
    errorlogFile = "failures.log";
  }
  // if all radii are successfully calculated and no error
  // file is named, only output summary to application log
  else {
    errorlog = false;
  }

  if (errorlog) {
    // Write details in error log
    Pvl results;
    results.setName("Results");
    results.addGroup(summaryGroup);
    if (numFailures > 0) {
      // if there are any failures, add comment to the summary log to alert user
      summaryGroup.addComment("Unable to calculate radius for all points. Point"
              " IDs for failures contained in [" + errorlogFile.name() + "].");
      PvlGroup failGroup = PvlGroup("Failures");
      failGroup.addComment("A point fails if we are unable to set universal "
               "ground or if the radius calculated is a special pixel value.");
      failGroup.addKeyword(PvlKeyword("PointIDs", failedIDs.toStdString()));
      results.addGroup(failGroup);
    }
    results.write(errorlogFile.expanded());
  }
  // Write summary to application log
  Application::Log(summaryGroup);
}
