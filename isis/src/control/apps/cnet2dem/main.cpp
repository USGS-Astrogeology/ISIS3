/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

#include <QFile>
#include <QtGlobal>
#include <QTextStream>
#include <QScopedPointer>
#include <QVector>

// boost library
#include <boost/foreach.hpp>

#include "nanoflann.hpp"

#include "ControlNet.h"
#include "ControlPointCloudPt.h"
#include "DatumFunctoid.h"
#include "FileList.h"
#include "FileName.h"
#include "IString.h"
#include "Latitude.h"
#include "Longitude.h"
#include "MapPointCollector.h"
#include "PointCloud.h"
#include "PointCloudTree.h"
#include "ProcessByLine.h"
#include "Progress.h"
#include "ProjectionFactory.h"
#include "Projection.h"
#include "SerialNumber.h"
#include "SpecialPixel.h"
#include "SurfacePoint.h"
#include "IException.h"
#include "TProjection.h"

using namespace std;
using namespace Isis;

// Control network manager
typedef QList<QSharedPointer<ControlNet> >    ControlNetList;

/**
 * Functor for normalized 3D-to-2D Euclidean distances
 *
 * @author 2015-03-14 Kris Becker
 *
 * @internal
 *   @history 2015-03-14 Kris Becker - Original version.
 */
template <class T> class Dist3Dto2D {
public:
  enum { Dimension = 3 };  // Needs all three coordinates
  Dist3Dto2D()  : m_znorm(1.0) { }
  Dist3Dto2D(const double znorm) : m_znorm(znorm) {  }
  ~Dist3Dto2D() { }

  inline int dimension() const {
    return ( Dimension );
  }

  double getZNorm() const {
    return ( m_znorm );
  }

  void setZNorm(const double znorm = 1.0) {
    m_znorm = znorm;
    return;
  }

  inline double operator()(const T &datum1, const T &datum2) const {
    return ( normalize(datum1.x(), datum1.y(), datum1.z(),
                       datum2.x(), datum2.y(), datum2.z()) );
  }

  inline double operator()(const double *datum1, const T &datum2) const {
    return ( normalize(datum1[0], datum1[1], datum1[2],
                      datum2.x(), datum2.y(), datum2.z()) );
  }

  inline double normalize(const double dx1, const double dy1, const double dz1,
                          const double dx2, const double dy2, const double dz2)
                          const {
    double scale = m_znorm / radius(dx2, dy2, dz2);
    double nx = dx2 * scale;
    double ny = dy2 * scale;
    double nz = dz2 * scale;
    return ( distance(dx1, dy1, dz1, nx, ny, nz) );
  }

  inline double distance(const double dx1, const double dy1, const double dz1,
                         const double dx2, const double dy2, const double dz2)
                         const {
    double dx = dx1 - dx2;
    double dy = dy1 - dy2;
    double dz = dz1 - dz2;
    return ( dx*dx + dy*dy + dz*dz );
  }

  inline double radius(const double dx, const double dy, const double dz) const {
    double v1max = qMax(qAbs(dx), qMax(qAbs(dy), qAbs(dz)));

    // We're done if its the zero vector
    if ( qFuzzyCompare(v1max+1.0, 1.0) ) { return ( 0.0 ); }

    // Compute magnitude of the vector
    double tmp0( dx / v1max );
    double tmp1( dy / v1max );
    double tmp2( dz / v1max );
    double normsqr = tmp0*tmp0 + tmp1*tmp1 + tmp2*tmp2;
    return ( v1max * std::sqrt(normsqr) );
  }

  private:
    double m_znorm;   // Normalization radius
};


// ControlNet point cloud types. Dist3d distance works for all types.
typedef ControlPointCloudPt             PointType;
typedef Dist3Dto2D<ControlPointCloudPt> DistanceType;

// Point cloud/kd-tree template definitions
typedef PointCloud<PointType, DistanceType>             CNetPointCloud;
typedef PointCloudTree<PointType, DistanceType>         CNetPointCloudTree;
typedef PointCloudSearchResult<PointType, DistanceType> ResultType;


