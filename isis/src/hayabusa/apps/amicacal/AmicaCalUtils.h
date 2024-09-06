#ifndef AmicaCalUtils_h
#define AmicaCalUtils_h

/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <cmath>
#include <string>
#include <vector>
#include <QString>

#include "Camera.h"
#include "CSVReader.h"
#include "IException.h"
#include "iTime.h"
#include "FileName.h"
#include "LineManager.h"
#include "NaifStatus.h"
#include "IString.h"
#include "Pvl.h"
#include "PvlGroup.h"

// OpenCV libraries
#include <opencv2/opencv.hpp>

/**
 * @author 2016-04-04 Tyler Wilson
 *
 *
 */
using namespace cv;
using namespace std;

#define KM_PER_AU 149597871

namespace Isis {


/**
 * @brief Load required NAIF kernels required for timing needs.
 *
 * This method maintains the loading of kernels for HAYABUSA timing and
 * planetary body ephemerides to support time and relative positions of planet
 * bodies.
 */

static void loadNaifTiming() {
  static bool naifLoaded = false;
  if (!naifLoaded) {

//  Load the NAIF kernels to determine timing data
    Isis::FileName leapseconds("$base/kernels/lsk/naif????.tls");
    leapseconds = leapseconds.highestVersion();
    Isis::FileName sclk("$hayabusa/kernels/sclk/hayabusa.tsc");
    Isis::FileName pck1("$hayabusa/kernels/tspk/de403s.bsp");
    Isis::FileName pck2("$hayabusa/kernels/tspk/sb_25143_140.bsp");
    Isis::FileName pck3("$hayabusa/kernels/spk/hay_jaxa_050916_051119_v1n.bsp");
    Isis::FileName pck4("$hayabusa/kernels/spk/hay_osbj_050911_051118_v1n.bsp");

//  Load the kernels
    QString leapsecondsName(leapseconds.expanded());
    QString sclkName(sclk.expanded());

    QString pckName1(pck1.expanded());
    QString pckName2(pck2.expanded());
    QString pckName3(pck3.expanded());
    QString pckName4(pck4.expanded());

    furnsh_c(leapsecondsName.toLatin1().data());
    furnsh_c(sclkName.toLatin1().data());

    furnsh_c(pckName1.toLatin1().data());
    furnsh_c(pckName2.toLatin1().data());
    furnsh_c(pckName3.toLatin1().data());
    furnsh_c(pckName4.toLatin1().data());


//  Ensure it is loaded only once
    naifLoaded = true;
  }
  return;
}


/**
 * @brief Computes the distance from the Sun to the observed body.
 *
 * This method requires the appropriate NAIK kernels to be loaded that
 * provides instrument time support, leap seconds and planet body ephemeris.
 *
 * @return @b double Distance in AU between Sun and observed body.
 */
static bool sunDistanceAU(Cube *iCube,
                          const QString &scStartTime,
                          const QString &target,
                          double &sunDist) {

  try {
    Camera *cam; 
    cam = iCube->camera();
    cam->SetImage (0.5, 0.5);
    sunDist = cam->sunToBodyDist() / KM_PER_AU;
  }
  catch(IException &e) {
    try {
      //  Ensure NAIF kernels are loaded
      loadNaifTiming();
      sunDist = 1.0;

      NaifStatus::CheckErrors();

      //  Determine if the target is a valid NAIF target
      SpiceInt tcode;
      SpiceBoolean found;
      bodn2c_c(target.toLatin1().data(), &tcode, &found);

      if (!found) return false;

      //  Convert starttime to et
      double obsStartTime;
      scs2e_c(-130, scStartTime.toLatin1().data(), &obsStartTime);

      //  Get the vector from target to sun and determine its length
      double sunv[3];
      double lt;
      spkpos_c(target.toLatin1().data(), obsStartTime, "J2000", "LT+S", "sun", sunv, &lt);
      NaifStatus::CheckErrors();

      double sunkm = vnorm_c(sunv);

      //  Return in AU units
      sunDist = sunkm / KM_PER_AU;
    }
    catch (IException &e) {
      std::string message = "Failed to calculate the sun-target distance.";
      throw IException(e, IException::User, message, _FILEINFO_);
    }
  }

  return true;
}


/**
 * @brief Translates a 1-banded Isis::Cube to an OpenMat object
 *
 * @author 2016-04-19 Tyler Wilson
 *
 * @param icube A pointer to the input cube
 *
 * @return @b Mat A pointer to the OpenMat object
 */
Mat * isis2mat(Cube *icube) {

  int nlines = icube->lineCount();
  int nsamples = icube->sampleCount();
  Mat *matrix = new Mat(nlines,nsamples,CV_64F);


  // Set up line manager and read in the data
  LineManager linereader(*icube);
  for (int line = 0 ; line < nlines ; line++) {
    linereader.SetLine(line+1);
    icube->read(linereader);
    for (int samp = 0 ;  samp < nsamples ; samp++) {
      matrix->at<double>(line,samp) = (double)linereader[samp];
    }
 }


return matrix;

}


/**
 * @brief Translates an OpenMat object to an ISIS::Cube with one band
 *
 * @author 2016-04-19 Tyler Wilson
 *
 * @param matrix A pointer to the OpenMat object
 *
 * @param cubeName The name of the Isis::Cube that is being created.
 *
 */
void mat2isis(Mat *matrix, QString cubeName) {

  int nlines = matrix->rows;
  int nsamples = matrix->cols;
  CubeAttributeOutput set;
  set.setPixelType(Real);

  Cube ocube;
  ocube.setDimensions(nsamples,nlines,1);
  ocube.create(cubeName,set);

  LineManager linewriter(ocube);

  for (int line =0; line < nlines; line++) {
    linewriter.SetLine(line+1);

    for ( int samp=0; samp<nsamples; samp++ ) {

      linewriter[samp] = matrix->at<double>(linewriter.Line()-1,samp);

    }
    ocube.write(linewriter);

  }

}


/**
 * @brief Translates/scales a cube using Bilinear Interpolation
 *
 * @author 2016-04-19 Tyler Wilson
 *
 * @param matrix A pointer to the OpenMat object
 *
 * @param cubeName The name of the ISIS::Cube that is being created.
 *
 */
void  translate(Cube *flatField, int *transform, QString fname) {

  Mat * originalMat = isis2mat(flatField);
  int scale = transform[0];

  int startsample = transform[1];
  int startline = transform[2];
  int lastsample = transform[3];
  int lastline = transform[4];

  int width  = (lastsample-startsample);
  int height = (lastline-startline);

  Size sz(flatField->lineCount()/scale,flatField->sampleCount()/scale);

  Mat * resizedMatrix = new Mat();

  Mat temp = *originalMat;


  Mat originalCropped = temp(Rect(startsample,startline,width+1,height+1));


  if (scale ==1) {
    mat2isis(&originalCropped,fname);
  }
  else {
    //Bilinear interpolation
    resize(originalCropped,*resizedMatrix,sz,INTER_LINEAR);
    mat2isis(resizedMatrix,fname);
  }

}


/**
 * @brief This function characterizes the point spread function
 * for near light sources.  See equation [1] in:
 * "Scattered light correction of Hayabusa/AMICA data and quantitative spectral
 * comparisons of Itokawa", Ishiguro (2014)
 *
 * @author 2016-04-19 Tyler Wilson
 *
 * @param alpha An empirically derived constant which is stored in
 *        $hayabusa/calibration/amica/amicaCalibration????.trn"
 *        Each filter has a different alpha value.
 *
 * @param r The distance (in pixels) from the optical center of the
 *        point source
 *
 * @return @b double An estimate of the diffusion of light from near light sources
 * at pixel with coordinates (x,y) relative to the central pixel (with coordinates (0,0) ).
 *
 */
static double f_focused(double alpha,int binning,double x,double y) {

        double X = x*binning;
        double Y = y*binning;
        double r = sqrt(X*X+Y*Y);
        return exp(-alpha*r);
}


/**
 * @brief This function characterizes an attenuation function modeled as a sum
 * of Gaussian functions.  See equation [2] in:
 * "Scattered light correction of Hayabusa/AMICA data and quantitative spectral
 * comparisons of Itokawa", Ishiguro (2014)
 *
 * @author 2016-04-19 Tyler Wilson
 *
 * @param A An empirically derived vector of constants of length N which is stored in
 *        $hayabusa/calibration/amica/amicaCalibration????.trn"
 *        Each filter has a different A vector.
 *
 * @param sigma An empirically derived vector of standard deviations of length N
 *        also stored in $hayabusa/calibration/amica/amicaCalibration????.trn"
 * @param N  The number of filter-bands (N=6)
 * @param binning  The number of lines/samples which are binned together.
 * @param x  The x-coordinate in pixels of the current pixel we are evaluating.
 * @param y  The y-coordinate in pixels of the current pixel we are evaluating.
 *
 * @return @b double An estimate of diffuse light at the point (x,y) relative to the
 * central pixel (at coordinates (0,0) ).
 *
 */

static double f_unfocused(double * A,double * sigma, int N,int binning,double x,double y)  {


  double X = binning*x;
  double Y = binning*y;

  double r = sqrt(X*X+Y*Y);
  double sum = 0;

  for (int i = 0; i < N; i ++)   {
    sum += (A[i]/(sigma[i]*sqrt(2.0*pi_c() ) ) )*exp(-(r*r)/(2*sigma[i]*sigma[i]) );
  }


  return sum;

}


/**
 * @brief This function returns a matrix of [size x size] of light distribution values.
 * The center pixel value is at the center of the matrix, and the values around the central
 * pixel represent the fraction of light intensity from the central pixel that seeps into the
 * neighboring pixels.
 * @param size  The dimension of the matrix (in pixels).
 * @param A An empirically derived vector of constants of length N which is stored in
 *        $hayabusa/calibration/amica/amicaCalibration????.trn"
 *        Each filter has a different A vector.
 * @param sigma sigma An empirically derived vector of standard deviations of length N
 *        also stored in $hayabusa/calibration/amica/amicaCalibration????.trn"
 * @param alpha  alpha An empirically derived constant which is stored in
 *        $hayabusa/calibration/amica/amicaCalibration????.trn"  Each filter has a different
 *        alpha value.
 * @param N
 * @param binning
 * @return @b double * A pointer to a [size x size] matrix of light distribution values.
 */
double * setPSFFilter(int size, double *A,double *sigma, double alpha,int N,int binning) {


  double * psfVals = new double[size*size];

  int i = 0;

  for (double y = -(size / 2) ; y <= (size / 2) ; y++) {
    for (double x = -(size / 2) ; x <= (size / 2) ; x++) {
       if (x == 0 && y ==0) {
         psfVals[i] = 0;
         i++;
       }
       else {
         psfVals[i]=f_unfocused(A,sigma,N,binning,x,y) +f_focused(alpha,binning,x,y);
         i++;
       }
    }
  }

  return psfVals;
}


}



#endif
