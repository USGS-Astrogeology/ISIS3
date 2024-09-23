/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

/**
 * @Brief This application creates three flatfield (Sensitivity Non-Uniformity Matrix) cubes used for calibration
 *
 * This application creates three flatfield (Sensitivity Non-Uniformity Matrix) cubes used for calibration.
 * The cubes consist of median, mean, and standard deviation values per pixel. Process varies for the three
 * cameras this application can be used for but general pixel stacking column approach is the same. The
 * three camera types are line-scan, push-frame, and framing. Invalid pixel values are changed to Isis::Null.
 *
 * The application uses a two step process.
 *
 * Step #1 - STACKING PIXELS INTO PIXEL COLUMNS
 *
 * The first part of the process is to stack cube pixels at their respective pixel locations.
 *
 * These stacked pixels are processed to produce the median, mean, and standard deviation values for the
 * pixel location, and to toss out invalid pixel values.  Normalization can also be done at this time if
 * images were not normalized prior. The mean is used to normalize images.
 *
 * Step #2 - PROCESSING STACKED PIXEL COLUMNS AND TRANSFERRING TO SNU
 *
 * Once pixels have been stacked into pixel columns mean and median averages, and standard devation for that
 * pixel location is saved to the SNU matrix (flatfield cubes).
 *
 * Mean
 *  -The summed pixels are divided by the number of pixels.
 * Standard Deviation
 *  -The mean average is used to calculate the standard deviation.
 * Median
 *  -The pixel column is sorted and the median is obtained.
 *
 * @author Victor Silva - 2016-08-17
 *
 * @internal
 *   @history 2016-08-17 Victor Silva - New app for lroc adapted from makeflat app
 *
 */
#include "Isis.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <new>
#include <string>
#include <stdlib.h>
#include <vector>

#include <QString>

#include "FileList.h"
#include "Histogram.h"
#include "ImageHistogram.h"
#include "IException.h"
#include "LineManager.h"
#include "PvlGroup.h"

using namespace std;
using namespace Isis;

// Container for two-dimensional matrix of doubles
typedef vector < vector<double> > Matrix2d;

// This method returns stats for a vector of doubles and excludes special pixels
void getVectorStats( vector<double> (&inVec), double (&mean), double (&median), double (&stdev) );

// Get normalization values for frames in list of cubes if so desired
void getCubeListNormalization( Matrix2d (&matrix), int (cubeWidth), int (frameHeight), long (frameLineCount), long (iLineCount) );

// Logs excluded files and frames
void exclude( int listIndex, string reason );

// Logs excluded files and frames
void exclude( int listIndex, int frame, string reason );

// Global variables
int static g_sampleCount;
int static g_numStdevs;
QString static g_msg;
Progress static g_prog;
FileList static g_list;

// This will keep track of excluded cubes from the cube list. The int is the index in the list (zero-based)
map < int, string > g_excludedCubes;

// Holds excluded frames. <cube index in list, frame index in cube> (zero-based)
map < pair<int, int>, string> g_excludedFrames;

// Holds exclusion details used for logging
vector < PvlObject > g_excludedDetails;

/**
   * @brief Main method
   *
   * Computes flatfield image for linescan, pushbroom, and framing
   * Cameras
   *
   * @throws IException::User "Only single band images accepted"
   * @throws IException::User "User selected lines value exceeds number of lines in cube"
   * @throws IException::Programmer "Could not write to output cube"
   *
   * @author 2016-08-17 Victor Silva
   *
   * @internal
   *   @history 2016-08-17 Victor Silva - New app for lroc adapted from makeflat app
   *
   */
