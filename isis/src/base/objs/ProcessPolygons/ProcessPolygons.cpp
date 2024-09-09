/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "ProcessPolygons.h"
#include "PolygonTools.h"
#include "Application.h"
#include "BoxcarCachingAlgorithm.h"
#include "SpecialPixel.h"

#include "geos/geom/CoordinateSequence.h"
#include "geos/geom/Coordinate.h"
#include "geos/geom/Envelope.h"
#include "geos/geom/LineString.h"
#include "geos/geom/Geometry.h"
#include "geos/geom/Point.h"
#include "geos/geom/prep/PreparedGeometryFactory.h"
#include "geos/geom/prep/PreparedPolygon.h"
#include "geos/util/IllegalArgumentException.h"
#include "geos/operation/overlay/snap/GeometrySnapper.h"

using namespace std;
namespace Isis {

  ProcessPolygons::ProcessPolygons() {
    m_useCenter = true;
    m_average = NULL;
    m_count = NULL;
    m_imagePoly = NULL;
  }



  /**
   *
   *
   * @param samples
   * @param lines
   * @param values
   */
  void ProcessPolygons::Rasterize(std::vector<double> &samples,
                                  std::vector<double> &lines,
                                  std::vector<double> &values) {

    m_sampleVertices = samples;
    m_lineVertices = lines;
    m_dns = values;
    FillPolygon(0);
  }



  /**
   * Rasterize multiband instruments where the bands have dependent geometry (i.e., the bands are
   * not geometrically registered).
   *
   * @param samples
   * @param lines
   * @param band
   * @param value
   */
  void ProcessPolygons::Rasterize(std::vector<double> &samples,
                                  std::vector<double> &lines,
                                  int &band, double &value) {

    m_sampleVertices = samples;
    m_lineVertices = lines;
    m_band = band;
    m_dns.clear();
    m_dns.push_back(value);

    FillPolygon(1);
  }


