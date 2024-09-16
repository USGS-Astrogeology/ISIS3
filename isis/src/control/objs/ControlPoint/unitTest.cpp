/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <iostream>

#include <boost/numeric/ublas/symmetric.hpp>

#include "ControlMeasure.h"
#include "ControlNet.h"
#include "ControlPoint.h"
#include "IException.h"
#include "Latitude.h"
#include "Longitude.h"
#include "Preference.h"
#include "SurfacePoint.h"

using namespace std;
using namespace boost::numeric::ublas;
using namespace Isis;

void printPoint(ControlPoint &p, bool = false);

/**
  * @brief Test ControlPoint object for accuracy and correct behavior.
  *
  * @history 2010-06-30  Tracie Sucharski, Updated for binary control net and
  *                         new keywords.
  * @history 2010-08-12  Tracie Sucharski,  Keywords changed AGAIN.. Added many
  *                         more tests for conversions between lat/lon/radius
  *                         and x/y/z and between simgas and covariance matrices.
  * @history 2011-06-07 Debbie A. Cook and Tracie Sucharski - Modified point types
  *                         Ground ------> Fixed
  *                         Tie----------> Free
  * @history 2015-02-17  Andrew Stebenne, changed a reference to a local filesystem to a dummy file
  *                         (dummy.cub) to make it clearer that the .cub file being referenced
  *                         wasn't necessary.
  * @history 2017-12-21 Kristin Berry - Added tests for newly added accessor methods.
  * @history 2018-01-04 Adam Goins - Replaced QDebug with std::cout. Removed commented out code for
  *                         Removed accessor methods.
 */
