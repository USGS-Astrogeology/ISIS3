 #include <cmath>

#include "grid.h"

#include "Angle.h"
#include "Camera.h"
#include "CameraFactory.h"
#include "GroundGrid.h"
#include "IException.h"
#include "Latitude.h"
#include "Longitude.h"
#include "ProcessByLine.h"
#include "Projection.h"
#include "ProjectionFactory.h"
#include "UniversalGroundMap.h"

using namespace std;
using namespace Isis;

namespace Isis {

  bool imageDrawLine(int line, int baseLine, int lineWidth, int lineInc);
  bool imageDrawSample(int sample, int baseSample, int lineWidth, int sampleInc);
  void changeBand(int band, GroundGrid *&latLonGrid, UniversalGroundMap *gmap, int ticks, 
                  bool extendGrid, bool walkBoundary, int numSamples, int numLines, Latitude minLat, 
                  Latitude maxLat, Longitude minLon, Longitude maxLon, Latitude baseLat, 
                  Longitude baseLon, Angle latInc, Angle lonInc);
  bool groundDrawPoint(int samp, int line, int lineWidth, GroundGrid *latLonGrid, 
                       bool latGrid = true);


  void grid(UserInterface &ui) {
    Cube icube;
    CubeAttributeInput inAtt = ui.GetInputAttribute("FROM");
    if (inAtt.bands().size() != 0) {
      icube.setVirtualBands(inAtt.bands());
    }
    icube.open(ui.GetCubeName("FROM"));
    grid(&icube, ui);
  }

