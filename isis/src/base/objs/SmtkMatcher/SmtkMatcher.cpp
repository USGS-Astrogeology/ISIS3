/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <iostream>
#include <iomanip>

#include <QSharedPointer>

#include "Camera.h"
#include "Projection.h"
#include "SpecialPixel.h"
#include "SmtkMatcher.h"


namespace Isis {

  /** Construct default matcher */
  SmtkMatcher::SmtkMatcher() : m_lhCube(0), m_rhCube(0), m_gruen(),
                               m_offImage(0), m_spiceErr(0),
                               m_useAutoReg(true) {
    randomNumberSetup();
  }

  /** Construct with Gruen definintions file */
  SmtkMatcher::SmtkMatcher(const QString &regdef) : m_lhCube(0),
                                                        m_rhCube(0),
                                                        m_gruen(0),
                                                        m_offImage(0),
                                                        m_spiceErr(0),
                                                        m_useAutoReg(true) {
    setGruenDef(regdef);
    randomNumberSetup();
  }

  /** Construct with Gruen definintions file and camera objects  */
  SmtkMatcher::SmtkMatcher(const QString &regdef, Cube *lhCube,
                           Cube *rhCube) : m_lhCube(lhCube),
                                           m_rhCube(rhCube),
                                           m_gruen(0), m_offImage(0),
                                           m_spiceErr(0),
                                           m_useAutoReg(true) {
    setGruenDef(regdef);
    randomNumberSetup();
  }

  /** Free random number generator in destructor */
  SmtkMatcher::~SmtkMatcher() {
    gsl_rng_free(r);
  }


  /** Assign cubes for matching */
  void SmtkMatcher::setImages(Cube *lhCube, Cube *rhCube) {
    m_lhCube = lhCube;
    m_rhCube = rhCube;
    return;
  }

  /**
   * @brief Initialize Gruen algorithm with definitions in Pvl file provided
   *
   * This method will initialize the Gruen algorithm with a standard AutoReg
   * definitions file.  It is re-entrant in that should an existing Gruen
   * object be present, it is freed and replaced by the one resulting from the
   * instantion with the regdef file provided.
   *
   * @author Kris Becker - 5/26/2011
   *
   * @param regdef Gruen definitions Pvl file
   */
  void SmtkMatcher::setGruenDef(const QString &regdef) {
    Pvl reg(regdef.toStdString());
    m_gruen = QSharedPointer<Gruen> (new Gruen(reg));  // Deallocation automatic
    return;
  }

  /**
   * @brief Determine if a point is valid in both left/right images
   *
   * This method accepts a point from the left hand image and determines if it
   * maps to a valid lat/lon coordinate in the left image.  It then takes the
   * lat/lon from the left and determines if it maps to a valid line/sample
   * int the right image.
   *
   * Both images must have cameras associated with them or an exception is
   * thrown.
   *
   * @author kbecker (5/25/2011)
   *
   * @param pnt Line/Sample coordinate in left image
   *
   * @return bool True if valid in both images, otherwise false
   */
  bool SmtkMatcher::isValid(const Coordinate &pnt) {
    validate();  // cubs and cameras
    Coordinate pnt2 = getLatLon(lhCamera(), pnt);
    if (pnt2.isValid()) {
      pnt2 = getLineSample(rhCamera(), pnt2);
    }
    return (pnt2.isValid());
  }

  /**
   * @brief Set file pattern for output subsearch chips
   *
   * This method should be used to set the output subsearch chip
   * file pattern that will be used to write the transformed chip
   * at each Gruen iteration.  This is handy to test the pattern
   * chip with the Gruen algorithm search chip.  A chip will be
   * generated for each search chip that is transformed for the
   * current iteration.
   *
   * Note this pattern is only validate for the next call to
   * Register().  It will be reset so that chips are not
   * automatically written for every call to Register().
   *
   * The pattern can be complete directory path and a file
   * pattern.  See Gruen for a complete description.
   *
   * @author kbecker (9/15/2011)
   *
   * @param pattern Set the file pattern of the output cube search
   *                chips.
   */
  void SmtkMatcher::setWriteSubsearchChipPattern(const QString &fileptrn) {
    validate();
    m_gruen->WriteSubsearchChips(fileptrn);
  }

