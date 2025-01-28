/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */


#include <ostream>
#include <cfloat>
#include <cmath>

#include <QMap>
#include <QPair>
#include <QRectF>
#include <QSizeF>
#include <QString>
#include <QStringList>

#include <boost/foreach.hpp>

#include "Application.h"
#include "CnetSuppression.h"
#include "CnetManager.h"
#include "IException.h"
#include "Progress.h"

#define EARLY_TERMINATION  true
#define RADIUS_CELLS        100

///#define DEBUG 1

namespace Isis {

  /**
   * Constructs an empty CnetSuppression object.
   *
   */
  CnetSuppression::CnetSuppression() : CnetManager(), m_cnet(), m_points(), m_results(),
                                       m_early_term(EARLY_TERMINATION),
                                       m_area() { }

  /**
   * Constructs a CnetSuppression object using a filename and a weight.
   *
   * @param cnetfile Filename of the controlnet file.
   * @param weight Weight to apply to all points in controlnet.
   *
   * @throws IException::User "Control Net filename [FILENAME] is invalid."
   */
  CnetSuppression::CnetSuppression(const QString &cnetfile, const double &weight) :
                                   CnetManager( ), m_cnet(), m_points(), m_results(),
                                   m_early_term(EARLY_TERMINATION),
                                   m_area() {

    Progress progress;

    try {
      m_cnet.reset(new ControlNet(cnetfile, &progress));
    }
    catch (IException &e) {
      throw e;
    }

    m_points = m_cnet->take();
    load(m_points, weight);
  }


  /**
   * Constructs a CnetSuppression object using a QSharedPointer to a
   * ControlNet object and a weight.
   *
   * @param cnet QSharedPointer to ControlNet object.
   * @param weight Weight to apply to all points in ControlNet.
   */
  CnetSuppression::CnetSuppression(QSharedPointer<ControlNet> &cnet, const double &weight) :
                                   CnetManager( ), m_cnet(), m_points(), m_results(),
                                   m_early_term(EARLY_TERMINATION),
                                   m_area() {

    m_cnet = cnet;

    m_points = m_cnet->take();
    load(m_points, weight);
  }


  /**
   * Constructs a CnetSuppression object using a CnetManager
   *
   * @param cman CnetManager used to construct a CnetSuppression.
   */
  CnetSuppression::CnetSuppression(const CnetManager &cman) :
                                   CnetManager(cman),
                                   m_cnet(), m_points(),
                                   m_results(),
                                   m_early_term(EARLY_TERMINATION),
                                   m_area() { }

  /**
   *
   * Destructor.
   *
   */
  CnetSuppression::~CnetSuppression() { }


  /**
   * Sets the early termination flag
   *
   * @param state Will terminate early if true
   */
  void CnetSuppression::setEarlyTermination(const bool &state) { //TODO this is not used right now at all?
    m_early_term = state;
    return;
  }