  void grid(Cube *icube, UserInterface &ui) {
    // We will be processing by line
    ProcessByLine p;
    p.SetInputCube(icube);
    p.SetOutputCube(ui.GetCubeName("TO"), ui.GetOutputAttribute("TO"), 
                    icube->sampleCount(), icube->lineCount(), icube->bandCount());

    QString mode = ui.GetString("MODE");

    bool outline = ui.GetBoolean("OUTLINE");
    bool ticks = ui.GetBoolean("TICKS");
    bool extendGrid = ui.GetBoolean("EXTENDGRID");

    int tickSize;
    bool diagonalTicks;
    if (ticks) {
      tickSize = ui.GetInteger("TICKSIZE") / 2;
      diagonalTicks = ui.GetBoolean("DIAGONALTICKS");
    }
    else {
      tickSize = 0;
      diagonalTicks = false;
    }

    int lineWidth = ui.GetInteger("LINEWIDTH") / 2;
   
    QString bval = ui.GetString("BKGNDVALUE").toUpper();

    bool useImageAsBkgn = false;
    double bkgndValue;

    if (bval == "IMAGE") {
      useImageAsBkgn = true;
      bkgndValue = Null;
    }
    if (bval == "HRS") {
      bkgndValue = Hrs;
    }
    else if (bval == "LRS") {
      bkgndValue = Lrs;
    }
    else if (bval == "NULL") {
      bkgndValue = Null;
    }
    else if (bval == "DN") {
      bkgndValue = ui.GetDouble("BKGNDDNVALUE");
    }

    QString lval = ui.GetString("LINEVALUE").toUpper();
    double lineValue;
    if (lval == "HRS") {
      lineValue = Hrs;
    }
    else if (lval == "LRS") {
      lineValue = Lrs;
    }
    else if (lval == "NULL") {
      lineValue = Null;
    }
    else if (lval == "DN") {
      if (ui.WasEntered("DNVALUE")) {
        lineValue = ui.GetDouble("DNVALUE");
      }
      else {
        throw IException(IException::User, "Must enter value in DNVALUE", _FILEINFO_);
      }
    }

    int inputSamples = icube->sampleCount();
    int inputLines = icube->lineCount();
    int inputBands = icube->bandCount();

    // Line & sample based grid
    if (mode == "IMAGE") {
      int baseLine = ui.GetInteger("BASELINE");
      int baseSample = ui.GetInteger("BASESAMPLE");
      int lineInc = ui.GetInteger("LINC");
      int sampleInc = ui.GetInteger("SINC");

      /**
       * Copies DN's from the input cube to the tracking cube, subtracts the old offset, and adds
       * the new offset to each pixel.
       *
       * @param in  Input cube
       * @param out Mosaic cube
       */
      auto imageGrid = [&](Buffer &in, Buffer &out)->void {
        for (int samp = 1; samp <= in.size(); samp++) {
          if (useImageAsBkgn)
            out[samp - 1] = in[samp - 1];
          else 
            out[samp - 1] = bkgndValue;

          if (!ticks) {
            if (imageDrawSample(samp, baseSample, lineWidth, sampleInc) 
                                        || imageDrawLine(in.Line(), baseLine, lineWidth, lineInc)) {
              out[samp - 1] = lineValue;
            }
          }
          // ticks!
          else {
            // tickSize is the width or height divided by 2, so integer rounding
            //   takes care of the current sample/line while doing +/- tickSize
            //   creates the appropriate width and height.

            // Vertical/Horizontal Ticks
            if (!diagonalTicks) {
              // horizontal test
              for (int sampleTest = samp - tickSize;
                  (sampleTest <= samp + tickSize) && (out[samp - 1] != lineValue);
                  sampleTest ++) {
                if (imageDrawLine(in.Line(), baseLine, lineWidth, lineInc) 
                                && imageDrawSample(sampleTest, baseSample, lineWidth, sampleInc)) {
                  out[samp - 1] = lineValue;
                }
              }

              // vertical test
              for (int lineTest = in.Line() - tickSize;
                  (lineTest <= in.Line() + tickSize) && (out[samp - 1] != lineValue);
                  lineTest ++) {
                if (imageDrawLine(lineTest, baseLine, lineWidth, lineInc) 
                                      && imageDrawSample(samp, baseSample, lineWidth, sampleInc)) {
                  out[samp - 1] = lineValue;
                }
              }
            }
            // Diagonal Ticks
            else {
              // top left to bottom right
              int sampleTest = samp - tickSize;
              int lineTest = in.Line() - tickSize;

              while ((out[samp - 1] != lineValue) &&
                    (lineTest <= in.Line() + tickSize) &&
                    (sampleTest <= samp + tickSize)) {
                if (imageDrawLine(lineTest, baseLine, lineWidth, lineInc) 
                                && imageDrawSample(sampleTest, baseSample, lineWidth, sampleInc)) {
                  out[samp - 1] = lineValue;
                }

                sampleTest ++;
                lineTest ++;
              }

              // top right to bottom left
              sampleTest = samp + tickSize;
              lineTest = in.Line() - tickSize;

              while ((out[samp - 1] != lineValue) &&
                    (lineTest <= in.Line() + tickSize) &&
                    (sampleTest >= samp - tickSize)) {
                if (imageDrawLine(lineTest, baseLine, lineWidth, lineInc) 
                                && imageDrawSample(sampleTest, baseSample, lineWidth, sampleInc)) {
                  out[samp - 1] = lineValue;
                }

                sampleTest --;
                lineTest ++;
              }
            }
          }
        }

        // draw outline
        if (outline) {
          if (in.Line() - 1 <= lineWidth * 2 ||
             in.Line() >= inputLines - lineWidth * 2) {
            for (int i = 0; i < in.size(); i++) {
              out[i] = Hrs;
            }
          }
          else {
            for (int i = 0; i <= lineWidth * 2; i++)
              out[i] = Hrs;

            for (int i = inputSamples - lineWidth * 2 - 1; i < in.size(); i++)
              out[i] = Hrs;
          }
        }
      };

      p.StartProcess(imageGrid);
      p.EndProcess();
    }
    // Lat/Lon based grid
    else {
      // if > 1 input band and IsBandIndependent = false, need to regenerate grid for 
      // each band
      bool recalculateForEachBand = false;
      if (icube->hasGroup("Instrument")) {
        if ((inputBands >= 2) && !(icube->camera()->IsBandIndependent())) {
          recalculateForEachBand = true; 
        }
      }

      UniversalGroundMap *gmap = new UniversalGroundMap(*icube, UniversalGroundMap::ProjectionFirst);

      GroundGrid *latLonGrid = new GroundGrid(gmap, ticks, extendGrid, inputSamples, inputLines);
      Latitude baseLat = Latitude(ui.GetDouble("BASELAT"), *latLonGrid->GetMappingGroup(), Angle::Degrees);
      Longitude baseLon = Longitude(ui.GetDouble("BASELON"), *latLonGrid->GetMappingGroup(), Angle::Degrees);
      Angle latInc = Angle(ui.GetDouble("LATINC"), Angle::Degrees);
      Angle lonInc = Angle(ui.GetDouble("LONINC"), Angle::Degrees);

      Progress progress;
      progress.SetText("Calculating Grid");

      Latitude minLat;
      if (ui.WasEntered("MINLAT")) {
        minLat = Latitude(ui.GetDouble("MINLAT"), *latLonGrid->GetMappingGroup(), Angle::Degrees);
      }

      Latitude maxLat;
      if (ui.WasEntered("MAXLAT")) {
        maxLat = Latitude(ui.GetDouble("MAXLAT"), *latLonGrid->GetMappingGroup(), Angle::Degrees);
      }

      Longitude minLon;
      if (ui.WasEntered("MINLON")) {
        minLon = Longitude(ui.GetDouble("MINLON"), *latLonGrid->GetMappingGroup(), Angle::Degrees);
      }

      Longitude maxLon;
      if (ui.WasEntered("MAXLON")) {
        maxLon = Longitude(ui.GetDouble("MAXLON"), *latLonGrid->GetMappingGroup(), Angle::Degrees);
      }

      latLonGrid->SetGroundLimits(minLat, minLon, maxLat, maxLon);

      // If the grid is not going to reach the min/max lon warn the user.
      if (!extendGrid) {
        // Check that the min/max lon values match the lon domain
        if ( latLonGrid->GetMappingGroup()->findKeyword("LongitudeDomain")[0] == "360" &&
            (latLonGrid->minLongitude().degrees() < 0.0 ||
              latLonGrid->maxLongitude().degrees() > 360.0) ) {
          std::string msg = "**WARNING** minimum longitude ["
                        + toString( latLonGrid->minLongitude().degrees() )
                        + "] and maximum longitude ["
                        + toString( latLonGrid->maxLongitude().degrees() )
                        + "] are not in the 360 degree longitude domain and "
                          "the EXTENDGRID parameter is set to false. "
                          "Output grid may not cover the entire map projection.";
          std::cerr << msg << std::endl;
        }
        else if ( latLonGrid->GetMappingGroup()->findKeyword("LongitudeDomain")[0] == "180" &&
                  (latLonGrid->minLongitude().degrees() < -180.0 ||
                  latLonGrid->maxLongitude().degrees() > 180.0) ) {
          std::string msg = "**WARNING** minimum longitude ["
                        + toString( latLonGrid->minLongitude().degrees() )
                        + "] and maximum longitude ["
                        + toString( latLonGrid->maxLongitude().degrees() )
                        + "] are not in the 180 degree longitude domain and "
                          "the EXTENDGRID parameter is set to false. "
                          "Output grid may not cover the entire map projection.";
          std::cerr << msg << std::endl;
        }
      }

      latLonGrid->CreateGrid(baseLat, baseLon, latInc, lonInc, &progress);

      bool walkBoundary = false;
      if (ui.GetBoolean("BOUNDARY")) {
        latLonGrid->WalkBoundary();
        walkBoundary = true;
      }

      int currentBand = 0;
      auto groundGrid = [&](Buffer &in, Buffer &out)->void {
        // check to see if we're in the same band 
        if ( (currentBand != in.Band()) && recalculateForEachBand ) {
          currentBand = in.Band(); 
          changeBand(currentBand, latLonGrid, gmap, ticks, extendGrid, walkBoundary, 
                  inputSamples, inputLines, minLat, maxLat, minLon, maxLon, baseLat, 
                  baseLon, latInc, lonInc);
        }

        for (int samp = 1; samp <= in.SampleDimension(); samp++) {
          if (!ticks) {
            if (groundDrawPoint(samp, in.Line(), lineWidth, latLonGrid)) {
              out[samp - 1] = lineValue;
            }
            else {
              if (useImageAsBkgn) {
                out[samp - 1] = in[samp - 1];
              }
              else {
                out[samp - 1] = bkgndValue;
              }
            }
          }
          else {
            // We need to check the grids for overlaps near current point
            out[samp - 1] = in[samp - 1];

            // tickSize is the width or height divided by 2, so integer rounding
            //   takes care of the current sample/line while doing +/- tickSize
            //   creates the appropriate width and height.

            // Vertical/Horizontal Ticks
            if (!diagonalTicks) {
              // horizontal test
              for (int sampleTest = samp - tickSize;
                  (sampleTest <= samp + tickSize) && (out[samp - 1] != Hrs);
                  sampleTest ++) {
                if (groundDrawPoint(sampleTest, in.Line(), lineWidth, latLonGrid, true) &&
                    groundDrawPoint(sampleTest, in.Line(), lineWidth, latLonGrid, false)) {
                  out[samp - 1] = lineValue;
                }
              }

              // vertical test
              for (int lineTest = in.Line() - tickSize;
                  (lineTest <= in.Line() + tickSize) && (out[samp - 1] != Hrs);
                  lineTest ++) {
                if (groundDrawPoint(samp, lineTest, lineWidth, latLonGrid, true) &&
                    groundDrawPoint(samp, lineTest, lineWidth, latLonGrid, false)) {
                  out[samp - 1] = lineValue;
                }
              }
            }
            // Diagonal ticks
            else {
              // top left to bottom right
              int sampleTest = samp - tickSize;
              int lineTest = in.Line() - tickSize;


              while ((out[samp - 1] != Hrs) &&
                    (lineTest <= in.Line() + tickSize) &&
                    (sampleTest <= samp + tickSize)) {
                if (groundDrawPoint(sampleTest, lineTest, lineWidth, latLonGrid, true) &&
                    groundDrawPoint(sampleTest, lineTest, lineWidth, latLonGrid, false)) {
                  out[samp - 1] = lineValue;
                }

                sampleTest++;
                lineTest++;
              }

              // top right to bottom left
              sampleTest = samp + tickSize;
              lineTest = in.Line() - tickSize;

              while ((out[samp - 1] != Hrs) &&
                    (lineTest <= in.Line() + tickSize) &&
                    (sampleTest >= samp - tickSize)) {
                if (groundDrawPoint(sampleTest, lineTest, lineWidth, latLonGrid, true) &&
                    groundDrawPoint(sampleTest, lineTest, lineWidth, latLonGrid, false)) {
                  out[samp - 1] = lineValue;
                }

                sampleTest--;
                lineTest++;
              }
            }
          }
        }

        // draw outline
        if (outline) {
          if (in.Line() - 1 <= lineWidth * 2 ||
             in.Line() >= inputLines - lineWidth * 2) {
            for (int i = 0; i < in.size(); i++) {
              out[i] = Hrs;
            }
          }
          else {
            for (int i = 0; i <= lineWidth * 2; i++)
              out[i] = Hrs;

            for (int i = inputSamples - lineWidth * 2 - 1; i < in.size(); i++)
              out[i] = Hrs;
          }
        }
      };

      p.StartProcess(groundGrid);
      p.EndProcess();

      delete latLonGrid;
      latLonGrid = NULL;
      
      delete gmap;
      gmap = NULL;
    }
  }