  /**
   * @brief Find the smallest eigen value on the given stack
   *
   * This static method iterates through a stack to find the best (smallest)
   * eigen value as computed by the Gruen registration algorithm.  If the stack
   * is empty or if for some unapparent reason why the best point cannot be
   * found, stack.end() is returned.
   *
   * @param stack  Stack to find the best point in.
   *
   * @return SmtkQStackIter   Returns an iterator with the best value
   */
  SmtkQStackIter SmtkMatcher::FindSmallestEV(SmtkQStack &stack) {
    SmtkQStackIter best(stack.begin()), current(stack.begin());
    while ( current != stack.end() ) {
      if (current.value().GoodnessOfFit() < best.value().GoodnessOfFit() ) {
        best = current;
      }
      ++current;
    }
    return (best);
  }


  /**
   * @brief Find the best eigen value using exponential distribution formula
   *
   * This method has the same objective as FindSmallestEV, but uses a different
   * test to find the best value.  It uses s randomly generated value within an
   * exponential distribution from the minimum to maximum occuring eigen value.
   *
   * Upon each call to this routine, the random seed is regenerated.
   *
   * NOTE:  This implementation differs somewhat from the ISIS2 version in that
   * the value of the computed eigenvalue is used as the best eigenvalue. This
   * implementation produced better distributoion of points
   *
   * @param stack       Stack to find the best value in
   * @param seedsample  The point within the exponential distribution of
   *                    interest
   * @param minEV       Minimum occuring eigen value
   * @param maxEV       Maximum occuring eigen value
   *
   * @return SmtkQStackIter Stack interator
   */
  SmtkQStackIter SmtkMatcher::FindExpDistEV(SmtkQStack &stack,
                                            const double &seedsample,
                                            const double &minEV,
                                            const double &maxEV) {

    if (stack.isEmpty()) return (stack.end());
    SmtkQStackIter best(stack.begin()), current(stack.begin());

    // Random number generator must scale between 0 and 1
    double randNum = gsl_rng_uniform (r);
    double t1 = -std::log(1.0 - randNum *
                          (1.0 - std::exp(-seedsample)))/seedsample;
    double pt = minEV + t1*(maxEV-minEV);
    double best_ev(DBL_MAX);
    while ( current != stack.end() ) {
      double test_ev(std::fabs(current.value().GoodnessOfFit()-pt));
      if ( test_ev  < best_ev) {
        best = current;
        best_ev = test_ev; // differs from ISIS2 which saved current eigenvalue
      }
      ++current;
    }
    return (best);
  }

  /**
   * @brief Valid a point prior to insertion
   *
   * This method can be used to valid a point prior to adding it to a point
   * stack.  It is a final validation check and should not be used in
   * production but used to debug.
   *
   * @author kbecker (6/2/2011)
   *
   * @param spnt SmtkPoint to check for valid coordiantes
   *
   * @return bool True if valid, false if not
   */
  bool SmtkMatcher::isValid(const SmtkPoint &spnt) {
    bool valid(true);

    if (!spnt.isValid()) {
      std::cout << "Point status is invalid!\n";
      valid = false;
    }

    // Is valid in left image?
    if (!inCube(lhCamera(), spnt.getLeft())) {
      std::cout << "Left point (" << spnt.getLeft().getLine() << ","
           << spnt.getLeft().getSample() << ") is invalid!\n";
      valid = false;
    }

    // Is valud in right image?
    if (!inCube(rhCamera(), spnt.getRight())) {
      std::cout << "Right point (" << spnt.getRight().getLine() << ","
           << spnt.getRight().getSample() << ") is invalid!\n";
      valid = false;
    }

    return (valid);
  }

  /**
   * This method takes a sample, line from the left-hand image and tries to
   * find the matching point in the right-hand image.
   *
   */
  SmtkPoint SmtkMatcher::Register(const Coordinate &lpnt,
                                  const AffineRadio &affrad) {
    return (Register(PointGeometry(lpnt), PointGeometry(), affrad));
  }

  /**
   * @brief Register a defined left/right point pair
   *
   * @author Kris Becker - 5/30/2011
   *
   * @param pnts   Points to register
   * @param affrad Affine/Radiometric parameters to use
   *
   * @return SmtkPoint Result of point registration
   */
  SmtkPoint SmtkMatcher::Register(const PointPair &pnts,
                                  const AffineRadio &affrad) {
    return (Register(PointGeometry(pnts.getLeft()),
                     PointGeometry(pnts.getRight()), affrad));
  }



