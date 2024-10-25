#ifndef BundleAdjust_h
#define BundleAdjust_h

/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <QObject> // parent class

// std lib
#include <vector>
#include <fstream>

// cholmod lib
#include <cholmod.h>

// Isis lib
#include "BundleControlPoint.h"
#include "BundleLidarPointVector.h"
#include "BundleObservationSolveSettings.h"
#include "BundleObservationVector.h"
#include "BundleResults.h"
#include "BundleSettings.h"
#include "BundleSolutionInfo.h"
#include "BundleTargetBody.h"
#include "Camera.h"
#include "CameraGroundMap.h"
#include "ControlMeasure.h"
#include "ControlNet.h"
#include "LidarData.h"
#include "LinearAlgebra.h"
#include "MaximumLikelihoodWFunctions.h"
#include "ObservationNumberList.h"
#include "Progress.h"
#include "SerialNumberList.h"
#include "SparseBlockMatrix.h"
#include "Statistics.h"

template< typename T > class QList;
template< typename A, typename B > class QMap;

namespace Isis {
  class Control;
  class ImageList;

  /**
   * @brief An image bundle adjustment object.
   *
   * BundleAdjust is used to perform a bundle adjustment on overlapping ISIS cubes.
   * Using the collineariy condition, BundleAdjust can construct a system of normal equations
   * and then using the CHOLMOD library, solve that system.
   *
   * @author 2006-05-30 Jeff Anderson, Debbie A. Cook, and Tracie Sucharski
   *
   * @internal
   *   @history 2005-05-30 Jeff Anderson, Debbie A. Cook & Tracie Sucharski - Original version
   *   @history 2007-05-29 Debbie A. Cook - Added new method iterationSummary and
   *                           changed points on held images to held instead of ground.
   *   @history 2007-07-12 Debbie A. Cook - Fixed bug in iteration statistics calculations in the
   *                           case of a single control point that was causing a divide by zero
   *                           error.
   *   @history 2007-08-25 Debbie A. Cook - Added methods and members to support instrument position
   *                           solution.
   *   @history 2007-09-17 Debbie A. Cook - Added ability to process in observation mode for Lunar
   *                           Orbiter.
   *   @history 2007-11-17 Debbie A. Cook - Added method SetSolution Method.
   *   @history 2007-12-21 Debbie A. Cook - Added member p_Degree and methods m_nsolveCamDegree and
   *                           ckDegree.
   *   @history 2008-01-11 Debbie A. Cook - Added observation mode functionality for spacecraft
   *                           position and upgraded ObservationNumber methods for compatability.
   *   @history 2008-01-14 Debbie A. Cook - Added code to solve for local radii.
   *   @history 2008-04-18 Debbie A. Cook - Added progress for ControlNet.
   *   @history 2008-06-18 Christopher Austin - Fixed ifndef.
   *   @history 2008-11-07 Tracie Sucharski - Added bool to constructors to indicate whether to
   *                           print iteration summary info to the session log. This was needed for
   *                           qtie which has no session log.
   *   @history 2008-11-22 Debbie A. Cook - Added code to wrap longitude to keep it in [0.0, 360.0].
   *   @history 2008-11-22 Debbie A. Cook - Added new call to get timeScale and set for the
   *                           observation along with basetime.
   *   @history 2008-11-26 Debbie A. Cook - Added check to applyHeldList for Ignored points and
   *                           measures.
   *   @history 2009-01-08 Debbie A. Cook - Revised AddPartials and PointPartial to avoid using the
   *                           camera methods to map a body-fixed vector to the camera because they
   *                           compute a new time for line scan cameras based on the lat/lon/radius
   *                           and the new time is used to retrieve Spice. The updated software uses
   *                           the Spice at the time of the measurement.
   *   @history 2009-02-15 Debbie A. Cook - Corrected focal length to include its sign and removed
   *                           obsolete calls to X/Y direction methods.  Also modified PointPartial
   *                           to use lat/lon/radius from the point instead of the camera.
   *   @history 2009-08-13 Debbie A. Cook - Corrected calculations of cudx and cudy so that they use
   *                           the signed focal length also.
   *   @history 2009-10-14 Debbie A. Cook - Modified AddPartials method to use new CameraGroundMap
   *                           method, GetXY.
   *   @history 2009-10-30 Debbie A. Cook - Improved error message in AddPartials.
   *   @history 2009-12-14 Debbie A. Cook - Updated SpicePosition enumerated partial type constants.
   *   @history 2010-03-19 Debbie A. Cook - Moved partials to GroundMap classes to support Radar
   *                           sensors and modified argument list for GroundMap method ComputeXY
   *                           since it now returns cudx and cudy.
   *   @history 2010-06-18 Debbie A. Cook - Added p_cnetFile as member since it was taken out of
   *                           ControlNet.
   *   @history 2010-07-09 Ken Edmundson - Added Folding in solution method (SPECIALK), error
   *                           propogation, statistical report, etc.
   *   @history 2010-08-13 Debbie A. Cook - Changed surface point from lat/lon/radius to body-fixed
   *                           XYZ.
   *   @history 2010-12-17 Debbie A. Cook - Merged Ken Edmundson version with system and updated to
   *                           new binary control net.
   *   @history 2011-02-01 Debbie A. Cook - Moved code to create point index map into its own method
   *                           to be called after the solution method has been set.
   *   @history 2011-02-17 Debbie A. Cook - Updated to use new parameter added to SpicePosition,
   *                           p_timeScale.
   *   @history 2011-03-05 Debbie A. Cook - Put point index creation back in init.  This will
   *                           prevent QRD and SVD from working if ground points are in the control
   *                           net.
   *   @history 2011-03-29 Ken Edmundson - Fixed bug in observation mode when solving for spacecraft
   *                           position and improved output.
   *   @history 2011-04-02 Debbie A. Cook - Updated to ControlPoint class changes regarding target
   *                           radii.  Also separated out 2 sets of calculations to test later for
   *                           efficiency.
   *   @history 2011-06-05 Debbie A. Cook - Changed checks for solution type to match change from
   *                           SPARSE to SPARSE-LU.
   *   @history 2011-06-07 Debbie A. Cook - and Tracie Sucharski - Modified point types from
   *                           Ground to Fixed and from Tie to Free.
   *   @history 2011-06-14 Debbie A. Cook - added method isHeld(int index) for preventing any
   *                           updates to held images.
   *   @history 2011-06-27 Debbie A. Cook - and Ken Edmundson Added names to top header fields of
   *                           .csv output and fixed bugs in sparse output.
   *   @history 2011-07-12 Ken Edmundson - Segmentation fault bugfix in OutputHeader method.
   *                           Previously was attempting to output camera angle sigmas when none had
   *                           been allocated.
   *   @history 2011-07-14 Ken Edmundson and Debbie Cook - Added new member, m_bDeltack to indicate
   *                           calling application was deltack (or qtie) and has potential to have a
   *                           single ControlPoint and ControlMeasure.
   *   @history 2011-08-08 Tracie Sucharski - Added method to return the iteration summary to be
   *                           used in qtie which does not have a log file. In SetImages, clear the
   *                           cameraMavtpv_targetBodyp and cameraList.  Added this back in (was
   *                           originally added on 2011-01-19), was deleted somewhere along the
   *                           line.
   *   @history 2011-09-28 Debbie A. Cook - Renamed SPARSE solve method to OLDSPARSE and CHOLMOD to
   *                           SPARSE.
   *   @history 2011-10-14 Ken Edmundson - Added call to m_pCnet->ClearJigsawRejected() to the
   *                           init() method to set all measure/point JigsawRejected flags to false
   *                           prior to bundle.
   *   @history 2011-12-09 Ken Edmundson - Memory leak fix in method cholmodInverse. Need call to
   *                           "cholmod_free_dense(&x,&m_cholmodCommon)" inside loop.
   *   @history 2011-12-20 Ken Edmundson - Fixes to outlier rejection. Added rejection multiplier
   *                           member variable, can be set in jigsaw interface.
   *   @history 2012-02-02 Debbie A. Cook - Added SetSolvePolyOverHermite method and members
   *                           m_bSolvePolyOverHermite and m_positionType.
   *   @history 2012-03-26 Orrin Thomas - Added maximum likelihood capabilities.
   *   @history 2012-05-21 Debbie A. Cook - Added initialization of m_dRejectionMultiplier.
   *   @history 2012-07-06 Debbie A. Cook - Updated Spice members to be more compliant with Isis
   *                           coding standards. References #972.
   *   @history 2012-09-28 Ken Edmundson - Initialized variables for bundle statistic computations.
   *                           The bundleout.txt file was modifed to show N/A for RMS, Min, Max of
   *                           Radius Sigmas when not solving for radius. References #783.
   *   @history 2013-11-12 Ken Edmundson - Programmers Note. References #813, #1521, #1653
   *                           #813 - Info echoed to screen when using Maximum Likelihood
   *                                  methods are now printed to print.prt file.
   *                           #1521 - The cout debug statements that appear on screen when
   *                                   updating images were removed from SpiceRotation.cpp
   *                           #1653 - Constraints were being applied for "Free" points that have
   *                                   constrained coordinates. Also found that a priori
   *                                   coordinates for these points were not being computed in
   *                                   ControlPoint::ComputeApriori, this has also been fixed.
   *   @history 2013-12-18 Tracie Sucharski - The ControlNet::GetNumberOfMeasuresInImage was
   *                           renamed to ControlNet::GetNumberOfValidMeasuresInImage and only
   *                           returns the number of valid (Ignore= False) measures.
   *   @history 2014-02-25 Ken Edmundson - Speed up and memory improvements to error propagation.
   *                           References #2031.
   *   @history 2014-05-16 Jeannie Backer - Added BundleSettings object to constructor inputs.
   *                           Cleaned and organized code. Updated to be more compliant with ISIS
   *                           coding standards.
   *   @history 2014-07-14 Kimberly Oyama - Added support for correlation matrix. Covariance matrix
   *                           is now written to a file and the location is saved as part of the
   *                           CorrelationMatrix object.
   *   @history 2014-07-23 Jeannie Backer - Modified to print "N/A" for rejection multiplier if
   *                           outlier rejection is turned off.
   *   @history 2014-09-18 Kimberly Oyama - Added a constructor for running the bunlde in a
   *                           separate thread.
   *   @history 2014-11-05 Ken Edmundson - Fixed memory bug. Wasn't releasing cholmod_factor m_L
   *                           every iteration. Now release every iteration but the last since we
   *   @history 2015-02-20 Jeannie Backer - Updated to be more compliant with ISIS coding standards.
   *   @history 2015-08-17 Jeannie Backer - Updated to be more compliant with ISIS coding standards.
   *   @history 2015-09-03 Jeannie Backer - Changed the name of the output correlation matrix file
   *                           from ["inverseMatrix" + random unique code + ".dat"] to
   *                           [BundleSettings::outputFilePrefix() + "inverseMatrix.dat"]. So that
   *                           the prefix can be used to specify the path of the location where the
   *                           matrix file should be written. Some improvements made to comply with
   *                           coding standards.
   *   @history 2015-09-10 Ian Humphrey - Fixed include for cholmod header after updating v005
   *                           libraries.
   *   @history 2016-07-11 Jesse Mapel - Changed m_bundleControlPoints to be a vector of
   *                           QSharedPointers to BundleControlPoints instead of a
   *                           BundleControlPointVector.  Fixes #4099.
   *   @history 2016-07-11 Jeannie Backer - Removed initialize(). Implementation was moved the the
   *                           bottom of init() method. Fixes #4161.
   *   @history 2016-08-03 Jesse Mapel - Changed BundleObservationVector to a vector of
   *                           QSharedPointers.  Fixes #4150.
   *   @history 2016-08-10 Jeannie Backer - Replaced boost vectors and matrices with
   *                           Isis::LinearAlgebra::Vector and Isis::LinearAlgebra::Matrix,
   *                           respectively. References #4163.
   *   @history 2016-08-15 Jesse Mapel - Moved write methods to BundleSolutionInfo.  Changed
   *                           constructors to always construct a new control network.
   *                           Fixes #4159.
   *   @history 2016-08-15 Ian Humphrey - Replaced ISIS ControlPoint and ControlMeasure uses with
   *                           BundleControlPoint and BundleMeasure. No longer need to check if the
   *                           BundleControlPoint or BundleMeasure is ignored before use, since we
   *                           only add non-ignored ControlPoints to the BundleControlPoint, and
   *                           we only add non-ignored ControlMeasures to the BundleControlPoint.
   *                           Fixes #4173, #4201.
   *   @history 2016-08-16 Jesse Mapel - Added error throw when covariance matrices are not
   *                           symmetric due to unstable data and settings.  Fixes #2302.
   *   @history 2016-08-17 Jesse Mapel - Moved all method implementations to the cpp file.
   *                           Fixes #4185.
   *   @history 2016-08-18 Jeannie Backer - Removed all references to deprecated solve methods
   *                           SpeckialK and OldSparse. Fixes #4162.
   *   @history 2016-08-23 Jesse Mapel - Removed output file calls.  Apps and objects that use
   *                           BundleAdjust must call output methods from BundleSolutionInfo.
   *                           Fixes #4279.
   *   @history 2016-08-24 Jesse Mapel - Updated documentation and member variable names.  Brought
   *                           closer to ISIS 3 coding standards.  Fixes #4183, #4188, #4235.
   *   @history 2016-08-28 Kelvin Rodriguez - Remvoed useless register keywords to squash warnigns
   *                         in clang. Part of porting to OS X 10.11.
   *   @history 2016-09-22 Ian Humphrey - Modified validateNetwork() so that validation status
   *                           messages are logged to stdout. Fixes #4313.
   *   @history 2016-10-05 Ian Humphrey - Modified errorPropagation_CHOLMOD() to check bundle
   *                           settings to see if it should generate the inverseMatrix.dat file.
   *                           References #4315.
   *   @history 2016-10-13 Ian Humphrey - Added constructor that takes a ControlNetQsp, so that
   *                           when jigsaw modifies a control net for a held image, the control
   *                           net can be passed as a shared pointer. Removed m_pHeldSnList,
   *                           isHeld(), checkHeldList(), applyHeldList(), two constructors that
   *                           used heldList parameters. Modified destructor to not delete
   *                           m_pHeldSnList, since it was removed. Fixes #4293.
   *   @history 2016-10-17 Adam Paquette - Cleaned up terminal output for readability and cohesion
   *                           Fixes #4263, #4311, #4312.
   *   @history 2016-10-18 Ian Humphrey - Modified iterationSummary() to always output
   *                           Rejected_Measures kewyord, regardless of outlier rejection being on
   *                           or off. Fixes #4461.
   *   @history 2016-10-25 Ian Humphrey - Modified iterationSumary() to always output
   *                           Rejected_Measures keyword, regarldess of outlier rjection being on
   *                           or off. Fixes #4461. Modified solveCholesky() and
   *                           computeRejectionLimit() so jigsaw's std out for Rejection Limit,
   *                           Sigma0, and Elapsed Time match ISIS production's jigsaw std out.
   *                           Fixes #4463.
   *   @history 2016-10-28 Ian Humphrey - Modified solveCholesky() and errorPropagation() to change
   *                           spacing in terminal output, so that it matches production.
   *                           References #4463.
   *   @history 2016-11-16 Ian Humphrey - Modified solveCholesky() to throw caught exceptions that
   *                           occur. Removed bundleException(QString) signal. Fixes #4483.
   *   @history 2016-12-01 Ian Humphrey - Modified outputBundleStatus()'s printf() call so that
   *                           there is no longer a -Wformat-security warning.
   *   @history 2017-05-01 Makayla Shepherd - Added imageLists() to track and return the images
   *                           bundled. Fixes #4818.
   *   @history 2017-05-09 Tracie Sucharski - Fixed an empty pointer in ::imgeLists method.
   *   @history 2017-05-09 Ken Edmundson - Speed improvements and error propagation bug fix.
   *                           Separated initializations for Normal Equations matrix out of
   *                           ::initializeCholmodLibraryVariables() into
   *                           ::initializeNormalEquationsMatrix(). Added
   *                           m_previousNumberImagePartials to avoid unnecessary resizing of the
   *                           "coeffImage" matrix in ::computePartials. New m_startColumn member in
   *                           SparseBlockColumnMatrix eliminates costly computation of leading
   *                           colums and rows. In ::errorPropagation method, call to get Q matrix
   *                           from BundleControlPoint was creating a copy instead of getting a
   *                           reference. References #4664.
   *   @history 2017-06-08 Makayla Shepherd - Modified imageLists() to close the image cube after
   *                           adding it to the image list. Fixes #4908.
   *   @history 2017-08-09 Summer Stapleton - Added a try/catch around the m_controlNet assignment
   *                           in each of the constructors to verify valid control net input.
   *                           Fixes #5068.
   *   @history 2017-09-01 Debbie A. Cook - Added BundleSettingsQsp as argument to
   *                            BundleControlPoint constructor and moved setWeights call from
   *                            BundleAdjust::init to BundleControlPoint constructor.  Don't allow
   *                            solving for triaxial radii when coordinate type is not Latitudinal.
   *                            Added new optional argument controlPointCoordType to ControlNet
   *                            constructor call.  References #4649 and #501.
   *   @history 2018-02-12 Ken Edmundson - Removed members m_xResiduals, m_yResiduals, and
   *                           m_xyResiduals and made them local to the computeResiduals() method.
   *                           Removed member m_bodyRadii, used only locally in init() method.
   *   @history 2018-05-22 Ken Edmundson - Modified methods bundleSolveInformation() and
   *                           solveCholeskyBR() to return raw pointers to a BundleSolutionInfo object.
   *                           Also modified resultsReady signal to take a raw pointer to a
   *                           BundleSolutionInfo object. This was done to avoid using a copy
   *                           constructor in the BundleSolutionInfo class because it is derived
   *                           from QObject. Note that we ultimately want to return a QSharedPointer
   *                           instead of a raw pointer.
   *   @history 2018-05-31 Debbie A. Cook - Moved productAlphaAV and control point parameter
   *                            correction code to BundleControlPoint.  Earlier revised errorPropagation to
   *                            compute the sigmas via the variance/covariance matrices instead of the sigmas.
   *                            This should produce more accurate results.  References #4649 and #501.
   *   @history 2018-06-14 Christopher Combs - Added getter method to tell if a bundle adjust was
   *                           aborted. Added emits for status updates to the run widget.
   *   @history 2018-06-18 Makayla Shepherd - Stopped command line output for ipce BundleAdjust.
   *                           Fixes #4171.
   *   @history 2018-06-27 Ken Edmundson - Now setting measure sigmas in BundleMeasure in init()
   *                           method; retrieving sigma and sqrt of weight in computePartials and
   *                           computeVtpv methods.
   *   @history 2018-09-06 Debbie A. Cook - (added to BundleXYZ branch on 2017-09-01)
   *                            Added BundleSettingsQsp as argument to BundleControlPoint constructor
   *                            and moved setWeights call from BundleAdjust::init to BundleControlPoint
   *                            constructor.  Don't allow solving for triaxial radii when coordinate type
   *                            is not Latitudinal. Added new optional argument controlPointCoordType
   *                            to ControlNet constructor call.  References #4649 and #501.
   *   @history 2018-09-06 Debbie A. Cook and Ken Edmundson - (added to BundleXYZ
   *                            branch on (2018-05-31).  Moved productAlphaAV and control point
   *                            parameter correction code to BundleControlPoint.  Earlier revised
   *                            errorPropagation to compute the sigmas via the variance/
   *                            covariance matrices instead of the sigmas.  This should produce
   *                            more accurate results.  References #4649 and #501.
   *   @history 2018-09-06 Debbie A. Cook - Removed obsolete member variables:
   *                            m_radiansToMeters, m_metersToRadians, and m_bodyRadii
   *                            which have been replaced with the local radius of a control
   *                            point for converting point sigmas to/from radians from/to meters.
   *                            References #4649 and #501.
   *   @history 2018-11-29 Ken Edmundson - Modifed init, initializeNormalEquationsMatrix, and
   *                           computePartials methods.
   *   @history 2019-04-29 Ken Edmundson - Modifications for bundle with lidar.
   *   @history 2019-05-15 Debbie A. Cook - The call to CameraGroundMap::GetXY in method
   *                            ComputePartials was modified to not check for points on the back side
   *                            of the planet when computing instrument coordinates during the bundle
   *                            adjustment.  In the future a control net diagnostic program might be
   *                            useful to detect any points not visible on an image based on the exterior
   *                            orientation of the image.  References #2591.
   *   @history 2024-10-21 Ken Edmundson - Simplified the computation of statistics for coordinate 3
   *                           (Radius or Z), to ensure correct output in the bundleout.txt file. We
   *                           report statistics on coordinate 3 UNLESS bundle and report types are
   *                           BOTH Latitudinal AND Radius is OFF.
   */
  class BundleAdjust : public QObject {
      Q_OBJECT
    public:
      BundleAdjust(BundleSettingsQsp bundleSettings,
                   const QString &cnetFile,
                   const QString &cubeList,
                   bool printSummary = true);
      BundleAdjust(BundleSettingsQsp bundleSettings,
                   const QString &cnetFile,
                   const QString &cubeList,
                   const QString &lidarDataFile,
                   bool printSummary = true);
      BundleAdjust(BundleSettingsQsp bundleSettings,
                   QString &cnet,
                   SerialNumberList &snlist,
                   bool printSummary = true);
      BundleAdjust(BundleSettingsQsp bundleSettings,
                   Control &cnet,
                   SerialNumberList &snlist,
                   bool bPrintSummary);
      BundleAdjust(BundleSettingsQsp bundleSettings,
                   ControlNet &cnet,
                   SerialNumberList &snlist,
                   bool printSummary = true);
      BundleAdjust(BundleSettingsQsp bundleSettings,
                   ControlNetQsp cnet,
                   const QString &cubeList,
                   bool printSummary = true);
      BundleAdjust(BundleSettingsQsp bundleSettings,
                   Control &control,
                   QList<ImageList *> imgList,
                   bool printSummary);
      ~BundleAdjust();
      BundleSolutionInfo*    solveCholeskyBR();

