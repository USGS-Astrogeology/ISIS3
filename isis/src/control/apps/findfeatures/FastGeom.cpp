/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */


#include <exception>

#include <opencv2/opencv.hpp>

#include "Constants.h"
#include "Distance.h"
#include "FastGeom.h"
#include "IException.h"
#include "ImageTransform.h"
#include "Latitude.h"
#include "Longitude.h"
#include "SurfacePoint.h"

namespace Isis {

/** Constructor */
FastGeom::FastGeom() : m_fastpts(25), m_tolerance(1.0), m_geomtype("camera"),
                       m_maxarea(3.0), m_parameters() {
  validate(m_geomtype);
}

/** Construct with parameters */
FastGeom::FastGeom(const PvlFlatMap &parameters) : m_fastpts(25),
                                                   m_tolerance(1.0),
                                                   m_geomtype("camera"),
                                                   m_maxarea(3.0),
                                                   m_parameters(parameters) {
  m_fastpts   = toInt(m_parameters.get("FastGeomPoints", "25"));
  m_tolerance = toDouble(m_parameters.get("FastGeomTolerance", "3.0"));
  m_geomtype  = m_parameters.get("GeomType", "camera").toLower();
  validate(m_geomtype);
}

/** Construct with individual parameters to compute the fast geom transform */
FastGeom::FastGeom(const int maxpts, const double tolerance, const bool crop,
         const bool preserve, const double &maxarea) :
         m_fastpts(maxpts), m_tolerance(tolerance), m_geomtype("camera"),
         m_maxarea(maxarea), m_parameters() {
  validate(m_geomtype);
}

/* Destructor */
FastGeom::~FastGeom() { }

/**
 * @brief Compute train->query fast geom transformation
 *
 *  This method computes a fast geometric transform that "projects" the train
 *  image into the query image space. This method assumes both images have
 *  camera models or map projections that can compute longitude/latitude
 *  coordinates from sample/line coordinates and back.
 *
 *  The query line/samples coordinates are converted to latitude/longitude
 *  coordinates. The latitude/longitude coordinates are then used in the train
 *  image to compute its corresponding line/sample. A minimum number of points
 *  (m_fastpts) are computed to create a perspective transform that maps image
 *  coordinates of the two images.
 *
 * @param query            Query image
 * @param train            Train image
 * @return ImageTransform* Pointer to FastGeom transform
 */
ImageTransform *FastGeom::compute(MatchImage &query, MatchImage &train,
                                  QLogger logger) const {

  // Shortcut reference directly to output stream method
  QDebugStreamType &logit = logger.logger();

  logit << "\n++++ Running FastGeom ++++\n";
  logit << "*** QueryImage: " << query.source().name() << "\n";
  logit << "*** TrainImage: " << train.source().name() << "\n";

  // Set default map to identity matrix
  cv::Mat mapper = cv::Mat::eye(3,3,CV_64FC1);

  // Image description
  RectArea qSize(0.0, 0.0, query.source().samples(), query.source().lines() );
  RectArea tSize(0.0, 0.0, train.source().samples(), train.source().lines() );

  // SANITY CHECK
  // Ensure both images have valid cameras
  QStringList errors;
  if ( !query.source().hasGeometry() ) {
    errors.append("Query image (" + query.source().name() +
                  ") does not support geometry operations (no camera/projection)!");
  }

  if ( !train.source().hasGeometry() ) {
    errors.append("Train image (" + train.source().name() +
                  ") does not support geometry operations (no camera/projection)!");
  }

  if ( errors.size() > 0 ) {
    logit << "--> Failed: " << errors.join("\n") << "\n";
    throw IException(IException::User, "--> FastGeom failed <--\n" + errors.join("\n"),
                     _FILEINFO_);
  }

  // Setup common processing parameters
  double q_samps = query.source().samples();
  double q_lines = query.source().lines();

  double t_samps = train.source().samples();
  double t_lines = train.source().lines();

  // Now get FOV tolerances. Default maps strictly within detector boundaries.
  double fg_q_sample_tolerance = toDouble(m_parameters.get("FastGeomQuerySampleTolerance", "0.0"));
  double fg_q_line_tolerance   = toDouble(m_parameters.get("FastGeomQueryLineTolerance",   "0.0"));
  double fg_t_sample_tolerance = toDouble(m_parameters.get("FastGeomTrainSampleTolerance", "0.0"));
  double fg_t_line_tolerance   = toDouble(m_parameters.get("FastGeomTrainLineTolerance",   "0.0"));

  // Train line/sample map restrictions
  double q_min_samp = 0.5 - fg_q_sample_tolerance;
  double q_max_samp = q_samps + 0.4999 + fg_q_sample_tolerance;
  double q_min_line = 0.5 - fg_q_line_tolerance;
  double q_max_line = q_lines + 0.4999 + fg_q_line_tolerance;
  FGFov  q_fov(q_min_samp, q_min_line, (q_max_samp-q_min_samp), (q_max_line-q_min_line));

  double t_min_samp = 0.5 - fg_t_sample_tolerance;
  double t_max_samp = t_samps + 0.4999 + fg_t_sample_tolerance;
  double t_min_line = 0.5 - fg_t_line_tolerance;
  double t_max_line = t_lines + 0.4999 + fg_t_line_tolerance;
  FGFov  t_fov(t_min_samp, t_min_line, (t_max_samp-t_min_samp), (t_max_line-t_min_line));

  // Select the newer radial or earlier grid method. You gotta choose the
  // Grid option correctly or you will get the radial algorithm
  QString fg_algorithm = m_parameters.get("FastGeomAlgorithm", "Radial").toLower();
  bool radial_method = ( "grid" != fg_algorithm );
  logit <<   "  FastGeomAlgorithm:            " << fg_algorithm << "\n";
  logit <<   "  FastGeomPoints:               " << m_fastpts << "\n";
  logit <<   "  FastGeomTolerance:            " << m_tolerance << "\n";
  logit <<   "  FastGeomQuerySampleTolerance: " << fg_q_sample_tolerance << "\n";
  logit <<   "  FastGeomQueryLineTolerance:   " << fg_q_line_tolerance << "\n";
  logit <<   "  FastGeomTrainSampleTolerance: " << fg_t_sample_tolerance << "\n";
  logit <<   "  FastGeomTrainLineTolerance:   " << fg_t_line_tolerance << "\n\n";

  // Only the good points to matrix solver
  std::vector<FGPoint>  q_infov_points, t_infov_points;
  int n_total_points = 0;

  // Run the requested algorithm
  if ( radial_method ) {

    n_total_points = radial_algorithm(query, train, q_fov, t_fov, m_parameters,
                                      q_infov_points, t_infov_points, logger);

  }
  else { // grid_method

    n_total_points = grid_algorithm(query, train, q_fov, t_fov, m_parameters,
                                    q_infov_points, t_infov_points, logger);

  }

  //// Latitude/Longitude mapping complete! Check status

  // Log results
  logit <<   "\n==> Geometric Correspondence Mapping complete <==\n";
  logit <<   "  TotalPoints:       " << n_total_points << "\n";
  logit.flush();

  // Compute homography if enough point ater in common FOVs of both images,
  // otherwise we report failure
  if ( n_total_points < m_fastpts ) {
    std::string mess = "Failed to get FOV geometry mapping for " + train.name() +
                    " to " + query.name() + " needing " + QString::number(m_fastpts) +
                    " but got " + QString::number(n_total_points) +" in train FOV.";
    logit << ">>> ERROR - " << mess << "\n";
    throw IException(IException::Programmer, mess, _FILEINFO_);
  }

  // Compute homography tranformation. Note the order of the point arrays.
  // This method computes the train to query transform. The inverse will
  // provide sample,line translations from query-to-train sample,line
  // coordinates
  std::vector<uchar> t_inliers;
  mapper = getTransformMatrix(t_infov_points, q_infov_points, t_inliers, m_tolerance, logger);

  // Report the transformation matrix
  logit <<   "\n  MatrixTransform:   \n";
  for (int i = 0 ; i < mapper.rows ; i++) {
    logit << "    ";
    QString comma("");
    for (int j = 0 ; j < mapper.cols ; j++) {
      logit << comma << mapper.at<double>(i,j);
      comma = ",";
    }
    logit << "\n";
  }
  logit << "\n";

  // The above matrix is for simply computing the direct fastgeom of the
  // training image into the image space of the query image, just like
  // cam2cam does.
  //
  // Now consider cropping to only the minimum common coverage or
  // preserving all the train image in the transformation (this option can
  // be really big and is not recommended for some situations!).

  // Set up the transform pointer for applying the desired affect. Do
  // preserve option first.
  QScopedPointer<GenericTransform> fastg(0);
  if ( "map" == m_geomtype ) {
    // Compute preserved image output size and translation matrix
    cv::Mat tMat;
    RectArea tSizeFull = ImageTransform::transformedSize(mapper,
                                                         tSize.size(),
                                                         tMat);

    // If it is within the area tolerance use it, otherwise just use the
    // same size as the output image below.
    if ( tSizeFull.area()  < (m_maxarea * qSize.area()) ) {
      fastg.reset(new GenericTransform("FastGeomMap", mapper,
                                       tSizeFull));
    }
  }
  else if ( "crop" == m_geomtype ) {

    // Crop train image to only common area in query image using inverse
    RectArea qbbox = ImageTransform::boundingBox( GenericTransform::computeInverse(mapper),
                                                  qSize,
                                                  tSize.size());


    // std::cout << "Compute query to train BB...\n";
    RectArea tbbox = ImageTransform::boundingBox( mapper, qbbox,
                                                   qSize.size());

    fastg.reset(new GenericTransform("FastGeomCrop", mapper, tbbox));
  }

 // If a mapper has not been allocated, allocate mapping to query image size
 // (as cam2cam does).
  if ( fastg.isNull() ) {
    fastg.reset(new GenericTransform("FastGeomCamera", mapper, qSize.size()));
  }

  return ( fastg.take() );
}

/**
 * @brief Radial algorithm to compute query/train geometric mapping correspondences
 *
 * This method computes a radial algorithm that can be configured by the user to
 * compute geometric correspondence between the query and train images. This data
 * will be used to compute a perspective homography matrix suitable to tranform the
 * train image into the query image space for spatial comparisons.
 *
 * @param query           Query image
 * @param train           Train image
 * @param q_fov           Query image detector FOV in lines/samples that could be
 *                          expanded
 * @param t_fov           Train image detector FOV in lines/samples that could be
 *                          expanded
 * @param parameters      Algorithm parameters that modifies processing
 * @param q_infov_points  Vector of line/sample image coordinates in query image
 * @param t_infov_points  Vector of line/sample image coordinates in train image
 *                          that geometrically maps to the same coordinates in the
 *                          query image
 * @param logger          Optional logging stream to report algorithm details
 * @return int            Total number of points in correspondence vectors
 */
int FastGeom::radial_algorithm(MatchImage &query, MatchImage &train,
                               const FastGeom::FGFov &q_fov,
                               const FastGeom::FGFov &t_fov,
                               const PvlFlatMap &parameters,
                               std::vector<FastGeom::FGPoint> &q_infov_points,
                               std::vector<FastGeom::FGPoint> &t_infov_points,
                               QLogger logger) const {


  // Shortcut reference directly to output stream method
  QDebugStreamType &logit = logger.logger();

  logit << "--> Using Radial Algorithm train-to-query mapping <--\n";

  // Compute maximum radial track at the center of the image to
  // the corner and scaling parameters
  double fg_max_radius    = std::sqrt( (q_fov.width * q_fov.width) + ( q_fov.height * q_fov.height) ) / 2.0;
  double fg_radial_seglen = toDouble(parameters.get("FastGeomRadialSegmentLength", "25.0"));
  double fg_point_count   = toDouble(parameters.get("FastGeomRadialPointCount", "5.0"));
  double fg_point_factor  = toDouble(parameters.get("FastGeomRadialPointFactor", "1.0"));

  // Ensure the number of rings
  if ( fg_radial_seglen <= 1.0 ) fg_radial_seglen = 1.5;
  int fg_number_rings = std::ceil( fg_max_radius / fg_radial_seglen );

  // See if user wants to secify the number of rings
  if  ( parameters.exists("FastGeomRadialSegments") ) {
    fg_number_rings = std::ceil(toDouble(parameters.get("FastGeomRadialSegments")));
  }

  // Ensure a reasonable number of rings are computed
  if ( fg_number_rings <= 1 ) fg_number_rings = 2;  // Center and one ring

  // Got to be >= 3 or risk getting colinear points
  if ( fg_point_count < 3) fg_point_count = 3;

  // Lets report what we got
  logit <<   "  FastGeomMaximumRadius:         " << fg_max_radius << "\n";
  logit <<   "  FastGeomRadialSegmentLength:   " << fg_radial_seglen << "\n";
  logit <<   "  FastGeomRadialPointCount:      " << fg_point_count << "\n";
  logit <<   "  FastGeomRadialPointFactor:     " << fg_point_factor << "\n";
  logit <<   "  FastGeomRadialSegments:        " << fg_number_rings << "\n";
  logit.flush();

  // Center of query image
  double c_x = q_fov.width / 2.0;
  double c_y = q_fov.height / 2.0;

  // Set up line/sample mapping correspondence between images
  std::vector<FGPoint>      q_points, t_points;
  std::vector<SurfacePoint> q_surface_points;
  std::vector<bool>         t_inFOV;

  // Let's track some numbers
  int n_total_points = 0;
  int n_image_points = 0;
  int n_mapped_points = 0;
  int n_train_fov = 0;

  // Clear point correspondence arrays
  q_infov_points.clear();
  t_infov_points.clear();

  double ring_radius = 0.0;
  for ( int ring = 0 ; ring < fg_number_rings ; ring++, ring_radius+=fg_radial_seglen ) {

    // Compute the number points / ring. First loop handles the degenerate case of
    // a single point at the center of the image.
    int rpoints = int(std::ceil( fg_point_count + ( fg_point_count * fg_point_factor) *
                                                    (double) (ring-1)) );
    if ( rpoints < 1 ) rpoints = 1;

    double d_theta = ( 360.0 / rpoints );

    // Compute the points on the circle by d_theta increments
    // double r_angle = d_theta / 2.0;
    double r_angle = 0.0;
    for ( int p = 0 ; p < rpoints ; p++, r_angle+=d_theta ) {

      // Potential point
      n_total_points++;

      // Convert angle to radians and compute point on the unit circle at the
      // current radius in a counterclockwise direction.
      double theta = r_angle * DEG2RAD;
      FGPoint q_coord( (c_x + (std::cos(theta) * ring_radius)),
                       (c_y - (std::sin(theta) * ring_radius)) );

      // Check to see if the point is in the image FOV
      if ( q_fov.contains(q_coord) ) {
        n_image_points++;

        // Convert the point and if valid add to list
        double t_x, t_y, t_r;
        SurfacePoint q_surfpt = query.source().getLatLon(q_coord.y, q_coord.x);
        if ( train.source().getLineSamp(q_surfpt, t_y, t_x, t_r) ) {
          n_mapped_points++;

          // Query q_coord set above
          FGPoint t_coord(t_x, t_y);
          q_points.push_back(q_coord);
          q_surface_points.push_back(q_surfpt);
          t_points.push_back(t_coord);

          // Test for those falling in train FOV
          bool inFOV = t_fov.contains(t_coord);
          t_inFOV.push_back(inFOV);
          if ( inFOV ) {
            n_train_fov++;

            // Save the good points
            q_infov_points.push_back(q_coord);
            t_infov_points.push_back(t_coord);
          }

        }
      }
    }
  }

  // Log results
  logit <<   "\n==> Radial Point Mapping complete <==\n";
  logit <<   "  TotalPoints:     " << n_total_points << "\n";
  logit <<   "  ImagePoints:     " << n_image_points << "\n";
  logit <<   "  MappedPoints:    " << n_mapped_points << "\n";
  logit <<   "  InTrainMapFOV:   " << n_train_fov << "\n";
  logit.flush();

  // Dump the points. m_parameters will be used to determine if the
  // data is to be written to a file.
  dump_point_mapping(query, train, "radial", m_parameters,
                     q_points, t_points, q_surface_points,
                     t_inFOV, logger);

  return ( n_train_fov );
}

/**
 * @brief Grid algorithm to compute query/train geometric mapping correspondences
 *
 * This method computes a grid algorithm that can be configured by the user to
 * compute geometric correspondence between the query and train images. This data
 * will be used to compute a perspective homography matrix suitable to tranform the
 * train image into the query image space for spatial comparisons.
 *
 * @param query           Query image
 * @param train           Train image
 * @param q_fov           Query image detector FOV in lines/samples that could be
 *                          expanded
 * @param t_fov           Train image detector FOV in lines/samples that could be
 *                          expanded
 * @param parameters      Algorithm parameters that modifies processing
 * @param q_infov_points  Vector of line/sample image coordinates in query image
 * @param t_infov_points  Vector of line/sample image coordinates in train image
 *                          that geometrically maps to the same coordinates in the
 *                          query image
 * @param logger          Optional logging stream to report algorithm details
 * @return int            Total number of points in correspondence vectors
 */
int FastGeom::grid_algorithm(MatchImage &query, MatchImage &train,
                              const FastGeom::FGFov &q_fov,
                              const FastGeom::FGFov &t_fov,
                              const PvlFlatMap &parameters,
                              std::vector<FGPoint> &q_infov_points,
                              std::vector<FGPoint> &t_infov_points,
                              QLogger logger) const {

  // Shortcut reference directly to output stream method
  QDebugStreamType &logit = logger.logger();

  logit << "--> Using Grid Algorithm train-to-query mapping <--\n";

  // Compute increment
  int   fg_minpts = qMax(m_fastpts, 16);
  int increment = std::ceil(std::sqrt( qMax(24.0, (double) fg_minpts) ));

  double fg_max_axis = qMax( qMax(q_fov.width, q_fov.height), qMax(t_fov.width, t_fov.height) );
  int v_max_iter     = int( fg_max_axis / 2.0 );

  int fg_grid_start_iter = toInt(  parameters.get("FastGeomGridStartIteration", "0") );
  int fg_grid_stop_iter  = toInt(  parameters.get("FastGeomGridStopIteration", toString(v_max_iter)) );
  int fg_grid_iter_step  = toInt(  parameters.get("FastGeomGridIterationStep", "1") );
  bool fg_save_all       = toBool( parameters.get("FastGeomGridSaveAllPoints", "false") );

  logit <<   "  FastGeomGridStartIteration: " << fg_grid_start_iter << "\n";
  logit <<   "  FastGeomGridStopIteration:  " << fg_grid_stop_iter << "\n";
  logit <<   "  FastGeomGridIterationStep:  " << fg_grid_iter_step << "\n";
  logit <<   "  FastGeomGridSaveAllPoints:  " << toString(fg_save_all) << "\n";
  logit <<   "  FastGeomPointIncrement:     " << increment << "\n";

  // Set up line/sample mapping correspondence between images
  std::vector<FGPoint>      q_points, t_points;
  std::vector<SurfacePoint> q_surface_points;
  std::vector<bool>         t_inFOV;

  // Clear point correspondence arrays
  q_infov_points.clear();
  t_infov_points.clear();

  // Let's track some numbers
  int n_total_points = 0;
  int n_image_points = 0;
  int n_mapped_points = 0;
  int n_train_fov = 0;

  // Loop control
  bool done = false;
  int iteration = fg_grid_start_iter;
  int currinc = 0;
  int n_iterations = 0;

  // Density grid point loop. First interation=0 will produce just enough
  // grid points (>=m_fastpts)
  while ( (iteration < fg_grid_stop_iter) && ( !done) ) {

    currinc = increment + ( iteration * 2 );
    n_iterations++;

    double sSpacing = qMax( 1.0, q_fov.width/(currinc*1.0) );
    double lSpacing = qMax( 1.0, q_fov.height/(currinc*1.0) );

    if ( qMax( sSpacing, lSpacing ) <= 1.0 ) done = true; // Last possible loop

    // Save all points computed if requested by user
    if ( !fg_save_all ) {
      q_points.clear();
      t_points.clear();
      t_inFOV.clear();
      q_surface_points.clear();
    }

    // Clear these for the matrix transform
    q_infov_points.clear();
    t_infov_points.clear();
    n_train_fov = 0;

    // Run the grid spacing loop
    for (int l = 0 ; l < currinc ; l++) {
      for ( int s = 0 ; s < currinc ; s++ ) {

        // Total point count
        n_total_points++;

        // Check to see if the point is in the image FOV
        FGPoint q_coord( ((sSpacing / 2.0 + sSpacing * s + 0.5) + q_fov.x),
                         ((lSpacing / 2.0 + lSpacing * l + 0.5) + q_fov.y) );
        if ( q_fov.contains(q_coord) ) {
          n_image_points++;

          // Convert the point and if valid add to list
          double t_x, t_y, t_r;
          SurfacePoint q_surfpt = query.source().getLatLon(q_coord.y, q_coord.x);
          if ( train.source().getLineSamp(q_surfpt, t_y, t_x, t_r) ) {
            n_mapped_points++;

            // Query q_coord set above
            FGPoint t_coord(t_x, t_y);
            q_points.push_back(q_coord);
            q_surface_points.push_back(q_surfpt);
            t_points.push_back(t_coord);

            // Test for those falling in train FOV
            bool inFOV = t_fov.contains(t_coord);
            t_inFOV.push_back(inFOV);
            if ( inFOV ) {
              n_train_fov++;

              // Save the good points
              q_infov_points.push_back(q_coord);
              t_infov_points.push_back(t_coord);
            }

          }
        }
      }
    }

    // If on this iteration we have enough points we are done
    if ( n_train_fov >= m_fastpts ) done = true;

    // Next iteration + step
    iteration += fg_grid_iter_step;
  }

  // Log results
  logit <<   "\n==> Grid Point Mapping complete <==\n";
  logit <<   "  FastGeomTotalGridIterations:  " << n_iterations << "\n";
  logit <<   "  TotalPoints:                  " << n_total_points << "\n";
  logit <<   "  ImagePoints:                  " << n_image_points << "\n";
  logit <<   "  MappedPoints:                 " << n_mapped_points << "\n";
  logit <<   "  InTrainMapFOV:                " << n_train_fov << "\n";
  logit.flush();

  // Dump the points. Method wil use the m_parameters to determine if the
  // data is written to a file.
  dump_point_mapping(query, train, "grid", m_parameters,
                     q_points, t_points, q_surface_points,
                     t_inFOV, logger);

  return ( n_train_fov );

}

/**
 * @brief Compute and apply the FastGeom matrix transformation
 *
 * This method computes and adds the FastGeom matrix transformation from
 *  the train image to the query image space. The underlying process may
 *  throw an error that would prevent the transform being added to the
 * processing chain of the train image. Callers should trap these errors
 * and handle as appropriate.
 *
 * @param query Query (MATCH) image
 * @param train Train (FROM/FROMLIST) image
 * @param logger Logging stream to report process
 */
void FastGeom::apply(MatchImage &query, MatchImage &train, QLogger logger) const {
  // Add the fastgeom mapping transform
  train.addTransform( compute(query, train, logger) );
  return;
}

/**
 * @brief Compute the transformation matrix from set of geometric mapping points
 *
 * This method will compute the transformation matrix from the set of
 * corresponding geometric points in query and train images. See @compute().
 *
 * @param querypts Sample/Line points in query image
 * @param trainpts Sample/Line points in train image
 * @param inliers  Returns mask of points used in martrix computation. Values
 *                   of false indicate an outlier
 * @param tolerance Outlier tolerance in pixels
 * @param logger    Output stream to report any processing details
 * @return cv::Mat  The transform
 */
cv::Mat FastGeom::getTransformMatrix(const std::vector<FastGeom::FGPoint> &querypts,
                                     const std::vector<FastGeom::FGPoint> &trainpts,
                                     std::vector<uchar> &inliers,
                                     const double tolerance,
                                     QLogger logger) {

  QDebugStreamType &logit = logger.logger();

  logit << "\n--> Running Homography Image Transform <---\n";

  logit << "  IntialPoints:       " << querypts.size() << "\n";
  logit << "  Tolerance:          " << tolerance << "\n";

  // Wrap OpenCV code in try/catch for any errors that may occur
  cv::Mat mapper;
  try {
    // Find homography using Least-Median robust method algorithm
    inliers.assign(querypts.size(), 0);
    mapper = cv::findHomography(querypts, trainpts, cv::LMEDS, tolerance, inliers);

    // Using the Least median method requires > 50% inliers.  Check that here.
    int nInliers(0);
    for (unsigned int i = 0 ; i < inliers.size() ; i++) {
      if ( 0 != inliers[i] )  {
        nInliers++;
      }
    }

    logit << "  TotalLmedsInliers:  " << nInliers << "\n";

    // If we have < 50% inliners, compute the RANSAC homography
    double inlierPercent = ((double) nInliers / (double) querypts.size()) * 100.0;
    logit << "  PercentPassing: " << inlierPercent << "\n";

    if ( 50.0 > inlierPercent ) {
      logit << "  LMEDS failed w/less than 50% inliers - computing RANSAC homography!\n";
      mapper = cv::findHomography(querypts, trainpts, cv::RANSAC, tolerance, inliers);

      int r_nInliers(0);
      for (unsigned int i = 0 ; i < inliers.size() ; i++) {
        if ( 0 != inliers[i] )  r_nInliers++;
      }
      logit << "  TotalRansacInliers: " << r_nInliers << "\n";

    }

    // Check for invalid/empty matrix which indicates OpenCV error
    if ( mapper.empty() ) {
      std::string msg = "Error computing homography matrix";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

  }
  catch ( std::exception &e ) {
    // This will also catch any ISIS error
    std::string msg = "Matrix transform error: " + QString(e.what());
    throw IException(IException::Programmer, msg, _FILEINFO_);
  }

  return (mapper);
}


/** Get parameters currently being used   */
PvlFlatMap FastGeom::getParameters() const {
  return (m_parameters);
}

/**
 * @brief Dump correspondence mapping data to file
 *
 * This method will write the details of the geometric correspondence mapping
 * computed by the grid or radial algorithm. The data is intended to produce
 * all points computed including points in the query image that did not
 * successfully map into the train image (t_inFOV).
 *
 *
 * @param query           Query image
 * @param train           Train image
 * @param method          Method (radial, grid) used to generate data. This
 *                           string is added to the output file name
 * @param parameters      Algorithm parameters that modifies processing
 * @param q_points        Vector of all line/sample image coordinates in query image
 * @param t_points        Vector of all line/sample image coordinates in train image
 *                          that geometrically maps to the same coordinates in the
 *                          query image
 * @param q_surface_points Surface point computed at all points. The latitude and
 *                           longitudes of the points are reported if valid
 *                           otherwise, NULLs are written.
 * @param t_inFOV         Boolean vector corresponding to valid mappings in
 *                           point arrays.
 * @param logger          Optional logging stream to report algorithm details
 */
void FastGeom::dump_point_mapping(MatchImage &query, MatchImage &train,
                      const QString &method, const PvlFlatMap &parameters,
                      const std::vector<FastGeom::FGPoint> &q_points,
                      const std::vector<FastGeom::FGPoint> &t_points,
                      const std::vector<SurfacePoint> &q_surface_points,
                      const std::vector<bool> &t_inFOV,
                      QLogger logger) const {

 // Lets dump the data if requested
  if ( toBool(m_parameters.get("FastGeomDumpMapping","False")) ) {

      // Shortcut reference directly to output stream method
    QDebugStreamType &logit = logger.logger();

    logit << "\n--> Dumping " << method << " points <---\n";

    FileName q_file( query.name().toStdString() );
    FileName t_file( train.name().toStdString() );

    QString csvout = QString::fromStdString(q_file.baseName()) + "_" + QString::fromStdString(t_file.baseName()) + "." +
                     method + ".fastgeom.csv";
    logit <<   "  PointDumpFile:     " << csvout << "\n";

    // Open output file for writing
    QDebugStream csvstrm = QDebugLogger::create( csvout,
                                                 (QIODevice::WriteOnly |
                                                 QIODevice::Truncate) );
    QDebugStreamType &csv(csvstrm->dbugout());

    // Write the header
    csv << "QuerySample,QueryLine,TrainSample,TrainLine,"
        << "Latitude,Longitude,Radius,X,Y,Z,InTrainFOV\n";

    // Write the data
    logit <<   "  TotalPoints:       " << q_points.size() << "\n";
    for ( unsigned int i = 0 ; i < q_points.size() ; i++) {
      const SurfacePoint &srfpt( q_surface_points[i] );

      csv << q_points[i].x << ", " << q_points[i].y << ", "
          << t_points[i].x << ", " << t_points[i].y << ", "
          << srfpt.GetLatitude().degrees() << ", "
          << srfpt.GetLongitude().degrees() << ", "
          << srfpt.GetLocalRadius().meters() << ", "
          << srfpt.GetX().meters() << ", "
          << srfpt.GetY().meters() << ", "
          << srfpt.GetZ().meters() << ", "
          << (t_inFOV[i] ? "True" : "False") << "\n";
    }
  }

  return;
}

/**
 * @brief Checks for valid fast geom transform options
 *
 * This method checks that geomtype is "camera", "crop" or "map". If some other
 * value is found, an exception is thrown.
 *
 * @param geomtype String specification of geom type - must be camera, crop or
 *                 map
 */
void FastGeom::validate( const QString &geomtype ) const {
  QStringList options;
  options << "camera" << "crop" << "map";
  if ( !options.contains(geomtype, Qt::CaseInsensitive)  ) {
    std::string mess = "FastGeom - invalid GEOMTYPE (" + geomtype + ")!"
                   " Must be CAMERA, CROP or MAP.";
    throw IException(IException::Programmer, mess, _FILEINFO_);
  }
}

}  // namespace Isis