  /**
   * @brief Register an SmtkPoint
   *
   *  This method will register an established SmtkPoint.  It will determine
   *  what parts of the point need to be completed in order for the
   *  registration to be valid.  For instance, a default initialization of an
   *  SmtkPoint may only requre the left point to be defined.  Or both points
   *  are defined but it has been registered but cloned (see Clone()).
   *
   *  It the point is deemed registered it will simply be returned as is
   *  without further processing.  So to register a point ensure it the
   *  m_register flag is set to false.
   *
   * @author Kris Becker - 5/30/2011
   *
   * @param spnt    SmtkPoint to register
   * @param affrad  Affine/Radiometrics to use for registration
   *
   * @return SmtkPoint A registered point.  Test for validity on return to
   *         evaluate success.
   */
  SmtkPoint SmtkMatcher::Register(const SmtkPoint &spnt,
                                  const AffineRadio &affrad) {

    //  If point is already registered don't do it again.
    if (spnt.isRegistered()) { return (spnt); }

    PointGeometry left = PointGeometry(spnt.getLeft());
    PointGeometry right = PointGeometry(spnt.getRight());

    return (Register(left, right, affrad));
  }


  /**
   * @brief Applies registration of two points
   *
   * This method applies the registration of a left and right point set.  The
   * points sets may only have the left point defined.  This method determines
   * the valid geometry of the left point.  If the right point is not defined,
   * it uses the left geometry to determine the right point to register the
   * left point with.  If the right point is defined, it verifies the point has
   * valid geometry mapping in the right image.  All points and geometry must
   * fall within the boundaries of the image and be valid or the point is
   * deemed invalid.
   *
   * Once the points is validated, registration is applied to the two points
   * using the left point as truth (or the pattern chip) and the right point
   * (search chip) is loaded according to the geometry of the left chip.  An
   * affine transform is immediately applied in the Gruen algorithm to apply
   * the user supplied state of the affine and radiometric parameters.
   *
   * @author Kris Becker - 6/4/2011
   *
   * @param lpg
   * @param rpg
   * @param affrad
   *
   * @return SmtkPoint
   */
  SmtkPoint SmtkMatcher::Register(const PointGeometry &lpg,
                                  const PointGeometry &rpg,
                                  const AffineRadio &affrad) {

    // Validate object state
    validate();

    // Test if the left point is defined. This will throw an error if this
    // situation occurs
    if (!lpg.getPoint().isValid()) {
      QString mess = "Left point is not defined which is required";
      throw IException(IException::Programmer, mess, _FILEINFO_);
    }

   // First we need a lat,lon from the left image to find the same place in
    // the right image.
    Coordinate lpnt = lpg.getPoint();
    Coordinate lgeom = lpg.getGeometry();
    if (!lgeom.isValid()) {
      lgeom = getLatLon(lhCamera(), lpnt);
    }

    // Construct geometry and check validity
    PointGeometry left = PointGeometry(lpnt, lgeom);
    if (!left.isValid()) {
      m_offImage++;
      return ( SmtkPoint(PointPair(lpnt), PointPair(lgeom)) );
    }

    // Validate the right point
    Coordinate rpnt = rpg.getPoint();
    Coordinate rgeom = rpg.getGeometry();
    if ( !rpnt.isValid() ) {
      if (rgeom.isValid()) {
        rpnt = getLineSample(rhCamera(), rgeom);
      }
      else {
        rpnt = getLineSample(rhCamera(), lgeom);
        rgeom = lgeom;
      }
    }
    else if (!rgeom.isValid() ){
      rgeom = getLatLon(rhCamera(), rpnt);
    }

    //  Construct and for good right geometry
    PointGeometry right = PointGeometry(rpnt, rgeom);
    if (!right.isValid()) {
      m_spiceErr++;
      return (SmtkPoint(PointPair(lpnt, rpnt), PointPair(lgeom, rgeom)));
    }

    try {
      // These calls are computationally expensive... can we fix it?
      m_gruen->PatternChip()->TackCube(lpnt.getSample(), lpnt.getLine());
      m_gruen->PatternChip()->Load(*m_lhCube);
      m_gruen->SearchChip()->TackCube(rpnt.getSample(), rpnt.getLine());
      m_gruen->SearchChip()->Load(*m_rhCube, *m_gruen->PatternChip(),
                                  *m_lhCube);
    }
    catch (IException &) {
      m_offImage++;  // Failure to load is assumed an offimage error
      return (SmtkPoint(PointPair(lpnt,rpnt), PointPair(lgeom,rgeom)));
    }

    // Register the points with incoming affine/radiometric parameters
    m_gruen->setAffineRadio(affrad);
    return (makeRegisteredPoint(left, right, m_gruen.data()));
  }