      QList<ImageList *> imageLists();
      bool isAborted();

    public slots:
      bool solveCholesky();
      void abortBundle();
      void outputBundleStatus(QString status);

      // accessors

      ControlNetQsp    controlNet();
      LidarDataQsp     lidarData();
      SerialNumberList *serialNumberList();
      QString          fileName(int index);
      QString          iterationSummaryGroup() const;
      bool             isConverged();
      Table            cMatrix(int index);
      Table            spVector(int index);
      QString          modelState(int index);
      int              numberOfImages() const;
      double           iteration() const;

    signals:
      void statusUpdate(QString);
      void error(QString);
      void iterationUpdate(int);
      void pointUpdate(int);
      void statusBarUpdate(QString);
      void resultsReady(BundleSolutionInfo *bundleSolveInformation);
      void finished();

    private:
      //TODO Should there be a resetBundle(BundleSettings bundleSettings) method
      //     that allows for rerunning with new settings? JWB
      void init(Progress *progress = 0);
      bool initializeNormalEquationsMatrix();
      bool validateNetwork();
      bool solveSystem();
      void iterationSummary();
      BundleSolutionInfo* bundleSolveInformation();
      bool computeBundleStatistics();
      void applyParameterCorrections();
      bool errorPropagation();
      void computeResiduals();
      double computeVtpv();
      bool computeRejectionLimit();
      bool flagOutliers();