  bool imageDrawLine(int line, int baseLine, int lineWidth, int lineInc) {
    bool drawLine = false;

    for (int y = line - lineWidth; y <= line + lineWidth; y++) {
      drawLine = drawLine || (y % lineInc == baseLine % lineInc);
    }
    // std::cout<<"Line: "<<line << " drawline: " << drawLine<<std::endl;
    return drawLine;
  }

  bool imageDrawSample(int sample, int baseSample, int lineWidth, int sampleInc) {
    bool drawSamp = false;

    for (int x = sample - lineWidth; x <= sample + lineWidth; x++) {
      drawSamp = drawSamp || (x % sampleInc == baseSample % sampleInc);
    }

    return drawSamp;
  }

  void changeBand(int band, GroundGrid *&latLonGrid, UniversalGroundMap *gmap, int ticks, 
                  bool extendGrid, bool walkBoundary, int numSamples, int numLines, Latitude minLat, 
                  Latitude maxLat, Longitude minLon, Longitude maxLon, Latitude baseLat, 
                  Longitude baseLon, Angle latInc, Angle lonInc) {
    Progress progress;

    // change band of UniversalGroundMap
    gmap->SetBand(band);

    // update latLonGrid to use new UniversalGroundMap
    latLonGrid = new GroundGrid(gmap, ticks, extendGrid, numSamples, numLines);

    // re-set old ground limits from GUI
    latLonGrid->SetGroundLimits(minLat, minLon, maxLat, maxLon);

    // If the grid is not going to reach the min/max lon warn the user.
    if (!extendGrid) {
      // Check that the min/max lon values match the lon domain
      if ( latLonGrid->GetMappingGroup()->findKeyword("LongitudeDomain")[0] == "360" &&
          (latLonGrid->minLongitude().degrees() < 0.0 ||
            latLonGrid->maxLongitude().degrees() > 360.0) ) {
        std::string msg = "**WARNING** minimum longitude ["
                      + toString( latLonGrid->minLongitude().degrees() )
                      + "] and maximum longitude ["
                      + toString( latLonGrid->maxLongitude().degrees() )
                      + "] are not in the 360 degree longitude domain and "
                        "the EXTENDGRID parameter is set to false. "
                        "Output grid may not cover the entire map projection for band["
                      + toString(band) + "].";
        std::cerr << msg << std::endl;
      }
      else if ( latLonGrid->GetMappingGroup()->findKeyword("LongitudeDomain")[0] == "180" &&
                (latLonGrid->minLongitude().degrees() < -180.0 ||
                latLonGrid->maxLongitude().degrees() > 180.0) ) {
        std::string msg = "**WARNING** minimum longitude ["
                      + toString( latLonGrid->minLongitude().degrees() )
                      + "] and maximum longitude ["
                      + toString( latLonGrid->maxLongitude().degrees() )
                      + "] are not in the 180 degree longitude domain and "
                        "the EXTENDGRID parameter is set to false. "
                        "Output grid may not cover the entire map projection for band["
                      + toString(band) + "].";
        std::cerr << msg << std::endl;
      }
    }

    QString progressMessage = QString("Recalculating grid for band %1").arg(band);
    progress.SetText(progressMessage);

    // re-set lat/lon base/in from GUI
    latLonGrid->CreateGrid(baseLat, baseLon, latInc, lonInc, &progress);

    if (walkBoundary) {
      latLonGrid->WalkBoundary();
    }
  }

  bool groundDrawPoint(int samp, int line, int lineWidth, GroundGrid *latLonGrid, 
                       bool latGrid) {
    bool drawPoint = false;
    // RETURN EARLY???
    for (int x = samp - lineWidth; x <= samp + lineWidth; x++) {
      drawPoint = drawPoint || latLonGrid->PixelOnGrid(x - 1, line - 1, latGrid);
    }

    for (int y = line - lineWidth; y <= line + lineWidth; y ++) {
      drawPoint = drawPoint || latLonGrid->PixelOnGrid(samp - 1, y - 1, latGrid);
    }

    return drawPoint;
  }
}
