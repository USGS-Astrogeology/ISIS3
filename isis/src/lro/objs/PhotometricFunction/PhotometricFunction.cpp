/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <SpiceUsr.h>
#include <SpiceZfc.h>
#include <SpiceZmc.h>

#include "Angle.h"
#include "Camera.h"
#include "DbProfile.h"
#include "PhotometricFunction.h"
#include "PvlObject.h"

using namespace std;

namespace Isis {
 /**
  * Construct Photometric function from Pvl and Cube file
  *
  * @param pvl photometric parameter files
  * @param cube Input cube file
  *
  * @author 2016-08-15 Victor Silva
  *
  * @internal
  *   @history 2016-08-15 Code adapted from lrowacpho written by Kris Becker
  *
  **/
  PhotometricFunction::PhotometricFunction( PvlObject &pvl, Cube &cube , bool useCamera ) {
    if (useCamera) {
      m_camera = cube.camera();
    }
  }


  /**
   * Destructor
   */
  PhotometricFunction::~PhotometricFunction() {}


  /**
   * Set the camera used to compute photometric angles.
   *
   * @param cam A pointer to the camera to be used
   */
  void PhotometricFunction::setCamera(Camera *cam) {
    m_camera = cam;
  }


  /**
   * Finds the name of the algorithm defined in a PVL object.
   *
   * @param pvl The pvl to find the algorithm name in.
   *
   * @return @b QString The algorithm name from the PVL object.
   */
  QString PhotometricFunction::algorithmName( const PvlObject &pvl ) {
    return QString::fromStdString(pvl.findObject("PhotometricModel").findGroup("Algorithm", Pvl::Traverse).findKeyword("Name")[0]);
  }


 /**
  * Computes Photometric function from cube attributes
  *
  * @param line line number in cube
  * @param sample sample number in cube
  * @param band band number in cube
  * @param useDem boolean to use provided Dem
  *
  * @return @b double photometry calculations
  *
  * @author 2016-08-15 Victor Silva
  *
  * @internal
  *   @history 2016-08-15 Victor Silva - Code adapted from lrowacpho written by Kris Becker
  *
  **/
  double PhotometricFunction::compute( const double &line, const double &sample, int band, bool useDem) {
    // Update band if necessary
    if (m_camera->Band() != band) {
       m_camera->SetBand(band);
    }
    // Return null if not able to set image
    if (!m_camera->SetImage(sample, line)) {
      return (Null);
    }
    // calculate photometric angles
    double i = m_camera->IncidenceAngle();
    double e = m_camera->EmissionAngle();
    double g = m_camera->PhaseAngle();
    bool success = true;

    if (useDem) {
      Angle phase, incidence, emission;
      m_camera->LocalPhotometricAngles(phase, incidence, emission, success);

      if (success) {
        g = phase.degrees();
        i = incidence.degrees();
        e = emission.degrees();
      }
    }

    if ( !success || i < minimumIncidenceAngle() || i > maximumIncidenceAngle() || e < minimumEmissionAngle() || e
                        > maximumEmissionAngle() || g < minimumPhaseAngle() || g > maximumPhaseAngle()) {
      return (Null);
    }

    return photometry(i, e, g, band);
  }


 /**
  * Mutator function to set minimum incidence angle
  *
  * @param double The new minimum incidence angle
  *
  * @author 2016-08-15 Victor Silva
  *
  * @internal
  *   @history 2016-08-15 Code adapted from lrowacpho written by Kris Becker
  *
  **/
  void PhotometricFunction::setMinimumIncidenceAngle(double angle) {
    m_minimumIncidenceAngle = angle;
  }


 /**
  * Mutator function to set maximum incidence angle
  *
  * @param double the new maximum incidence angle
  *
  * @author 2016-08-15 Victor Silva
  *
  * @internal
  *   @history 2016-08-15 Code adapted from lrowacpho written by Kris Becker
  *
  **/
  void PhotometricFunction::setMaximumIncidenceAngle(double angle) {
    m_maximumIncidenceAngle = angle;
  }