      // normal equation matrices methods

      bool formNormalEquations();
      bool computePartials(LinearAlgebra::Matrix  &coeffTarget,
                           LinearAlgebra::Matrix  &coeffImage,
                           LinearAlgebra::Matrix  &coeffPoint3D,
                           LinearAlgebra::Vector  &coeffRHS,
                           BundleMeasure          &measure,
                           BundleControlPoint     &point);
      bool formMeasureNormals(LinearAlgebra::MatrixUpperTriangular &N22,
                              SparseBlockColumnMatrix              &N12,
                              LinearAlgebra::VectorCompressed      &n1,
                              LinearAlgebra::Vector                &n2,
                              LinearAlgebra::Matrix                &coeffTarget,
                              LinearAlgebra::Matrix                &coeffImage,
                              LinearAlgebra::Matrix                &coeffPoint3D,
                              LinearAlgebra::Vector                &coeffRHS,
                              int                                  observationIndex);
      int formPointNormals(LinearAlgebra::MatrixUpperTriangular &N22,
                           SparseBlockColumnMatrix              &N12,
                           LinearAlgebra::Vector                &n2,
                           LinearAlgebra::Vector                &nj,
                           BundleControlPointQsp                &point);
      int formLidarPointNormals(LinearAlgebra::MatrixUpperTriangular &N22,
                                SparseBlockColumnMatrix              &N12,
                                LinearAlgebra::Vector                &n2,
                                LinearAlgebra::Vector                &nj,
                                BundleLidarControlPointQsp           &point);
      bool formWeightedNormals(LinearAlgebra::VectorCompressed  &n1,
                               LinearAlgebra::Vector            &nj);