void IsisMain() {

  // Set attributes
  UserInterface &ui(Application::GetUserInterface());
  g_list = FileName(ui.GetFileName("FROMLIST").toStdString());
  g_numStdevs = abs(ui.GetDouble("STDEVTOL"));

  // Input cube attributes
  bool normalize = false;
  long iLineCount, oLineCount; // sample count is the same for both input/output cubes
  int cubeWidthPixels = 0, frameHeightLines = 0, framesPerCube, frameLineCount;

  // Setup Sensitivity Non-uniformity Matrix cubes (flatfielding matrix) and line managers
  Cube *oStdevCube = NULL, *oMedianCube = NULL, *oMeanCube = NULL;
  LineManager *oStdevlineMgr = NULL, *oMeanLineMgr = NULL, *oMedianLineMgr = NULL;

  // Get some info from first cube in list
  Cube firstCube;
  firstCube.open(QString::fromStdString(g_list[0].toString()));
  g_sampleCount = firstCube.sampleCount();
  iLineCount = firstCube.lineCount();

  if (firstCube.bandCount() != 1) {
    string err = "Only single band images accepted. \n";
    throw IException(IException::User, err, _FILEINFO_);
  }

  firstCube.close();
  // to normalize or not
  if (ui.GetString("NORMALIZATION") == "YES") {
      normalize = true;
      frameHeightLines = abs(ui.GetInteger("HEIGHT_LINES"));
      cubeWidthPixels = abs(ui.GetInteger("WIDTH_PIXELS"));
  }
  // User selection of camera type will set addt'l attributes
  if (ui.GetString("IMAGETYPE") == "FRAMING") {
    frameLineCount = oLineCount = iLineCount;
  }
  else if (ui.GetString("IMAGETYPE") == "LINESCAN") {
    oLineCount = 1;
    frameLineCount = abs(ui.GetInteger("NUMLINES"));
    if (iLineCount != frameLineCount) {
      string err = "User selected lines value (" + IString(frameLineCount) +
         ") exceeds number of lines in cube (" + IString(iLineCount) + ". \n";
      throw IException(IException::User, err, _FILEINFO_);
    }
  }
  else {
    oLineCount = frameLineCount = abs(ui.GetInteger("FRAMELETHEIGHT"));
  }
  // Number of frames in each cube
  framesPerCube = (iLineCount / frameLineCount); //integer division
  // 3d matrix of type pair to hold mean and stdev for normalization
  Matrix2d normMatrix;
  normMatrix.resize(g_list.size(), vector<double> (framesPerCube, 1.0));
  // Get normalization values from list of cubes if user selects normalization
  if (normalize) {
    getCubeListNormalization(normMatrix, cubeWidthPixels, frameHeightLines, frameLineCount, iLineCount);
  }
  // Create empty snu-flatfield output cubes and line managers
  oStdevCube = new Cube();
  oMedianCube = new Cube();
  oMeanCube = new Cube();
  // Set cube dimensions
  oStdevCube->setDimensions(g_sampleCount, oLineCount, 1);
  oMedianCube->setDimensions(g_sampleCount, oLineCount, 1);
  oMeanCube->setDimensions(g_sampleCount, oLineCount, 1);
  // Set name of cubes
  oStdevCube->create(QString::fromStdString(FileName(ui.GetCubeName("TO").toStdString()).expanded() + ".stdev.cub"));
  oMedianCube->create(QString::fromStdString(FileName(ui.GetCubeName("TO").toStdString()).expanded() + ".median.cub"));
  oMeanCube->create(QString::fromStdString(FileName(ui.GetCubeName("TO").toStdString()).expanded() + ".mean.cub"));
  // create new line manager
  oStdevlineMgr = new LineManager(*oStdevCube);
  oMedianLineMgr = new LineManager(*oMedianCube);
  oMeanLineMgr = new LineManager(*oMeanCube);
  // Set status msg
  g_msg = "Stacking pixels into columns and processing them.";
  g_prog.SetText(g_msg);
  g_prog.SetMaximumSteps(oLineCount + 1);
  // Stack pixels at each pixel location into pixel columns
  for (long oLine = 1; oLine <= oLineCount; oLine++) {
    g_prog.CheckStatus();
    // 2d matrix to hold pixel columns
    Matrix2d pixelMatrix;
    pixelMatrix.resize(g_sampleCount, vector<double>(g_list.size() * framesPerCube, Isis::Null));
    for (int listIndex = 0; listIndex < g_list.size(); listIndex++) {
      Cube tmp2;
      tmp2.open(QString::fromStdString(g_list[listIndex].toString()));
      // Only run for cubes with one band
      if (tmp2.bandCount() != 1) {
        string err = "Warning: This cube has too many bands(" + IString(tmp2.bandCount()) +
          " and will be excluded). Only single band images accepted. \n";
        cerr << err << endl;
        exclude(listIndex, err);
        // Go directly to next cube. Do not stop at GO. Do not collect $200.
        continue;
      }
      // Reset frame for every cube in list
      int frame = 0;
      // Cube line has to match oLine as we are slicing the cubes one line at a time to stack pixels
      for (long cubeLine = oLine; cubeLine <= iLineCount; cubeLine = cubeLine + frameLineCount) {
        LineManager cubeMgr(tmp2);
        cubeMgr.SetLine(cubeLine);
        tmp2.read(cubeMgr);
        double pixelVal;
        // Iterate through columns
        for (int column = 0; column < g_sampleCount; column++) {
          pixelVal = cubeMgr[column];
          int frameIndex = listIndex * framesPerCube + frame;
          // If frame avg is zero, will cause div/zero error. Set pixel to 0? nil?
          (normMatrix[listIndex][frame])? pixelVal = (pixelVal/(normMatrix[listIndex][frame])):pixelVal = Isis::Null;
          pixelMatrix[column][frameIndex] = pixelVal;
        }
        // Next frame
        frame++;
      }
      tmp2.close();
    }
    // Process stacked pixel columns and write to snu-flatfield output cubes
    // Set ouput cubeLine manager line to cube line
    oStdevlineMgr->SetLine(oLine);
    oMedianLineMgr->SetLine(oLine);
    oMeanLineMgr->SetLine(oLine);
    // Set values as we iterate through columns
    for (int column = 0; column < g_sampleCount; column++) {
      double columnMean, columnMedian, columnStdev;
      getVectorStats(pixelMatrix[column], columnMean, columnMedian, columnStdev);
      (*oStdevlineMgr)[column] = columnStdev;
      (*oMedianLineMgr)[column] = columnMedian;
      (*oMeanLineMgr)[column] = columnMean;
    }
    try {
      // Write the stats saved at the pixel location to the snu matrix cubes
      oStdevCube->write(*oStdevlineMgr);
      oMeanCube->write(*oMeanLineMgr);
      oMedianCube->write(*oMedianLineMgr);
    }
    catch (IException &e) {
      //cout << "It's no use at this point. Just go. Leave me here. I'll only slow you down.";
      string err = "Could not write to output cube " + IString(FileName(ui.GetCubeName("TO").toStdString()).expanded()) + ".\n";
      throw IException(IException::Programmer, err, _FILEINFO_);
    }
    pixelMatrix.clear();
  }
  // Clean-up
  normMatrix.clear();
  if (oStdevlineMgr ){
    delete oStdevlineMgr;
    oStdevlineMgr = NULL;
  }
  if(oMeanLineMgr){
    delete oMeanLineMgr;
    oMeanLineMgr = NULL;
  }
  if(oMedianLineMgr){
    delete oMedianLineMgr;
    oMedianLineMgr = NULL;
  }
  if(oStdevCube) {
    oStdevCube->close();
    delete oStdevCube;
    oStdevCube = NULL;
  }
  if(oMeanCube) {
    oMeanCube->close();
    delete oMeanCube;
    oMeanCube = NULL;
  }
  if(oMedianCube) {
    oMedianCube->close();
    delete oMedianCube;
    oMedianCube = NULL;
  }
  // If user wanted an exclusion file created, create it pues.
  if(ui.WasEntered("EXCLUDE")) {
    Pvl excludeFile;
    for(unsigned int i = 0; i < g_excludedDetails.size(); i++) {
      excludeFile.addObject(g_excludedDetails[i]);
    }
    excludeFile.write(FileName(ui.GetFileName("EXCLUDE").toStdString()).expanded());
  }
} // end of main


