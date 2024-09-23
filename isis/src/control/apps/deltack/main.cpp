/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

#include <cmath>
#include <QtGlobal>
#include <QList>
#include <vector>

#include <SpiceUsr.h>

#include "BundleAdjust.h"
#include "BundleResults.h"
#include "BundleSettings.h"
#include "BundleSolutionInfo.h"
#include "Camera.h"
#include "CameraFactory.h"
#include "ControlMeasure.h"
#include "ControlNet.h"
#include "ControlPoint.h"
#include "Distance.h"
#include "History.h"
#include "IException.h"
#include "iTime.h"
#include "Latitude.h"
#include "Longitude.h"
#include "Process.h"
#include "Progress.h"
#include "PvlObject.h"
#include "Quaternion.h"
#include "SerialNumberList.h"
#include "SpiceRotation.h"
#include "Table.h"
#include "TableRecord.h"

using namespace std;
using namespace Isis;

Distance GetRadius(QString filename, Latitude lat, Longitude lon);
BundleSettingsQsp bundleSettings();

void Vector2VectorRotation(const double v1[3], const double v2[3], double rmat[3][3]);
void ApplyRotation(const double R[3][3], Table &ckTable);
void printMatrix(const SpiceDouble m[3][3]);


void IsisMain() {
  Progress progress;
  UserInterface &ui = Application::GetUserInterface();
  QString filename = ui.GetCubeName("FROM");
  //ControlNet m_cnet(ui.GetFileName("NET"),&progress);
  //QList<ControlPoint *> cntrlPts = m_cnet.GetPoints();
  //int npts = cntrlPts.length();
  //for (int i =0; i< npts;i++) {
         //cntrlPts[i]->SetAprioriSurfacePoint(SurfacePoint(lat1, lon1, rad1));


 // }
  try {
    // Create a serial number list
    SerialNumberList serialNumberList;
    serialNumberList.add(filename);

    // Get the coordinate for updating the camera pointing
    // We will want to make the camera pointing match the lat/lon at this
    // line sample
    double samp1 = ui.GetDouble("SAMP1");
    double line1 = ui.GetDouble("LINE1");
    Latitude lat1(ui.GetDouble("LAT1"), Angle::Degrees);
    Longitude lon1(ui.GetDouble("LON1"), Angle::Degrees);
    QString method = ui.GetString("METHOD").toLower();

    // This stuff will be needed later
    Cube c;
    c.open(filename, "rw");
    // we will check for target name inside the SetTarget() call

    // Prepare for update to the cube history
    History hist = c.readHistory();

    //----------------------------------------------------------------------------------
    // Execute the requested method
    //----------------------------------------------------------------------------------
    PvlGroup results("DeltackResults");
    results += PvlKeyword("Method", method.toStdString());
    if ( "direct" == method ) {
      Camera *v_cam = c.camera();

      // Map the latitude/longitude to a line/sample of the desired update
      // cout << "Input Lat, Lon = " << lat1.degrees() << "," << lon1.degrees() << "\n";
      results += PvlKeyword("Lat1", Isis::toString(lat1.degrees()), "degrees");
      results += PvlKeyword("Lon1", Isis::toString(lon1.degrees()), "degrees");
      if ( !v_cam->SetUniversalGround(lat1.degrees(), lon1.degrees()) ) {
        std::string mess = "Geometry coordinate does not map into image at location (" +
                       Isis::toString(lat1.degrees()) + "," + Isis::toString(lon1.degrees()) + ")";
        throw IException(IException::User, mess, _FILEINFO_);
      }

      // Get the surface coordinate in body fixed
      // cout << "Sample, Line = " << v_cam->Sample() << "," << v_cam->Line() << "\n";
      results += PvlKeyword("Lat1Lon1Sample", Isis::toString(v_cam->Sample()));
      results += PvlKeyword("Lat1Lon1Line", Isis::toString(v_cam->Line()));
      double pt2[3];
      v_cam->Coordinate(pt2);


      // Retrieve the current geometry of a point to use as reference for the update
      results += PvlKeyword("Samp1", Isis::toString(samp1));
      results += PvlKeyword("Line1", Isis::toString(line1));
      if ( !v_cam->SetImage(samp1, line1) ) {
        // Ignore the SetImage() error as long as the coordinate is a valid
        // image coordinate.
        if (!v_cam->InCube() ) {
          std::string mess = "Image coordinate is outside image coordinates at point (" +
                         Isis::toString(samp1) + "," + Isis::toString(line1) + ")";
          throw IException(IException::User, mess, _FILEINFO_);
        }

        // At this point, we only need the look direction which is always
        // set at this stage - just don't have surface geometry.
        PvlKeyword offbody = PvlKeyword("Samp1Line1Lat");
        offbody.addComment("Does not intersect surface - can still adjust pointing!");
        results += offbody;
        results += PvlKeyword("Samp1Line1Lon");
      }
      else {
        results += PvlKeyword("Samp1Line1Lat",  Isis::toString(v_cam->GetLatitude().degrees()), "degrees");
        results += PvlKeyword("Samp1Line1Lon", Isis::toString(v_cam->GetLongitude().degrees()), "degrees");
      }

      // Get vector to surface from S/C and S/C position in body-fixed.
      // This works even if the line/samp does not intersect the body
      // because the camera->SpacecraftSurfaceVector() returns the
      // look direction which is not dependent upon success for this
      // case! Means it works for off body corrections!
      vector <double> scpt1(3);
      double scpos1[3];
      v_cam->SpacecraftSurfaceVector(&scpt1[0]);
      v_cam->instrumentBodyFixedPosition(scpos1);

      // Compute vector from S/C position 1 to surface point 2
      vector<double> scpt2(3);
      vsub_c( pt2, scpos1, &scpt2[0]);

      vector<double> ldir1, ldir2;
      ldir1 = v_cam->bodyRotation()->J2000Vector(scpt1);
      ldir2 = v_cam->bodyRotation()->J2000Vector(scpt2);


      // Compute angle difference of update
      double j2kAngle = vsep_c(&ldir1[0], &ldir2[0]);
      results += PvlKeyword("AdjustedAngle", Isis::toString(j2kAngle * dpr_c()), "degrees");

      // Compute rotation of vectors
      SpiceDouble R[3][3];
      Vector2VectorRotation(&ldir1[0], &ldir2[0], R);

      // Ok, now retrieve the pointing table (quaternions) and apply the offset
      Table o_cmat = v_cam->instrumentRotation()->Cache("InstrumentPointing");

      // Determine type of pointing table we are dealing with here.
      int nfields = o_cmat[0].Fields();

      // Four or more fields indicates we have quaterions stored in the table
      if ( nfields > 3 ) {
        ApplyRotation(R, o_cmat);
      }
      // We have three fields which indicates euler angle polynimials. We must
      // handle this differently.
      else {

        // We only know how to handle a cache with just four records. Anything
        // else and we have to abort...
        if ( o_cmat.Records() != 4 ) {
          std::string mess = "Expect only 4 records for polynomial cache but got "
                         + Isis::toString(o_cmat.Records()) + " instead!";
          throw IException(IException::User, mess, _FILEINFO_);
        }

        // Get the line cache and apply rotation using that cache. Then refit
        // to polynomials
        Table lcache = v_cam->instrumentRotation()->LineCache(QString::fromStdString(o_cmat.Name()));
        ApplyRotation(R, lcache);
        v_cam->instrumentRotation()->LoadCache(lcache);
        v_cam->instrumentRotation()->SetPolynomial();
        o_cmat = v_cam->instrumentRotation()->Cache("InstrumentPointing");
      }

      // Write out a description in the spice table
      results += PvlKeyword("RecordsUpdated", Isis::toString(o_cmat.Records()));
      QString deltackComment = "deltackDirectAdjusted = " + Isis::iTime::CurrentLocalTime();
      o_cmat.Label().addComment(deltackComment.toStdString());

      // Write out the updated pointing dataset
      c.write(o_cmat);
    }
    else { // ( "bundle" == method )

      Distance rad1;
      if (ui.WasEntered("RAD1")) {
        rad1 = Distance(ui.GetDouble("RAD1"), Distance::Meters);
      }
      else {
        rad1 = GetRadius(ui.GetCubeName("FROM"), lat1, lon1);
      }

      // In order to use the bundle adjustment class we will need a control
      // network
      ControlMeasure * m = new ControlMeasure;
      m->SetCubeSerialNumber(serialNumberList.serialNumber(0));
      m->SetCoordinate(samp1, line1);

      //   m->SetType(ControlMeasure::Manual);
      m->SetType(ControlMeasure::RegisteredPixel);

      ControlPoint * p = new ControlPoint;
      p->SetAprioriSurfacePoint(SurfacePoint(lat1, lon1, rad1));
      p->SetId("Point1");
      p->SetType(ControlPoint::Fixed);
      p->Add(m);

      ControlNet cnet;
      //  cnet.SetType(ControlNet::ImageToGround);
      cnet.AddPoint(p);

      // We need the target body
      cnet.SetTarget(*c.label());

      // See if they wanted to solve for twist
      if (ui.GetBoolean("TWIST")) {
        double samp2 = ui.GetDouble("SAMP2");
        double line2 = ui.GetDouble("LINE2");
        Latitude lat2(ui.GetDouble("LAT2"), Angle::Degrees);
        Longitude lon2(ui.GetDouble("LON2"), Angle::Degrees);
        Distance rad2;
        if (ui.WasEntered("RAD2")) {
          rad2 = Distance(ui.GetDouble("RAD2"), Distance::Meters);
        }
        else {
          rad2 = GetRadius(ui.GetCubeName("FROM"), lat2, lon2);
        }

        ControlMeasure * m = new ControlMeasure;
        m->SetCubeSerialNumber(serialNumberList.serialNumber(0));
        m->SetCoordinate(samp2, line2);
        m->SetType(ControlMeasure::Manual);

        ControlPoint * p = new ControlPoint;
        p->SetAprioriSurfacePoint(SurfacePoint(lat2, lon2, rad2));
        p->SetId("Point2");
        p->SetType(ControlPoint::Fixed);
        p->Add(m);

        cnet.AddPoint(p);
      }

      // Bundle adjust to solve for new pointing
      BundleSettingsQsp settings = bundleSettings();
      BundleAdjust *bundleAdjust = new BundleAdjust(settings, cnet, serialNumberList);
      // bundleAdjust->savePreAdjustedCacheTimes();

      QObject::connect( bundleAdjust, SIGNAL( statusUpdate(QString) ),
                        bundleAdjust, SLOT( outputBundleStatus(QString) ) );

      BundleSolutionInfo *bundleSolution = bundleAdjust->solveCholeskyBR();


      // Output bundle adjust files
      bundleSolution->outputText();
      bundleSolution->outputResiduals();

      Table cmatrix = bundleAdjust->cMatrix(0);


      // Write out a description in the spice table
      QString deltackComment = "deltackAdjusted = " + Isis::iTime::CurrentLocalTime();
      cmatrix.Label().addComment(deltackComment.toStdString());
      //PvlKeyword description("Description");
      //description = "Camera pointing updated via deltack application";
      //cmatrix.Label().findObject("Table",Pvl::Traverse).addKeyword(description);

      c.write(cmatrix);

      delete bundleAdjust;
      delete bundleSolution;
    }

    // Now do final clean up as the update was successful if we reach here...

    // Check for existing polygon, if exists delete it
    if (c.label()->hasObject("Polygon")) {
      c.label()->deleteObject("Polygon");
    }

    // Update status
    results += PvlKeyword("Status", "Camera pointing updated");

    // Update history entry
    PvlObject hEntry =  Isis::iApp->History();
    hEntry.addGroup(results);
    hist.AddEntry(hEntry);
    c.write(hist);

    // clean up
    c.close();

    // Report the results group
    Application::Log(results);
  }
  catch (IException &e) {
    std::string msg = "Unable to update camera pointing for [" + filename.toStdString() + "]";
    throw IException(e, IException::Unknown, msg, _FILEINFO_);
  }

  // All done!
  return;

}

