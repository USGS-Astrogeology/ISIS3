#include "LidarData.h"

#include <QDebug>
#include <QString>

#include "Angle.h"
#include "ControlMeasure.h"
#include "Distance.h"
#include "FileName.h"
#include "iTime.h"
#include "Latitude.h"
#include "LidarControlPoint.h"
#include "Longitude.h"
#include "Preference.h"
#include "SurfacePoint.h"

#include "boost/numeric/ublas/symmetric.hpp"

using namespace std;
using namespace Isis;
using namespace boost::numeric::ublas;

void print(const LidarData &lidarData);

/**
 * Unit test for the LIDARData class.
 *
 * @internal
 *   @history 2018-01-29 Ian Humphrey - original version.
 *   @history 2018-01-31 Ian Humphrey - added tests for points() and insert().
 *   @history 2018-03-28 Debbie A Cook - added apriori surface point and 
 *                               adjusted surface point to the LidarData test.  Also
 *                               added a sort of the control points before outputting
 *                               them to compare to make sure they are in a 
 *                               consistent order.
 */
int main(int argc, char *argv[]) {
  // Set up our unit test preferences
  Preference::Preferences(true);

  // LidarData data;
  // FileName csvFile("RDR_98E100E_60N62NPointPerRow_csv_table-original.csv");
  // data.readCsv(csvFile);

  // Test LidarData()
  cout << "Testing default constructor... " << endl;
  LidarData defaultData;
  cout << "\tnumber of points: " << defaultData.points().size() << endl;
  cout << endl;

  // // Test LidarData(FileName)
  // cout << "Testing LidarData(FileName)... " << endl;
  // cout << endl;

  // Test insert(QSharedPointer<LidarControlPoint>)
  cout << "Testing insert(QSharedPointer<LidarControlPoint>)... " << endl;
  iTime time("2018-01-31T14:05:00.1234");
  double range = 55.0;
  double sigmaRange = 0.1;
  QSharedPointer<LidarControlPoint> lcp =
      QSharedPointer<LidarControlPoint>(new LidarControlPoint());
  lcp->setTime(time);
  iTime check = lcp->time();
  lcp->setRange(range);
  lcp->setSigmaRange(sigmaRange);
  lcp->SetId("testLidarControlPoint");
  defaultData.insert(lcp);
  cout << "\tnumber of points: " << defaultData.points().size() << endl;
  cout << "\tname of point:    " << defaultData.points().first()->GetId() << endl;
  cout << "\ttime of point:    " << defaultData.points().first()->time().UTC() << endl;
  cout << endl;

  // Test write() JSON format
  cout << "Testing write(FileName)... " << endl;
  LidarData mockData;
  double lat, lon, rad;
  lat = 50.0;
  lon = 100.0;
  rad = 1000.0;
  double latAd = 50.2;
  double lonAd = 100.1;
  double radAd = 1001.0;
  boost::numeric::ublas::symmetric_matrix<double, upper> aprioriMatrix(3);
  aprioriMatrix.clear();
  aprioriMatrix(0, 0) = .01;
  aprioriMatrix(1, 1) = 0.0121;
  aprioriMatrix(2, 2) = 100.;
  boost::numeric::ublas::symmetric_matrix<double, upper> adjustedMatrix(3);
  adjustedMatrix.clear();
  adjustedMatrix(0, 0) = .02;
  adjustedMatrix(1, 1) = 0.0144;
  adjustedMatrix(2, 2) = 81.;
  
  for (int i = 1; i < 11; i++) {
    time += 60.0;
    range += 10.0;
    lcp = QSharedPointer<LidarControlPoint>(new LidarControlPoint());
    lcp->setTime(time);
    lcp->setRange(range);
    lcp->setSigmaRange(sigmaRange);
    lcp->SetId("testLidarControlPoint" + QString::number(i));
    lat += 1.0;
    lon += 1.0;
    SurfacePoint sp(Latitude(lat, Angle::Units::Degrees),
                    Longitude(lon, Angle::Units::Degrees),
                    Distance(rad, Distance::Units::Kilometers));
    latAd += 1.0;
    lonAd += 1.0;
    SurfacePoint sp2(Latitude(latAd, Angle::Units::Degrees),
                    Longitude(lonAd, Angle::Units::Degrees),
                    Distance(radAd, Distance::Units::Kilometers));
    sp.SetSphericalMatrix(aprioriMatrix);
    lcp->SetAprioriSurfacePoint(sp);
    sp2.SetSphericalMatrix(adjustedMatrix);
    lcp->SetAdjustedSurfacePoint(sp2);
    for (int j = 0; j < 2; j++) {
      ControlMeasure *measure = new ControlMeasure();
      measure->SetCoordinate((double) i, (double) j);
      measure->SetCubeSerialNumber("SN_" + QString::number(i) + "-" + QString::number(j));
      lcp->Add(measure);
    }
    mockData.insert(lcp);
  }
  FileName outputFile("./test.json");
  cout << outputFile.extension() << endl;
  mockData.write(outputFile, LidarData::Json);
  cout << endl;

  // Test write() with no data

  // Test read() with no data

  // Test write() binary format
  cout << outputFile.extension() << endl;
  outputFile = outputFile.setExtension("dat");
  cout << outputFile.extension() << endl;
  cout << outputFile.expanded() << endl;
  mockData.write(outputFile, LidarData::Binary);

  // Test read() binary format
  cout << "Testing read(FileName) from binary data... " << endl;
  LidarData fromBinary;
  fromBinary.read(outputFile);
  print(fromBinary);
  cout << endl;

  // Test read()
  cout << "Testing read(FileName) from JSON data... " << endl;
  LidarData fromJson;
  outputFile = outputFile.setExtension("json");
  fromJson.read(outputFile);
  print(fromJson);
  cout << endl;

  // Clean up
  remove("test.dat");
  remove("test.json");

}


