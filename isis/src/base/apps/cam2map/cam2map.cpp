#include "cam2map.h"

#include "Application.h"
#include "Camera.h"
#include "CubeAttribute.h"
#include "IException.h"
#include "IString.h"
#include "ProjectionFactory.h"
#include "PushFrameCameraDetectorMap.h"
#include "Pvl.h"
#include "Target.h"
#include "TProjection.h"

using namespace std;

namespace Isis {

  // Global variables
  void bandChange(const int band);
  Cube *icube;
  Camera *incam;

  void cam2map(UserInterface &ui, Pvl *log) {
    // Open the input cube
    Cube icube;
    CubeAttributeInput inAtt = ui.GetInputAttribute("FROM");
    if (inAtt.bands().size() != 0) {
      icube.setVirtualBands(inAtt.bands());
    }
    icube.open(ui.GetCubeName("FROM"));

    // Get the map projection file provided by the user
    Pvl userMap;
    userMap.read(ui.GetFileName("MAP").toStdString());
    PvlGroup &userGrp = userMap.findGroup("Mapping", Pvl::Traverse);

    cam2map(&icube, userMap, userGrp, ui, log);
  }


  void cam2map(Cube *icube, Pvl &userMap, PvlGroup &userGrp, UserInterface &ui, Pvl *log) {
    ProcessRubberSheet p;
    cam2map(icube, userMap, userGrp, p, ui, log);
  }