// Compute the radius at the lat/lon
Distance GetRadius(QString filename, Latitude lat, Longitude lon) {
  Cube cube(filename.toStdString(), "r");
  Camera *sensor = CameraFactory::Create(cube);
  sensor->SetGround(SurfacePoint(lat, lon, sensor->LocalRadius(lat, lon)));
  Distance radius = sensor->LocalRadius();
  if (!radius.isValid()) {
    std::string msg = "Could not determine radius from DEM at lat/lon [";
    msg += Isis::toString(lat.degrees()) + "," + Isis::toString(lon.degrees()) + "]";
    throw IException(IException::Unknown, msg, _FILEINFO_);
  }
  return radius;
}



BundleSettingsQsp bundleSettings() {
  UserInterface  &ui = Application::GetUserInterface();
  BundleSettingsQsp settings = BundleSettingsQsp(new BundleSettings);
  // =========================================================================================//
  // ============= Use the bundle settings to initialize member variables ====================//
  // =========================================================================================//
  settings->setValidateNetwork(false);
  //  set the following:
  //     solve observation mode = false
  //     update cube label      = false
  //     error propagation      = false
  //     solve radius           = false
  //     latitude sigma         = 1000.0
  //     longitude sigma        = 1000.0
  //     radius sigma           = Null since we are not solving for radius
  //     outlier rejection      = false
  settings->setSolveOptions(false, false, false, false, SurfacePoint::Latitudinal,
                            SurfacePoint::Latitudinal, 1000.0, 1000.0, Isis::Null);
  settings->setOutlierRejection(false);

  // =========================================================================================//
  // For deltack, we only have one observation solve settings, for now........................//
  // =========================================================================================//
  QList<BundleObservationSolveSettings> observationSolveSettingsList;
  BundleObservationSolveSettings observationSolveSettings;

  // use defaults
  //       pointing option sigmas -1.0
  //       ckDegree = ckSolveDegree = 2
  //       fitOverExisting = false
  //       angle sigma = angular velocity sigma = angular acceleration sigma = -1.0
  observationSolveSettings.setInstrumentPointingSettings(
      BundleObservationSolveSettings::AnglesOnly, ui.GetBoolean("TWIST"));

  // NOTE: no need to set position sigmas or solve degrees since we are not solving for any
  // position factors
  //       position option sigmas default to -1.0
  //       spkDegree = spkSolveDegree = 2
  //       solveOverHermiteSpline = false
  //       position sigma = velocity sigma = acceleration sigma = -1.0
  observationSolveSettings.setInstrumentPositionSettings(
      BundleObservationSolveSettings::NoPositionFactors);

  observationSolveSettingsList.append(observationSolveSettings);
  settings->setObservationSolveOptions(observationSolveSettingsList);
  // ===========================================================================================//
  // =============== End Bundle Observation Solve Settings =====================================//
  // ===========================================================================================//

  settings->setConvergenceCriteria(BundleSettings::ParameterCorrections,
                                  ui.GetDouble("SIGMA0"),
                                  ui.GetInteger("MAXITS"));

  settings->setOutputFilePrefix("");


  //************************************************************************************************

  return settings;
}

