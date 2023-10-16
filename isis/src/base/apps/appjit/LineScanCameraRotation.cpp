#include <string>
#include <algorithm>
#include <vector>
#include <cfloat>
#include <cmath>
#include <iomanip>

#include<ale/Rotation.h>

#include "Cube.h"
#include "LineScanCameraRotation.h"
#include "Quaternion.h"
#include "BasisFunction.h"
#include "LeastSquares.h"
#include "BasisFunction.h"
#include "PolynomialUnivariate.h"
#include "IString.h"
#include "iTime.h"
#include "IException.h"
#include "Table.h"
#include "NaifStatus.h"

// Declarations for binding for Naif Spicelib routine refchg_ that does not have
// a wrapper
extern int refchg_(integer *frame1, integer *frame2, doublereal *et,
                   doublereal *rotate);

namespace Isis {
  /**
   * Construct an empty SpiceRotation class using a valid Naif frame code to
   * set up for getting rotation from Spice kernels.  See required reading
   * ftp://naif.jpl.nasa.gov/pub/naif/toolkit_docs/C/ascii/individual_docs/naif_ids.req
   *
   * @param frameCode Valid naif frame code.
   */
  LineScanCameraRotation::LineScanCameraRotation(int frameCode, Isis::Cube &cube, std::vector<double> timeCache, double tol) : SpiceRotation(frameCode) {
    // Initialize optional paramters;
    p_pitchRate = 0.;
    p_yaw = 0.;

    // Load the Spice kernels to get state matrices
    p_spi = 0;
    p_spi = new Isis::Spice(cube);

    // Make sure the kernels are written to the labels and not just the tables (blobs)
    Pvl &lab = *cube.label();
    if(!p_spi->hasKernels(lab)) {
      std::string msg = "The master file must contain the kernel files.  Rerun spiceinit with attach=no";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    // Get the kernel group and save the instrument pointing keyword
    Isis::PvlGroup kernels = lab.findGroup("Kernels", Isis::Pvl::Traverse);
    p_ckKeyword = kernels["InstrumentPointing"];

    p_cacheTime = timeCache;
//    std::cout<<std::setprecision(24);
//    std::cout<<timeCache.at(0)<<"-"<<timeCache.at(50000)<<std::endl;

    InitConstantRotation(p_cacheTime[0]);

    p_cachesLoaded = false;
    p_spi->instrumentRotation()->SetFrame(frameCode);
    p_crot = p_spi->instrumentRotation();
    p_prot = p_spi->bodyRotation();
    p_spos = p_spi->instrumentPosition();
    // Load the line scan specific rotation matrix caches before loading the regular Spice caches because
    // the CreateCache method will unload all the kernels after the caches are created
    LoadCache();
    p_spi->createCache(timeCache[0], timeCache[timeCache.size()-1], timeCache.size(), tol);
  }



  /**
   *  Destroys the LineScanComeraRotation object
   */
  LineScanCameraRotation::~LineScanCameraRotation() {
    if(p_spi != 0) delete p_spi;
  }



  /** Cache J2000 rotation quaternion over a time range.
   *
   * This method will load an internal cache with frames over a time
   * range.  This prevents the NAIF kernels from being read over-and-over
   * again and slowing an application down due to I/O performance.  Once the
   * cache has been loaded then the kernels can be unloaded from the NAIF
   * system.
   *
   * @internal
   * @history 2010-12-23  Debbie A. Cook Added set of full cache time
   *                       parameters
   */
  void LineScanCameraRotation::LoadCache() {
    NaifStatus::CheckErrors();

    double startTime = p_cacheTime[0];
    int size = p_cacheTime.size();
    double endTime = p_cacheTime[size-1];
    SetFullCacheParameters(startTime, endTime, size);

    // TODO  Add a label value to indicate pointing is already decomposed to line scan angles
    // and set p_pointingDecomposition=none,framing angles, or line scan angles.
    // Also add a label value to indicate jitterOffsets=jitterFileName
    // Then we can decide whether to simply grab the crot angles or do new decomposition and whether
    // to apply jitter or throw an error because jitter has already been applied.

    // *** May need to do a frame trace and load the frames (at least the constant ones) ***

    // Loop and load the cache
    double state[6];
    double lt;
    NaifStatus::CheckErrors();

    double R[3];  // Direction of radial axis of line scan camera
    double C[3];  // Direction of cross-track axis
    double I[3];  // Direction of in-track axis
    double *velocity;
    std::vector<double> IB(9);
    std::vector<double> CI(9);
    SpiceRotation *prot = p_spi->bodyRotation();
    SpiceRotation *crot = p_spi->instrumentRotation();

    std::vector<ale::Rotation> rotationCache;
    for(std::vector<double>::iterator i = p_cacheTime.begin(); i < p_cacheTime.end(); i++) {
      double et = *i;

      prot->SetEphemerisTime(et);
      crot->SetEphemerisTime(et);

      // The following code will be put into method LoadIBcache()
      spkezr_c("MRO", et, "IAU_MARS", "NONE", "MARS", state, &lt);
      NaifStatus::CheckErrors();

      // Compute the direction of the radial axis (3) of the line scan camera
      vscl_c(1. / vnorm_c(state), state, R); // vscl and vnorm only operate on first 3 members of state

      // Compute the direction of the cross-track axis (2) of the line scan camera
      velocity  =  state + 3;
      vscl_c(1. / vnorm_c(velocity), velocity, C);
      vcrss_c(R, C, C);

      // Compute the direction of the in-track axis (1) of the line scan camera
      vcrss_c(C, R, I);

      // Load the matrix IB and enter it into the cache
      vequ_c(I, (SpiceDouble( *)) &IB[0]);
      vequ_c(C, (SpiceDouble( *)) &IB[3]);
      vequ_c(R, (SpiceDouble( *)) &IB[6]);
      p_cacheIB.push_back(IB);
      // end IB code

      // Compute the CIcr matrix - in-track, cross-track, radial frame to constant frame
      mxmt_c((SpiceDouble( *)[3]) & (crot->TimeBasedMatrix())[0], (SpiceDouble( *)[3]) & (prot->Matrix())[0],
             (SpiceDouble( *)[3]) &CI[0]);

      // Put CI into parent cache to use the parent class methods on it
      mxmt_c((SpiceDouble( *)[3]) &CI[0], (SpiceDouble( *)[3]) &IB[0], (SpiceDouble( *)[3]) &CI[0]);
      rotationCache.push_back(ale::Rotation(CI));
    }

    if (m_orientation) {
      delete m_orientation;
      m_orientation = NULL;
    }

    if (ConstantRotation().size() > 1) {
      m_orientation = new ale::Orientations(rotationCache, p_cacheTime, std::vector<ale::Vec3d>(),
                                            ale::Rotation(ConstantMatrix()), ConstantFrameChain(), TimeFrameChain());
    }
    else {
      m_orientation = new ale::Orientations(rotationCache, p_cacheTime, std::vector<ale::Vec3d>(),
                                            ale::Rotation(1,0,0,0), ConstantFrameChain(), TimeFrameChain());
    }

    p_cachesLoaded = true;
    SetSource(Memcache);

    NaifStatus::CheckErrors();
  }

  /** Cache J2000 rotation over existing cached time range using polynomials
   *
   * This method will reload an internal cache with matrices
   * formed from rotation angles fit to polynomials over a time
   * range.
   *
   * @param function1   The first polynomial function used to
   *                    find the rotation angles
   * @param function2   The second polynomial function used to
   *                    find the rotation angles
   * @param function3   The third polynomial function used to
   *                    find the rotation angles
   */
  void LineScanCameraRotation::ReloadCache() {
    NaifStatus::CheckErrors();

    // Make sure caches are already loaded
    if(!p_cachesLoaded) {
      std::string msg = "A LineScanCameraRotation cache has not been loaded yet";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    // Create polynomials fit to angles & use to reload cache
    Isis::PolynomialUnivariate function1(p_degree);
    Isis::PolynomialUnivariate function2(p_degree);
    Isis::PolynomialUnivariate function3(p_degree);

    // Get the coefficients of the polynomials already fit to the angles of rotation defining [CI]
    std::vector<double> coeffAng1;
    std::vector<double> coeffAng2;
    std::vector<double> coeffAng3;
    GetPolynomial(coeffAng1, coeffAng2, coeffAng3);

    // Reset linear term to center around zero -- what works best is either roll-avg & pitchavg+ or pitchavg+ & yawavg-
//    coeffAng1[1] -= 0.0000158661225;
//    coeffAng2[1] = 0.0000308433;
//    coeffAng3[0] = -0.001517547;
    if(p_pitchRate)  coeffAng2[1] = p_pitchRate;
    if(p_yaw)  coeffAng3[0] = p_yaw;

    // Load the functions with the coefficients
    function1.SetCoefficients(coeffAng1);
    function2.SetCoefficients(coeffAng2);
    function3.SetCoefficients(coeffAng3);

    double CI[3][3];
    double IJ[3][3];
    double rtime;
    SpiceRotation *prot = p_spi->bodyRotation();
    std::vector<double> CJ;
    CJ.resize(9);

    std::vector<ale::Rotation> rotationCache;
    for(std::vector<double>::size_type pos = 0; pos < p_cacheTime.size(); pos++) {
      double et = p_cacheTime.at(pos);
      rtime = (et - GetBaseTime()) / GetTimeScale();
      double angle1 = function1.Evaluate(rtime);
      double angle2 = function2.Evaluate(rtime);
      double angle3 = function3.Evaluate(rtime);

// Get the first angle back into the range Naif expects [180.,180.]
      if(angle1 < -1 * pi_c()) {
        angle1 += twopi_c();
      }
      else if(angle1 > pi_c()) {
        angle1 -= twopi_c();
      }

      eul2m_c((SpiceDouble) angle3, (SpiceDouble) angle2, (SpiceDouble) angle1,
              p_axis3,                    p_axis2,                    p_axis1,
              CI);
      mxm_c((SpiceDouble( *)[3]) & (p_jitter->SetEphemerisTimeHPF(et))[0], CI, CI);

      prot->SetEphemerisTime(et);
      mxm_c((SpiceDouble( *)[3]) & (p_cacheIB.at(pos))[0], (SpiceDouble( *)[3]) & (prot->Matrix())[0], IJ);
      mxm_c(CI, IJ, (SpiceDouble( *)[3]) &CJ[0]);

      rotationCache.push_back(ale::Rotation(CJ));
    }

    if (m_orientation) {
      delete m_orientation;
      m_orientation = NULL;
    }

    if (ConstantRotation().size() > 1) {
      m_orientation = new ale::Orientations(rotationCache, p_cacheTime, std::vector<ale::Vec3d>(),
                                            ale::Rotation(ConstantMatrix()), ConstantFrameChain(), TimeFrameChain());
    }
    else {
      m_orientation = new ale::Orientations(rotationCache, p_cacheTime, std::vector<ale::Vec3d>(),
                                            ale::Rotation(1,0,0,0), ConstantFrameChain(), TimeFrameChain());
    }

    // Set source to cache to get updated values
    SetSource(SpiceRotation::Memcache);

    // Make sure SetEphemerisTime updates the matrix by resetting it twice (in case the first one
    // matches the current et.  p_et is private and not available from the child class
    NaifStatus::CheckErrors();
    SetEphemerisTime(p_cacheTime[0]);
    SetEphemerisTime(p_cacheTime[1]);

    NaifStatus::CheckErrors();
  }

}
