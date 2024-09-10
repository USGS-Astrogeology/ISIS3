#ifndef DatumFunctoid_h
#define DatumFunctoid_h

/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <cmath>
#include <QList>
#include <QString>
#include <QSharedPointer>

#include <opencv2/opencv.hpp>

#include "ControlPointCloudPt.h"
#include "MapPointCollector.h"
#include "PvlFlatMap.h"
#include "Statistics.h"

// Add NaturalNeighbor headers.
extern "C" {
#include "nn.h"
}

namespace Isis {

  /**
   * @brief Gather ControlPoints for generation of an output map (radius) pixel
   *
   * This class will collect ControlPoint candidates that are within
   * a tolerance generation of an output radius map pixel
   *
   * @author 2015-11-16 Kris Becker
   *
   * @internal
   *   @history 2015-11-16 Kris Becker - Original Version
   *   @history 2016-08-28 Kelvin Rodriguez - Removed usused private member variables to
   *            eliminate unused member variables warnings in clang. Part of porting to OS X 10.11.
   */

  class DatumFunctoid {
    public:
      DatumFunctoid() : m_parameters(), m_name("DatumFunctoid") { }
      DatumFunctoid(const QString &name,
                    const PvlFlatMap &parameters = PvlFlatMap() ) :
                    m_parameters(parameters), m_name(name) {
        m_name = m_parameters.get("Name", m_name);
      }

      virtual ~DatumFunctoid() { }

      QString name() const { return ( m_name ); }

      virtual double value(const MapPointCollector &m)  = 0;
      virtual DatumFunctoid *clone(const PvlFlatMap &parameters) = 0;

      const PvlFlatMap &parameters() const {
        return ( m_parameters );
      }

    protected:
      PvlFlatMap  m_parameters; // List of parameters

    private:
      QString     m_name;   // Name of functoid
  };


  /**
   * @brief Return the average radius for the point
   *
   * @author 2015-11-16 Kris Becker
   *
   * @internal
   *   @history 2015-11-16 Kris Becker - Original Version
   */
  class AverageRadius : public DatumFunctoid {
    public:
      AverageRadius() : DatumFunctoid("AverageRadius") { }
      AverageRadius(const QString &name,
                    const PvlFlatMap &parameters = PvlFlatMap()) :
                    DatumFunctoid(name, parameters) { }
      virtual ~AverageRadius() { }
      virtual double value(const MapPointCollector &m) {
        return (m.getRadiusStatistics().Average());
      }
      virtual DatumFunctoid *clone(const PvlFlatMap &parameters = PvlFlatMap()) {
        QString v_name = parameters.get("Name", name());
        return ( new AverageRadius(v_name) );
      }
  };

  /**
   * @brief Return the median radius
   *
   * @author 2015-11-16 Kris Becker
   *
   * @internal
   *   @history 2015-11-16 Kris Becker - Original Version
   */
  class MedianRadius : public DatumFunctoid {
    public:
      MedianRadius() : DatumFunctoid("MedianRadius") { }
      MedianRadius(const QString &name,
                    const PvlFlatMap &parameters = PvlFlatMap()) :
                    DatumFunctoid(name, parameters) { }
      virtual ~MedianRadius() { }
      virtual double value(const MapPointCollector &m) {
        double v = Null;
        if ( m.size() == 0 )  return ( v );
        if ( m.size() == 1 )  return ( m.getPoint(0).radius() );

        QVector<double> radii;
        for (int i = 0 ; i < m.size() ; i++) {
          radii.push_back(m.getPoint(i).radius());
        }

        std::sort(radii.begin(), radii.end());
        if ( (radii.size() % 2) == 0) {
          int ndx0 = (radii.size() - 1) / 2;
          v = (radii[ndx0] + radii[ndx0+1]) / 2.0;
        }
        else {
          v = radii[radii.size()/2];
        }
        return ( v );
      }

      virtual DatumFunctoid *clone(const PvlFlatMap &parameters = PvlFlatMap()) {
        QString v_name = parameters.get("Name", name());
        return ( new MedianRadius(v_name) );
      }
  };