  /**
   * This method does the actuall reading and writing to the cube
   * file.  The Flag parameter is there to help out where the two
   * Rasterize method need to behave differently during this
   * operation.  Most notibly, when we set the position of the
   * bricks and when we are calculating the average using the
   * given value or values.
   *
   * @param Flag
   */
  void ProcessPolygons::FillPolygon(int Flag) {

    // Create a sample/line polygon for the input pixel vertices
    geos::geom::CoordinateSequence pts;
    for (unsigned int i = 0; i < m_sampleVertices.size(); i++) {
      pts.add(geos::geom::Coordinate(m_sampleVertices[i], m_lineVertices[i]));
    }
    pts.add(geos::geom::Coordinate(m_sampleVertices[0], m_lineVertices[0]));

    try {
      //  Create a polygon from the pixel vertices.  This polygon may have spikes or other
      //  problems such as multiple polygons.  Despike, then make sure we have a single polygon.
      //  Do not rasterize pixel if despiking fails or there are multiple polygons.
      geos::geom::Polygon *spikedPixelPoly = Isis::globalFactory->createPolygon(
          globalFactory->createLinearRing(pts)).release();

      const geos::geom::Polygon *projectedInputPixelPoly;

      if (spikedPixelPoly->isValid()) {
        projectedInputPixelPoly = spikedPixelPoly;
      }
      else {
        geos::geom::MultiPolygon *despikedPixelPoly;
        try {
          despikedPixelPoly = PolygonTools::Despike(spikedPixelPoly);
        }
        catch (IException &e) {
          delete spikedPixelPoly;
          return;
        }

        try {
          despikedPixelPoly = PolygonTools::Despike(spikedPixelPoly);
        }
        catch (IException &e) {
          delete spikedPixelPoly;
          return;
        }

        if (despikedPixelPoly->getNumGeometries() > 1) return;
        projectedInputPixelPoly =
            dynamic_cast<const geos::geom::Polygon *>(despikedPixelPoly->getGeometryN(0));
      }

      /* If there is not an intersecting polygon, there is no reason to go on.*/
      if (!projectedInputPixelPoly->intersects(m_imagePoly)) return;

      geos::geom::MultiPolygon *intersectPoly = PolygonTools::MakeMultiPolygon(
          m_imagePoly->intersection(projectedInputPixelPoly).release());
      geos::geom::prep::PreparedPolygon *preparedPoly =
        new geos::geom::prep::PreparedPolygon(intersectPoly);
      const geos::geom::Envelope *envelope = intersectPoly->getEnvelopeInternal();

      // Go thru each coord. in the envelope and ask if it is within the polygon
      for (double x = floor(envelope->getMinX()); x <= ceil(envelope->getMaxX()); x++) {
        if (x == 0) continue;

        for (double y = floor(envelope->getMinY()); y <= ceil(envelope->getMaxY()); y++) {
          if (y == 0) continue;

          bool contains;

          if (m_useCenter) {
            geos::geom::Coordinate c(x, y);
            geos::geom::Point *p = Isis::globalFactory->createPoint(c).release();
            contains = preparedPoly->contains(p);
            delete p;
          }
          else {
            geos::geom::CoordinateSequence tpts;
            tpts.add(geos::geom::Coordinate(x - 0.5, y - 0.5));
            tpts.add(geos::geom::Coordinate(x + 0.5, y - 0.5));
            tpts.add(geos::geom::Coordinate(x + 0.5, y + 0.5));
            tpts.add(geos::geom::Coordinate(x - 0.5, y + 0.5));
            tpts.add(geos::geom::Coordinate(x - 0.5, y - 0.5));

            geos::geom::Polygon *outPixelFootPrint = Isis::globalFactory->createPolygon(
                                      globalFactory->createLinearRing(tpts)).release();
            contains = preparedPoly->intersects(outPixelFootPrint);
            delete outPixelFootPrint;
          }

          if (contains) {

            // Read spectral noodle from samp, line position
            m_average->SetBasePosition((int)(x + 0.5), (int)(y + 0.5), 1);
            this->OutputCubes[0]->read(*m_average);
            m_count->SetBasePosition((int)(x + 0.5), (int)(y + 0.5), 1);
            this->OutputCubes[1]->read(*m_count);

            //  Process each band in the buffer
            for (unsigned int i = 0; i < m_dns.size(); i++) {

              int band;
              if (Flag == 0) {
                band = i;
              }
              else {
                band = m_band - 1;
              }

              double inputDn = m_dns[i];

              // The input dn is good
              if (IsValidPixel(inputDn)) {
                if (IsValidPixel((*m_average)[band])) {
                  double currentCount = (*m_count)[band];
                  double newCount = ++((*m_count)[band]);
                  double currentAverage = (*m_average)[band];
                  (*m_average)[band] = ((currentAverage * currentCount) + inputDn) / newCount;
                }
                else {
                  (*m_average)[band] = inputDn;
                  (*m_count)[band] = 1;
                }
              }

              // The input dn is special
              else {
                if (((*m_average)[band] == Isis::Null) || (inputDn != Null)) {
                  (*m_average)[band] = inputDn;
                }
              }

            } /*End for each band*/

            // Write spectral noodles back out to average and count cubes
            this->OutputCubes[0]->write(*m_average);
            this->OutputCubes[1]->write(*m_count);

          } /*End if (contains)*/

        } /*End for y*/

      } /*End for x*/

      delete projectedInputPixelPoly;
      delete intersectPoly;
      delete preparedPoly;

    } /*end try*/

    catch(geos::util::IllegalArgumentException *ill) {
      std::string msg = "ERROR! geos exception 1 [";
      msg += (std::string)ill->what() + "]";
      delete ill;
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }/*end catch*/

  }


  /**
   *
   * @deprecated Please use Finalize()
   */
  void ProcessPolygons::EndProcess() {

    delete m_imagePoly;
    delete m_average;
    delete m_count;
    Process::EndProcess();
  }


