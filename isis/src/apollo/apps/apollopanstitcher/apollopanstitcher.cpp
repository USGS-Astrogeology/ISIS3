/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

/*
Program uses the fiducial marks along the edges of Apollo Panoramic Images to stich eight sub-scans into a single conitinuous image

*/
#include "apollopanstitcher.h"

//C++ standard libraries if needed
#include <math.h>
#include <stdio.h>

//QT libraries if needed if needed
#include <QFile>

//third party libraries if needed
#include <Ransac.h>

//Isis Headers if needed
#include  "AutoReg.h"
#include  "AutoRegFactory.h"
#include  "Centroid.h"
#include  "CentroidApolloPan.h"
#include  "Chip.h"
#include  "Cube.h"
#include  "CubeAttribute.h"
#include  "Interpolator.h"
#include  "ProcessMosaic.h"
#include  "ProcessRubberSheet.h"
#include  "Pvl.h"
#include  "PvlGroup.h"
#include  "PvlKeyword.h"
#include  "PvlObject.h"
#include  "Table.h"
#include  "Trans2d3p.h"

/**
 *
 *  @internal
 *    @History 2011-11-02 Orrin Thomas - Original Version.
 *
 *    @History 2017-06-29 Christopher Combs - Moved initialization of trans[7] to before
 *                        the loop that uses it. Fixes #4948.
 *
 */

using namespace std;
using namespace Isis;

//constants for 5 micro resolution scans
#define SCALE   10.0    //scale used for down sizingS patternS an search chips
#define SEARCHh 1400.0  //number of lines (in 5-micron-pixels) in search space for the first
                        // fiducial
#define SEARCHc  350.0  //number of samples per edge(in 5-micron-pixels) in each sub-search area
#define AVERs  5286.0   //average smaples (in 5-micron-pixels) between fiducials
#define AVERl  23459.0  //average diference (in 5-micron-pixels) between the top and bottom
                        //  fiducials
#define TRANS_N 28520.0 //nomimal dx between scan lines, scani x + 28532 =
                        //  (approx) scani+1 x
                        //  also the size of the search area for the first fiducial

double R_MOON[3];

typedef struct TRANS2d3p { //2d 3 parameter transformation
  double theta;
  double dx;
  double dy;
  double averR;  //average residual
  double maxR;  //max resiudal
  double limit[2];  //where to stop using this transformation, will eventually be a point in the
                    //coordinate system of the new cube, a vertical line through this point will
                    //be the dividing line
}TRANS2d3p;