/**
 * @brief Compute rotation matrix of one vector into another
 *
 * This function computes the 3x3 rotation matrix of one vector into another
 * using Rodriques' formula. See
 * https://math.stackexchange.com/questions/293116/rotating-one-3d-vector-to-another.
 *
 * The basic equation is:
 *
 *     R = I + sin(theta) * A + (1 - cos(theta)) * A^2
 *
 *  where I is the identity matrix, theta is essentially the separation angle of
 *  the two vectors and A is the skew matrix of the cross product of the two
 *  vectors.  Note that if theta ~= 0, then the identity matrix is returned.
 *
 *  Note this implementation does not handle the case where (pi - theta) ~= 0 as
 *  it doesn't seem possible in this case (choose for x any vector orthogonal to
 *  v1).
 *
 * @author 2017-05-22 Kris Becker
 *
 * @param v1  Vector to rotate into v2
 * @param v2  Desired vector of rotation
 * @param rmat  Returns 3x3 rotation matrix to rotate v1 -> v2
 */
void Vector2VectorRotation(const double v1[3], const double v2[3], double rmat[3][3]) {

  // Compute cross and dot products to get theta and skew matrix
  SpiceDouble x[3];
  vcrss_c(v1, v2, x);

  SpiceDouble xnorm  = vnorm_c(x);
  vscl_c(1.0/xnorm, x, x);

  // Compute theta angle using dot product
  SpiceDouble theta = std::acos( vdot_c(v1, v2) / (vnorm_c(v1) * vnorm_c(v2)) );

  // If there is no separation angle (i.e., vectors are the same), return identity
  if ( qFuzzyCompare( 1.0 + theta, 1.0 + 0.0) ) {
    ident_c ( rmat );
    return;
  }

  // Need identity matrix
  SpiceDouble I[3][3];
  ident_c ( I );

  // Skew-symmetric matrix A corresponding to x
  SpiceDouble A[3][3] = {
                          {   0.0,  -x[2],  x[1] },
                          {  x[2],    0.0, -x[0] },
                          { -x[1],   x[0],   0.0 }
                        };

  // Scale skew matrix by sin(theta)
  SpiceDouble sinTA[3][3];
  vsclg_c(std::sin(theta), A, 9, (SpiceDouble( *)) sinTA[0]);

  // Compute A^2
  SpiceDouble A2[3][3];
  mxm_c(A, A, A2);

  SpiceDouble cosTA2[3][3];
  vsclg_c((1.0 - std::cos(theta)), A2, 9, (SpiceDouble( *)) cosTA2[0]);

  // Compute R
  vaddg_c(I, sinTA, 9, (SpiceDouble( *)) rmat[0]);
  vaddg_c(rmat, cosTA2, 9, (SpiceDouble( *)) rmat[0]);

  // Invert for the proper rotation
  invert_c(rmat, rmat);
  return;
}