int main() {
  Preference::Preferences(true);

  std::cout << "ControlPoint unitTest" << std::endl;

  ControlPoint cp("C151");

  cp.SetType(ControlPoint::Fixed);
  cp.SetIgnored(true);
  cp.SetAprioriSurfacePointSource(ControlPoint::SurfacePointSource::Basemap);
  // dummy.cub does not exist: reference provided for testing.
  cp.SetAprioriSurfacePointSourceFile("./dummy.cub");
  cp.SetAprioriRadiusSource(ControlPoint::RadiusSource::DEM);
  cp.SetAprioriRadiusSourceFile("$base/dems/molaMarsPlanetaryRadius0003.cub");

  SurfacePoint point(Displacement(-424.024048, Displacement::Meters),
      Displacement(734.4311949, Displacement::Meters),
      Displacement(529.919264, Displacement::Meters),
      Distance(10, Distance::Meters),
      Distance(50, Distance::Meters),
      Distance(20, Distance::Meters));
  cp.SetAdjustedSurfacePoint(point);
  cp.SetAprioriSurfacePoint(point);
  cp.SetEditLock(true);

  ControlMeasure *cm1 = new ControlMeasure;
  cm1->SetCubeSerialNumber("Test1");
  cm1->SetIgnored(true);
  cm1->SetCoordinate(1.0, 2.0);
  cm1->SetResidual(-3.0, 4.0);
  cm1->SetDiameter(15.0);
  cm1->SetAprioriSample(2.0);
  cm1->SetAprioriLine(5.0);
  cm1->SetSampleSigma(.01);
  cm1->SetLineSigma(.21);
  cm1->SetChooserName("seedgrid");
  cm1->SetDateTime("2005-05-03T00:00:00");

  std::cout << "Adding ControlMeasure with cube serial number [" << cm1->GetCubeSerialNumber().toStdString() << "]" << std::endl; // Cube Serial Number "Test1"
  cp.Add(cm1);

  printPoint(cp);

  ControlMeasure *cm2 = new ControlMeasure;
  cm2->SetCubeSerialNumber("Test2");
  cm2->SetIgnored(true);
  cm2->SetCoordinate(100.0, 200.0);
  cm2->SetDiameter(15.0);
  cm2->SetAprioriSample(2.0);
  cm2->SetAprioriLine(5.0);
  cm2->SetSampleSigma(.01);
  cm2->SetLineSigma(.21);
//  cm2->SetType(Isis::ControlMeasure::Reference);
  cm2->SetResidual(-2.0, 2.0);
  cm2->SetChooserName("seedgrid");
  cm2->SetDateTime("2005-05-03T00:00:00");
  std::cout << "Adding ControlMeasure with cube serial number [" << cm2->GetCubeSerialNumber().toStdString() << "]" << std::endl; // Cube Serial Number "Test2"
  cp.Add(cm2);
  std::cout << "Testing Edit Locking... ";
  cp.SetRefMeasure(cm2);
  if (cp.GetRefMeasure() != cm2) {
    cp.SetEditLock(false);
    cp.SetRefMeasure(cm2);
    if (cp.GetRefMeasure() == cm2) {
      std::cout << "ok!" << std::endl;
    }
    else {
      std::cout << "Failed!" << std::endl;
    }
  }
  else {
    std::cout << "Failed!" << std::endl;
  }
  std::cout << std::endl;
  cp.SetEditLock(false);
  cp.SetRefMeasure(cm2);
  cp.SetEditLock(true);

  printPoint(cp);

  std::cout << "Testing copy constructor..." << std::endl;
  ControlPoint copy(cp);
  std::cout << "\t Also testing == operator" << std::endl;
  bool equal = (cp == cp);
  std::cout << "\t\t original == its self (yes)? " << equal << std::endl;
  copy.SetEditLock(false);
  copy.SetRefMeasure(0);
  equal = (cp == copy);
  std::cout << "\t\t original == copy (no)? " << equal << std::endl;
  printPoint(cp);
  std::cout << "Testing assignment operator..." << std::endl;
  ControlPoint assignment = copy;
  printPoint(assignment);

  // Should be successful
  std::cout << "Deleting ControlMeasure with cube serial number [" << cp.getCubeSerialNumbers().at(0).toLatin1().data() << "]" << std::endl;
  std::cout << "Measure type: " << ControlMeasure::MeasureTypeToString(cp.GetMeasure(0)->GetType()).toStdString() << std::endl;
  cp.Delete(0);
  printPoint(cp);
//  std::cout << "ReferenceIndex = " << cp.GetReferenceIndex();

//  std::cout << "ReferenceIndex = " << cp.GetReferenceIndex();

  std::cout << std::endl;
  std::cout << "Test adding control measures with identical serial numbers ..." << std::endl;
  try {
    cp.Add(cm2);
  }
  catch (IException &e) {
    e.print();
  }

  std::cout << std::endl;
  std::cout << "Test SetAdjustedSurfacePoint ... " << std::endl;
  SurfacePoint surfPt(cp.GetAdjustedSurfacePoint());
  std::cout << "X = " << surfPt.GetX().meters() << std::endl;
  std::cout << "Y = " << surfPt.GetY().meters() << std::endl;
  std::cout << "Z = " << surfPt.GetZ().meters() << std::endl;
  std::cout << "Latitude = " << surfPt.GetLatitude().degrees() << std::endl;
  std::cout << "Longitude = " << surfPt.GetLongitude().degrees() << std::endl;
  std::cout << "Radius = " << surfPt.GetLocalRadius().meters() << std::endl;
  surfPt.SetSpherical(Latitude(32, Angle::Degrees),
      Longitude(120, Angle::Degrees),
      Distance(1000, Distance::Meters));
  cp.SetAdjustedSurfacePoint(surfPt);
  surfPt = cp.GetAdjustedSurfacePoint();
  std::cout << "X = " << surfPt.GetX().meters() << std::endl;
  std::cout << "Y = " << surfPt.GetY().meters() << std::endl;
  std::cout << "Z = " << surfPt.GetZ().meters() << std::endl;
  std::cout << "Latitude = " << surfPt.GetLatitude().degrees() << std::endl;
  std::cout << "Longitude = " << surfPt.GetLongitude().degrees() << std::endl;
  std::cout << "Radius = " << surfPt.GetLocalRadius().meters() << std::endl;

  std::cout << std::endl;
  std::cout << "Test conversions for apriori/adjusted covariance matrices ... " << std::endl;

  symmetric_matrix<double, upper> covar;
  covar.resize(3);
  covar.clear();
  covar(0, 0) = 100.;
  covar(0, 1) = 3.55271368e-15;
  covar(0, 2) = -1.81188398e-13;
  covar(1, 0) = 3.55271368e-15;
  covar(1, 1) = 2500.;
  covar(1, 2) = 3.41060513e-13;
  covar(2, 0) = -1.81188398e-13;
  covar(2, 1) = 3.41060513e-13;
  covar(2, 2) = 400.;

  point.SetRectangularMatrix(covar);
  cp.SetAprioriSurfacePoint(point);

  //c.SetAprioriCovariance();
  point = cp.GetAprioriSurfacePoint();
  std::cout << "Apriori Sigma X = " << point.GetXSigma().meters() << std::endl;
  std::cout << "Apriori Sigma Y = " << point.GetYSigma().meters() << std::endl;
  std::cout << "Apriori Sigma Z = " << point.GetZSigma().meters() << std::endl;

  point = cp.GetAdjustedSurfacePoint();
  std::cout << "Adjusted Sigma X = " << point.GetXSigma().meters() << std::endl;
  std::cout << "Adjusted Sigma Y = " << point.GetYSigma().meters() << std::endl;
  std::cout << "Adjusted Sigma Z = " << point.GetZSigma().meters() << std::endl;

  std::cout << std::endl;

  std::cout << "Testing IsReferenceExplicit..." << std::endl;
  std::cout << "cp:                    " << cp.IsReferenceExplicit() << std::endl;

  ControlMeasure *cm3 = new ControlMeasure;
  cm3->SetCubeSerialNumber("Test1");
  cm3->SetIgnored(true);
  cm3->SetCoordinate(1.0, 2.0);
  cm3->SetResidual(-3.0, 4.0);
  cm3->SetDiameter(15.0);
  cm3->SetAprioriSample(2.0);
  cm3->SetAprioriLine(5.0);
  cm3->SetSampleSigma(.01);
  cm3->SetLineSigma(.21);
  cm3->SetChooserName("seedgrid");
  cm3->SetDateTime("2005-05-03T00:00:00");

  ControlMeasure *cm4 = new ControlMeasure;
  cm4->SetCubeSerialNumber("Test2");
  cm4->SetIgnored(true);
  cm4->SetCoordinate(1.0, 2.0);
  cm4->SetResidual(-3.0, 4.0);
  cm4->SetDiameter(15.0);
  cm4->SetAprioriSample(2.0);
  cm4->SetAprioriLine(5.0);
  cm4->SetSampleSigma(.01);
  cm4->SetLineSigma(.21);
  cm4->SetChooserName("seedgrid");
  cm4->SetDateTime("2005-05-03T00:00:00");

  ControlPoint newCp;
  std::cout << "newCp:                 " << newCp.IsReferenceExplicit() << std::endl;
  newCp.Add(cm3);
  std::cout << "newCp with implicit:   " << newCp.IsReferenceExplicit() << std::endl;
  newCp.Add(cm4);
  newCp.SetRefMeasure(cm3);
  std::cout << "newCp with explicit:   " << newCp.IsReferenceExplicit() << std::endl;
  newCp.Delete(cm3);
  std::cout << "newCp reverted to implicit:   " << newCp.IsReferenceExplicit() << std::endl;

  std::cout << "\ntesting getMeasures method..." << std::endl;
  ControlMeasure * alpha = new ControlMeasure;
  alpha->SetCubeSerialNumber("alpha");
  ControlMeasure * beta = new ControlMeasure;
  beta->SetCubeSerialNumber("beta");
  ControlPoint getMeasuresTestPoint;
  getMeasuresTestPoint.Add(alpha);
  getMeasuresTestPoint.Add(beta);
  QList< ControlMeasure * > measures = getMeasuresTestPoint.getMeasures();
  foreach (ControlMeasure * measure, measures) {
    std::cout << measure->GetCubeSerialNumber().toStdString() << std::endl;
  }
  beta->SetIgnored(true);
  measures = getMeasuresTestPoint.getMeasures(true);
  foreach (ControlMeasure * measure, measures) {
    std::cout << measure->GetCubeSerialNumber().toStdString() << std::endl;
  }

  std::cout << "\ntesting error handling for StringToPointType..." << std::endl;
  try {
    ControlPoint::StringToPointType("aoeu");
  }
  catch (IException &e) {
    std::cout << "  " << e.toString() << std::endl;
  }
}