  /**
   * Performs a suppression on all cubes associated with the CnetSuppression object and returns the
   * results as a Results object. An input bitmask will be used to mask all pointsets associated with
   * all cubes before running the suppression.
   *
   * @param minpts minimum points to keep in the result set
   * @param maxpts maximum points to keep in the result set.
   * @param min_radius The minimum radius to use for the suppression calculation.
   * @param tolerance A multiplicative tolerance on the number of maxpoints to return.
   * @param bm A BitMask to apply to the input point set.
   *
   * @return @b CnetSuppression::Results The Results set for the suppression run.
   */
  CnetSuppression::Results CnetSuppression::suppress(const int &minpts, const int &maxpts,
                                                     const double &min_radius,
                                                     const double &tolerance,
                                                     const CnetSuppression::BitMask &bm) {

    QMap<QString, int> cubePnts = getCubeMeasureCount();

    QMapIterator<QString, int> cubeset(cubePnts);
    QVector<QPair<QString, int> > pntcount;
    while ( cubeset.hasNext() ) {
      cubeset.next();
      pntcount.append(qMakePair(cubeset.key(), cubeset.value()));
    }

    // Sort to find the image with the highest measure count as a starting point
    std::sort(pntcount.begin(), pntcount.end(), SortSerialsByPntSize());

#if defined(DEBUG)
    for (int i = 0; i < pntcount.size() ; i++) {
      std::cout << pntcount[i].first << "\t" << pntcount[i].second << "\n";
    }
#endif

    // Suppress points in highest to lowest count
    m_results.clear();
    BitMask myBm = bm;
    Results final;
    for ( int p = 0 ; p < pntcount.size() ; p++) {
      QString serialno = pntcount[p].first;

#if defined(DEBUG)
//      int pts = pntcount[p].second;
      std::cout << "\n--> Serial: " << serialno.toStdString() << "\n";
#endif

      Results r = suppress(serialno, minpts, maxpts, min_radius,
                           tolerance, myBm);
      m_results.append(r);

#if defined(DEBUG)
      std::cout << "  Total Saved: " << r.size() << " at cell radius "
                << r.m_radius << "\n";
#endif

      final = merge(r,final);
      myBm = final.m_selected;
    }

    return (final);
  }


  /**
   * Performs a suppression on the PointSet associated with a single cube as indicated by
   * its serial number and returns the results as a new PointSet. An input bitmask will
   * be used to mask the input pointset before running the suppression.
   *
   * @param serialno The serial number for the image/cube to run suppression on.
   * @param minpts The minimum possible points to keep after a suppression run.
   * @param maxpts The maximum possible points to keep after a suppression run.
   * @param min_radius The minimum radius to use for the suppression calculation.
   * @param tolerance A multiplicative tolerance on the number of maxpoints to return.
   * @param bm A BitMask to apply to the input point set.
   *
   * @return @b CnetSuppression::Results The Result set for the suppression run.
   */
  CnetSuppression::Results CnetSuppression::suppress(const QString &serialno, const int minpts,
                                                     const int &maxpts, const double &min_radius,
                                                     const double &tolerance,
                                                     const CnetSuppression::BitMask &bm) {
    PointSet cubeset = getCubeMeasureIndices(serialno);

#if defined(DEBUG)
    std::cout << "  PointSetSize: " << cubeset.size() << "\n";
#endif

    return ( suppress(cubeset, minpts, maxpts, min_radius, tolerance, bm) );
  }


