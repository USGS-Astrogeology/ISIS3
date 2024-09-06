/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "PixelFOV.h"

#include <QDebug>
#include <QList>
#include <QPoint>
#include <QPointF>
#include <QScopedPointer>

#include <geos/geom/CoordinateSequence.h>
#include <geos/geom/CoordinateSequence.h>
#include <geos/geom/LineString.h>
#include <geos/geom/MultiPolygon.h>
#include <geos/geom/Polygon.h>

#include "Camera.h"
#include "CameraDistortionMap.h"
#include "PolygonTools.h"

using namespace std;

namespace Isis {

  /**
   * Constructs an empty PixelFOV object
   *
   */
  PixelFOV::PixelFOV() {
  }


  /**
   * Destroys a PixelFOV object/
   *
   */
  PixelFOV::~PixelFOV() {
  }


  /**
   * @brief Produces an fov for the given line sample.
   *
   * This produces instantaneous fovs at several times during a pixel's exposure.
   * Then, it combines those instantaneous fovs into a full fov for the entire duration of
   * the pixel.  By default produces an instantaneous fov.  A full fov can be produced by
   * using numIfovs > 1.
   *
   * @param camera A pointer to the cube's camera.
   * @param sample The sample of the pixel.
   * @param line The line of the pixel.
   * @param numIfovs The number of instantaneous fovs that will be combined.
   *                 Defaults to 1, ie. an instantaneous fov.
   *
   * @return @b QList<QList<QPointF>> A list of points defining the boundary of the full fov.
   *                                  If the pixel crosses the 360/0 boundary, this
   *                                  will contain 2 lists of points, one on each side of the
   *                                  boundary.
   *
   * @throws IException::Programmer "The number of instantaneous field of views must be a
   *                                 positive integer."
   */
  QList< QList<QPointF> > PixelFOV::latLonVertices(Camera &camera,
                                                   const double sample,
                                                   const double line,
                                                   const int numIfovs) const {

    // Output list
    QList< QList<QPointF> > boundaryVertices;

    // Polygon pieces are sorted based on average longitude.  lowerVertices contains pieces with
    // average longitude less than 180.  upperVertices contains the rest.
    QList<QPointF> lowerVertices;
    QList<QPointF> upperVertices;


    if (numIfovs < 1) {
      std::string msg = "The number of instantaneous field of views must be a positive integer.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    // If computing an instantaneous fov
    if (numIfovs == 1) {

      camera.SetImage(line, sample);
      boundaryVertices.append(instantaneousFov(camera));
      return boundaryVertices;
    }

    // If computing a full fov
    else {

      // Collect the instantaneous fovs
      double timeStep = 0.0;
      try {
        timeStep = camera.exposureDuration(line, sample)/(numIfovs - 1);
      }
      catch (IException &e) {
        throw IException(e, IException::Unknown, "Unable to get FOV for full exposure.", _FILEINFO_);
      }
      for (int i = 0; i < numIfovs; i++) {
        camera.SetImage(line, sample,
                        timeStep * i - camera.exposureDuration(line, sample)/2);
        QList<QPointF> iFov = instantaneousFov(camera);

        // If the Ifov does not intersect the target move on
        if (iFov.isEmpty()) {
          break;
        }

        // Determine if the Ifov needs to be split
        bool crosses = false;
        int numVerts = iFov.size();
        for (int j = 0; j < numVerts - 1; j++) {
          if (fabs(iFov[j].y() - iFov[j+1].y()) > 180.0) {
            crosses = true;
          }
        }

        // If it needs to be split
        if (crosses) {
          QList< QList<QPointF> > splitIFov = splitIfov(iFov);

          // Sort the pieces based on average longitude.
          for (int j = 0; j < splitIFov.size(); j++) {
            double averageLong = 0;
            int numSubVerts = splitIFov[j].size();
            for (int k = 0; k < numSubVerts; k++) {
              averageLong += splitIFov[j][k].y();
            }
            averageLong = averageLong / numSubVerts;
            if (averageLong < 180) {
              lowerVertices.append(splitIFov[j]);
            }
            else {
              upperVertices.append(splitIFov[j]);
            }
          }
        }

        // If it does not need to be split
        else {
          // Put the Ifov in the proper class
          double averageLong = 0;
          for (int j = 0; j < numVerts; j++) {
            averageLong += iFov[j].y();
          }
          averageLong = averageLong / numVerts;
          if (averageLong < 180) {
            lowerVertices.append(iFov);
          }
          else {
            upperVertices.append(iFov);
          }
        }
      }
    }

    // Compute convex hulls for the two sets of points and append to output list.
    // If a set is empty then it is not output.
    if (!lowerVertices.isEmpty()) {
      boundaryVertices.append(envelope(lowerVertices));
    }
    if (!upperVertices.isEmpty()) {
      boundaryVertices.append(envelope(upperVertices));
    }

    return boundaryVertices;
  }


  /**
   * Compute the instantaneous fov for the pixel and time that the input camera
   * is set to.  By default the fov will be defined by the four corner points of the pixel,
   * but individual camera models may override this in Camera::PixelIfovOffsets().
   * The longitude coordinates will always been in 0-360 domain.
   *
   * @param camera The camera used to compute the fov.
   *
   * @return @b QList<QPointF> The lat, lon points defining the boundary of the fov.
   *
   * @see Camera::PixelIfovOffsets
   */
  QList<QPointF> PixelFOV::instantaneousFov(Camera &camera) const {

    QList<QPointF> vertices;

    QList<QPointF> offsets = camera.PixelIfovOffsets();
    int numVertices = offsets.size();

    double saveLook[3];
    double newLook[3];
    double unitNewLook[3];
    camera.LookDirection(saveLook);
    double focalLength = camera.FocalLength();

    //  For highly distorted instruments, take fpx, fpy (which are undistorted) convert to distorted,
    //  add offsets, undistort.  Only need to worry about if distortion high on a pixel to pixel
    // basis.  If this is done, save samp/line and reset the camera (SetImage).
    double scale = focalLength / saveLook[2];
    for (int i = 0; i < numVertices; i++) {
      double focalPlaneX = saveLook[0] * scale;
      double focalPlaneY = saveLook[1] * scale;
      focalPlaneX += offsets[i].x();
      focalPlaneY += offsets[i].y();
      newLook[0] = focalPlaneX;
      newLook[1] = focalPlaneY;
      newLook[2] = camera.DistortionMap()->UndistortedFocalPlaneZ();
      vhat_c(newLook, unitNewLook);
      if (camera.SetLookDirection(unitNewLook)) {
        vertices.append(QPointF(camera.UniversalLatitude(), camera.UniversalLongitude()));
      }
    }
    //  Reset look direction back to center of pixel
    camera.SetLookDirection(saveLook);
    return vertices;
  }


  /**
   * Produces a list of boundary points for the convex hull containing the input vertices.
   *
   * @param vertices The list of points to be enveloped.
   *
   * @return @b QList<QPointF> A List of points defining the enveloping polygon.
   */
  QList<QPointF> PixelFOV::envelope(QList<QPointF> vertices) const{

    //Put the vertices in a line string
    QScopedPointer<geos::geom::CoordinateSequence> points(new geos::geom::CoordinateSequence());

    for (int i = 0; i < vertices.size(); i++) {
      points->add(geos::geom::Coordinate(vertices[i].x(), vertices[i].y()));
    }
    QScopedPointer<geos::geom::LineString> pointString(Isis::globalFactory->createLineString(
                                                                           *points).release());

    //Compute a convex hull for the line string
    QScopedPointer<geos::geom::Geometry> boundingHull(pointString->convexHull().release());

    //Get the points
    geos::geom::CoordinateSequence *boundingPoints = boundingHull->getCoordinates().release();

    QList<QPointF> boundingVertices;
    for (unsigned int i = 0; i < boundingPoints->getSize(); i++) {
      boundingVertices.append(QPointF(boundingPoints->getAt(i).x,boundingPoints->getAt(i).y));
    }

    return boundingVertices;
  }


  /**
   * Split an instantaneous field of view across the 360/0 boundary.
   *
   * @param vertices A list of points defining the boundary of the unsplit IFOV
   *
   * @return @b QList<QList<QPointF>> A list of point clouds defining the boundaries of the pieces
   *                                  of the split IFOV.  Each point cloud represents a component
   *                                  of the IFOV.
   *
   * @see PolygonTools::SplitPolygonOn360
   */
  QList< QList<QPointF> > PixelFOV::splitIfov(QList<QPointF> vertices) const{
    // Create output list.
    QList< QList<QPointF> > splitPoints;

    // Create a polygon to split.
    QScopedPointer<geos::geom::CoordinateSequence> pts(new geos::geom::CoordinateSequence());
    for (int i = 0; i < vertices.size(); i++) {
      pts->add(geos::geom::Coordinate(vertices[i].y(), vertices[i].x()));
    }
    pts->add(geos::geom::Coordinate(vertices[0].y(), vertices[0].x()));
    QScopedPointer<geos::geom::Polygon> originalPoly(Isis::globalFactory->createPolygon(
                                                       globalFactory->createLinearRing(*pts)).release());

    // Split the polygon
    QScopedPointer<geos::geom::MultiPolygon> splitPolygons(
        PolygonTools::SplitPolygonOn360(originalPoly.data()));

    // Extract the vertices coordinates.
    QList< QList<QPointF> > splitVertices;
    for (unsigned int i = 0; i < splitPolygons->getNumGeometries(); i++) {
      QList<QPointF> subVertices;
      // The following objects don't need to be deleted, as the splitPolygons object will
      // delete them when it is deleted.
      const geos::geom::Polygon *subPolygon =
          dynamic_cast<const geos::geom::Polygon *>(splitPolygons->getGeometryN(i));
      geos::geom::CoordinateSequence *subCoordinates = subPolygon->getExteriorRing()->getCoordinates().release();
      for (unsigned int j = 0; j < subCoordinates->getSize(); j++) {
        subVertices.append(QPointF(subCoordinates->getAt(j).y,subCoordinates->getAt(j).x));
      }

      // Put the vertices in the output list.
      splitPoints.append(subVertices);

    }

    return splitPoints;
  }
}