/**
 * @brief This function gets normalization values for a list of cubes
 *
 * It uses the user specified area of the image to be used for normalizing all
 * the values at each pixel location. The user enters a height and width
 * percentage of the frame.
 *
 * @param matrix  3D matrix that holds cube frame information such as stdev/normAvg
 * @param int cubeWidth size of cube in samples
 * @param int frameHeight size of frame in rows
 * @param long frameLineCount number of lines to make a frame
 * @param long iLineCount number of lines total in cube
 *
 * @throws IException::User "This selection will yield less than 1 pixel (width). This is not enough to normalize."
 * @throws IException::User "This percentage will yield less than 1 line. This is not enough to normalize."
 *
 * @author 2016-08-17 Victor Silva
 *
 * @internal
 *   @history 2016-08-17 Victor Silva - New app for lroc adapted from makeflat app
 *
 */
void getCubeListNormalization(Matrix2d &matrix, int cubeWidth, int frameHeight, long frameLineCount, long iLineCount) {

  for(int listIndex = 0; listIndex < g_list.size(); listIndex++) {
    // open cubes in list unless they have already been excluded
    if( g_excludedCubes.find(listIndex) != g_excludedCubes.end() ) {
      continue;
    }
    Cube tmp;
    tmp.open(QString::fromStdString(g_list[listIndex].toString()));
    PvlGroup normResults("NormalizationResults");
    g_msg = "Getting frame mean avg " + QString::number((int) listIndex + 1) + "/";
    g_msg += QString::number((int) g_list.size()) + " (" + QString::fromStdString(g_list[listIndex].name()) + ")";
    g_prog.SetText(g_msg);
    g_prog.SetMaximumSteps(g_list.size());
    g_prog.CheckStatus();
    // width:
    int startSample = 0, endSample = g_sampleCount;
    if (cubeWidth){
      if(cubeWidth <= g_sampleCount) {
        startSample = (g_sampleCount - cubeWidth) / 2;
        endSample = (g_sampleCount - 1) - startSample;
      }
      else {
        string err = "This selection will yield less than 1 pixel (width). This is not enough to normalize. \n";
        throw IException(IException::User, err, _FILEINFO_);
      }
    }
    // height:
    long startLine, endLine;
    int lineShift = 0;
    (frameHeight == 0) ? frameHeight = frameLineCount : lineShift = (frameLineCount - frameHeight)/ 2;
    for(long cubeLine = 1; cubeLine <= iLineCount; cubeLine = cubeLine + frameLineCount) {
      startLine = cubeLine + lineShift;
      endLine = startLine + frameHeight;

      if( (startLine < 0)||(endLine - startLine) < 0) {
        string err = "This percentage will yield less than 1 line. This is not enough to normalize. \n";
        throw IException(IException::User, err, _FILEINFO_);
      }
      Histogram hist = ImageHistogram(tmp, 1, &g_prog, startSample, startLine, endSample, endLine, 0, true);
      int frame = cubeLine/frameLineCount;
      double normalizationAverage = hist.Average();
      matrix[listIndex][frame] = normalizationAverage;
      normResults += PvlKeyword("FileName", g_list[listIndex].toString());
      normResults += PvlKeyword("Frame", toString(frame));
      normResults += PvlKeyword("Frame_MeanAverage", toString(normalizationAverage));
      Application::Log(normResults);
    }
    tmp.close();
  }
}