      // dedicated matrix functions

      void productAB(SparseBlockColumnMatrix &A,
                     SparseBlockRowMatrix    &B);
      void accumProductAlphaAB(double                alpha,
                               SparseBlockRowMatrix  &A,
                               LinearAlgebra::Vector &B,
                               LinearAlgebra::Vector &C);
      bool invert3x3(LinearAlgebra::MatrixUpperTriangular &m);
      bool productATransB(LinearAlgebra::MatrixUpperTriangular &N22,
                          SparseBlockColumnMatrix              &N12,
                          SparseBlockRowMatrix                 &Q);

      // CHOLMOD library methods

      bool initializeCHOLMODLibraryVariables();
      bool freeCHOLMODLibraryVariables();
      bool loadCholmodTriplet();

      // member variables

      BundleSettingsQsp m_bundleSettings;                    //!< Contains the solve settings.
      BundleResults  m_bundleResults;                        //!< Stores the results of the
                                                             //!< bundle adjust.
      ControlNetQsp m_controlNet;                            //!< Output control net.
      QString m_cnetFileName;                                //!< The control net filename.

      QVector <BundleControlPointQsp> m_bundleControlPoints; //!< Vector of control points.
      BundleLidarPointVector m_bundleLidarControlPoints;     //!< Vector of lidar points.