  /**
   * @brief Create a valid, unregistered SmtkPoint
   *
   * This method is typically used to create a point from a control point
   * network.  The point is deemed registered, but not necessarily by Gruen.
   * Therefore, it is set as unregistered.
   *
   * The left and right coordinates are deemed valid and geometry for both
   * points is computed and verified (either the points are off image or does
   * not map to a lat/long).
   *
   * The points is set as valid and when Register() is called it will likely be
   * run through the Gruen algorithm.
   *
   * In essence, a valid, but unregistered SmtkPoint is returned if all the
   * line/sample coordinates and geometry check out.
   *
   * @author Kris Becker - 5/30/2011
   *
   * @param left  Left point
   * @param right Right point
   *
   * @return SmtkPoint
   */
  SmtkPoint SmtkMatcher::Create(const Coordinate &left,
                                const Coordinate &right) {

    // Check out left image point
    Coordinate lgeom = getLatLon(lhCamera(), left);
    if (!lgeom.isValid() ) {
      m_offImage ++;
      return (SmtkPoint(PointPair(left,right), PointPair(lgeom)));
    }

    // Check out right image point
    Coordinate rgeom = getLatLon(rhCamera(), right);
    if (!rgeom.isValid() ) {
      m_offImage ++;
      return (SmtkPoint(PointPair(left,right), PointPair(lgeom,rgeom)));
    }

    //  Make the point
    SmtkPoint spnt = SmtkPoint(PointPair(left,right), PointPair(lgeom,rgeom));
    spnt.m_matchpt.m_analysis.setZeroState();
    spnt.m_isValid = true;
    return (spnt);
  }


  /**
   * @brief Clone a point set from a nearby (left image) point and Gruen affine
   *
   * This method is used to clone a PointPair from a new point and an Affine
   * transform.  Assume the left point in the set is at the center of the box
   * and compute the offset of the point using the Affine.  Apply it the right
   * point.
   *
   * @param point    Chip center location
   * @param newpoint New point to clone to via application of affine transform
   * @param affine   The affine transform to apply to get new (right) clone
   *                  point
   *
   * @return PointPair The clone point from the original
   */
  SmtkPoint SmtkMatcher::Clone(const SmtkPoint &point,
                               const Coordinate &left) {

    //  Computes local chip location (chipLoc) with Affine.  This gives offset
    //   in right pixel space.  Simply add result to right point to get the
    //   new cloned right point.
    Coordinate offset = left - point.getLeft();
    Coordinate right = point.getRight() + point.getAffine().getPoint(offset);
    PointPair cpoint = PointPair(left, right);

    MatchPoint mpt = point.m_matchpt;
    mpt.m_point = cpoint;

    //  Currently will not have any valid geometry
    SmtkPoint spnt = SmtkPoint(mpt, PointGeometry(right), PointPair());
    spnt.m_registered = false;
    spnt.m_isValid = inCube(lhCamera(), left) && inCube(rhCamera(), right);
    return (spnt);
  }

  /**
   * @brief Initialize the random number generator
   *
   * This randome number generator uses the GSL version.  It can be set using
   * envirnment variables.  Please see the documenation at the GSL site
   * http://www.gnu.org/software/gsl/manual/html_node/Random-Number-Generation.html.
   *
   * @author kbecker (6/3/2011)
   */
  void SmtkMatcher::randomNumberSetup() {
    (void) GSL::GSLUtility::getInstance();
    gsl_rng_env_setup();
    T = gsl_rng_default;
    r = gsl_rng_alloc (T);
    return;
  }
  /**
   * @brief Validates the state of the Camera and Gruen algoritm
   *
   * Basically ensures all the pointers are initialized.
   *
   * @author Kris Becker - 5/1/2011
   *
   * @param throwError Throws an exception if user sets this to true.
   *                   Otherwise status can be determined from the return
   *                   value.
   *
   * @return bool  If throwError is not set, this will return status.
   *                 throwError will cause an exception instead if true.
   */
  bool SmtkMatcher::validate(const bool &throwError) const {
    bool isGood(true);
    if (!m_lhCube) isGood = false;
    if (!m_rhCube) isGood = false;
    if (!m_gruen.data()) isGood = false;
    if ((!isGood) && throwError) {
      QString mess = "Images/match algorithm not initialized!";
      throw IException(IException::Programmer, mess, _FILEINFO_);
    }
    return (isGood);
  }