void printPoint(Isis::ControlPoint &p, bool testRect) {
  bool wasLocked = p.IsEditLocked();
  p.SetEditLock(false);
  p.SetChooserName("cnetref");
  p.SetDateTime("2005-05-03T00:00:00");
  p.SetEditLock(wasLocked);

  std::cout << std::endl;
  std::cout << "Testing point has ChooserName: ";
  if ( p.HasChooserName() ) {
    std::cout << "TRUE" << std::endl;
  }
  else {
    std::cout << "FALSE" << std::endl;
  }

  std::cout << "Testing point has DateTime:    ";
  if ( p.HasDateTime() ) {
    std::cout << "TRUE" << std::endl;
  }
  else {
    std::cout << "FALSE" << std::endl;
  }
  std::cout << std::endl;
  ControlNet net;

  ControlPoint *copyPoint = new ControlPoint(p);
  wasLocked = copyPoint->IsEditLocked();
  copyPoint->SetEditLock(false);
  for (int i = 0; i < copyPoint->GetNumMeasures(); i++) {
    ControlMeasure *measure = copyPoint->GetMeasure(i);
    bool explicitLock = measure->IsEditLocked();
    measure->SetEditLock(false);
    measure->SetChooserName("seedgrid");
    measure->SetDateTime("2005-05-03T00:00:00");
    measure->SetEditLock(explicitLock);
  }
  copyPoint->SetEditLock(wasLocked);

  net.AddPoint(copyPoint);
  net.SetNetworkId("Identifier");
  net.SetCreatedDate("Yesterday");
  net.SetModifiedDate("Yesterday");
  net.Write("./tmp.net", true);
  Pvl tmp("./tmp.net");
  cout << "Printing point:\n" << tmp << "\nDone printing point." << std::endl;
  std::cout << std::endl;
  remove("./tmp.net");

  // Add test with coordinate type set to Rectangular *** TBD *** Add test once
  //  changes are made to the protocol buffer.
  if (testRect) {
    ControlNet recNet;
    recNet.AddPoint(copyPoint);
    recNet.SetNetworkId("Identifier");
    recNet.SetCreatedDate("Yesterday");
    recNet.SetModifiedDate("Yesterday");
    recNet.Write("./tmpR.net", true);
    Pvl tmpR("./tmpR.net");
    cout << "Printing rectangular net point:\n" << tmp << "\nDone printing point."
         << endl << endl;
    remove("./tmpR.net");
  }
}