/**
 * @brief This function returns the median, mean, and standard deviation for a vector
 *        of valid pixels
 *
 * It will return Isis::Null if it doesn't find any valid pixels. The function builds
 * a vector of valid pixels. It also calculates the mean and standard deviation of the
 * vector. The function also sorts the vector and finds the median. If there are an even
 * number of valid pixels, it will average (mean) the two middle pixels and return
 * that average as the median. It does not return the median pixel but rather its
 * value or average value of the two middle pixels.
 *
 * @param inVec   Vector of pixel values (double)
 * @param mean    Value of vector mean (double)
 * @param median  Value of vector median (double)
 * @param stdev   Value of vector stdev (double)
 *
 * @author 2016-08-17 Victor Silva
 *
 * @internal
 *   @history 2016-08-17 Victor Silva - New app for lroc adapted from makeflat app
 */
void getVectorStats(vector<double> &inVec, double &mean, double &median, double &stdev) {
  // Set defaults
  int medianIndex = 0, v_size = 0;
  double sum = 0.0, devSum = 0.0;
  median = Isis::Null;
  mean = Isis::Null;
  stdev = Isis::Null;
  // Check for empty initial vector
  if(!(inVec.empty())) {
    vector <double> v1;
    v_size = int(inVec.size());
    // strip invalid pixels
    for (int i = 0; i < v_size; i++) {
      if (!(IsNullPixel(inVec[i]) || IsSpecial(inVec[i]))){
        v1.push_back(inVec[i]);
        sum += inVec[i];
      }
    }
    // check to see if we got any valid pixels
    if (!(v1.empty())) {
      v_size = int(v1.size());
      // First pass - toss out outliers
      double tempMean = sum / v_size;
      for (int i = 0; i < v_size; i++) {
        devSum += pow((v1[i] - tempMean), 2);
      }
      double tempStdev = sqrt(devSum / (v_size));
      // Reset for second pass
      devSum = 0;
      sum = 0;
      vector <double> v2;
      for(int i = 0; i < v_size; i++ ){
        if( (abs(tempMean - v1[i])) <= (tempStdev * g_numStdevs) ){
          v2.push_back(v1[i]);
          sum += v1[i];
        }
      }
      // Second pass - get stats
      if (!(v2.empty())) {
        v_size = int(v2.size());
        // mean
        mean = sum / v_size;
        // stdev process
        for ( int i = 0; i < v_size; i++ ) {
          devSum += pow((v2[i] - mean), 2);
        }
        stdev= sqrt(devSum / (v_size));
        // median
        bool even = ((v_size % 2) == 0);
        sort(v2.begin(), v2.end());
        medianIndex = (v_size - 1) / 2;
        if (even) {
          median = double(((v2[medianIndex]) + (v2[medianIndex + 1])) / 2.0000);
        }
        else {
          median = double(v2[medianIndex]);
        }
        v2.clear();
      }
      v1.clear();
    }
  }
}