  /**
   * @brief Return the standard deviation radius for the point
   *
   * @author 2015-11-16 Kris Becker
   *
   * @internal
   *   @history 2015-11-16 Kris Becker - Original Version
   */
  class StandardDeviationRadius : public DatumFunctoid {
    public:
      StandardDeviationRadius() : DatumFunctoid("StandardDeviationRadius") { }
      StandardDeviationRadius(const QString &name,
                              const PvlFlatMap &parameters = PvlFlatMap()) :
                              DatumFunctoid(name, parameters) { }
      virtual ~StandardDeviationRadius() { }
      virtual double value(const MapPointCollector &m) {
        return (m.getRadiusStatistics().StandardDeviation());
      }


      virtual DatumFunctoid *clone(const PvlFlatMap &parameters = PvlFlatMap()) {
        QString v_name = parameters.get("Name", name());
        return ( new StandardDeviationRadius(v_name) );
      }
  };



  /**
   * @brief Return the maximum radius for the point
   *
   * @author 2015-11-16 Kris Becker
   *
   * @internal
   *   @history 2015-11-16 Kris Becker - Original Version
   */
  class MaximumRadius : public DatumFunctoid {
    public:
      MaximumRadius() : DatumFunctoid("MaximumRadius") { }
      MaximumRadius(const QString &name,
                    const PvlFlatMap &parameters = PvlFlatMap()) :
                    DatumFunctoid(name, parameters) { }
      virtual ~MaximumRadius() { }
      virtual double value(const MapPointCollector &m) {
        return (m.getRadiusStatistics().Maximum());
      }

      virtual DatumFunctoid *clone(const PvlFlatMap &parameters = PvlFlatMap()) {
        QString v_name = parameters.get("Name", name());
        return ( new MaximumRadius(v_name) );
      }
  };

  /**
   * @brief Return the minimum radius for the point
   *
   * @author 2015-11-16 Kris Becker
   *
   * @internal
   *   @history 2015-11-16 Kris Becker - Original Version
   */
  class MinimumRadius : public DatumFunctoid {
    public:
      MinimumRadius() : DatumFunctoid("MinimumRadius") { }
      MinimumRadius(const QString &name,
                    const PvlFlatMap &parameters = PvlFlatMap()) :
                    DatumFunctoid(name, parameters) { }
      virtual ~MinimumRadius() { }
      virtual double value(const MapPointCollector &m) {
        return (m.getRadiusStatistics().Minimum());
      }
      virtual DatumFunctoid *clone(const PvlFlatMap &parameters = PvlFlatMap()) {
        QString v_name = parameters.get("Name", name());
        return ( new MinimumRadius(v_name) );
      }
  };


  /**
   * @brief Return the average L2 distance from the reference point
   *
   * @author 2015-11-16 Kris Becker
   *
   * @internal
   *   @history 2015-11-16 Kris Becker - Original Version
   */
  class AverageDistance : public DatumFunctoid {
    public:
      AverageDistance() : DatumFunctoid("AverageDistance") { }
      AverageDistance(const QString &name,
                      const PvlFlatMap &parameters = PvlFlatMap()) :
                      DatumFunctoid(name, parameters) { }
      virtual ~AverageDistance() { }
      virtual double value(const MapPointCollector &m) {
        return (m.getDistanceStatistics().Average());
      }

      virtual DatumFunctoid *clone(const PvlFlatMap &parameters = PvlFlatMap()) {
        QString v_name = parameters.get("Name", name());
        return ( new AverageDistance(v_name) );
      }
  };