  /**
   *
   *
   */
  void ProcessPolygons::Finalize() {

    delete m_imagePoly;
    delete m_average;
    delete m_count;
    Process::Finalize();
  }


  /**
   * This gives the option to append to the cube
   *
   * @param avgFileName
   * @param countFileName
   *
   * @return Isis::Cube*
   */
  Isis::Cube *ProcessPolygons::AppendOutputCube(const QString &avgFileName,
      const QString &countFileName) {

    FileName *file = new FileName(avgFileName.toStdString());
    QString path = QString::fromStdString(file->path());
    QString filename = QString::fromStdString(file->baseName());
    QString extension = QString::fromStdString(file->extension());

    /*Open the average file with read/write permission*/
    Cube *averageCube = new Cube();
    averageCube->open(avgFileName, "rw");
    AddOutputCube(averageCube);

    /*Now open the count file with read/write permission*/
    Cube *countCube = new Cube();

    if (countFileName == "") {
      /*if the countFileName was set to nothing, then we use the default count
      file name.*/
      QString openFile = path + "/" + filename + "-count-." + extension;
      countCube->open(openFile, "rw");

    }
    else {
      countCube->open(countFileName, "rw");
    }

    AddOutputCube(countCube);
    return countCube;
  }



  /**
   *
   *
   * @param avgFileName
   * @param countFileName
   * @param nsamps
   * @param nlines
   * @param nbands
   */
  void ProcessPolygons::SetStatCubes(const QString &avgFileName, const
                                      QString &countFileName,
                                      Isis::CubeAttributeOutput &atts,
                                      const int nsamps, const int nlines,
                                      const int nbands) {

    this->Process::SetOutputCube(avgFileName, atts, nsamps, nlines, nbands);
    this->Process::SetOutputCube(countFileName, atts, nsamps, nlines, nbands);

    OutputCubes[0]->addCachingAlgorithm(new BoxcarCachingAlgorithm());
    OutputCubes[1]->addCachingAlgorithm(new BoxcarCachingAlgorithm());

    geos::geom::CoordinateSequence imagePts;

    imagePts.add(geos::geom::Coordinate(0.0, 0.0));
    imagePts.add(geos::geom::Coordinate(0.0, this->OutputCubes[0]->lineCount()));
    imagePts.add(geos::geom::Coordinate(this->OutputCubes[0]->sampleCount(),
                                        this->OutputCubes[0]->lineCount()));
    imagePts.add(geos::geom::Coordinate(this->OutputCubes[0]->sampleCount(), 0.0));
    imagePts.add(geos::geom::Coordinate(0.0, 0.0));

    m_imagePoly = Isis::globalFactory->createPolygon(
                    globalFactory->createLinearRing(imagePts)).release();

    m_average = new Brick(*this->OutputCubes[0], 1, 1, nbands);
    m_count = new Brick(*this->OutputCubes[1], 1, 1, nbands);
  }



  /**
   *
   *
   * @param parameter
   * @param nsamps
   * @param nlines
   * @param nbands
   */
  void ProcessPolygons::SetStatCubes(const QString &parameter,
                                      const int nsamps, const int nlines,
                                      const int nbands) {

    QString avgString =
      Application::GetUserInterface().GetCubeName(parameter);

    Isis::CubeAttributeOutput atts =
      Application::GetUserInterface().GetOutputAttribute(parameter);

    FileName *file = new FileName(avgString.toStdString());
    QString path = QString::fromStdString(file->path());
    QString filename = QString::fromStdString(file->baseName());
    QString countString = path + "/" + filename + "-count";
    SetStatCubes(avgString, countString, atts, nsamps, nlines, nbands);

  }


  /**
   * Sets the algorithm for how output pixels are rasterized
   *
   * @param useCenter
   *
   */
  void ProcessPolygons::SetIntersectAlgorithm(const bool useCenter) {
    m_useCenter = useCenter;
  }


} /* end namespace isis*/

