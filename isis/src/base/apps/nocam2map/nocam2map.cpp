#include <algorithm>

#include <QList>
#include <QString>
#include <QStringList>
#include <QVector>

#include <SpiceUsr.h>

#include "Application.h"
#include "Cube.h"
#include "Brick.h"
#include "Constants.h"
#include "Cube.h"
#include "IString.h"
#include "LeastSquares.h"
#include "NaifStatus.h"
#include "nocam2map.h"
#include "PolynomialBivariate.h"
#include "ProcessRubberSheet.h"
#include "ProjectionFactory.h"
#include "Statistics.h"
#include "Target.h"
#include "TextFile.h"
#include "TProjection.h"

using namespace std;
using namespace Isis;


namespace Isis {
  static void DeleteTables(Pvl *label, PvlGroup kernels);

  void nocam2map(UserInterface &ui, Pvl *log) {
    QString inputFileName = ui.GetCubeName("FROM");
    Cube iCube(inputFileName.toStdString());
    nocam2map(&iCube, ui, log);
  }


  void nocam2map(Cube *inCube, UserInterface &ui, Pvl *log) {
    //Create a process to create the input cubes
    Process p;

    //Create the input cubes, matching sample/lines
    Cube *latCube = p.SetInputCube(ui.GetCubeName("LATCUB"), ui.GetInputAttribute("LATCUB"), SpatialMatch);
    Cube *lonCube = p.SetInputCube(ui.GetCubeName("LONCUB"), ui.GetInputAttribute("LONCUB"), SpatialMatch);
  
    //A 1x1 brick to read in the latitude and longitude DN values from
    //the specified cubes
    Brick latBrick(1, 1, 1, latCube->pixelType());
    Brick lonBrick(1, 1, 1, lonCube->pixelType());
  
    //Set the sample and line increments
    float sinc = (inCube->sampleCount() * 0.10);
    if (ui.WasEntered("SINC")) {
      sinc = ui.GetInteger("SINC");
    }
  
    float linc = (inCube->lineCount() * 0.10);
    if (ui.WasEntered("LINC")) {
      linc = ui.GetInteger("LINC");
    }
  
    //Set the degree of the polynomial to use in our functions
    int degree = ui.GetInteger("DEGREE");
  
    //We are using a polynomial with two variables
    PolynomialBivariate sampFunct(degree);
    PolynomialBivariate lineFunct(degree);
  
    //We will be solving the function using the least squares method
    LeastSquares sampSol(sampFunct);
    LeastSquares lineSol(lineFunct);
  
    //Setup the variables for solving the stereographic projection
    //x = cos(latitude) * sin(longitude - lon_center)
    //y = cos(lat_center) * sin(latitude) - sin(lat_center) * cos(latitude) * cos(longitude - lon_center)
  
    //Get the center lat and long from the input cubes
    double lat_center = latCube->statistics()->Average() * PI / 180.0;
    double lon_center = lonCube->statistics()->Average() * PI / 180.0;
  
  
    /**
     * Loop through lines and samples projecting the latitude and longitude at those
     * points to stereographic x and y and adding these points to the LeastSquares
     * matrix.
     */
    for (float i = 1; i <= inCube->lineCount(); i += linc) {
      for (float j = 1; j <= inCube->sampleCount(); j += sinc) {
        latBrick.SetBasePosition(j, i, 1);
        latCube->read(latBrick);
        if (IsSpecial(latBrick.at(0))) continue;
        double lat = latBrick.at(0) * PI / 180.0;
        lonBrick.SetBasePosition(j, i, 1);
        lonCube->read(lonBrick);
        if (IsSpecial(lonBrick.at(0))) continue;
        double lon = lonBrick.at(0) * PI / 180.0;
  
        //Project lat and lon to x and y using a stereographic projection
        double k = 2 / (1 + sin(lat_center) * sin(lat) + cos(lat_center) * cos(lat) * cos(lon - lon_center));
        double x = k * cos(lat) * sin(lon - lon_center);
        double y = k * (cos(lat_center) * sin(lat)) - (sin(lat_center) * cos(lat) * cos(lon - lon_center));
  
        //Add x and y to the least squares matrix
        vector<double> data;
        data.push_back(x);
        data.push_back(y);
        sampSol.AddKnown(data, j);
        lineSol.AddKnown(data, i);
  
        //If the sample increment goes past the last sample in the line, we want to
        //always read the last sample..
        if (j != inCube->sampleCount() && j + sinc > inCube->sampleCount()) {
          j = inCube->sampleCount() - sinc;
        }
      }
      //If the line increment goes past the last line in the cube, we want to
      //always read the last line..
      if (i != inCube->lineCount() && i + linc > inCube->lineCount()) {
        i = inCube->lineCount() - linc;
      }
    }
  
    //Solve the least squares functions using QR Decomposition
    try {
      sampSol.Solve(LeastSquares::QRD);
      lineSol.Solve(LeastSquares::QRD);
    }
    catch (IException &e) {
      FileName inFile = inCube->fileName().toStdString();   
      std::string msg = "Unable to calculate transformation of projection for [" + inFile.expanded() + "].";
      throw IException(e, IException::Unknown, msg, _FILEINFO_);
    }
  
    //If the user wants to save the residuals to a file, create a file and write
    //the column titles to it.
    TextFile oFile;
    if (ui.WasEntered("RESIDUALS")) {
      oFile.Open(ui.GetFileName("RESIDUALS"), "overwrite");
      oFile.PutLine("Sample,\tLine,\tX,\tY,\tSample Error,\tLine Error\n");
    }
  
    //Gather the statistics for the residuals from the least squares solutions
    Statistics sampErr;
    Statistics lineErr;
    vector<double> sampResiduals = sampSol.Residuals();
    vector<double> lineResiduals = lineSol.Residuals();
    for (int i = 0; i < (int)sampResiduals.size(); i++) {
      sampErr.AddData(sampResiduals[i]);
      lineErr.AddData(lineResiduals[i]);
    }
  
    //If a residuals file was specified, write the previous data, and the errors to the file.
    if (ui.WasEntered("RESIDUALS")) {
      for (int i = 0; i < sampSol.Rows(); i++) {
        vector<double> data = sampSol.GetInput(i);
        QString tmp = "";
        tmp += QString::number(sampSol.GetExpected(i));
        tmp += ",\t";
        tmp += QString::number(lineSol.GetExpected(i));
        tmp += ",\t";
        tmp += QString::number(data[0]);
        tmp += ",\t";
        tmp += QString::number(data[1]);
        tmp += ",\t";
        tmp += QString::number(sampResiduals[i]);
        tmp += ",\t";
        tmp += QString::number(lineResiduals[i]);
        oFile.PutLine(tmp + "\n");
      }
    }
    oFile.Close();
  
    //Records the error to the log
    PvlGroup error("Error");
    error += PvlKeyword("Degree", std::to_string(degree));
    error += PvlKeyword("NumberOfPoints", std::to_string((int)sampResiduals.size()));
    error += PvlKeyword("SampleMinimumError", std::to_string(sampErr.Minimum()));
    error += PvlKeyword("SampleAverageError", std::to_string(sampErr.Average()));
    error += PvlKeyword("SampleMaximumError", std::to_string(sampErr.Maximum()));
    error += PvlKeyword("SampleStdDeviationError", std::to_string(sampErr.StandardDeviation()));
    error += PvlKeyword("LineMinimumError", std::to_string(lineErr.Minimum()));
    error += PvlKeyword("LineAverageError", std::to_string(lineErr.Average()));
    error += PvlKeyword("LineMaximumError", std::to_string(lineErr.Maximum()));
    error += PvlKeyword("LineStdDeviationError", std::to_string(lineErr.StandardDeviation()));

    Application::Log(error);

    //Close the input cubes for cleanup
    p.EndProcess();
  
    //If we want to warp the image, then continue, otherwise return
    if (!ui.GetBoolean("NOWARP")) {
      //Creates the mapping group
      Pvl mapFile;
      mapFile.read(ui.GetFileName("MAP").toStdString());
      PvlGroup &mapGrp = mapFile.findGroup("Mapping", Pvl::Traverse);
  
      //Reopen the lat and long cubes
      latCube = new Cube();
      latCube->setVirtualBands(ui.GetInputAttribute("LATCUB").bands());
      latCube->open(ui.GetCubeName("LATCUB"));
  
      lonCube = new Cube();
      lonCube->setVirtualBands(ui.GetInputAttribute("LONCUB").bands());
      lonCube->open(ui.GetCubeName("LONCUB"));
  
      PvlKeyword targetName;
  
      //If the user entered the target name
      if (ui.WasEntered("TARGET")) {
        targetName = PvlKeyword("TargetName", ui.GetString("TARGET").toStdString());
      }
      //Else read the target name from the input cube
      else {
        Pvl fromFile;
        fromFile.read(inCube->fileName().toStdString());
        targetName = fromFile.findKeyword("TargetName", Pvl::Traverse);
      }
  
      mapGrp.addKeyword(targetName, Pvl::Replace);
  
      PvlKeyword equRadius;
      PvlKeyword polRadius;
      //If the user entered the equatorial and polar radii
      if (ui.WasEntered("EQURADIUS") && ui.WasEntered("POLRADIUS")) {
        equRadius = PvlKeyword("EquatorialRadius", std::to_string(ui.GetDouble("EQURADIUS")));
        polRadius = PvlKeyword("PolarRadius", std::to_string(ui.GetDouble("POLRADIUS")));
      }
      //Else read them from the pck
      else {
        PvlGroup radii = Target::radiiGroup(QString::fromStdString(targetName[0]));
        equRadius = radii["EquatorialRadius"];
        polRadius = radii["PolarRadius"];
      }
      mapGrp.addKeyword(equRadius, Pvl::Replace);
      mapGrp.addKeyword(polRadius, Pvl::Replace);
  
  
      //If the latitude type is not in the mapping group, copy it from the input
      if (!mapGrp.hasKeyword("LatitudeType")) {
        if (ui.GetString("LATTYPE") == "PLANETOCENTRIC") {
          mapGrp.addKeyword(PvlKeyword("LatitudeType", "Planetocentric"), Pvl::Replace);
        }
        else {
          mapGrp.addKeyword(PvlKeyword("LatitudeType", "Planetographic"), Pvl::Replace);
        }
      }
  
      //If the longitude direction is not in the mapping group, copy it from the input
      if (!mapGrp.hasKeyword("LongitudeDirection")) {
        if (ui.GetString("LONDIR") == "POSITIVEEAST") {
          mapGrp.addKeyword(PvlKeyword("LongitudeDirection", "PositiveEast"), Pvl::Replace);
        }
        else {
          mapGrp.addKeyword(PvlKeyword("LongitudeDirection", "PositiveWest"), Pvl::Replace);
        }
      }
  
      //If the longitude domain is not in the mapping group, assume it is 360
      if (!mapGrp.hasKeyword("LongitudeDomain")) {
        mapGrp.addKeyword(PvlKeyword("LongitudeDomain", "360"), Pvl::Replace);
      }
  
      //If the default range is to be computed, use the input lat/long cubes to determine the range
      if (ui.GetString("DEFAULTRANGE") == "COMPUTE") {
        //NOTE - When computing the min/max longitude this application does not account for the
        //longitude seam if it exists. Since the min/max are calculated from the statistics of
        //the input longitude cube and then converted to the mapping group's domain they may be
        //invalid for cubes containing the longitude seam.
  
        Statistics *latStats = latCube->statistics();
        Statistics *lonStats = lonCube->statistics();
  
        double minLat = latStats->Minimum();
        double maxLat = latStats->Maximum();
  
        bool isOcentric = ((std::string)mapGrp.findKeyword("LatitudeType")) == "Planetocentric";
  
        if (isOcentric) {
          if (ui.GetString("LATTYPE") != "PLANETOCENTRIC") {
            minLat = TProjection::ToPlanetocentric(minLat, (double)equRadius, (double)polRadius);
            maxLat = TProjection::ToPlanetocentric(maxLat, (double)equRadius, (double)polRadius);
          }
        }
        else {
          if (ui.GetString("LATTYPE") == "PLANETOCENTRIC") {
            minLat = TProjection::ToPlanetographic(minLat, (double)equRadius, (double)polRadius);
            maxLat = TProjection::ToPlanetographic(maxLat, (double)equRadius, (double)polRadius);
          }
        }
  
        int lonDomain = (int)mapGrp.findKeyword("LongitudeDomain");
        double minLon = lonDomain == 360 ? TProjection::To360Domain(lonStats->Minimum()) :
                                                      TProjection::To180Domain(lonStats->Minimum());
        double maxLon = lonDomain == 360 ? TProjection::To360Domain(lonStats->Maximum()) :
                                                      TProjection::To180Domain(lonStats->Maximum());
  
        bool isPosEast = ((std::string)mapGrp.findKeyword("LongitudeDirection")) == "PositiveEast";
  
        if (isPosEast) {
          if (ui.GetString("LONDIR") != "POSITIVEEAST") {
            minLon = TProjection::ToPositiveEast(minLon, lonDomain);
            maxLon = TProjection::ToPositiveEast(maxLon, lonDomain);
          }
        }
        else {
          if (ui.GetString("LONDIR") == "POSITIVEEAST") {
            minLon = TProjection::ToPositiveWest(minLon, lonDomain);
            maxLon = TProjection::ToPositiveWest(maxLon, lonDomain);
          }
        }
  
        if (minLon > maxLon) {
          double temp = minLon;
          minLon = maxLon;
          maxLon = temp;
        }
  
        mapGrp.addKeyword(PvlKeyword("MinimumLatitude", std::to_string(minLat)), Pvl::Replace);
        mapGrp.addKeyword(PvlKeyword("MaximumLatitude", std::to_string(maxLat)), Pvl::Replace);
        mapGrp.addKeyword(PvlKeyword("MinimumLongitude", std::to_string(minLon)), Pvl::Replace);
        mapGrp.addKeyword(PvlKeyword("MaximumLongitude", std::to_string(maxLon)), Pvl::Replace);
      }
  
      //If the user decided to enter a ground range then override
      if (ui.WasEntered("MINLAT")) {
        mapGrp.addKeyword(PvlKeyword("MinimumLatitude",
                                     std::to_string(ui.GetDouble("MINLAT"))), Pvl::Replace);
      }
  
      if (ui.WasEntered("MAXLAT")) {
        mapGrp.addKeyword(PvlKeyword("MaximumLatitude",
                                     std::to_string(ui.GetDouble("MAXLAT"))), Pvl::Replace);
      }
  
      if (ui.WasEntered("MINLON")) {
        mapGrp.addKeyword(PvlKeyword("MinimumLongitude",
                                     std::to_string(ui.GetDouble("MINLON"))), Pvl::Replace);
      }
  
      if (ui.WasEntered("MAXLON")) {
        mapGrp.addKeyword(PvlKeyword("MaximumLongitude",
                                     std::to_string(ui.GetDouble("MAXLON"))), Pvl::Replace);
      }
  
      //If the pixel resolution is to be computed, compute the pixels/degree from the input
      if (ui.GetString("PIXRES") == "COMPUTE") {
        latBrick.SetBasePosition(1, 1, 1);
        latCube->read(latBrick);
  
        lonBrick.SetBasePosition(1, 1, 1);
        lonCube->read(lonBrick);
  
        //Read the lat and long at the upper left corner
        double a = latBrick.at(0) * PI / 180.0;
        double c = lonBrick.at(0) * PI / 180.0;
  
        latBrick.SetBasePosition(latCube->sampleCount(), latCube->lineCount(), 1);
        latCube->read(latBrick);
  
        lonBrick.SetBasePosition(lonCube->sampleCount(), lonCube->lineCount(), 1);
        lonCube->read(lonBrick);
  
        //Read the lat and long at the lower right corner
        double b = latBrick.at(0) * PI / 180.0;
        double d = lonBrick.at(0) * PI / 180.0;
  
        //Determine the angle between the two points
        double angle = acos(cos(a) * cos(b) * cos(c - d) + sin(a) * sin(b));
        //double angle = acos((cos(a1) * cos(b1) * cos(b2)) + (cos(a1) * sin(b1) * cos(a2) * sin(b2)) + (sin(a1) * sin(a2)));
        angle *= 180 / PI;
  
        //Determine the number of pixels between the two points
        double pixels = sqrt(pow(latCube->sampleCount() - 1.0, 2.0) + pow(latCube->lineCount() - 1.0, 2.0));
  
        //Add the scale in pixels/degree to the mapping group
        mapGrp.addKeyword(PvlKeyword("Scale",
                                     std::to_string(pixels / angle), "pixels/degree"),
                          Pvl::Replace);
        if (mapGrp.hasKeyword("PixelResolution")) {
          mapGrp.deleteKeyword("PixelResolution");
        }
      }
  
  
      // If the user decided to enter a resolution then override
      if (ui.GetString("PIXRES") == "MPP") {
        mapGrp.addKeyword(PvlKeyword("PixelResolution",
                                     std::to_string(ui.GetDouble("RESOLUTION")), "meters/pixel"),
                          Pvl::Replace);
        if (mapGrp.hasKeyword("Scale")) {
          mapGrp.deleteKeyword("Scale");
        }
      }
      else if (ui.GetString("PIXRES") == "PPD") {
        mapGrp.addKeyword(PvlKeyword("Scale",
                                     std::to_string(ui.GetDouble("RESOLUTION")), "pixels/degree"),
                          Pvl::Replace);
        if (mapGrp.hasKeyword("PixelResolution")) {
          mapGrp.deleteKeyword("PixelResolution");
        }
      }
  
      //Create a projection using the map file we created
      int samples, lines;
      TProjection *outmap = (TProjection *) ProjectionFactory::CreateForCube(mapFile, samples, lines,
                                                                             false);
  
      //Create a process rubber sheet
      ProcessRubberSheet r;
  
      //Set the input cube
      r.SetInputCube(inCube);
  
      double tolerance = ui.GetDouble("TOLERANCE") * outmap->Resolution();
  
      //Create a new transform object
      Transform *transform = new NoCam2Map(sampSol, lineSol, outmap,
                                           latCube, lonCube,
                                           ui.GetString("LATTYPE") == "PLANETOCENTRIC",
                                           ui.GetString("LONDIR") == "POSITIVEEAST",
                                           tolerance, ui.GetInteger("ITERATIONS"),
                                           inCube->sampleCount(), inCube->lineCount(),
                                           samples, lines);
  
      //Allocate the output cube and add the mapping labels
      Cube *oCube = r.SetOutputCube(ui.GetCubeName("TO"), ui.GetOutputAttribute("TO"), transform->OutputSamples(),
                                    transform->OutputLines(),
                                    inCube->bandCount());
      oCube->putGroup(mapGrp);
      
      PvlGroup kernels;
      Pvl *label=oCube->label();
      
      if ( oCube->hasGroup("Kernels") ) {
        kernels=oCube->group("Kernels");
        DeleteTables(label, kernels);
        oCube->deleteGroup("Kernels");
      }
      if ( label->hasObject("NaifKeywords") ) {
        label->deleteObject("NaifKeywords");
      } 
  
      //Determine which interpolation to use
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
  
      //Warp the cube
      r.StartProcess(*transform, *interp);
      r.EndProcess();
  
      // add mapping to print.prt
      PvlGroup mapping = outmap->Mapping();

      Application::Log(mapping);

      //Clean up
      delete latCube;
      delete lonCube;
  
      delete outmap;
      delete transform;
      delete interp;
    }
  }
  
  
  // Transform object constructor
  NoCam2Map::NoCam2Map(LeastSquares sample, LeastSquares line, TProjection *outmap,
                       Cube *latCube, Cube *lonCube,
                       bool isOcentric, bool isPosEast,
                       double tolerance, int iterations,
                       const int inputSamples, const int inputLines,
                       const int outputSamples, const int outputLines) {
  
    p_sampleSol = &sample;
    p_lineSol = &line;
    p_outmap = outmap;
    p_inputSamples = inputSamples;
    p_inputLines = inputLines;
    p_outputSamples = outputSamples;
    p_outputLines = outputLines;
    p_latCube = latCube;
    p_lonCube = lonCube;
    p_isOcentric = isOcentric;
    p_isPosEast = isPosEast;
    p_tolerance = tolerance;
    p_iterations = iterations;
    p_latCenter = p_latCube->statistics()->Average() * PI / 180.0;
    p_lonCenter = p_lonCube->statistics()->Average() * PI / 180.0;
    p_radius = p_outmap->LocalRadius(p_latCenter);
  }
  
  
  