  /**
   * @brief Return the median L2 distance from the reference point
   *
   * @author 2015-11-16 Kris Becker
   *
   * @internal
   *   @history 2015-11-16 Kris Becker - Original Version
   */
  class MedianDistance : public DatumFunctoid {
    public:
      MedianDistance() : DatumFunctoid("MedianDistance") { }
      MedianDistance(const QString &name,
                     const PvlFlatMap &parameters = PvlFlatMap()) :
                     DatumFunctoid(name, parameters) { }
      virtual ~MedianDistance() { }
      virtual double value(const MapPointCollector &m) {
        double v = Null;
        if ( m.size() == 0 )  return ( v );
        if ( m.size() == 1 )  return ( m.getDistance(0) );

        QVector<double> distances;
        for (int i = 0 ; i < m.size() ; i++) {
          distances.push_back(m.getDistance(i));
        }
        std::sort(distances.begin(), distances.end());;

        if ( (distances.size() % 2) == 0) {
          int ndx0 = (distances.size() - 1) / 2;
          v = (distances[ndx0] + distances[ndx0+1]) / 2.0;
        }
        else {
          v = distances[distances.size()/2];
        }
        return ( v );
      }

      virtual DatumFunctoid *clone(const PvlFlatMap &parameters = PvlFlatMap()) {
        QString v_name = parameters.get("Name", name());
        return ( new MedianDistance(v_name) );
      }
  };


  /**
   * @brief Return the standard deviation distance for the point
   *
   * @author 2015-11-16 Kris Becker
   *
   * @internal
   *   @history 2015-11-16 Kris Becker - Original Version
   */
  class StandardDeviationDistance : public DatumFunctoid {
    public:
      StandardDeviationDistance() : DatumFunctoid("StandardDeviationDistance") { }
      StandardDeviationDistance(const QString &name,
                                const PvlFlatMap &parameters = PvlFlatMap()) :
                                DatumFunctoid(name, parameters) { }
      virtual ~StandardDeviationDistance() { }
      virtual double value(const MapPointCollector &m) {
        return (m.getDistanceStatistics().StandardDeviation());
      }

      virtual DatumFunctoid *clone(const PvlFlatMap &parameters = PvlFlatMap()) {
        QString v_name = parameters.get("Name", name());
        return ( new StandardDeviationDistance(v_name) );
      }
  };


  /**
   * @brief Return the maximum distance for the point
   *
   * @author 2015-11-16 Kris Becker
   *
   * @internal
   *   @history 2015-11-16 Kris Becker - Original Version
   */
  class MaximumDistance : public DatumFunctoid {
    public:
      MaximumDistance() : DatumFunctoid("MaximumDistance") { }
      MaximumDistance(const QString &name,
                      const PvlFlatMap &parameters = PvlFlatMap()) :
                      DatumFunctoid(name, parameters) { }
      virtual ~MaximumDistance() { }
      virtual double value(const MapPointCollector &m) {
        return (m.getDistanceStatistics().Maximum());
      }

      virtual DatumFunctoid *clone(const PvlFlatMap &parameters = PvlFlatMap()) {
        QString v_name = parameters.get("Name", name());
        return ( new MaximumDistance(v_name) );
      }
  };

  /**
   * @brief Return the minimum distance for the point
   *
   * @author 2015-11-16 Kris Becker
   *
   * @internal
   *   @history 2015-11-16 Kris Becker - Original Version
   */
  class MinimumDistance : public DatumFunctoid {
    public:
      MinimumDistance() : DatumFunctoid("MinimumDistance") { }
      MinimumDistance(const QString &name,
                      const PvlFlatMap &parameters = PvlFlatMap()) :
                      DatumFunctoid(name, parameters) { }
      virtual ~MinimumDistance() { }
      virtual double value(const MapPointCollector &m) {
        return (m.getDistanceStatistics().Minimum());
      }

      virtual DatumFunctoid *clone(const PvlFlatMap &parameters = PvlFlatMap()) {
        QString v_name = parameters.get("Name", name());
        return ( new MinimumDistance(v_name) );
      }
  };