/**
 * @brief This function excludes cubes and logs them
 * Adds excluded cube files to containers that are used when creating the
 * exclusion file. It also logs the exclusions to the application log window.
 *
 * @param listIndex  Location in list for a cube (zero-indexed)
 * @param reason     Exclusion reason
 *
 * @author 2016-08-17 Victor Silva
 *
 * @internal
 *   @history 2016-08-17 Victor Silva - New app for lroc adapted from makeflat app
 */
void exclude(int listIndex, string reason) {
  if(g_excludedCubes.find(listIndex) == g_excludedCubes.end()) {
    g_excludedCubes.insert(pair<int, string>(listIndex, reason));
    PvlObject exclusion("Excluded_Items");
    PvlGroup excludedFiles("Excluded_Files");
    excludedFiles += PvlKeyword("FileName", g_list[listIndex].toString());
    excludedFiles += PvlKeyword("Reason", reason);
    Application::Log(excludedFiles);
    exclusion.addGroup(excludedFiles);
    g_excludedDetails.push_back(exclusion);
  }
}


/**
 * @brief This function excluded frames and logs them
 * Adds excluded frames in the cube files to containers that are used when
 * creating the exclusion file. It also logs the exclusions to the application
 * log window.
 *
 * param listIndex          Location in list for a cube (zero-indexed)
 * param frame              Location in cube for frame (zero-indexed)
 * param reason             Exclusion reason
 *
 * @author 2016-08-17 Victor Silva
 *
 * @internal
 *   @history 2016-08-17 Victor Silva - New app for lroc adapted from makeflat app
 */
void exclude(int listIndex, int frame, string reason){
  if(g_excludedFrames.find(pair<int,int>(listIndex,frame)) == g_excludedFrames.end()) {
    g_excludedFrames.insert(pair < pair < int, int > , string > (pair<int, int>(listIndex, frame), reason));
    PvlObject exclusion("Excluded_Items");
    PvlGroup excludedFiles("Excluded_Frames");
    excludedFiles += PvlKeyword("Frame_from_cube", g_list[listIndex].toString());
    excludedFiles += PvlKeyword("Frame_number", toString(frame));
    excludedFiles += PvlKeyword("Exclusion_reason", reason);
    Application::Log(excludedFiles);
    exclusion.addGroup(excludedFiles);
    g_excludedDetails.push_back(exclusion);
  }
}