  /**
   * Performs a suppression on the input PointSet and returns the result as a new PointSet.
   * An input bitmask will be used to mask the input pointset before running the suppression.
   *
   * @param points The point set to run suppression on.
   * @param minpts The minimum possible points to keep after a suppression run.
   * @param maxpts The maximum possible points to keep after a suppression run.
   * @param min_radius The minimum radius to use for the suppression calculation.
   * @param tolerance A tolerance factor which scales the size of the search space for suppression.
   * @param bm A BitMask to apply to the input point set.
   *
   * @return @b CnetSuppression::Results The Result set for the suppression run.
   */
  CnetSuppression::Results CnetSuppression::suppress(const CnetSuppression::PointSet &points,
                                                     const int &minpts, const int &maxpts,
                                                     const double &min_radius,
                                                     const double &tolerance,
                                                     const CnetSuppression::BitMask &bm) {

    // Bounding box of control points
    QRectF d = domain(points);

    double max_radius = qMax(d.width(), d.height());
    int num = qMax(qFloor(max_radius - min_radius), 11); //TODO not sure where the 11 came from...?
    QVector<double> radii = linspace(min_radius, max_radius, num,
                                     1.0/qSqrt(2.0) );

    QPointF botR = d.bottomRight();

#if defined(DEBUG)
    QPointF topL = d.topLeft();
    std::cout << "  Domain((x), (y)): (" << topL.x() << "," << botR.x()
              << "), (" << topL.y() << "," << botR.y() << ")\n";
    std::cout << "  Min.Max, count Radius: " << min_radius << ", "
              << max_radius << " ," << num << "\n";
#endif

    // Get scaled points to save
    int v_maxpts = int ( (double) maxpts * getScale( d.size() ) );
    v_maxpts = qMax(v_maxpts, minpts);
    int pnttol = qFloor( (v_maxpts * tolerance) + 0.5 );


    // Determine if any previously selected points are contained in this set
    PointSet fixed( contains(bm, points) );

#if defined(DEBUG)
    std::cout << "  FixedPoints: " << fixed.size() << " - ToSave: "
              << v_maxpts << " (scaled) - Pnttol: " << pnttol << "\n";
#endif

    Results result(size(), d, 1.0);
    result.add(fixed);

    // Check for a termination condition on input
    if (  result.size() > (v_maxpts - pnttol)  ) {

#if defined(DEBUG)
      std::cout << " ++> Initial condition met - return input set\n";
#endif

      m_results.append(result);
      return (result);
    }

    // Binary loop around cell radius list
    int bmin = 0;
    int bmax = radii.size() - 1;
    while ( (bmax-bmin) > 1 ) {
      int bmid = (bmin + bmax) / 2;

#if defined(DEBUG)
//      std::cout << "  \nBSearchIndx: (" << bmin << "," << bmid << "," << bmax << ")\n";
#endif

      double cell_size = radii[bmid];
      int n_x_cells = qCeil(botR.x() / cell_size);
      int n_y_cells = qCeil(botR.y() / cell_size);
      BOOST_ASSERT ( n_x_cells > 0 );
      BOOST_ASSERT ( n_y_cells > 0 );

#if defined(DEBUG)
      std::cout << "  CellRadius(x,y): " << cell_size << " - "
                << "Grid(" << n_x_cells << "," << n_y_cells << ")\n";
#endif

      // Create initial coverage grid with fixed points
      GridMask grid(n_x_cells, n_y_cells, false);
      result = Results(size(), d, cell_size);
      result.add(fixed);
      cover(grid, fixed, cell_size);

#if defined(DEBUG)
      int nIcov = cover(grid, fixed, cell_size);
      std::cout << "  InitialCoverage: " << nIcov << "\n";
#endif

      // Evaluate all points
      int x_center, y_center;
      for ( int i = 0 ; i < points.size() ; i++) {

        cellIndex(points[i], cell_size, x_center,  y_center);
        BOOST_ASSERT ( x_center < n_x_cells );
        BOOST_ASSERT ( y_center < n_y_cells );

        // Got one, update result state
        if ( false == grid[x_center][y_center] ) {

          // First check to see if we have exceeded the requested results set
          result.add(points[i]);
          if ( m_early_term ) {
            if ( result.size() > (v_maxpts + pnttol) ) {
              bmin = bmid;
              break;
            }
          }

          // Compute cell coverage
          cover(grid, x_center, y_center, cell_size);

#if defined(DEBUG)
          int ncov = cover(grid, x_center, y_center, cell_size);
          std::cout << "  TotalCovered: " << ncov << "(" << nCovered(grid)
                    << ") of " << (n_x_cells*n_y_cells) << "\n";
#endif

        }
      }

      // Results of search...
//      std::cout << "  PointsFound: " << result.size() << "\n";

      // Now determine if we have enough points to call it good
      if (  (result.size() >= (v_maxpts - pnttol) ) &&
            (result.size() <= (v_maxpts + pnttol) ) ) {
        bmax = bmin;  // This will terminate the binary search
      }
      else {
        if ( result.size() < v_maxpts ) {
          bmax = bmid;
        }
        else {
          bmin = bmid;
        }

        // Test failure condition?  Not sure yet if we need to do anything
      }
    }

    // Caller should check to determine if success
    return (result);
  }