namespace Isis {
  void apolloPanStitcher(UserInterface &ui) {
    Chip patternS,searchS; //scaled pattern and search chips
    Cube  *panC[8], //panoramic image
           outputC, //outputC
           fidC;    //Fiducial image
    int regStatus,
        scanS,   //samples in a sub scan
        scanL,   //lines in a sub scan
        refL,    //number of lines in the patternS
        refS;    //number of samples in the patternS

    Pvl pvl;

    bool foundFirst;

    int i,j,k;

    double sampleFrom, sampleTo;  //limits for rubber sheeting and mosaicing the sub-scans
    double play;  //how close a pixel must be to be considered on the ellipse

    //vector of possible solutions to a transformation-all transformations resulting from any
    //  possible combination of minimal sets of data are calculated to find the best solution
    //  in an outlier resistant way
    std::vector <TRANS2d3p> solV;
    TRANS2d3p sol;      //individual solution to be add to the above vector

    TRANS2d3p trans[8];    //final solutions for the transformations-one for each sub-scan

    QString fileName,tempIString;
    QString fileBaseName, tempString;

    double l = 1, s = 1, sample, line, temp,//line and sample coordinates for looping through the panC
           ct,  //"cosine of theta" used to prevent evaluted the trig function thousands of times
           st;  //"sine of theta"   used to prevent evaluted the trig function thousands of times

    std::vector <std::vector <double> > scanFid;  //fiducials of Scan i
    std::vector <std::vector <double> > conFid;  //fiducials of Scan i
    std::vector <double> fid(2);      //individual fiducial measurement
    std::vector <double> pairFid(4);    //pair of conjugate fiducial coordinates

    //read the image resolutions and scale the constants acordingly
    double  resolution = ui.GetDouble("MICRONS"),    //pixel size in microns
            scale            = SCALE  *5.0/resolution,  //reduction scale for quicker autoregistrations
            searchHeight     = SEARCHh*5.0/resolution,  //number of lines (in 5-micron-pixels) in
                                                        //  search space for the first fiducial
            searchCellSize   = SEARCHc*5.0/resolution,  //height/width of search chips block
            averageSamples   = AVERs  *5.0/resolution,  //scaled smaples between fiducials
            nominalTrans     = TRANS_N*5.0/resolution,  //nominal sample direction overlap between
                                                        // successive sub scans
            averageLines     = AVERl  *5.0/resolution;  //scaled average diference between the top
                                                        //  and bottom fiducials

    if (15.0 / resolution < 1.5) {
      play = 1.5;
    }
    else {
      play = 15.0 / resolution;
    }

    //copy the patternS chip (the entire ApolloPanFiducialMark.cub)
    FileName fiducialFileName("$apollo15/calibration/ApolloPanFiducialMark.cub");
    fidC.open(fiducialFileName.expanded(),"r");
    if (!fidC.isOpen()) {
      string msg = "Unable to open the fiducial patternS cube: ApolloPanFiducialMark.cub\n";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    refL = fidC.lineCount();
    refS = fidC.sampleCount();
    //scaled pattern chip for fast matching
    patternS.SetSize((int)floor((refS-2)/SCALE), (int)floor((refL-2)/SCALE));
    patternS.TackCube((refS-1)/2, (refL-1)/2);
    patternS.Load(fidC, 0, SCALE);

    //make sure the eight "FROM" cubes exist, and can can be opened for reading
    fileBaseName = ui.GetString("FILE_BASE");
    for (i = 1; i <= 8; i++) {
      panC[i-1] = new Cube;
      fileName = fileBaseName + "-000" + toString(i) + ".cub";
      panC[i-1]->open(fileName, "r");
      if (!panC[i-1]->isOpen()) {
        string msg = "Unable to open input cube: " + IString(fileName) + "\n";
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }

    //parameters for maximum correlation autoregestration
    // see: ///usgs/pkgs/isis3nightly2011-09-21/isis/doc/documents/patternSMatch/patternSMatch.html#DistanceTolerance
    FileName fiducialPvl("$ISISROOT/appdata/templates/apollo/PanFiducialFinder.def");
    pvl.read(fiducialPvl.expanded().toStdString());  //read in the autoreg parameters
    AutoReg *arS = AutoRegFactory::Create(pvl);

    *arS->PatternChip() = patternS;  //patternS chip is constant

    //set up a centroid measurer
    //Centroid centroid;
    CentroidApolloPan centroid(resolution);
    Chip inputChip,selectionChip;
    if (panC[0]->pixelType() == 1) { //UnsignedByte
      centroid.setDNRange(12, 1e99);  //8 bit bright target
    }
    else {
      centroid.setDNRange(3500, 1e99);  //16 bit bright target
    }
    inputChip.SetSize(int(ceil(200 * 5.0 / resolution)), int(ceil(200 * 5.0 / resolution)));

    //find conjugate finducials and calcualte transformations
    for (i = 0; i < 7; i++) {
      //for each scan segment 0-6
      //1. find the probable conjugate fiducials (those with the smallest sample coordinates)
      //2. use the nominal transformation to find their matches in the next scan
      //3. robustly calculate the transformation (that is in a manner resistent to erroneous
      //   measurements)

      //Step 1:  find the probable conjugate fiducials (those with the smallest sample coordinates)
      scanS = panC[i+1]->sampleCount();

      scanFid.clear();

      //search sizes are constanst
      searchS.SetSize(int(searchCellSize/scale),int(searchCellSize/scale));
      //now start searching along a horizontal line for the first fiducial mark
      foundFirst = false;
      for (l = searchCellSize / 2;
           l < searchHeight + searchCellSize / 2.0 && !foundFirst;
           l = l + (searchCellSize - 125 * 5.0 / resolution) ) {
        for (s = searchCellSize / 2;
             s < (nominalTrans + searchCellSize) / 2.0 && !foundFirst;
             s = s + (searchCellSize - 125 * 5.0 / resolution) ) {
          searchS.TackCube(s, l);
          searchS.Load(*panC[i], 0, scale);
          *arS->SearchChip() = searchS;
          regStatus = arS->Register();
          if (regStatus == AutoReg::SuccessPixel) {
            inputChip.TackCube(arS->CubeSample(), arS->CubeLine());
            inputChip.Load(*panC[i], 0, 1);
            inputChip.SetCubePosition(arS->CubeSample(), arS->CubeLine());
            //continuous dynamic range selection
            centroid.selectAdaptive(&inputChip, &selectionChip);
            //selectionChip.Write("firstselectionTemp.cub");
            //elliptical trimming/smoothing
            if (centroid.elipticalReduction(&selectionChip, 95, play, 2000)) {
              //center of mass to reduce selection to a single measure
              centroid.centerOfMass(&selectionChip, &sample, &line);
              inputChip.SetChipPosition(sample, line);
              fid[0] = inputChip.CubeSample();
              fid[1] = inputChip.CubeLine();
              scanFid.push_back(fid);
              foundFirst = true;  //once the first fiducial is found stop
            }
          }
        }
      }
      if (s >= averageLines + searchCellSize / 2.0) {
         QString msg = "Unable to locate a fiducial mark in the input cube [" + panC[i]->fileName()
                        + "].  Check FROM and MICRONS parameters.";
         throw IException(IException::Io, msg, _FILEINFO_);
         return;
      }


      for (s = scanFid[0][0], l = scanFid[0][1]; s + nominalTrans < scanS; s+= averageSamples / 2.0) {
        //look for the bottom fiducial
        searchS.TackCube(s, l + averageLines);
        searchS.Load(*panC[i], 0, scale);
        *arS->SearchChip()   = searchS;
        regStatus = arS->Register();
        if (regStatus == AutoReg::SuccessPixel) {
          inputChip.TackCube( arS->CubeSample(), arS->CubeLine());
          inputChip.Load(*panC[i], 0, 1);
          inputChip.SetCubePosition(arS->CubeSample(), arS->CubeLine());
        }
        else {
          inputChip.TackCube(s, l + averageLines);
          inputChip.Load(*panC[i], 0, 1);
          inputChip.SetCubePosition(s, l + averageLines);
        }
        //continuous dynamic range selection
        centroid.selectAdaptive(&inputChip, &selectionChip);
        //elliptical trimming/smoothing... if this fails move on
        if (centroid.elipticalReduction(&selectionChip, 95, play, 2000) != 0) {
          //center of mass to reduce selection to a single measure
          centroid.centerOfMass(&selectionChip,&sample,&line);
          inputChip.SetChipPosition(sample,line);
          fid[0] = inputChip.CubeSample();
          fid[1] = inputChip.CubeLine();
          scanFid.push_back(fid);
        }

        //look for the top fiducial
        //is this is the first time through the loop?
        if (s == scanFid[0][0]) {
          continue;  //then the top fiducial was already found
        }
        searchS.TackCube(s, l);
        searchS.Load(*panC[i], 0, scale);
        *arS->SearchChip() = searchS;
        regStatus = arS->Register();
        if (regStatus == AutoReg::SuccessPixel) {
          inputChip.TackCube(arS->CubeSample(), arS->CubeLine());
          inputChip.Load(*panC[i], 0, 1);
          inputChip.SetCubePosition(arS->CubeSample(), arS->CubeLine());
        }
        else {
          inputChip.TackCube(s, l + averageLines);
          inputChip.Load(*panC[i], 0, 1);
          inputChip.SetCubePosition(s, l + averageLines);
        }
        //continuous dynamic range selection
        centroid.selectAdaptive(&inputChip, &selectionChip);
        //elliptical trimming/smoothing... if this fails move on
        if (centroid.elipticalReduction(&selectionChip, 95, play, 2000) != 0) {
          //center of mass to reduce selection to a single measure
          centroid.centerOfMass(&selectionChip, &sample, &line);
          inputChip.SetChipPosition(sample, line);
          //s and l are refined here to help follow any trends in the image/film
          s=fid[0] = inputChip.CubeSample();
          l=fid[1] = inputChip.CubeLine();
          scanFid.push_back(fid);
        }
      }

      // STEP 2. use the nominal transformation to find their matches in the next scan
      conFid.clear();
      for (j = scanFid.size() - 1; j >= 0; j--) {
        searchS.TackCube(scanFid[j][0] + nominalTrans,scanFid[j][1]);
        searchS.Load(*panC[i+1], 0, scale);
        *arS->SearchChip() = searchS;
        regStatus = arS->Register();
        //if autoreg is successful a smaller window can be used for the following steps
        if (regStatus == AutoReg::SuccessPixel) {
          inputChip.SetSize(int(200 * 5.0 / resolution), int(150 * 5.0 / resolution));
          inputChip.TackCube(arS->CubeSample(), arS->CubeLine());
          inputChip.Load(*panC[i+1], 0, 1);
          inputChip.SetCubePosition(arS->CubeSample(), arS->CubeLine());
        }
        else {  //if autoreg is unsuccessful, a larger window will be used
          inputChip.SetSize(int(ceil(200 * 5.0 / resolution)), int(ceil(200 * 5.0 / resolution)));
          inputChip.TackCube(scanFid[j][0] + nominalTrans, scanFid[j][1]);
          inputChip.Load(*panC[i+1], 0, 1);
          inputChip.SetCubePosition(scanFid[j][0] + nominalTrans, scanFid[j][1]);
        }
        //continuous dynamic range selection
        centroid.selectAdaptive(&inputChip, &selectionChip);
        //elliptical trimming/smoothing... if this fails move on
        if (centroid.elipticalReduction(&selectionChip, 95, play, 2000) != 0) {
          //center of mass to reduce selection to a single measure
          centroid.centerOfMass(&selectionChip, &sample, &line);
          inputChip.SetChipPosition(sample, line);
          //now that the complete conjugate pair has been located save it
          pairFid[2] = inputChip.CubeSample();
          pairFid[3] = inputChip.CubeLine();
          pairFid[0] = scanFid[j][0];
          pairFid[1] = scanFid[j][1];
          conFid.push_back(pairFid);
        }
      }
      //STEP 3. robustly calculate the transformation (that is in a manner resistent to erroneous
      //        measurements)
      //calculate the transformation from i+1 to i
      //RANSAC2 algorithim
        //Find two point solutions and then attempt to generalize to more points
        //this helps to identify measurement blunders by seeing if certain points are excluded
        //consistently--or equivalently if certain solutions keep repeating themselves
        //the limitation in this case is that there will never be more than 6 points, if the number
        //of points drops to <=three blunder detection becomes impossible

       /*This is a Gauss-Helemert non-linear least squares adjustment

        Matrices
    a  partials wrt unknowns
    b  partials wrt to measured quantities
    w  constant portion of linearized equations evaluate using estimates of unknown parameters
    p  p covariance matrix of measured quantities
    m  b*p*transpose(b)  which is the propogated covariance of the design equations
    delta  is vector of corrections to estimated unknowns
    v  residual vector

        linearized math model:
    a*delta + b*v = w

        normal equation:
    transpose(a)*inverse(m)*a*delta = transpose(a)*inverse(m)*w

        Solution:
    delta = minverse(transpose(a)*inverse(m)*a)*transpose(a)*inverse(m)*w
          iterate until corrections in delta are insignificant

        In this case to keep all resdiuals in pixel units and weight each observation identically
        p is modeled as the identity matrix thus the solution can be built using a, b, m, and w
        submatrices (indicated with a dot suffix) as follows

        Also in this particular case m is constant (proof given below) and thus un-necessary

        Normal equation:
    sum(transpose(adot)*adot)*delta = sum(transpose(adot)*wdot)  or  ata*delta = atw

        solution:
    delta = inverse(ata)*atw  iterate until corrections in delta are insignificant
      */

      //clear out previous data
      solV.clear();
      //matrix names loosely following the naming convention of Uotilia see large comment above
      double wdot[2], adot[2][3], ata[6], atf[3];

      int l, m, n, o, ni;  //additonal indices for matrix multipilcations, etc

      for (j = 0; j < int(conFid.size()) - 1; j++) {
        for (k = j + 1; k < int(conFid.size()); k++) {
          //Starting by finding the two point solution: using the following close form algorithim
          sol.theta = atan2(conFid[j][3] - conFid[k][3], conFid[j][2] - conFid[k][2]) -
                      atan2(conFid[j][1] - conFid[k][1], conFid[j][0] - conFid[k][0]);
          sol.dx = conFid[j][2] - conFid[j][0] * cos(sol.theta) + conFid[j][1] * sin(sol.theta);
          sol.dy = conFid[j][3] - conFid[j][0] * sin(sol.theta) - conFid[j][1] * cos(sol.theta);

          //generalize to include all points that are in general agreement with the solution
          //NOTE: this is a Gauss-Helmert least squares optimization
          //iteration limit is imposed here
          //  apon successful convergance the code breaks out of this for loop
          for (ni = 0; ni < 50; ni++) {
            //initailize normal equation
            for (m = 0; m < 3; m++)
              atf[m]=ata[m]=0.0;
            for (; m < 6; m++)
              ata[m] = 0.0;

            ct = cos(sol.theta);  //cos and sin computed only once
            st = sin(sol.theta);

            for (m = 0; m < int(conFid.size()); m++) {  //for each point
              wdot[0] = -(conFid[m][0] * ct - conFid[m][1] * st + sol.dx - conFid[m][2]);
              wdot[1] = -(conFid[m][0] * st + conFid[m][1] * ct + sol.dy - conFid[m][3]);

              // the following calculation of mdot matrix is left in the code for ease of
              //  understanding, the matrix is constant (as shown below)
              // and thus neither caculated nor stored in memory
              //partials wrt measured quantities
              //bdot[0][0] = ct;  //partial of equation 1 wrt to x scani
              //bdot[1][0] = st;  //partial of equation 2 wrt to x scani

              //bdot[0][1] = -st;   //partial of equation 1 wrt to y scani
              //bdot[1][1] = ct;  //partial of equation 2 wrt to y scani

              //bdot[0][2] = -1.0;  //partial of equation 1 wrt to x scani+1
              //bdot[1][2] = 0.0;  //partial of equation 2 wrt to x scani+1

              //bdot[0][3] = 0.0;  //partial of equation 1 wrt to y scani+1
              //bdot[1][3] = -1.0;  //partial of equation 2 wrt to y scani+1

              //covariance_matrix assumed identity--that keeps are weighted residuals in pixel
              // units for easy interpretation
              //mdot = bdot*covariance_matrix*transpose(bdot);

              //mdot[0] = bdot[0][0]*bodt[0][0] +
              //          bdot[0][1]*bodt[0][1] +
              //          bdot[0][2]*bodt[0][2] +
              //          bdot[0][3]*bodt[0][3]
              //        = ct*ct + st*st + 1 + 0 = 2
              //mdot[1] = bdot[0][0]*bodt[1][0] +
              //          bdot[0][1]*bodt[1][1] +
              //          bdot[0][2]*bodt[1][2] +
              //          bdot[0][3]*bodt[1][3]
              //        = ct*st - ct*st + 0 + 0 = 0
              //mdot[2] = bdot[1][0]*bodt[1][0] +
              //          bdot[1][1]*bodt[1][1] +
              //          bdot[1][2]*bodt[1][2] +
              //          bdot[1][3]*bodt[1][3]
              //        = st*st + ct*ct + 0 + 1 = 2

              //mdot inverse
              //mdot[0] = 0.5;
              //mdot[1] = 0.0;
              //mdot[2] = 0.5;

              if (sqrt(0.5 * (wdot[0] * wdot[0] + wdot[1] * wdot[1])) > 3.0) {
                continue;  //if the R^2 residual is greater than 3.0 pixels go on to the next point
              }

              //partials wrt unknowns
              adot[0][0] = -conFid[m][0]*st - conFid[m][1]*ct;  //wrt to theta
              adot[1][0] =  conFid[m][0]*ct - conFid[m][1]*st;

              adot[0][1] = 1.0;    //wrt dx
              adot[1][1] = 0.0;

              adot[0][2] = 0.0;    //wrt dy
              adot[1][2] = 1.0;

              //build the normal equations
              //add transpose(adot)*adot to ata
              //note: becuase m is constant, and every weight is equal, it is irrelevant and ignored
              for (l = 0; l < 3; l++) {
                for (n = 0; n <= l; n++) {
                  for (o = 0; o < 2; o++) {
                    ata[isymp(l,n)] += adot[o][l] * adot[o][n];
                  }
                }
              }

              //add transpose(adot)*wdot to atf
              //note: becuase m is constant, and every weight is equal, it is irrelevant and ignored
              atf[0] += adot[0][0] * wdot[0] + adot[1][0] * wdot[1];
              atf[1] += adot[0][1] * wdot[0] + adot[1][1] * wdot[1];
              atf[2] += adot[0][2] * wdot[0] + adot[1][2] * wdot[1];
            }

            //solve normal equations
            //calculation is done in place as delta is returned in atf
            if (choleski_solve(ata, atf, 3, 2) != 1) {
              continue;  //if the solution fails move on to the next two point solution
            }

            //check for nonsense numbers
            if (atf[2] != atf[2]) continue;

            //add corections
            sol.theta += atf[0];
            sol.dx += atf[1];
            sol.dy += atf[2];


            if (fabs(atf[0]) < 1e-10 && fabs(atf[0]) < 1e-5 && fabs(atf[0]) < 1e-5) {
              // solution converged
              // find residual stats
              // Note: residuals are caluclated for all points-including any that might have been
              // excluded from the solution above
              for (m = 0, sol.maxR = 0.0, sol.averR = 0.0, sol.limit[0] = 0.0, sol.limit[1] = 0.0;
                   m<int(conFid.size());
                   m++) {  // for each point
                wdot[0] = -( conFid[m][0]*ct - conFid[m][1]*st + sol.dx - conFid[m][2]);
                wdot[1] = -( conFid[m][0]*st + conFid[m][1]*ct + sol.dy - conFid[m][3]);
                temp = sqrt(wdot[0]*wdot[0] + wdot[1]*wdot[1]);

                if (temp > sol.maxR) {
                  sol.maxR = temp;
                }

                sol.averR += temp;

                sol.limit[0] +=  conFid[m][0];
                sol.limit[1] +=  conFid[m][1];
              }
              sol.averR /= double(conFid.size());

              // save the solution
              solV.push_back(sol);
              break; // break out of iteration counting loop
            }
          } // iteration counting for loop
        }
      } // end of two point solutions loops
      if (solV.size() < 1) {
        string msg = "ERROR: unable to to calculate transformation coefficients. Check FILE_BASE and"
                     " MICRON parameters.\n";
        throw IException(IException::Io, msg, _FILEINFO_);
        return;
      }

      // now that all the two point solutions have been found go through and choose the best
      // first lets thin by maximum residual
      temp = solV[0].maxR;  // find the lowest max residual
      for (j = 1; j < int(solV.size()); j++) {
        if (solV[j].maxR < temp) {
          temp = solV[j].maxR;
        }
      }
      // toss out any solution that has a maximum residual higher than ceil(lowest max residual)
      temp = ceil(temp);
      for (j = int(solV.size())-1; j >= 0; j--) {
        if (solV[j].maxR > temp) {
          solV.erase(solV.begin() + j);
        }
      }

      // then by average residual
      temp = solV[0].averR;  // find the lowest average residual
      for (j = 1; j < int(solV.size()); j++) {
        if (solV[j].averR < temp) {
          temp = solV[j].averR;
        }
      }
      // toss out any solutions that have average resiudals higher than ceil(lowest average residual)
      temp = ceil(temp);
      for (j = int(solV.size()) - 1; j >= 0; j--) {
        if (solV[j].averR > temp) {
          solV.erase(solV.begin() + j);
        }
      }

      // finally choose the transformation with the lowest theta angle as the final filter
      k = 0;
      for (j = 1; j < int(solV.size()); j++) {
        if (fabs(solV[j].theta) < fabs(solV[k].theta)) {
          k = j;
        }
      }

      // save solV[k]
      trans[i].theta = solV[k].theta;
      trans[i].dx = solV[k].dx;
      trans[i].dy = solV[k].dy;
      trans[i].averR = solV[k].averR;  // average residual
      trans[i].maxR = solV[k].maxR;  // max resiudal
      trans[i].limit[0] = solV[k].limit[0] / double(conFid.size());
      trans[i].limit[1] = solV[k].limit[1] / double(conFid.size());
    }// end of scan loop

    // translation from 8 to 8
    trans[7].theta = 0.0;
    trans[7].dx = 0.0;
    trans[7].dy = 0.0;

    // We now have seven transformations that convert from scan i to scan i+1 for i = 0 to 7
    // as scan 8 is the left most scan and has a nominally identity transform from its sample-line
    // system to the stitched cube sample-line system. Combine transformations so that each transforms
    // is from scan i to scan 8
    for (i = 0; i < 7; i++) {
      for (j = i + 1; j < 8; j++) {
        // Make i equal to the combined transformation of i and j
        trans[i].theta = atan2( cos(trans[j].theta)*sin(trans[i].theta) +
                                sin(trans[j].theta)*cos(trans[i].theta),
                                cos(trans[j].theta)*cos(trans[i].theta) -
                                sin(trans[j].theta)*sin(trans[i].theta));

        temp        = cos(trans[j].theta)*trans[i].dx - sin(trans[j].theta)*trans[i].dy
                      + trans[j].dx;
        trans[i].dy = sin(trans[j].theta)*trans[i].dx + cos(trans[j].theta)*trans[i].dy
                      + trans[j].dy;

        trans[i].dx = temp;
      }
    }



    // Now lets find the extents of the stitched image
    double minS = 1,
           maxS = panC[7]->sampleCount(),
           minL = 1,
           maxL = panC[7]->lineCount();

    for (i = 0; i < 7; i++) {
      scanS = panC[i]->sampleCount();
      scanL = panC[i]->lineCount();

      // Convert the four corner points to the scan 8 domain and determine the greatest extents
      temp = cos(trans[i].theta) - sin(trans[i].theta) + trans[i].dx;
      if (temp < minS) minS = temp;
      if (temp > maxS) maxS = temp;
      temp = cos(trans[i].theta)*scanS - sin(trans[i].theta) + trans[i].dx;
      if (temp < minS) minS = temp;
      if (temp > maxS) maxS = temp;
      temp = cos(trans[i].theta)*scanS - sin(trans[i].theta)*scanL + trans[i].dx;
      if (temp < minS) minS = temp;
      if (temp > maxS) maxS = temp;
      temp = cos(trans[i].theta) - sin(trans[i].theta)*scanL + trans[i].dx;
      if (temp < minS) minS = temp;
      if (temp > maxS) maxS = temp;

      temp = sin(trans[i].theta) + cos(trans[i].theta) + trans[i].dy;
      if (temp < minL) minL = temp;
      if (temp > maxL) maxL = temp;
      temp = sin(trans[i].theta)*scanS + cos(trans[i].theta) + trans[i].dy;
      if (temp < minL) minL = temp;
      if (temp > maxL) maxL = temp;
      temp = sin(trans[i].theta)*scanS + cos(trans[i].theta)*scanL + trans[i].dy;
      if (temp < minL) minL = temp;
      if (temp > maxL) maxL = temp;
      temp = sin(trans[i].theta) + cos(trans[i].theta)*scanL + trans[i].dy;
      if (temp < minL) minL = temp;
      if (temp > maxL) maxL = temp;
    }

    // Update the transformations to make the minimum line = 1
    for (i = 0; i < 8; i++) {
      trans[i].dy += 1- minL;
    }

    maxL += ceil(1 - minL);
    minL = 1;

    // Update the transformations to make the minimum sample = 1
    for (i = 0; i < 8; i++) {
      trans[i].dx += 1- minS;
    }

    maxS += ceil(1 - minS);
    minS = 1;

    // trans[i].limit was previous calculated as the center of mass of the conjugate fiducials in the
    // scani system
    // Transform it now into the stitched cube system to be used as a limit between scans
    for (i = 0; i < 7; i++) {
      temp       = trans[i].limit[0]*cos(trans[i].theta) -
                   trans[i].limit[1]*sin(trans[i].theta) + trans[i].dx;
      trans[i].limit[1] = trans[i].limit[0]*sin(trans[i].theta) +
                          trans[i].limit[1]*cos(trans[i].theta) + trans[i].dy;
      trans[i].limit[0] = floor(temp);
    }



    // Finally shift and invert the transformations so that they convert from the rubber sheeted
    // sub-cubes back to the sub-scans. The result of this will be transformations that will convert
    // from a set of coordinate systems that differ only by an integral transformation back to
    // sub-scans, each sub-scan will be rubber-sheeted into one of these systems, and the limit[0]
    // values record where they belong in the stiched image/system.
    for (i = 0; i < 8; i++) {
      if (i == 7) {
        sampleFrom = 1.0;
      }
      else {
        sampleFrom = trans[i].limit[0];
      }
      //a shift so that the transform puts sampleFrom at sample 1 of the rubber sheeted sub cubes
      trans[i].dx += -sampleFrom+1;
      trans[i].theta = -trans[i].theta;
      temp = -cos(trans[i].theta) * trans[i].dx + sin(trans[i].theta) * trans[i].dy;
      trans[i].dy = -sin(trans[i].theta) * trans[i].dx - cos(trans[i].theta) * trans[i].dy;
      trans[i].dx = temp;
    }

    // Make the final maxes integral
    maxS = ceil(maxS);
    maxL = ceil(maxL);

    // Attributes of input and ouput cubes to process class
    CubeAttributeOutput att;
    CubeAttributeInput attI;

    attI.setAttributes("some.cub+1"); //will only be processing one band from the input

    //make output attributes match the input
    att.setFileFormat( panC[0]->format() );
    att.setByteOrder(  panC[0]->byteOrder() );
    att.setPixelType(  panC[0]->pixelType() );
    if (panC[0]->labelsAttached()) {
      att.setLabelAttachment(AttachedLabel);
    }
    else {
      att.setLabelAttachment(DetachedLabel);
    }

    //define an output cube
    outputC.setDimensions(int(maxS), int(maxL), 1);
    outputC.setPixelType(panC[0]->pixelType());  //set pixel type
    tempString = ui.GetCubeName("TO");
    outputC.create(tempString);
    outputC.close();  //closing the output cube so that it can be opened by the mosaic process
    ProcessMosaic mosaic;
    mosaic.SetOutputCube("TO", ui);
    mosaic.SetBandBinMatch(false);


    //transform and mosaic the content from each scan
    for (i = 0; i < 8; i++) {  //for each scan
      FileName tempFile = FileName::createTempFile("$temporary/tempscan.cub");

      scanS = panC[i]->sampleCount();
      scanL = panC[i]->lineCount();

      //define the sample range
      if (i == 0) {
        sampleTo = maxS;
      }
      else {
        sampleTo = trans[i-1].limit[0];
      }
      if (i == 7) {
        sampleFrom = 1.0;
      }
      else {
        sampleFrom = trans[i].limit[0];
      }

      Interpolator bilinearInt(Interpolator::BiLinearType);
      ProcessRubberSheet rubberS;

      //use ProcessRubber sheet to create the sub-cube for this scanz
      Trans2d3p transform(trans[i].theta, trans[i].dx, trans[i].dy,
                          int(sampleTo - sampleFrom),
                          int(maxL));
      rubberS.SetInputCube(panC[i]);
      rubberS.SetOutputCube(tempFile.expanded(),
                            att,
                            transform.OutputSamples(),
                            transform.OutputLines(),
                            1);
      rubberS.Progress()->SetText(
          QObject::tr("Transforming Scan%1: ").arg(i+1).toStdString().c_str());
      rubberS.StartProcess(transform, bilinearInt);
      //end process closes the cubes and deletes the dynamically allocated memory (panC[i])
      rubberS.EndProcess();

      //use process mosaic to add the sub cube to the stiched cube
      mosaic.SetInputCube(tempFile.expanded(),
                          attI,
                          1,
                          1,
                          1,
                          transform.OutputSamples(),
                          transform.OutputLines(),
                          1);
      rubberS.Progress()->SetText(QObject::tr("Mosaicing Scan%1: ").arg(i+1).toStdString().c_str());
      mosaic.StartProcess(int(sampleFrom), 1, 1);

      //clear input cube
      mosaic.ClearInputCubes();

      QFile::remove(tempFile.expanded());  //delete temporary cube if it exists
    }
    mosaic.EndProcess();
  }
}