  // Transform method mapping output line/samps to lat/lons to input line/samps
  bool NoCam2Map::Xform(double &inSample, double &inLine,
                        const double outSample, const double outLine) {
    if (!p_outmap->SetWorld(outSample, outLine)) return false;
  
    if (outSample > p_outputSamples) return false;
    if (outLine > p_outputLines) return false;
  
    //Get the known latitude and longitudes from the projection
    //Convert to the input's latitude/longitude domain if necessary
    double lat_known, lon_known;
  
    if (p_outmap->IsPlanetocentric()) {
      if (!p_isOcentric) lat_known = p_outmap->ToPlanetographic(p_outmap->Latitude());
      else lat_known = p_outmap->Latitude();
    }
    else {
      if (p_isOcentric) lat_known = p_outmap->ToPlanetocentric(p_outmap->Latitude());
      else lat_known = p_outmap->Latitude();
    }
  
    if (p_outmap->IsPositiveEast()) {
      if (!p_isPosEast) lon_known = p_outmap->ToPositiveWest(p_outmap->Longitude(), 360);
      else lon_known = p_outmap->Longitude();
    }
    else {
      if (p_isPosEast) lon_known = p_outmap->ToPositiveEast(p_outmap->Longitude(), 360);
      else lon_known = p_outmap->Longitude();
    }
  
    lat_known *= PI / 180.0;
    lon_known *= PI / 180.0;
  
    //Project the known lat/long to x/y using the stereographic projection
    double k_known = 2 / (1 + sin(p_latCenter) * sin(lat_known) + cos(p_latCenter) * cos(lat_known) * cos(lon_known - p_lonCenter));
    double x_known = k_known * cos(lat_known) * sin(lon_known - p_lonCenter);
    double y_known = k_known * (cos(p_latCenter) * sin(lat_known)) - (sin(p_latCenter) * cos(lat_known) * cos(lon_known - p_lonCenter));
  
    vector<double> data_known;
    data_known.push_back(x_known);
    data_known.push_back(y_known);
  
    //Get the sample/line guess from the least squares solutions
    double sample_guess = p_sampleSol->Evaluate(data_known);
    double line_guess = p_lineSol->Evaluate(data_known);
  
    //If the sample/line guess is out of bounds return false
    if (sample_guess < -1.5) return false;
    if (line_guess < -1.5) return false;
    if (sample_guess > p_inputSamples + 1.5) return false;
    if (line_guess > p_inputLines + 1.5) return false;
  
    if (sample_guess < 0.5) sample_guess = 1;
    if (line_guess < 0.5) line_guess = 1;
    if (sample_guess > p_inputSamples + 0.5) sample_guess = p_inputSamples;
    if (line_guess > p_inputLines + 0.5) line_guess = p_inputLines;
  
    //Create a bilinear interpolator
    Interpolator interp(Interpolator::BiLinearType);
  
    //Create a 2x2 buffer to read the lat and long cubes
    Portal latPortal(interp.Samples(), interp.Lines(),
                     p_latCube->pixelType() ,
                     interp.HotSample(), interp.HotLine());
  
    Portal lonPortal(interp.Samples(), interp.Lines(),
                     p_lonCube->pixelType() ,
                     interp.HotSample(), interp.HotLine());
  
    //Set the buffers positions to the sample/line guess and read from the lat/long cubes
    latPortal.SetPosition(sample_guess, line_guess, 1);
    p_latCube->read(latPortal);
  
    lonPortal.SetPosition(sample_guess, line_guess, 1);
    p_lonCube->read(lonPortal);
  
    //Get the lat/long guess from the interpolator
    double lat_guess = interp.Interpolate(sample_guess, line_guess, latPortal.DoubleBuffer()) * PI / 180.0;
    double lon_guess = interp.Interpolate(sample_guess, line_guess, lonPortal.DoubleBuffer()) * PI / 180.0;
  
    //Project the lat/long guess to x/y using the stereographic projection
    double k_guess = 2 / (1 + sin(p_latCenter) * sin(lat_guess) + cos(p_latCenter) * cos(lat_guess) * cos(lon_guess - p_lonCenter));
    double x_guess = k_guess * cos(lat_guess) * sin(lon_guess - p_lonCenter);
    double y_guess = k_guess * (cos(p_latCenter) * sin(lat_guess)) - (sin(p_latCenter) * cos(lat_guess) * cos(lon_guess - p_lonCenter));
  
    //Calculate the difference between the known x/y to the x/y from our least squares solutions
    double x_diff = abs(x_guess - x_known) * p_radius;
    double y_diff = abs(y_guess - y_known) * p_radius;
  
    //If the difference is above the tolerance, correct it until it is below the tolerance or we have iterated through a set amount of times
    int iteration = 0;
    while (x_diff > p_tolerance || y_diff > p_tolerance) {
      if (iteration++ >= p_iterations) return false;
  
      //Create a 1st order polynomial function
      PolynomialBivariate sampFunct(1);
      PolynomialBivariate lineFunct(1);
  
      //Create a least squares solution
      LeastSquares sampConverge(sampFunct);
      LeastSquares lineConverge(lineFunct);
  
      //Add the points around the line/sample guess point to the least squares matrix
      for (int i = (int)(line_guess + 0.5) - 1; i <= (int)(line_guess + 0.5) + 1; i++) {
        //If the line is out of bounds, then skip it
        if (i < 1 || i > p_inputLines) continue;
        for (int j = (int)(sample_guess + 0.5) - 1; j <= (int)(sample_guess + 0.5) + 1; j++) {
          //If the sample is out of bounds, then skip it
          if (j < 1 || j > p_inputSamples) continue;
  
          latPortal.SetPosition(j, i, 1);
          p_latCube->read(latPortal);
          if (IsSpecial(latPortal.at(0))) continue;
          double n_lat = latPortal.at(0) * PI / 180.0;
  
          lonPortal.SetPosition(j, i, 1);
          p_lonCube->read(lonPortal);
          if (IsSpecial(lonPortal.at(0))) continue;
          double n_lon = lonPortal.at(0) * PI / 180.0;
  
          //Conver the lat/lon to x/y using the stereographic projection
          double n_k = 2 / (1 + sin(p_latCenter) * sin(n_lat) + cos(p_latCenter) * cos(n_lat) * cos(n_lon - p_lonCenter));
          double n_x = n_k * cos(n_lat) * sin(n_lon - p_lonCenter);
          double n_y = n_k * (cos(p_latCenter) * sin(n_lat)) - (sin(p_latCenter) * cos(n_lat) * cos(n_lon - p_lonCenter));
  
          //Add the points to the least squares solution
          vector<double> data;
          data.push_back(n_x);
          data.push_back(n_y);
          sampConverge.AddKnown(data, j);
          lineConverge.AddKnown(data, i);
        }
      }
      //TODO: What if solve can't and throws an error?
  
      //Solve the least squares functions
      sampConverge.Solve(LeastSquares::QRD);
      lineConverge.Solve(LeastSquares::QRD);
  
      //Try to solve the known data with our new function
      sample_guess = sampConverge.Evaluate(data_known);
      line_guess = lineConverge.Evaluate(data_known);
  
      //If the new sample/line is out of bounds return false
      if (sample_guess < -1.5) return false;
      if (line_guess < -1.5) return false;
      if (sample_guess > p_inputSamples + 1.5) return false;
      if (line_guess > p_inputLines + 1.5) return false;
  
      if (sample_guess < 0.5) sample_guess = 1;
      if (line_guess < 0.5) line_guess = 1;
      if (sample_guess > p_inputSamples + 0.5) sample_guess = p_inputSamples;
      if (line_guess > p_inputLines + 0.5) line_guess = p_inputLines;
  
      //Set the buffers positions to the sample/line guess and read from the lat/long cubes
      latPortal.SetPosition(sample_guess, line_guess, 1);
      p_latCube->read(latPortal);
  
      lonPortal.SetPosition(sample_guess, line_guess, 1);
      p_lonCube->read(lonPortal);
  
      //Get the lat/long guess from the interpolator
      lat_guess = interp.Interpolate(sample_guess, line_guess, latPortal.DoubleBuffer()) * PI / 180.0;
      lon_guess = interp.Interpolate(sample_guess, line_guess, lonPortal.DoubleBuffer()) * PI / 180.0;
  
      //Project the lat/long guess to x/y using the stereographic projection
      k_guess = 2 / (1 + sin(p_latCenter) * sin(lat_guess) + cos(p_latCenter) * cos(lat_guess) * cos(lon_guess - p_lonCenter));
      x_guess = k_guess * cos(lat_guess) * sin(lon_guess - p_lonCenter);
      y_guess = k_guess * (cos(p_latCenter) * sin(lat_guess)) - (sin(p_latCenter) * cos(lat_guess) * cos(lon_guess - p_lonCenter));
  
      //Calculate the difference between the known x/y to the x/y from our least squares solutions
      x_diff = abs(x_guess - x_known) * p_radius;
      y_diff = abs(y_guess - y_known) * p_radius;
    }
  
  
    //Set the input sample/line to the sample/line we've determined to be the closest fit
    inSample = sample_guess;
    inLine = line_guess;
    return true;
  }
  
  
  // Function to delete unwanted tables in header
  void DeleteTables(Pvl *label, PvlGroup kernels) {
    //Delete any tables in header corresponding to the Kernel
    const QString tableStr("Table");
    const QString nameStr("Name");
  
  
    //Setup a list of tables to delete with predetermined values and any tables in the kernel.
    //If additional tables need to be removed, they can be added to the list of tables that
    //detine the 'tmpTablesToDelete' QString array directly below.  
    QString tmpTablesToDelete[] = {"SunPosition","BodyRotation","InstrumentPointing",
                                                                "InstrumentPosition"};
    std::vector<QString> tablesToDelete;
    int sizeOfTablesToDelete = (int) sizeof(tmpTablesToDelete)/sizeof(*tmpTablesToDelete);
    for (int i = 0; i < sizeOfTablesToDelete; i++) {
      tablesToDelete.push_back( tmpTablesToDelete[i] );
    }
    for (int j=0; j < kernels.keywords(); j++) {   
      if (kernels[j].operator[](0) == tableStr.toStdString())  {
        bool newTableToDelete=true;
        for (int k = 0; k<sizeOfTablesToDelete; k++) {
          if ( tablesToDelete[k].toStdString() == kernels[j].name() ) {
            newTableToDelete=false;
            break;
          }
        }
        if (newTableToDelete) {
          tablesToDelete.push_back( QString::fromStdString(kernels[j].name()) );
          sizeOfTablesToDelete++;
        }
      }
    } 
    int tablesToDeleteSize = (int) tablesToDelete.size();
    //Now go through and find all entries in the label corresponding to our unwanted keywords
    std::vector<int> indecesToDelete;
    int indecesToDeleteSize=0;
    for (int k=0; k < label->objects(); k++) {
      PvlObject &currentObject=(*label).object(k);
      if (currentObject.name() == tableStr.toStdString()) {
        PvlKeyword &nameKeyword = currentObject.findKeyword(nameStr.toStdString());
        for (int l=0; l < tablesToDeleteSize; l++) {
          if ( nameKeyword[0] == tablesToDelete[l].toStdString() ) {
            indecesToDelete.push_back(k-indecesToDeleteSize);
            indecesToDeleteSize++;
            //(*label).deleteObject(k);
            //tableDeleted = true;
            break;
          }
        }
      }
    }
    //Now go through and delete the corresponding tables
    for (int k=0; k < indecesToDeleteSize; k++) {
      (*label).deleteObject(indecesToDelete[k]);
    }
  }

  
  int NoCam2Map::OutputSamples() const {
    return p_outputSamples;
  }
  
  int NoCam2Map::OutputLines() const {
    return p_outputLines;
  }
  

}