  /**
   * @brief Return the signed typed count of radius points
   *
   * This functor returns the total number of pixels in the point collection. It
   * additionally applies a signed (+-1) to indicate if the FLANN search type was
   * nearest neighbor (-1) or radius (+1).
   *
   * @author 2015-11-16 Kris Becker
   *
   * @internal
   *   @history 2015-11-16 Kris Becker - Original Version
   */
  class TypeCount : public DatumFunctoid {
    public:
      TypeCount() : DatumFunctoid("TypeCount") { }
      TypeCount(const QString &name,
                const PvlFlatMap &parameters = PvlFlatMap()) :
                DatumFunctoid(name, parameters) { }
      virtual ~TypeCount() { }
      virtual double value(const MapPointCollector &m) {
        double count = m.getRadiusStatistics().ValidPixels();
        double sign = (m.getSearchType() == MapPointCollector::NearestNeighbor) ?
                       -1 : 1;
        return ( count * sign );
      }


      virtual DatumFunctoid *clone(const PvlFlatMap &parameters = PvlFlatMap()) {
        QString v_name = parameters.get("Name", name());
        return ( new TypeCount(v_name) );
      }

    private:
  };


  /**
   * @brief Return the radius computed using Shepards weighted algorithm
   *
   * @author 2015-11-16 Kris Becker
   *
   * @internal
   *   @history 2015-11-16 Kris Becker - Original Version
   */
  class ShepardsRadius : public DatumFunctoid {
    public:
      ShepardsRadius() : DatumFunctoid("ShepardsRadius"), m_power(2) { }
      ShepardsRadius(const QString &name,
                     const PvlFlatMap &parameters = PvlFlatMap(),
                     const int power = 2) :
                     DatumFunctoid(name, parameters), m_power(power) {
        m_power = parameters.get("Power", QString::number(m_power)).toInt();
      }

      virtual ~ShepardsRadius() { }
      virtual double value(const MapPointCollector &m) {
        double radius(Null);
        if ( m.size() > 0 ) {

          // First compute the sums of the distances to weight the result
          double distsum(0.0);
          for (int i = 0 ; i < m.size() ; i++ ) {
            Q_ASSERT ( m.getDistance(i) >= 0.0 );
            distsum += ShepardsWeight( m.getDistance(i), m_power );
          }

          // Now compute the weighted sum
          radius = 0.0;
          for (int r = 0 ; r < m.size() ; r++ ) {
            double rad = m.getPoint(r).radius();
            Q_ASSERT (rad >= 0.0 );
            radius += (rad * ( ShepardsWeight(m.getDistance(r)) / distsum ) );
          }
        }
        return (radius);
      }


      virtual DatumFunctoid *clone(const PvlFlatMap &parameters = PvlFlatMap()) {
        QString v_name = parameters.get("Name", name());
        return ( new ShepardsRadius(v_name, parameters) );
      }

    private:
      int   m_power;  // Power to compute weight

      double ShepardsWeight(const double distance, const int power = 2) {
        double d(distance);
        for (int i = 1 ; i < power ; i++ ) {  d *= distance; }
        return ( 1.0 / d );
      }
  };


  /**
   * @brief Return the radius computed using Shepards weighted algorithm
   *
   * @author 2015-11-16 Kris Becker
   *
   * @internal
   *   @history 2015-11-16 Kris Becker - Original Version
   */
  class FrankeNelsonRadius : public DatumFunctoid {
    public:
      FrankeNelsonRadius() : DatumFunctoid("FrankeNelsonRadius") { }
      FrankeNelsonRadius(const QString &name,
                         const PvlFlatMap &parameters = PvlFlatMap()) :
                         DatumFunctoid(name, parameters) { }
      virtual ~FrankeNelsonRadius() { }
      virtual double value(const MapPointCollector &m) {
        double radius(Null);

        if ( m.size() > 2 ) {

          // First find the maximum distance
          double R(-DBL_MAX);
          for (int i = 0 ; i < m.size() ; i++ ) {
            if ( m.getDistance(i) > R ) R = m.getDistance(i);
          }

          // Now compute the normalizer (denominator)
          double Rhj_sqr(0.0);
          for (int j = 0 ; j < m.size() ; j++ ) {
            double w = ( R - m.getDistance(j) ) / ( R * m.getDistance(j) );
            Rhj_sqr += (w * w);
          }

          radius = 0.0;
          for (int r = 0 ; r < m.size() ; r++ ) {
            double w = ( R - m.getDistance(r) ) / ( R * m.getDistance(r) );
            double wi = ( w * w ) / Rhj_sqr;
            radius += ( wi * m.getPoint(r).radius() );
          }
        }
        else if ( 2 == m.size() ) {
          // Cannot apply weighted algorithm so just use average
          radius = m.getRadiusStatistics().Average();
        }
        else if ( 1 == m.size() ) {
          // Only one radius - just use it
          radius = m.getPoint(0).radius();
        }

        return ( radius );
      }