  /**
   * Write out a Results object to an output control network.
   *
   * @param onetfile Filename for output control network
   * @param result Object containing the results of a suppression calculation
   * @param saveall If true, copies all points to the output control net, even if ignored.
   * @param netid Control network id
   */
  void CnetSuppression::write(const QString &onetfile, const Results &result,
                              const bool saveall, const QString &netid) {
    // Create new network
    QScopedPointer<ControlNet> onet;
    if ( m_cnet.isNull() ) {
      onet.reset( new ControlNet() );
      onet->SetNetworkId("cnetsuppress");
      onet->SetUserName(Application::UserName());
      onet->SetDescription("Network created from suppression of control point set");
      onet->SetCreatedDate(Application::DateTime());
    }
    else {
      onet.reset( new ControlNet(*m_cnet) );
    }

    if ( !netid.isEmpty() ) { onet->SetNetworkId(netid); }

    // Set states from result set and add points if
    for ( int i = 0 ; i < size() ; i++ ) {
      ControlPoint *p = point(i);
      p->SetIgnored( !result.m_selected[i] );
      if ( (!saveall) && result.m_selected[i] ) { onet->AddPoint(p); }
    }

    // Save all original points with altered ignored status
    if ( saveall ) {
      for ( int i = 0; i < m_points.size(); i++ ) { //TODO: is this the same as size()
        onet->AddPoint( m_points[i] );
      }
    }

    // Write the points
    onet->Write(onetfile);
    (void) onet->take();  // Not necessary to retain these points
    return;
  }


  /**
   * Gets the control net associated with the CnetSuppresssion.
   *
   * @return @b const ControlNet* The control net for this CnetSuppression
   */
  const ControlNet *CnetSuppression::net() const {
    return ( m_cnet.data() );
  }


  /**
   * Gets the index of an input IndexPoint
   *
   * @param p An IndexPoint to get the index of.
   *
   * @return @b int The index of the input index point.
   */
  int CnetSuppression::index(const CnetSuppression::IndexPoint &p) const {
    return (p.first);
  }


  /**
   * Gets the control measure from the input IndexPoint.
   *
   * @param p The IndexPoint to get the control measure from
   *
   * @return @b ControlMeasure* The control measure of the input IndexPoint
   */
  ControlMeasure *CnetSuppression::measure(const CnetSuppression::IndexPoint &p) const {
    return ( p.second );
  }


  /**
   * Create a BitMask of a specified size for an input PointSet.
   *
   * @param nbits The size of a BitMask to create.
   * @param p The PointSet used to create a BitMask.
   *
   * @return @b CnetSuppression::BitMask A bit mask with entires set to true if their index is in
   *                             the input PointSet.
   */
  CnetSuppression::BitMask CnetSuppression::maskPoints(int nbits,
                                                        const CnetSuppression::PointSet &p)
                                                        const {
    BOOST_ASSERT (nbits > 0);
    BitMask bm(nbits, false);
    for (int i = 0 ; i < p.size() ; i++) {
      BOOST_ASSERT(p[i].first < nbits);
      bm[p[i].first] = true;
    }
    return (bm);
  }


  /**
   * Use an input BitMask to mask the input PointSet, and return the results.
   *
   * @param bm BitMask contining true for indices to keep and false for indicies to get rid of.
   * @param pset The PointSet to apply the BitMask to.
   *
   * @return @b CnetSuppression::PointSet A PointSet containing only the points not masked out.
   */
  CnetSuppression::PointSet CnetSuppression::contains(const CnetSuppression::BitMask &bm,
                                                      const CnetSuppression::PointSet &pset) const {
    PointSet result;
    if ( bm.dim1() == 0 ) { return (result); }

    BOOST_FOREACH ( const IndexPoint &p, pset) {
      BOOST_ASSERT ( index(p) < bm.dim1() );
      if ( bm[index(p)] == true) {
        result.append(p);
      }
    }
    return (result);
  }