      QString m_lidarFileName;                               //!< Input lidar point filename.
      LidarDataQsp m_lidarDataSet;                           //!< Output lidar data.
      int m_numLidarConstraints;                             //!< TODO: temp

      BundleObservationVector m_bundleObservations;          /**!< Vector of observations.
                                                                   Each observation contains one or
                                                                   more images.*/
      SerialNumberList *m_serialNumberList;                  //!< List of image serial numbers.
      BundleTargetBodyQsp m_bundleTargetBody;                /**!< Contains information about the
                                                                   target body.*/
      bool m_abort;                                          //!< If the bundle should abort.
      QString m_iterationSummary;                            /**!< Summary of the most recently
                                                                   completed iteration.*/
      bool m_printSummary;                                   /**!< If the iteration summaries
                                                                   should be output to the log
                                                                   file.*/
      bool m_cleanUp;                                        /**!< If the serial number lists
                                                                   need to be deleted by
                                                                   the destructor.*/
      int m_rank;                                            //!< The rank of the system.
      int m_iteration;                                       //!< The current iteration.
      double m_iterationTime;                                //!< Time for last iteration
      int m_numberOfImagePartials;                           //!< number of image-related partials.
      QList<ImageList *> m_imageLists;                        /**!< The lists of images used in the
                                                                   bundle.*/