  /**
   * @brief Determines if the line/sample is within physical cube boundaries
   *
   * @author Kris Becker - 5/1/2011
   *
   * @param camera Camera to determine dimensions
   * @param sample Sample location
   * @param line   Line location
   *
   * @return bool
   */
  bool SmtkMatcher::inCube(const Camera &camera, const Coordinate &pnt) const {
    if (!pnt.isValid()) return (false);
    if ( (pnt.getSample() < 0.5) || (pnt.getLine() < 0.5) ) return false;
    if ( (pnt.getSample() > (camera.Samples() + 0.5)) ||
         (pnt.getLine()   > (camera.Lines()   + 0.5)) ) {
      return false;
    }

    return true;
  }


  /**
   * @brief Compute latitude, longitude from line,sample
   *
   * @author Kris Becker - 5/23/2011
   *
   * @param camera Camera object to compute lat/lon coordinates in
   * @param pnt    Line/Sample coordinate of point to compute
   *
   * @return Coordinate Returns a latitude/longitude pair of point
   */
  Coordinate SmtkMatcher::getLatLon(Camera &camera, const Coordinate &pnt) {
    // Check if pixel coordinate is in the left image
    Coordinate geom;
    if (pnt.isValid()) {
      if (inCube(camera, pnt)) {
        if ( camera.SetImage(pnt.getSample(), pnt.getLine()) ) {
          double latitude = camera.UniversalLatitude();
          double longitude = camera.UniversalLongitude();
          geom.setLatLon(latitude, longitude);
        }
      }
    }
    return (geom);
  }

  /**
   * @brief Compute line,sample from latitude, longitude
   *
   * @author Kris Becker - 5/23/2011
   *
   * @param camera Camera object to compute lat/lon coordinates in
   * @param pnt    Latitude/longitude coordinate to compute
   *
   * @return Coordinate Returns a line/sample pair of point
   */
  Coordinate SmtkMatcher::getLineSample(Camera &camera,
                                        const Coordinate &geom) {
    // Check if pixel coordinate is in the left image
    Coordinate pnt;
    if (geom.isValid()) {
      if ( camera.SetUniversalGround(geom.getLatitude(), geom.getLongitude()) ) {
        if ( camera.InCube() ) {
          pnt.setLineSamp(camera.Line(), camera.Sample());
        }
      }
    }
    return (pnt);
  }

  /**
   * @brief Create an SmtkPoint from Gruen match result
   *
   * This method applies the Gruen registration to the points as provided.  It
   * assumes the points have been already set up in the gruen algorithm (@see
   * Register()) and simply calls the Gruen::RegisterI() function.
   *
   * The result is the transformed into a SmtkPoint base upon the result of the
   * registration.  The status of the point is set to reflect the registration
   * processing result.
   *
   * @author Kris Becker - 5/26/2011
   *
   * @param left  Left point with geometry
   * @param right RIght point with geometry
   * @param gruen Gruen algorithm to use to register points
   *
   * @return SmtkPoint
   */
  SmtkPoint SmtkMatcher::makeRegisteredPoint(const PointGeometry &left,
                                             const PointGeometry &right,
                                             Gruen *gruen) {

    if (gruen->Register() != AutoReg::SuccessSubPixel) {
      return (SmtkPoint(PointPair(left.getPoint(),right.getPoint()),
                         PointPair(left.getGeometry(), right.getGeometry())));
    }

    // Registration point data
    MatchPoint match = gruen->getLastMatch();

    // Compute new right coordinate data
    Coordinate rcorr = Coordinate(gruen->CubeLine(), gruen->CubeSample());
    Coordinate rgeom = getLatLon(rhCamera(), rcorr);
    PointGeometry rpoint = PointGeometry(rcorr, rgeom);

    //  Get left and original right geometry
    PointPair pgeom = PointPair(left.getGeometry(), right.getGeometry());

    // Create SMTK point and determine status/validity
    SmtkPoint spnt = SmtkPoint(match, rpoint, pgeom);

    //  Check for valid mapping
    if ( !rpoint.isValid() ) {
      m_offImage++;
      spnt.m_isValid = false;
    }
    else {
     // Check for distance error
      double dist = (rcorr - right.getPoint()).getDistance();
      if (dist > gruen->getSpiceConstraint()) {
        m_spiceErr++;
        spnt.m_isValid = false;
      }
      else {
        spnt.m_isValid = match.isValid();
      }
    }

    return (spnt);
  }

}