  /**
   * Calculates the (x,y) coordinates for an IndexPoint inside of a cell
   * of input size.
   *
   * @param p The index point to calculate the x_center and y_center of.
   * @param cell_size Size of the cell.
   * @param x_center The x-coordinate of the center of the cell (Result).
   * @param y_center The y-coordinate of the center of the cell (Result).
   */
  void CnetSuppression::cellIndex(const CnetSuppression::IndexPoint &p,
                                  const double &cell_size,
                                  int &x_center, int &y_center) const {
    x_center = int((measure(p)->GetSample()) / cell_size);
    y_center = int((measure(p)->GetLine())   / cell_size);
    return;
  }


  /**
   * Determine the number of (x,y) positions in the input grid which are set to true.
   *
   * @param grid The grid to calculate the number of covered points from.
   *
   * @return @b int The number of convered points; the number of (x,y) positions for which the
   *                bitmask is set to true.
   */
  int CnetSuppression::nCovered(const CnetSuppression::GridMask &grid) const {
    int ncov(0);
    for ( int x = 0 ; x < grid.dim1() ; x++) {
      for ( int y = 0 ; y < grid.dim2() ; y++) {
        if ( grid[x][y] == true) ncov++;
      }
    }
    return (ncov);
  }


  /**
   * Update a grid to contain true values where it is covered by cells as defined by the input
   * PointSet.
   *
   * @param grid The grid to update by setting to true grid entries which are contained within cells
   *             definted by the input PointSet.
   * @param points The input PointSet to use to update the grid.
   * @param cell_size The size of a cell.
   *
   * @return @b int The number of grid entries updated.
   */
  int CnetSuppression::cover(CnetSuppression::GridMask &grid,
                             const CnetSuppression::PointSet &points,
                             const double &cell_size) const {

    int x_center, y_center, ncov(0);
    BOOST_FOREACH ( const IndexPoint &p, points ) {
      cellIndex(p, cell_size, x_center, y_center);
      ncov = cover(grid, x_center, y_center, cell_size);
    }
    return (ncov); //ncov should be += for _all_ covered?
  }


  /**
   * Update a grid to contain true values where it overlaps a rectangle defined by the
   * input paramters and return the number of values contained in this rectange.
   *
   * @param grid The grid to update by setting to true grid entries contained within the
   *             rectangle defined by the paramters passed to this function.
   * @param x_center The x-coordinate of the center of the grid.
   * @param y_center The y-coordinate of the center of the grid.
   * @param cell_size The size of a cell.
   *
   * @return @b int The number of covered cells.
   */
  int CnetSuppression::cover(CnetSuppression::GridMask &grid,
                             const int &x_center, const int &y_center,
                             const double &cell_size) const {

    int n_x_cells = grid.dim1();
    int n_y_cells = grid.dim2();

    // Compute the cover. NOTE this is a rectangle, NOT euclidean distance!!!
    int g_x_min = qMax(x_center - int(cell_size+0.5), 0);
    int g_x_max = qMin(x_center + int(cell_size+0.5), n_x_cells-1);
    int g_y_min = qMax(y_center - int(cell_size+0.5), 0);
    int g_y_max = qMin(y_center + int(cell_size+0.5), n_y_cells-1);

#if defined(DEBUG)
    std::cout << "  CellCover: (" << g_x_min << "," << g_x_max << ")("
              << g_y_min << "," << g_y_max << ")\n";
#endif

    grid.subarray(g_x_min, g_x_max, g_y_min, g_y_max) = true;
    int ncov = (g_x_max-g_x_min+1) * (g_y_max-g_y_min+1);
    return ( ncov );
  }


  /**
   * Merge two PointSets together and return the result.
   *
   * @param s1 The first PointSet to be merged.
   * @param s2 The second PointSet to be merged.
   *
   * @return @b CnetSuppression::PointSet The merged PointSet.
   */
  CnetSuppression::PointSet CnetSuppression::merge(const CnetSuppression::PointSet &s1,
                                                   const CnetSuppression::PointSet &s2) const {
    BitMask bm2 = maskPoints(size(), s2);
    PointSet merged = s2;
    for ( int i = 0 ; i < s1.size() ; i++) {
      if ( !bm2[s1[i].first] ) {
        merged.append(s1[i]);
      }
    }
    return (merged);
  }