 /**
  * Mutator function to set minimum emission angle
  *
  * @param double The new minimum emission angle
  *
  * @author 2016-08-15 Victor Silva
  *
  * @internal
  *   @history 2016-08-15 Code adapted from lrowacpho written by Kris Becker
  *
  **/
  void PhotometricFunction::setMinimumEmissionAngle(double angle) {
    m_minimumEmissionAngle = angle;
  }


 /**
  * Mutator function to set maximum emission angle
  *
  * @param double The new maximum emission angle
  *
  * @author 2016-08-15 Victor Silva
  *
  * @internal
  *   @history 2016-08-15 Code adapted from lrowacpho written by Kris Becker
  *
  **/
  void PhotometricFunction::setMaximumEmissionAngle(double angle) {
    m_maximumEmissionAngle = angle;
  }


 /**
  * Mutator function to set minimum phase angle
  *
  * @param double The new minimum phase angle
  *
  * @author 2016-08-15 Victor Silva
  *
  * @internal
  *   @history 2016-08-15 Code adapted from lrowacpho written by Kris Becker
  *
  **/
  void PhotometricFunction::setMinimumPhaseAngle(double angle) {
    m_minimumPhaseAngle = angle;
  }


 /**
  * Mutator function to set maximum phase angle
  *
  * @param double The new maximum phase angle
  *
  * @author 2016-08-15 Victor Silva
  *
  * @internal
  *   @history 2016-08-15 Code adapted from lrowacpho written by Kris Becker
  *
  **/
  void PhotometricFunction::setMaximumPhaseAngle(double angle) {
    m_maximumPhaseAngle = angle;
  }


 /**
  * Accessor method to access minimum incidence angle
  *
  * @return @b double The minimum incidence angle
  *
  * @author 2016-08-15 Victor Silva
  *
  * @internal
  *   @history 2016-08-15 Code adapted from lrowacpho written by Kris Becker
  *
  **/
  double PhotometricFunction::minimumIncidenceAngle() {
    return m_minimumIncidenceAngle;
  }


 /**
  * Accessor method to access maximum incidence angle
  *
  * @return @b double The maximum incidence angle
  *
  * @author 2016-08-15 Victor Silva
  *
  * @internal
  *   @history 2016-08-15 Code adapted from lrowacpho written by Kris Becker
  *
  **/
  double PhotometricFunction::maximumIncidenceAngle() {
    return m_maximumIncidenceAngle;
  }


 /**
  * Accessor method to access minimum emission angle
  *
  * @return @b double The minimum emission angle
  *
  * @author 2016-08-15 Victor Silva
  *
  * @internal
  *   @history 2016-08-15 Code adapted from lrowacpho written by Kris Becker
  *
  **/
  double PhotometricFunction::minimumEmissionAngle() {
    return m_minimumEmissionAngle;
  }


 /**
  * Accessor method to access maximum emission angle
  *
  * @return @b double The maximum emission angle
  *
  * @author 2016-08-15 Victor Silva
  *
  * @internal
  *   @history 2016-08-15 Code adapted from lrowacpho written by Kris Becker
  *
  **/
  double PhotometricFunction::maximumEmissionAngle() {
    return m_maximumEmissionAngle;
  }


 /**
  * Accessor method to access minimum phase angle
  *
  * @return @b double The minimum phase angle
  *
  * @author 2016-08-15 Victor Silva
  *
  * @internal
  *   @history 2016-08-15 Code adapted from lrowacpho written by Kris Becker
  *
  **/
  double PhotometricFunction::minimumPhaseAngle() {
    return m_minimumPhaseAngle;
  }


 /**
  * Accessor method to access maximum phase angle
  *
  * @return @b double The maximum phase angle
  *
  * @author 2016-08-15 Victor Silva
  *
  * @internal
  *   @history 2016-08-15 Code adapted from lrowacpho written by Kris Becker
  *
  **/
  double PhotometricFunction::maximumPhaseAngle() {
    return m_maximumPhaseAngle;
  }

}