      virtual DatumFunctoid *clone(const PvlFlatMap &parameters = PvlFlatMap()) {
        QString v_name = parameters.get("Name", name());
        return ( new FrankeNelsonRadius(v_name) );
      }

  };



  /**
   * @brief Natural Neighbor interpolation method
   *
   * @author 2015-11-16 Kris Becker
   *
   * @internal
   *   @history 2015-11-16 Kris Becker - Original Version
   */
  class NaturalNeighborRadius : public DatumFunctoid {
    public:
      NaturalNeighborRadius() : DatumFunctoid("NaturalNeighborRadius") { }
      NaturalNeighborRadius(const QString &name,
                         const PvlFlatMap &parameters = PvlFlatMap()) :
                         DatumFunctoid(name, parameters) { }
      virtual ~NaturalNeighborRadius() { }
      virtual double value(const MapPointCollector &m) {
        double radius(Null);

        if ( m.size() > 2 ) {
          // Create delaney diagram - may want to check for noisy points

          int npts = m.size();
          QVector<point> points(npts);
          for (int i = 0 ; i < npts ; i++) {
            getpoint(m.getPoint(i), &points[i]);
          }

          delaunay *d = delaunay_build(npts, &points[0], 0, 0, 0, 0);
          nnpi *nn = nnpi_create(d);

          point pout;
          getpoint(m.getSource(), &pout);
          nnpi_interpolate_point(nn, &pout);

          // Compute radius and see if its valid
          radius = m.getSource().radius(pout.x, pout.y, pout.z);
          if ( std::isnan(radius) ) radius = Null;

          // Clean up
          nnpi_destroy(nn);
          delaunay_destroy(d);
        }
        else if ( 2 == m.size() ) {
          // Cannot apply weighted algorithm so just use average
          radius = m.getRadiusStatistics().Average();
        }
        else if ( 1 == m.size() ) {
          // Only one radius - just use it
          radius = m.getPoint(0).radius();
        }

        return ( radius );
      }

      virtual DatumFunctoid *clone(const PvlFlatMap &parameters = PvlFlatMap()) {
        QString v_name = parameters.get("Name", name());
        return ( new NaturalNeighborRadius(v_name) );
      }

    private:
      inline point *getpoint(const ControlPointCloudPt &p, point *nnp) const {
        double xyzw[4];
        p.getGroundCoordinates(xyzw);
        nnp->x = xyzw[0];
        nnp->y = xyzw[1];
        nnp->z = xyzw[2];
        return (nnp);
      }
  };


  // Definitions
  typedef QSharedPointer<DatumFunctoid>  SharedDatumFunctoid;
  typedef QList<SharedDatumFunctoid>     DatumFunctoidList;


  /**
   * @brief Create DatumFunctoids as provided in parameter form
   *
   * @author 2015-11-16 Kris Becker
   *
   * @internal
   *   @history 2015-11-16 Kris Becker - Original Version
   */
  class DatumFunctoidFactory {
    public:
      static DatumFunctoidFactory *getInstance();

      QStringList algorithms() const;

      DatumFunctoidList create(const QString &specs,
                               const bool &errorIfEmpty = true) const;
      DatumFunctoid *make(const QString &spec) const;

    private:
      static DatumFunctoidFactory *m_maker;
      DatumFunctoidList            m_functoids;

      DatumFunctoidFactory();
      ~DatumFunctoidFactory();
      static void DieAtExit();

      void init();
      PvlFlatMap parseParameters(const QString &parameters) const;
  };


}  // namespace Isis
#endif