/**
 * @brief Apply rotation matrix to each quaterion stored in the pointing table
 *
 * This routine will apply a 3x3 rotation matrix to every record in the table.
 * The table is assumed to be an InstrumentPointing compatible (CK) table
 * containing at least four elements/row. The first four elements are assumed to
 * be quaterions that are converted to a matrix such that simple matrix
 * multiplication is applied to achieve an updated pointing quaternion. The
 * results are stored back into the table.
 *
 * @author 2017-08-01 Kris Becker
 *
 * @param R     The constant angular pointing matrix that will be applied
 * @param table Instrument pointing table containing quaternions
 */
void ApplyRotation(const double R[3][3], Table &table) {

  // Sanity check...
  if ( table[0].Fields() < 4 ) {
    std::string mess = "Expect at least 4 fields for quaternion cache but got "
                   + Isis::toString(table.Records()) + " instead!";
    throw IException(IException::User, mess, _FILEINFO_);
  }

  // Update each record
  for (int tr = 0 ; tr < table.Records() ; tr++) {
    TableRecord &rec = table[tr];

    std::vector<double> j2000Quat;
    j2000Quat.push_back((double)rec[0]);
    j2000Quat.push_back((double)rec[1]);
    j2000Quat.push_back((double)rec[2]);
    j2000Quat.push_back((double)rec[3]);

    // Set up a formal quaterian and get the rotation matrix
    Quaternion q(j2000Quat);
    std::vector<double> CJ = q.ToMatrix();

    // Apply the constant offset
    mxm_c( (SpiceDouble( *)[3]) &CJ[0], R, (SpiceDouble( *)[3]) &CJ[0] );

    // Reassign the updated matrix and covert back to quaternion
    q.Set( CJ );
    j2000Quat = q.GetQuaternion();
    rec[0] = j2000Quat[0];
    rec[1] = j2000Quat[1];
    rec[2] = j2000Quat[2];
    rec[3] = j2000Quat[3];

    table.Update(rec, tr);
  }

  return;
}

/** Fancy printing of matrix...   */
void printMatrix(const SpiceDouble m[3][3]) {
  cout << "RMatrix:\n";
  for (int i = 0 ; i < 3 ; i++) {
    for (int j = 0 ; j < 3 ; j++) {
      cout << m[i][j] << " ";
    }
    cout << "\n";
  }
  return;
}