// int cmp (QSharedPointer<LidarControlPoint> lcp1, QSharedPointer<LidarControlPoint> lcp2) {
int cmpLessThan (QSharedPointer<LidarControlPoint> lcp1, QSharedPointer<LidarControlPoint> lcp2) {
  // Trim off the id name and only keep the number to compare
  int num1 = ((lcp1->GetId()).remove(0, 21)).toInt();
  int num2 = ((lcp2->GetId()).remove(0, 21)).toInt();
  return num1 < num2;
}


void print(const LidarData &lidarData) {
  QList< QSharedPointer<LidarControlPoint> > points = lidarData.points();
  
  // Order the control points so test runs will list points in a consistent order
  std::sort(points.begin(), points.end(), cmpLessThan);
  
  std::cout << "LidarData:" << std::endl;
  foreach (QSharedPointer<LidarControlPoint> point, points) {
    std::cout << "\tLidarControlPoint:" << std::endl;
    std::cout << "\t\tid: " << point->GetId() << std::endl;;

    // Print the apriori surface point
    SurfacePoint sp = point->GetAprioriSurfacePoint();
    double lat, lon, rad;
    lat = sp.GetLatitude().planetocentric(Angle::Units::Degrees);
    lon = sp.GetLongitude().positiveEast(Angle::Units::Degrees);
    rad = sp.GetLocalRadius().kilometers();
    symmetric_matrix<double, upper> aprioriMatrix = sp.GetSphericalMatrix();
    std::cout << "\t\tlatitude:  " << lat << std::endl;
    std::cout << "\t\tlongitude: " << lon << std::endl;
    std::cout << "\t\tradius:    " << rad << std::endl;
    std::cout << "\t\trange:     " << point->range() << std::endl;
    std::cout << "\t\tsigmaRange:" << point->sigmaRange() << std::endl;
    // std::cout << "\t\ttime:      " << point->time().Et() << std::endl;
    std::cout << "\t\ttime:      " << point->time().UTC() << std::endl;
    std::cout << "\t\tmatrix:      " << aprioriMatrix << std::endl;

    // Print the adjusted surface point
    SurfacePoint sp2 = point->GetAdjustedSurfacePoint();
    double lat2, lon2, rad2;
    lat2 = sp2.GetLatitude().planetocentric(Angle::Units::Degrees);
    lon2 = sp2.GetLongitude().positiveEast(Angle::Units::Degrees);
    rad2 = sp2.GetLocalRadius().kilometers();
    symmetric_matrix<double, upper> adjustedMatrix = sp2.GetSphericalMatrix();
    std::cout << "\t\tadjustedLatitude:  " << lat2 << std::endl;
    std::cout << "\t\tadjustedLongitude: " << lon2 << std::endl;
    std::cout << "\t\tadjustedRadius:    " << rad2 << std::endl;
    std::cout << "\t\tadjustedMatrix:      " << adjustedMatrix << std::endl;
    
    QList<ControlMeasure *> measures = point->getMeasures();
    foreach (ControlMeasure *measure, measures) {
      std::cout << "\t\tControlMeasure: " << std::endl;
      std::cout << "\t\t\tline:   " << measure->GetLine() << std::endl;
      std::cout << "\t\t\tsample: " << measure->GetSample() << std::endl;
      std::cout << "\t\t\tSN:     " << measure->GetCubeSerialNumber() << std::endl;
      std::cout << "\t\t#END_ControlMeasure." << std::endl;
    }
    std::cout << "\t#END_LidarControlPoint." << std::endl << std::endl;
  }
  std::cout << std::endl;
}