  /**
   * Merge two CnetSuppression::Results objects together and return the result.
   *
   * @param r1 The first Results to be merged.
   * @param r2 The second Results to be merged.
   *
   * @return @b CnetSuppression::Results The merged Results.
   */
  CnetSuppression::Results CnetSuppression::merge(const CnetSuppression::Results &r1,
                                                  const CnetSuppression::Results &r2) const {
    if ( !r1.isValid() ) { return (r2); }
    if ( !r2.isValid() ) { return (r1); }

    // Both are good, merge them
    Results merged;
    merged.m_candidates = orMasks(r1.m_candidates, r2.m_candidates);
    merged.m_selected   = orMasks(r1.m_selected, r2.m_selected);
    merged.m_points     = merge(r1.m_points, r2.m_points);
    return ( merged );
  }

  QRectF CnetSuppression::domain(const CnetSuppression::PointSet &pts) const {
    QPointF topL(DBL_MAX, DBL_MAX);
    QPointF botR(-DBL_MAX, -DBL_MAX);
    BOOST_FOREACH ( IndexPoint p, pts)  {
      topL.setX(qMin(topL.x(), measure(p)->GetSample()) );
      topL.setY(qMin(topL.y(), measure(p)->GetLine()) );

      botR.setX(qMax(botR.x(), measure(p)->GetSample()) );
      botR.setY(qMax(botR.y(), measure(p)->GetLine()) );

    }
    return ( QRectF(topL, botR) );
  }


  /**
   * Calculates the ratio between the area contained in a square of side-length d and the area
   * of the CnetSuppression.
   *
   * @param d The length of a side of a square.
   *
   * @return @b double The fraction: area of a square of size d / the area of the CnetSuppression.
   */
  double CnetSuppression::getScale(const QSizeF &d) const {
    if ( m_area.isEmpty() ) {  m_area = d;  }
    double area = d.width() * d.height();
    double pcnt = area / (m_area.width() * m_area.height());
    return ( pcnt );
  }


  /**
   * Computes and returns the bitwise 'or' of the two input bitmasks.
   *
   * @param b1 First input BitMask
   * @param b2 Second input BitMask
   *
   * @return @b CnetSuppression::BitMask A BitMask which is the bitwise result of 'b1 or b2'
   */
   CnetSuppression::BitMask CnetSuppression::orMasks(const CnetSuppression::BitMask &b1,
                                                     const CnetSuppression::BitMask &b2) const {
    BOOST_ASSERT ( b1.dim1() == b2.dim1() );
    BitMask omask(b1.dim1());
    for (int i = 0 ; i < b1.dim1() ; i++) {
      omask[i] = b1[i] || b2[i];
    }
    return (omask);
  }


  /**
   * Creates and returns a vector which contains num entries, starting at dmin and ending at dmax.
   * The entires in-between span the space between dmin and dmax with equal steps each time. The
   * entire vector is multiplied by an input scale factor.
   *
   * @param dmin The minimum value of the vector.
   * @param dmax The maximum value of the vector.
   * @param num The number of entries that should be in the returned vector.
   * @param scale Multiplicative scale factor to multiply the entire vector by.
   *
   * @return @b QVector<double> A vector of size num with entries starting at dmin and ending
   *            at dmax with values of equal increment spanning the space between them and
   *            with all entries multiplied by scale.
   */
  QVector<double> CnetSuppression::linspace(const double dmin,
                                            const double dmax,
                                            const int num,
                                            const double &scale) const {
    double inc = (dmax - dmin) / (double) (num - 1);
    QVector<double> ndarray(num);
    for (int i = 0 ; i < num ; i++) {
      ndarray[i] = (dmin + (inc * (double) i)) * scale;
    }
    ndarray[num-1] = dmax * scale;
    return ( ndarray );
  }


} // namespace Isis