  void cam2map(Cube *icube, Pvl &userMap, PvlGroup &userGrp, ProcessRubberSheet &p,
                UserInterface &ui, Pvl *log){

    // Get the camera from the input cube
    p.SetInputCube(icube);
    incam = icube->camera();

    // Make sure it is not the sky
    if (incam->target()->isSky()) {
      std::string msg = "The image [" + ui.GetCubeName("FROM") +
                    "] is targeting the sky, use skymap instead.";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    // Get the mapping grop
    Pvl camMap;
    incam->BasicMapping(camMap);
    PvlGroup &camGrp = camMap.findGroup("Mapping");

    // Make the target info match the user mapfile
    double minlat, maxlat, minlon, maxlon;
    incam->GroundRange(minlat, maxlat, minlon, maxlon, userMap);
    camGrp.addKeyword(PvlKeyword("MinimumLatitude", std::to_string(minlat)), Pvl::Replace);
    camGrp.addKeyword(PvlKeyword("MaximumLatitude", std::to_string(maxlat)), Pvl::Replace);
    camGrp.addKeyword(PvlKeyword("MinimumLongitude", std::to_string(minlon)), Pvl::Replace);
    camGrp.addKeyword(PvlKeyword("MaximumLongitude", std::to_string(maxlon)), Pvl::Replace);


    // We want to delete the keywords we just added if the user wants the range
    // out of the mapfile, otherwise they will replace any keywords not in the
    // mapfile

    // Use the updated label to create the output projection
    int samples, lines;
    TProjection *outmap = NULL;
    bool trim = ui.GetBoolean("TRIM");
    bool occlusion = ui.GetBoolean("OCCLUSION");

    // Make sure the target name of the input cube and map file match.
    if (userGrp.hasKeyword("TargetName") && !icube->group("Instrument").findKeyword("TargetName").isNull()) {
      if (!PvlKeyword::stringEqual(incam->target()->name().toStdString(), userGrp.findKeyword("TargetName")[0])) {
        std::string msg = "The TargetName: [" + incam->target()->name().toStdString() + "] of the input cube: [" + icube->fileName().toStdString() +
                      "] does not match the TargetName: [" + userGrp.findKeyword("TargetName")[0] + "] of the map file: [" +
                      ui.GetFileName("MAP").toStdString() + "].";
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }

    if ( !ui.GetBoolean("MATCHMAP") ) {
      if (ui.GetString("DEFAULTRANGE") == "MAP") {
        camGrp.deleteKeyword("MinimumLatitude");
        camGrp.deleteKeyword("MaximumLatitude");
        camGrp.deleteKeyword("MinimumLongitude");
        camGrp.deleteKeyword("MaximumLongitude");
      }
      // Otherwise, remove the keywords from the map file so the camera keywords
      // will be propogated correctly
      else {
        while(userGrp.hasKeyword("MinimumLatitude")) {
          userGrp.deleteKeyword("MinimumLatitude");
        }
        while(userGrp.hasKeyword("MinimumLongitude")) {
          userGrp.deleteKeyword("MinimumLongitude");
        }
        while(userGrp.hasKeyword("MaximumLatitude")) {
          userGrp.deleteKeyword("MaximumLatitude");
        }
        while(userGrp.hasKeyword("MaximumLongitude")) {
          userGrp.deleteKeyword("MaximumLongitude");
        }
      }

      // If the user decided to enter a ground range then override
      if ( ui.WasEntered("MINLON") ) {
        userGrp.addKeyword(PvlKeyword("MinimumLongitude",
                                      std::to_string(ui.GetDouble("MINLON"))), Pvl::Replace);
      }

      if ( ui.WasEntered("MAXLON") ) {
        userGrp.addKeyword(PvlKeyword("MaximumLongitude",
                                      std::to_string(ui.GetDouble("MAXLON"))), Pvl::Replace);
      }

      if ( ui.WasEntered("MINLAT") ) {
        userGrp.addKeyword(PvlKeyword("MinimumLatitude",
                                      std::to_string(ui.GetDouble("MINLAT"))), Pvl::Replace);
      }

      if ( ui.WasEntered("MAXLAT") ) {
        userGrp.addKeyword(PvlKeyword("MaximumLatitude",
                                      std::to_string(ui.GetDouble("MAXLAT"))), Pvl::Replace);
      }

      // If they want the res. from the mapfile, delete it from the camera so
      // nothing gets overriden
      if (ui.GetString("PIXRES") == "MAP") {
        camGrp.deleteKeyword("PixelResolution");
      }
      // Otherwise, delete any resolution keywords from the mapfile so the camera
      // info is propogated over
      else if (ui.GetString("PIXRES") == "CAMERA") {
        if (userGrp.hasKeyword("Scale")) {
          userGrp.deleteKeyword("Scale");
        }
        if (userGrp.hasKeyword("PixelResolution")) {
          userGrp.deleteKeyword("PixelResolution");
        }
      }

      // Copy any defaults that are not in the user map from the camera map file
      for (int k = 0; k < camGrp.keywords(); k++) {
        if (!userGrp.hasKeyword(camGrp[k].name())) {
          userGrp += camGrp[k];
        }
      }

      // If the user decided to enter a resolution then override
      if (ui.WasEntered("PIXRES")) {
        if (ui.GetString("PIXRES") == "MPP") {
          userGrp.addKeyword(PvlKeyword("PixelResolution",
                                        std::to_string(ui.GetDouble("RESOLUTION"))),
                            Pvl::Replace);
          if (userGrp.hasKeyword("Scale")) {
            userGrp.deleteKeyword("Scale");
          }
        }
        else if (ui.GetString("PIXRES") == "PPD") {
          userGrp.addKeyword(PvlKeyword("Scale",
                                        std::to_string(ui.GetDouble("RESOLUTION"))),
                            Pvl::Replace);
          if (userGrp.hasKeyword("PixelResolution")) {
            userGrp.deleteKeyword("PixelResolution");
          }
        }
      }

      // See if the user want us to handle the longitude seam


      if ( (ui.GetString("DEFAULTRANGE") == "CAMERA" || ui.GetString("DEFAULTRANGE") == "MINIMIZE") ) {
        if (incam->IntersectsLongitudeDomain(userMap)) {
          if (ui.GetString("LONSEAM") == "AUTO") {
            if ((int) userGrp["LongitudeDomain"] == 360) {
              userGrp.addKeyword(PvlKeyword("LongitudeDomain", "180"), Pvl::Replace);
              if (incam->IntersectsLongitudeDomain(userMap)) {
                // Its looks like a global image so switch back to the users preference
                userGrp.addKeyword(PvlKeyword("LongitudeDomain", "360"), Pvl::Replace);
              }
            }
            else {
              userGrp.addKeyword(PvlKeyword("LongitudeDomain", "360"), Pvl::Replace);
              if (incam->IntersectsLongitudeDomain(userMap)) {
                // Its looks like a global image so switch back to the
                // users preference
                userGrp.addKeyword(PvlKeyword("LongitudeDomain", "180"), Pvl::Replace);
              }
            }
            // Make the target info match the new longitude domain
            double minlat, maxlat, minlon, maxlon;
            incam->GroundRange(minlat, maxlat, minlon, maxlon, userMap);
            if (!ui.WasEntered("MINLAT")) {
              userGrp.addKeyword(PvlKeyword("MinimumLatitude", std::to_string(minlat)), Pvl::Replace);
            }
            if (!ui.WasEntered("MAXLAT")) {
              userGrp.addKeyword(PvlKeyword("MaximumLatitude", std::to_string(maxlat)), Pvl::Replace);
            }
            if (!ui.WasEntered("MINLON")) {
              userGrp.addKeyword(PvlKeyword("MinimumLongitude", std::to_string(minlon)), Pvl::Replace);
            }
            if (!ui.WasEntered("MAXLON")) {
              userGrp.addKeyword(PvlKeyword("MaximumLongitude", std::to_string(maxlon)), Pvl::Replace);
            }
          }

          else if (ui.GetString("LONSEAM") == "ERROR") {
            std::string msg = "The image [" + ui.GetCubeName("FROM") + "] crosses the " +
                          "longitude seam";
            throw IException(IException::User, msg, _FILEINFO_);
          }
        }
      }

      // Determine the image size
      if (ui.GetString("DEFAULTRANGE") == "MINIMIZE") {
        outmap = (TProjection *) ProjectionFactory::CreateForCube(userMap, samples, lines, *incam);
        trim = false;
      }
      else {//if (ui.GetString("DEFAULTRANGE") == "CAMERA" || DEFAULTRANGE = MAP) {
        outmap = (TProjection *) ProjectionFactory::CreateForCube(userMap, samples, lines, false);
      }
  //     else {
  //       outmap = (TProjection *) ProjectionFactory::CreateForCube(userMap, samples, lines, false);
  //     }
    }
    else { // MATCHMAP=TRUE
      //does this have any affect on anything?
      camGrp.deleteKeyword("MinimumLatitude");
      camGrp.deleteKeyword("MaximumLatitude");
      camGrp.deleteKeyword("MinimumLongitude");
      camGrp.deleteKeyword("MaximumLongitude");
      camGrp.deleteKeyword("PixelResolution");
      // Copy any defaults that are not in the user map from the camera map file
      outmap = (TProjection *) ProjectionFactory::CreateForCube(userMap,
                                                                samples,
                                                                lines,
                                                                true);//change usrmap to camgrp?
    }


    // Output the mapping group used to the Gui session log
    PvlGroup cleanMapping = outmap->Mapping();

    // Allocate the output cube and add the mapping labels
    QString fname = ui.GetCubeName("TO");
    Isis::CubeAttributeOutput &atts = ui.GetOutputAttribute("TO");
    Cube *ocube = p.SetOutputCube(fname, atts, samples, lines, icube->bandCount());

    ocube->putGroup(cleanMapping);

    // Set up the interpolator
    Interpolator *interp = NULL;
    if (ui.GetString("INTERP") == "NEARESTNEIGHBOR") {
      interp = new Interpolator(Interpolator::NearestNeighborType);
    }
    else if (ui.GetString("INTERP") == "BILINEAR") {
      interp = new Interpolator(Interpolator::BiLinearType);
    }
    else if (ui.GetString("INTERP") == "CUBICCONVOLUTION") {
      interp = new Interpolator(Interpolator::CubicConvolutionType);
    }

    // See if we need to deal with band dependent camera models
    if (!incam->IsBandIndependent()) {
      p.BandChange(bandChange);
    }

    //  See if center of input image projects.  If it does, force tile
    //  containing this center to be processed in ProcessRubberSheet.
    //  TODO:  WEIRD ... why is this needed ... Talk to Tracie ... JAA??
    double centerSamp = icube->sampleCount() / 2.;
    double centerLine = icube->lineCount() / 2.;
    if (incam->SetImage(centerSamp, centerLine)) {
      if (outmap->SetUniversalGround(incam->UniversalLatitude(),
                                    incam->UniversalLongitude())) {
        p.ForceTile(outmap->WorldX(), outmap->WorldY());
      }
    }
    // Create an alpha cube group for the output cube
    if (!ocube->hasGroup("AlphaCube")) {
      PvlGroup alpha("AlphaCube");
      alpha += PvlKeyword("AlphaSamples", std::to_string(icube->sampleCount()));
      alpha += PvlKeyword("AlphaLines", std::to_string(icube->lineCount()));
      alpha += PvlKeyword("AlphaStartingSample", std::to_string(0.5));
      alpha += PvlKeyword("AlphaStartingLine", std::to_string(0.5));
      alpha += PvlKeyword("AlphaEndingSample", std::to_string(icube->sampleCount() + 0.5));
      alpha += PvlKeyword("AlphaEndingLine", std::to_string(icube->lineCount() + 0.5));
      alpha += PvlKeyword("BetaSamples", std::to_string(icube->sampleCount()));
      alpha += PvlKeyword("BetaLines", std::to_string(icube->lineCount()));
      ocube->putGroup(alpha);
    }

    // We will need a transform class
    Transform *transform = 0;

    // Okay we need to decide how to apply the rubbersheeting for the transform
    // Does the user want to define how it is done?
    if (ui.GetString("WARPALGORITHM") == "FORWARDPATCH") {
      transform = new cam2mapForward(icube->sampleCount(),
                                     icube->lineCount(),
                                     incam,
                                     samples,
                                     lines,
                                     outmap,
                                     trim);

      int patchSize = ui.GetInteger("PATCHSIZE");
      if (patchSize <= 1) {
        patchSize = 3; // Make the patchsize reasonable
      }
      p.setPatchParameters(1, 1, patchSize, patchSize, patchSize-1, patchSize-1);

      p.processPatchTransform(*transform, *interp);
    }

    else if (ui.GetString("WARPALGORITHM") == "REVERSEPATCH") {
      transform = new cam2mapReverse(icube->sampleCount(),
                                     icube->lineCount(), incam, samples,lines,
                                     outmap, trim, occlusion);

      int patchSize = ui.GetInteger("PATCHSIZE");
      int minPatchSize = 4;
      if (patchSize < minPatchSize) {
        patchSize = minPatchSize;
      }
      p.SetTiling(patchSize, patchSize);

      p.StartProcess(*transform, *interp);
    }

    // The user didn't want to override the program smarts.
    // Handle framing cameras.  Always process using the backward
    // driven system (tfile).
    else if (incam->GetCameraType() == Camera::Framing) {
      transform = new cam2mapReverse(icube->sampleCount(),
                                     icube->lineCount(), incam, samples,lines,
                                     outmap, trim, occlusion);
      p.SetTiling(4, 4);
      p.StartProcess(*transform, *interp);
    }

    // The user didn't want to override the program smarts.
    // Handle linescan cameras.  Always process using the forward
    // driven patch option. Faster and we get better orthorectification
    //
    // TODO:  For now use the default patch size.  Need to modify
    // to determine patch size based on 1) if the limb is in the file
    // or 2) if the DTM is much coarser than the image
    else if (incam->GetCameraType() == Camera::LineScan) {
      transform = new cam2mapForward(icube->sampleCount(),
                                     icube->lineCount(), incam, samples,lines,
                                     outmap, trim);

      p.processPatchTransform(*transform, *interp);
    }

    // The user didn't want to override the program smarts.
    // Handle pushframe cameras.  Always process using the forward driven patch
    // option.  It is much faster than the tfile method.  We will need to
    // determine patch sizes based on the size of the push frame.
    //
    // TODO: What if the user has run crop, enlarge, or shrink on the push
    // frame cube.  Things probably won't work unless they do it just right
    // TODO: What about the THEMIS VIS Camera.  Will tall narrow (128x4) patches
    // work okay?
    else if (incam->GetCameraType() == Camera::PushFrame) {
      transform = new cam2mapForward(icube->sampleCount(),
                                     icube->lineCount(), incam, samples,lines,
                                     outmap, trim);

      // Get the frame height
      PushFrameCameraDetectorMap *dmap = (PushFrameCameraDetectorMap *) incam->DetectorMap();
      int frameSize = dmap->frameletHeight() / dmap->LineScaleFactor();

      // Check for even/odd cube to determine starting line
      PvlGroup &instGrp = icube->label()->findGroup("Instrument", Pvl::Traverse);
      int startLine = 1;

      // Get the alpha cube group in case they cropped the image
      AlphaCube acube(*icube);
      double betaLine = acube.AlphaLine(1.0);
      if (fabs(betaLine - 1.0) > 0.0000000001) {
        if (fabs(betaLine - (int) betaLine) > 0.00001) {
          string msg = "Input file is a pushframe camera cropped at a ";
          msg += "fractional pixel.  Can not project";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        int offset = (((int) (betaLine + 0.5)) - 1) % frameSize;
        startLine -= offset;
      }

      if ((QString::fromStdString(instGrp["Framelets"])).toUpper() == "EVEN") {
        startLine += frameSize;
      }

      p.setPatchParameters(1, startLine, 5, frameSize,
                           4, frameSize * 2);

      p.processPatchTransform(*transform, *interp);
    }

    // The user didn't want to override the program smarts.  The other camera
    // types have not be analyized.  This includes Radar and Point.  Continue to
    // use the reverse geom option with the default tiling hints
    else {
      transform = new cam2mapReverse(icube->sampleCount(),
                                     icube->lineCount(), incam, samples,lines,
                                     outmap, trim, occlusion);

      int tileStart, tileEnd;
      incam->GetGeometricTilingHint(tileStart, tileEnd);
      p.SetTiling(tileStart, tileEnd);

      p.StartProcess(*transform, *interp);
    }

    // Wrap up the warping process
    p.EndProcess();

    // add mapping to print.prt
    Application::Log(cleanMapping);

    // Cleanup
    delete outmap;
    delete transform;
    delete interp;
  }

  // Transform object constructor
  cam2mapForward::cam2mapForward(const int inputSamples, const int inputLines,
                                 Camera *incam, const int outputSamples,
                                 const int outputLines, TProjection *outmap,
                                 bool trim) {
    p_inputSamples = inputSamples;
    p_inputLines = inputLines;
    p_incam = incam;

    p_outputSamples = outputSamples;
    p_outputLines = outputLines;
    p_outmap = outmap;

    p_trim = trim;
  }

  // Transform method mapping input line/samps to lat/lons to output line/samps
  bool cam2mapForward::Xform(double &outSample, double &outLine,
                             const double inSample, const double inLine) {
    // See if the input image coordinate converts to a lat/lon
    if (!p_incam->SetImage(inSample,inLine)) {
      return false;
    }

    // Does that ground coordinate work in the map projection
    double lat = p_incam->UniversalLatitude();
    double lon = p_incam->UniversalLongitude();
    if (!p_outmap->SetUniversalGround(lat,lon)) return false;

    // See if we should trim
    if ((p_trim) && (p_outmap->HasGroundRange())) {
      if (p_outmap->Latitude() < p_outmap->MinimumLatitude()) return false;
      if (p_outmap->Latitude() > p_outmap->MaximumLatitude()) return false;
      if (p_outmap->Longitude() < p_outmap->MinimumLongitude()) return false;
      if (p_outmap->Longitude() > p_outmap->MaximumLongitude()) return false;
    }

    // Get the output sample/line coordinate
    outSample = p_outmap->WorldX();
    outLine = p_outmap->WorldY();

    // Make sure the point is inside the output image
    if (outSample < 0.5) return false;
    if (outLine < 0.5) return false;
    if (outSample > p_outputSamples + 0.5) return false;
    if (outLine > p_outputLines + 0.5) return false;

    // Everything is good
    return true;
  }

  int cam2mapForward::OutputSamples() const {
    return p_outputSamples;
  }

  int cam2mapForward::OutputLines() const {
    return p_outputLines;
  }


  // Transform object constructor
  cam2mapReverse::cam2mapReverse(const int inputSamples, const int inputLines,
                                 Camera *incam, const int outputSamples,
                                 const int outputLines, TProjection *outmap,
                                 bool trim, bool occlusion) {
    p_inputSamples = inputSamples;
    p_inputLines = inputLines;
    p_incam = incam;

    p_outputSamples = outputSamples;
    p_outputLines = outputLines;
    p_outmap = outmap;

    p_trim = trim;
    p_occlusion = occlusion;
  }

  // Transform method mapping output line/samps to lat/lons to input line/samps
  bool cam2mapReverse::Xform(double &inSample, double &inLine,
                             const double outSample, const double outLine) {
    // See if the output image coordinate converts to lat/lon
    if (!p_outmap->SetWorld(outSample, outLine)) return false;

    // See if we should trim
    if ((p_trim) && (p_outmap->HasGroundRange())) {
      if (p_outmap->Latitude() < p_outmap->MinimumLatitude()) return false;
      if (p_outmap->Latitude() > p_outmap->MaximumLatitude()) return false;
      if (p_outmap->Longitude() < p_outmap->MinimumLongitude()) return false;
      if (p_outmap->Longitude() > p_outmap->MaximumLongitude()) return false;
    }

    // Get the universal lat/lon and see if it can be converted to input line/samp
    double lat = p_outmap->UniversalLatitude();
    double lon = p_outmap->UniversalLongitude();

    if (!p_incam->SetUniversalGround(lat, lon)) return false;

    // Make sure the point is inside the input image
    if (p_incam->Sample() < 0.5) return false;
    if (p_incam->Line() < 0.5) return false;
    if (p_incam->Sample() > p_inputSamples + 0.5) return false;
    if (p_incam->Line() > p_inputLines + 0.5) return false;

    // Everything is good
    inSample = p_incam->Sample();
    inLine = p_incam->Line();

    // Good to ground one last time to check for occlusion
    p_incam->SetImage(inSample, inLine);

    if (p_occlusion){
      if (abs(lat - p_incam->UniversalLatitude()) > 0.00001 || abs(lon - p_incam->UniversalLongitude()) > 0.00001) {
        return false;
      }
    }

    return true;
  }

  int cam2mapReverse::OutputSamples() const {
    return p_outputSamples;
  }

  int cam2mapReverse::OutputLines() const {
    return p_outputLines;
  }

  void bandChange(const int band) {
    // band dependant band change
    incam->SetBand(band);
  }
}