      // ==========================================================================================
      // === BEYOND THIS PLACE (THERE BE DRAGONS) all refers to the folded bundle solution.     ===
      // === Currently, everything uses the CHOLMOD library,                                    ===
      // === there is no dependence on the least-squares class.                                 ===
      // ==========================================================================================

      //! Inverse of the normal equations matrix.  Set by cholmodInverse.
      boost::numeric::ublas::symmetric_matrix<
          double,
          boost::numeric::ublas::upper,
          boost::numeric::ublas::column_major > m_normalInverse;
      cholmod_common  m_cholmodCommon;                       /**!< Contains object parameters,
                                                                   statistics, and workspace used
                                                                   by the CHOLMOD library.*/
      LinearAlgebra::Vector m_RHS;                           /**!< The right hand side of the
                                                                   normal equations.*/
      SparseBlockMatrix m_sparseNormals;                     /**!< The sparse block normal
                                                                   equations matrix.  Used to
                                                                   populate m_cholmodTriplet and
                                                                   for error propagation.*/
      cholmod_triplet  *m_cholmodTriplet;                    /**!< The CHOLMOD triplet
                                                                   representation of the sparse
                                                                   normal equations matrix.
                                                                   Created from m_sparseNormals
                                                                   and then used to create
                                                                   m_cholmodNormal.*/
      cholmod_sparse *m_cholmodNormal;                       /**!< The CHOLMOD sparse normal
                                                                   equations matrix used by
                                                                   cholmod_factorize to solve the
                                                                   system. Created from
                                                                   m_cholmodTriplet.*/
      cholmod_factor *m_L;                                   /**!< The lower triangular L matrix
                                                                   from Cholesky decomposition.
                                                                   Created from m_cholmodNormal by
                                                                   cholmod_factorize.*/
      LinearAlgebra::Vector m_imageSolution;                 /**!< The image parameter solution
                                                                   vector.*/

      int m_previousNumberImagePartials;                     /**!< used in ::computePartials method
                                                                   to avoid unnecessary resizing
                                                                   of the coeffImage matrix.*/
  };
}

#endif
