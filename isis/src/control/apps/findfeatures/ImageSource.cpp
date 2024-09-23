/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <string>
#include <vector>
#include <numeric>
#include <iostream>
#include <sstream>

#include <QList>
#include <QMutexLocker>
#include <QScopedPointer>
#include <QStringList>

#include "Camera.h"
#include "CameraFactory.h"
#include "CubeAttribute.h"
#include "Cube.h"
#include "Distance.h"
#include "FileName.h"
#include "GenericTransform.h"
#include "Histogram.h"
#include "ImageHistogram.h"
#include "IException.h"
#include "ImageSource.h"
#include "IString.h"
#include "Latitude.h"
#include "LineManager.h"
#include "Longitude.h"
#include "SerialNumber.h"
#include "SpecialPixel.h"
#include "Stretch.h"
#include "SurfacePoint.h"
#include "Projection.h"
#include "TProjection.h"

// Enable thread safety
#define MAKE_THREAD_SAFE 1

namespace Isis {


ImageSource::ImageSource() :  m_data( new SourceData() ) { }

ImageSource::ImageSource(const QString &name,
                         const bool geometryOnly,
                         const double minPercent,
                         const double maxPercent) :
                         m_data( new SourceData(name) ) {

  if ( geometryOnly == true) {
    initGeometry();
  }
  else {
    load(name, minPercent, maxPercent);
  }
}

ImageSource::ImageSource(const QString &name, const cv::Mat &image,
                         const QString &identity) :
                         m_data( new SourceData(name, image, identity) )  {
  if ( identity.isEmpty() ) {
    m_data->m_serialno = m_data->m_name;
  }
}

ImageSource::ImageSource(const ImageSource &other,
                         const bool getGeometry) : m_data( other.m_data ) {
  if ( getGeometry == true ) {
    initGeometry();
  }
}

bool ImageSource::hasGeometry() const {
  return ( ( hasProjection() || hasCamera() ) );
}

bool ImageSource::hasProjection() const {
  return (  0 != m_data->m_projection );
}

bool ImageSource::hasCamera() const {
  return ( 0 != m_data->m_camera );
}


QString ImageSource::getTargetName() const {
  if ( hasProjection() ) {
    PvlGroup mapping = m_data->m_projection->Mapping();
    return QString::fromStdString(mapping["TargetName"][0]);
  }
  else if ( hasCamera() ) {
    return ( m_data->m_camera->targetName() );
  }
  return (QString());
}



void ImageSource::load(const QString &name, const double minPercent,
                       const double maxPercent) {

  // A load operation will create a new copy of the data if counter > 1!!!
  m_data.detach();
  m_data->m_name = name;

#if defined(MAKE_THREAD_SAFE)
  QMutexLocker lock(m_data->m_mutex);  // Thread locking for ISIS IO activities
#endif

  // Load the image
  load(minPercent, maxPercent);
  return;
}

void ImageSource::load(const double minPercent,const double maxPercent) {

  QString name = m_data->m_name;
  FileName ifile(name.toStdString());

  // Handle ISIS cube specifically. If its not a cube, use OpenCV's image
  // reader
  if ( "cub" == ifile.extension() ) {
    Cube cube;
    CubeAttributeInput attTrans(name.toStdString());
    std::vector<QString> bandTrans = attTrans.bands();
    cube.setVirtualBands(bandTrans);
    cube.open(QString::fromStdString(ifile.expanded()), "r");

    if( cube.bandCount() != 1 ) {
      std::string msg = "Input cube " + name.toStdString() + " must only have one band!";
      throw IException(IException::User, msg, _FILEINFO_);
    }


    // Determine projection capabilities
    m_data->m_serialno = SerialNumber::Compose(cube, true);
    initGeometry( cube );

    try {
      // convert the cube data to a OpenCV Mat matrix
      // get a histogram from the specified single-band data
      QScopedPointer<Histogram> hist(getHistogram(cube));

      // obtain a minimum and maximum for the data that could be used for a
      // good contrast stretch
      double minDN = hist->Percent(minPercent);
      double maxDN = hist->Percent(maxPercent);

      // setup the stretch object
      Stretch stretch;
      stretch.AddPair(minDN, VALID_MIN1);
      stretch.AddPair(maxDN, VALID_MAX1);

      // setup the special pixel values for 8-bit data
      stretch.SetNull(NULL1);
      stretch.SetLis(LOW_INSTR_SAT1);
      stretch.SetLrs(LOW_REPR_SAT1);
      stretch.SetHis(HIGH_INSTR_SAT1);
      stretch.SetHrs(HIGH_REPR_SAT1);

      int nlines = cube.lineCount();
      int nsamps = cube.sampleCount();
      m_data->m_image = cv::Mat(nlines, nsamps, CV_8UC1); // CV_8UC1 -- 8-bit 1-channel data

      // Set up line manager and read in data, stretch and put it output image
      LineManager linereader(cube);
      for (int line = 0 ; line < nlines ; line++) {
        linereader.SetLine(line+1, 1);
        cube.read(linereader);
        for (int samp = 0 ;  samp < nsamps ; samp++) {
          m_data->m_image.at<unsigned char>(line,samp) = (unsigned char) stretch.Map(linereader[samp]);
        }
      }
    }
    catch (IException &ie) {
       throw IException(ie, IException::Programmer,
                         "Could not read and create grayscale image from " +
                          name.toStdString(), _FILEINFO_);
    }
  }
  else {
    // Assume OpenCV can read it
    try {
      m_data->m_image = cv::imread(ifile.expanded(),
                                   cv::IMREAD_GRAYSCALE);
      if ( m_data->m_image.empty() ) {
        std::string mess = "Failed to read image from " + name.toStdString();
        throw IException(IException::User, mess, _FILEINFO_);
      }

      m_data->m_serialno = QString::fromStdString(ifile.baseName());

      // Here we could check for world files and construct geometry!!
    }
    catch (cv::Exception &e) {
      std::string mess = "OpenCV cannot process file " + name.toStdString() + " " +
                      e.what();
      throw IException(IException::User, mess, _FILEINFO_);
    }
  }
  return;
}


SurfacePoint ImageSource::getLatLon(const double &line, const double &sample) {
  SurfacePoint point;
  if ( !hasGeometry() ) return (point);

#if defined(MAKE_THREAD_SAFE)
  QMutexLocker lock(m_data->m_mutex);  // Thread locking for ISIS Camera activites
#endif

  // Check for projection first and translate
  if ( hasProjection() ) {
    TProjection *proj = m_data->projection();
    if ( proj->SetWorld(sample, line) ) {
      double lat    = proj->UniversalLatitude();
      double lon    = proj->UniversalLongitude();
      double radius = proj->LocalRadius(lat);
      point.SetSphericalCoordinates(Latitude(lat, Angle::Degrees),
                                    Longitude(lon, Angle::Degrees),
                                    Distance(radius, Distance::Meters));
    }
  }
  else if ( hasCamera() ) {
    Camera *camera = m_data->camera();
    if ( camera->SetImage(sample, line) ) {
      point = camera->GetSurfacePoint();
    }
  }

  return ( point );
}


bool ImageSource::getLineSamp(const SurfacePoint &point,
                              double &line, double &samp,
                              double &radius) {
  line = samp = radius = Null;
  if ( !hasGeometry() ) return (false);
  if ( !point.Valid() ) return (false);

  bool isGood(false);

  double lat = point.GetLatitude().degrees();
  double lon = point.GetLongitude().degrees();


#if defined(MAKE_THREAD_SAFE)
  QMutexLocker lock(m_data->m_mutex);  // Thread locking for ISIS Camera activites
#endif

  // Check for projection first and translate
  if ( hasProjection() ) {
     TProjection *proj = m_data->projection();
    if ( proj->SetUniversalGround(lat, lon) ) {
      isGood = true;
      line   = proj->WorldY();
      samp   = proj->WorldX();
      radius = proj->LocalRadius();
    }
  }
  else if ( hasCamera() ) {
    Camera *camera = m_data->camera();
    if ( camera->SetUniversalGround(lat, lon) ) {
      isGood = true;
      line   = camera->Line();
      samp   = camera->Sample();
      radius = camera->LocalRadius().meters();
    }
  }

  return ( isGood );
}

cv::Mat ImageSource::getGeometryMapping(ImageSource &match,
                                        const int &minpts,
                                        const double &tol,
                                        const cv::Rect &subarea) {

  cv::Mat mapper = cv::Mat::eye(3,3,CV_64FC1);
  if ( !hasGeometry()   ) return (mapper);
  if ( !match.hasGeometry()  ) return (mapper);

  // Compute increment
  int v_minpts = qMax(minpts, 16);
  double increment = (int) (std::sqrt(std::max(24.0, (double) (v_minpts - 1))) + 1.0);

  cv::Rect iSize(0.0f, 0.0f, samples(), lines());
  if ( subarea.area() != 0.0 ) {  iSize = subarea;  }

  int nsamps = iSize.width;
  int nlines = iSize.height;

  int ssamp = iSize.x;
  int sline = iSize.y;

  // Set up line/sample loops
  std::vector<cv::Point2f> source, train;
  bool done(false);

  while ( ((int) source.size() < v_minpts) && (!done) ) {

    double sSpacing = qMax(1.0, (double) nsamps / (double)(increment));
    double lSpacing = qMax(1.0, (double) nlines / (double) (increment));
    if ( qMax(sSpacing, lSpacing) <= 1 ) done = true;  // Last possible loop

    source.clear();
    train.clear();

  #if 0
    std::cout << "\nSSamp, NSamps: " << ssamp << ", " << nsamps << "\n";
    std::cout << "SLine, NLines: " << sline << ", " << nlines << "\n";
    std::cout << "SINC, LINC:    " << sSpacing << ", " << lSpacing << "\n";
    std::cout << "Increment:     " << increment << "\n";
  #endif

    for (int l = 0 ; l < increment ; l++) {
      for ( int s = 0 ; s < increment ; s++ ) {
        int line = (int)(lSpacing / 2.0 + lSpacing * l + 0.5) + sline;
        int samp = (int)(sSpacing / 2.0 + sSpacing * s + 0.5) + ssamp;
        double oline, osamp, oradius;
        if ( match.getLineSamp(getLatLon(line, samp), oline, osamp, oradius) ) {
          source.push_back(cv::Point2f(samp, line));
          // std::cout << "SourcePt["<<s<<","<<l<<"]: " << samp << ", " << line << "\n";
          train.push_back(cv::Point2f(osamp, oline));
          // std::cout << " TrainPt["<<s<<","<<l<<"]: " << osamp << ", " << oline << "\n";
        }
      }
    }

    // std::cout << "TotalPoints: " << source.size() << "\n";
    increment += 2.0;
  }

  // Compute homography if enough point
  if ( (int) source.size() < v_minpts ) {
    std::string mess = "Failed to get geometry mapping for " + match.name().toStdString() +
                   " to " + name().toStdString() + " needing " + Isis::toString(v_minpts) +
                   " but only could get " + std::to_string(source.size()) +".";
    throw IException(IException::Programmer, mess, _FILEINFO_);
  }

  // Find homography using Least-Median robust method algorithm
  std::vector<uchar> inliers(source.size(),0);
  mapper = cv::findHomography(source, train, cv::LMEDS, tol, inliers);

  // Using the Least median method requires > 50% inliers.  Check that here.
  int nInliers(0);
  for (unsigned int i = 0 ; i < inliers.size() ; i++) {
    if ( 0 != inliers[i] )  nInliers++;
  }

  // If we have < 50% inliners, compute the RANSAC homography
  double inlierPercent = ((double) nInliers / (double) source.size()) * 100.0;
  if ( 50.0 > inlierPercent ) {
    // std::cout << "LMEDS failed with only " << inlierPercent << "% - computing RANSAC homography!\n";
    mapper = cv::findHomography(source, train, cv::RANSAC, tol, inliers);
  }

  return (mapper);
}

Histogram *ImageSource::getHistogram(Cube &cube) const {
  QScopedPointer<Histogram> hist(new ImageHistogram(cube, 1));
  LineManager line(cube);

  for(int i = 1; i <= cube.lineCount(); i++) {
    line.SetLine(i, 1);
    cube.read(line);
    hist->AddData(line.DoubleBuffer(), line.size());
  }

  return ( hist.take() );
}




bool ImageSource::initGeometry() {
  FileName ifile(m_data->m_name.toStdString());
  Cube cube;
  CubeAttributeInput attTrans(m_data->m_name.toStdString());
  std::vector<QString> bandTrans = attTrans.bands();
  cube.setVirtualBands(bandTrans);
  cube.open(QString::fromStdString(ifile.expanded()), "r");

  if( cube.bandCount() != 1 ) {
    std::string msg = "Input cube " + m_data->m_name.toStdString() +
                  " must only have one band!";
   throw IException(IException::User, msg, _FILEINFO_);
  }

  // Get the geometry
  m_data->m_serialno = SerialNumber::Compose(cube, true);
  return ( initGeometry(cube) );
}


bool ImageSource::initGeometry(Cube &cube) {

  // Determine projection capabilities
  bool gotOne(false);
  try {
    if ( cube.isProjected() ) {
      m_data->m_projection = ProjectionFactory::CreateFromCube( *cube.label() );
      gotOne = true;
    }

    // Try camera also, independently
    m_data->m_camera= ( CameraFactory::Create(cube) );
    gotOne = true;
  }
  catch (IException &ie) {
     //  Didn't get both or neither.
  }
  return ( gotOne );
}


} // namespace Isis