void IsisMain() {

  // We will be processing by line
  ProcessByLine p;
  UserInterface &ui = Application::GetUserInterface();

  // Now parse the input datum string to determine the bands of the output
  // cube. Note the order of the bands will be as the user has specified them.
  QString algorithm = ui.GetString("ALGORITHM");
  DatumFunctoidFactory *dfactory = DatumFunctoidFactory::getInstance();

//  QStringList functions = dfactory->algorithms();
//  std::cout << "\nList of Algorithms\n" << functions.join("\n") << "\n";

  DatumFunctoidList functors = dfactory->create(algorithm);

  QStringList cnetfiles;
  if ( ui.WasEntered("CNET") ) {
    cnetfiles.append(ui.GetAsString("CNET"));
  }

  if ( ui.WasEntered("CNETLIST") ) {
    FileList list_o_nets(ui.GetFileName("CNETLIST").toStdString());
    BOOST_FOREACH ( FileName cfile, list_o_nets ) {
      cnetfiles.append(QString::fromStdString(cfile.original()));
    }
  }

  // Ok, if we end up with one net, it must have been entered in CNETBASE.
  if ( (cnetfiles.size() < 1) ) {
    std::string mess = "Must enter a control net inc CNET or a list in CNETLIST";
    throw IException(IException::User, mess, _FILEINFO_);
  }
  // Create the point cloud container and load the control networks
  QScopedPointer<CNetPointCloud> cloud(new CNetPointCloud());

  // Collect some stuff from input nets for the output net
  QString netid;
  QString target;
  QString description;
  BigInt allPoints(0);
  BOOST_FOREACH ( QString cfile, cnetfiles ) {
    std::cout << "\nLoading " << cfile.toStdString() << "...\n";
    Progress c_progress;
    QScopedPointer<ControlNet> cnet( new ControlNet(cfile, &c_progress) );
    if ( netid.isEmpty() )       { netid = cnet->GetNetworkId(); }
    if ( target.isEmpty() )      { target = cnet->GetTarget(); }
    if ( description.isEmpty() ) { description = cnet->Description(); }

    // Get all control points by taking ownership from the control net
    int npoints(0);
    QList<ControlPoint *> points = cnet->take();
    BOOST_FOREACH ( ControlPoint *point, points) {
      ControlPointCloudPt cpt(point, ControlPointCloudPt::Ground,
                              ControlPointCloudPt::Exclusive);
      if ( cpt.isValid() ) {
        cloud->addPoint(cpt);
        npoints++;
      }
    }
    std::cout << "Added " << npoints << " of " << points.size() << "\n";
    allPoints += points.size();

    // Instead of having to save the ControlNet instances, we take ownership
    // of all the points from ControlNet and turn it over to the cloud...
    //
    //  cnetlist.append( QSharedPointer<ControlNet> ( cnet.take() ) );
  }
  std::cout << "\nTotal " << cloud->size() << " of " << allPoints << "\n";

  std::cout << "\nCreating output DEM to determine 3-D normalization...\n";
  //Get the map projection file provided by the user
  Pvl userMap;
  userMap.read(ui.GetFileName("MAP").toStdString());
  // PvlGroup &mapGrp = userMap.FindGroup("Mapping", Pvl::Traverse);

  int csamps, clines;
  Projection *proj = ProjectionFactory::CreateForCube(userMap, csamps, clines, true);
  PvlGroup cleanMap = proj->Mapping();
  Cube *ocube = p.SetOutputCube( "TO", csamps, clines, functors.size() );

  ocube->putGroup(cleanMap);
  delete proj;
  TProjection *tproj = (TProjection *) ocube->projection();
  double znorm = tproj->LocalRadius(tproj->TrueScaleLatitude());
  std::cout << "3D Normalization: " << znorm << "\n";

  std::cout << "\nCreating cloud kd-tree..\n";
  int kd_nodes = ui.GetInteger("KDNODES");
  cloud->getDistance().setZNorm(znorm);
  CNetPointCloudTree cloud_t( cloud.take(), kd_nodes );
  std::cout << "Done...\n";

  //  Set trimming option
  bool trim = ui.GetBoolean("TRIM");

  //  Set up efficient test variables
  QString search_type = ui.GetString("SEARCH").toLower();
  bool both_searches =   ("both" == search_type);
  bool radial_search =   ( both_searches || ( "radial" == search_type) );
 // bool neighbor_search = ( both_searches || ( "neighbors" == search_type ) );

  double search_radius(Null);
  double search_radius_sq(Null);

  // Only worry about this if a range search is requested
  if ( radial_search) {
    if (ui.WasEntered("DISTANCE")) {
      search_radius = ui.GetDouble("DISTANCE");
    }
    else {
      // Compute range from center of pixel to corner in meters
      double res = tproj->Resolution();
      double half_res_sq  = (res / 2.0) * (res / 2.0);
      search_radius = sqrt(half_res_sq + half_res_sq);
      cout << "Search RANGE computed from Map Resolution: " << search_radius
           << " <meters>\n";
    }
    search_radius_sq = search_radius * search_radius;
  }

  // Determine search criteria
  int neighbors = ui.GetInteger("NEIGHBORS");
  int minpoints = ui.GetInteger("MINPOINTS");

  // Now determine if radius noise filtering is requested
  bool do_radius_filter = ui.WasEntered("SIGMARADIUS");
  double sigma = 999.0;
  if ( do_radius_filter ) {
    sigma = ui.GetDouble("SIGMARADIUS");
  }
  // Get the real tile sizes and allocate the buffer accordingly
  PvlObject &icube = ocube->label()->findObject("IsisCube");
  PvlObject &core = icube.findObject("Core");
  int tsamps = core["TileSamples"];
  int tlines = core["TileLines"];
  Brick tile(*ocube, tsamps, tlines, functors.size() );
  int bOffset = tsamps * tlines;  // Index offset to next band (spectrum) value

  Progress mapper;
  mapper.SetText("mapping");
  mapper.SetMaximumSteps(tile.Bricks());
  mapper.CheckStatus();

 //  Process data using 3-D brick
  int npixels = tsamps * tlines;
  int nbands = tile.BandDimension();
  SurfacePoint point;

  for ( int brick = 1 ; brick <= tile.Bricks() ; brick++ ) {
    tile.SetBrick(brick);

    // Tile processing...
    for ( int index = 0 ; index < npixels ; index++ ) {

      int samp, line, band;
      tile.Position(index, samp, line, band);

      // Intialize output spectrum to NULLs
      QVector<double> datum(nbands, Null);

      //  Map only valid projection translation
      if ( (samp <= csamps) && (line <= clines) &&
           ( tproj->SetWorld(samp, line) ) ) {
#if 0
        std::cout << "Line: " << line << "  Sample: " << samp << "\n";
        std::cout << "Lat:  " << tproj->UniversalLatitude()
                  << " Long: " << tproj->UniversalLongitude() << "\n";
#endif

        // Trim if requested
        bool mapit(true);
        if ( ( trim ) && ( tproj->HasGroundRange() ) ) {
          if ( tproj->Latitude()  < tproj->MinimumLatitude()  ) mapit = false;
          if ( tproj->Latitude()  > tproj->MaximumLatitude()  ) mapit = false;
          if ( tproj->Longitude() < tproj->MinimumLongitude() ) mapit = false;
          if ( tproj->Longitude() > tproj->MaximumLongitude() ) mapit = false;
        }

        // Plot it only if its within mapping boundary conditions
        if ( mapit ) {
          double lat = tproj->UniversalLatitude();
          double lon = tproj->UniversalLongitude();
          double radius = tproj->LocalRadius(lat);

          point.SetSphericalCoordinates(Latitude(lat, Angle::Degrees),
                                        Longitude(lon, Angle::Degrees),
                                        Distance(radius, Distance::Meters));

          // Search the PC cloud
          ControlPoint pt;
          pt.SetAprioriSurfacePoint(point);
          ControlPointCloudPt cpt(&pt, ControlPointCloudPt::Ground,
                                  ControlPointCloudPt::Shared,
                                  "MapPoint");

          // There are several combinations to consider
          //    1) RADIAL search from RANGE <meters> at the lat/lon pixel center
          //    2) NEIGHBOR search selecting the NEIGHBORS closest to the center
          //    3) BOTH searches requested will apply the RADIAL search first, then
          //        and only if MINPOINTS points resulting from the RADIAL
          //        search are within RANGE <meters>, otherwise a NEIGHBOR
          //        search is performed.
          ResultType results;
          if ( both_searches ) {
            results = cloud_t.radius_query(cpt, search_radius_sq);
            if ( minpoints > results.size() ) {
              results = cloud_t.neighbor_query(cpt, neighbors);
            }
          }
          else if ( radial_search ) {
            results = cloud_t.radius_query(cpt, search_radius_sq);
          }
          else {  // ( neighbor_search == search_type)
            results = cloud_t.neighbor_query(cpt, neighbors);
          }

          // Extract points and prepare for processing
          MapPointCollector mpoint;
          if ( ResultType::Radius == results.type() ) mpoint.setSearchType(MapPointCollector::Radius);
          else                                        mpoint.setSearchType(MapPointCollector::NearestNeighbor);

          // Extract point set and optionally apply noise filter
          results.forEachPair(mpoint);
          if ( do_radius_filter ) {  mpoint.removeNoise(sigma); }

          // Compute values for each functor
          for ( int i = 0 ; i < functors.size() ; i++) {
            datum[i] = functors[i]->value(mpoint);
          }
        }
      }

      // Copy data values to output data brick
      int ndx = index;
      for (int v = 0 ; v < datum.size() ; v++) {
        tile[ndx] = datum[v];
        ndx += bOffset;
      }
    }

    ocube->write(tile);
    mapper.CheckStatus();
  }

  PvlKeyword fname("Name");
  PvlKeyword cnumber("Number");
  for (int i = 0 ; i < functors.size() ; i++) {
    fname.addValue(functors[i]->name().toStdString());
    cnumber.addValue(Isis::toString(i+1));
  }

  // Create the BandBin group
  PvlGroup bbin("BandBin");
  bbin += fname;
  bbin += cnumber;

  // Dup the Number for Center and Width
  cnumber.setName("Center");
  bbin += cnumber;
  cnumber.setName("Width");
  bbin += cnumber;

  // Write BandBin
  icube.addGroup(bbin);

  p.EndProcess();
}
