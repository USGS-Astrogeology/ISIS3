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

#include <QDebug>
#include <QtDebug>
#include <QList>
#include <QStringList>
#include <QTime>

#include <opencv2/opencv.hpp>

// boost library
#include <boost/foreach.hpp>

#include "Application.h"
#include "IException.h"
#include "IString.h"
#include "FileName.h"
#include "RobustMatcher.h"


namespace Isis {

/** Setup a default robust matcher with SURF detector/extractor and
 *  BFMatcher matcher with default parameters to each element
 *
 *  @see init(const QString &name, const PvlFlatMap(), const bool
 *       useDefaults(false)).
 */
RobustMatcher::RobustMatcher() : MatcherAlgorithms(), QLogger(),
                                 m_name("RobustMatcher"), m_parameters() {
  init();
}


RobustMatcher::RobustMatcher(const QString &name) :
                             MatcherAlgorithms(),
                             QLogger(),m_name(name),
                             m_parameters() {
  init();
}

RobustMatcher::RobustMatcher(const QString &name,
                             const MatcherAlgorithms &algorithms,
                             const PvlFlatMap &parameters,
                             const QLogger &logger) :
                             MatcherAlgorithms(algorithms),QLogger(logger),
                             m_name(name), m_parameters() {
  init(parameters);
}


/** Destructor requires no actions */
RobustMatcher::~RobustMatcher() { }

/**
 * @brief Construct required interface with match image pairs
 *
 * @author 2015-10-03 Kris Becker
 *
 * @param query    Query image
 * @param trainer  Train image
 *
 * @return MatchPair Returns the results of the matcher
 */
MatchPair RobustMatcher::match(cv::Mat& query, cv::Mat& train) const {
  MatchImage v_query(ImageSource("Query", query, "Query"));
  MatchImage v_train(ImageSource("Train", train, "Train"));
  return ( match(v_query, v_train) );
}

/**
 * @brief Construct required interface for multi-matching images
 *
 * @author 2015-10-03 Kris Becker
 *
 * @param query    Query image
 * @param trainers List of train images
 *
 * @return MatchPairQList List of matched pairs
 */
MatchPairQList RobustMatcher::match(cv::Mat& query,
                                    std::vector<cv::Mat>& trainers) const {

  MatchImage v_query(ImageSource("Query", query, "Query"));
  MatchImageQList v_trainers;
  for (unsigned int i = 0 ; i < trainers.size() ; i++) {
    QString id = "Train[" + QString::number(i) + "]";
    v_trainers.push_back(MatchImage(ImageSource(id, trainers[i], id)));
  }
  return ( match(v_query, v_trainers) );
}


// Match feature points for image pair sources using robust outlier detection
MatchPair RobustMatcher::match(MatchImage &query, MatchImage &train) const {

   // Announce our intensions
  if ( isDebug() ) {
     logger() << "\n@@ matcher-pair started on " << Application::DateTime() << "\n";
     logger() << "\n+++++++++++++++++++++++++++++\n";
     logger() << "Entered RobustMatcher::match(MatchImage &query, MatchImage &trainer)...\n";
     logger() << "  Specification:   " << name() << "\n";
     logger().flush();
   }

   const bool onErrorThrow = false;  // Conditions for managed matching

   // Setup
   MatchImage v_query = query.clone();
   MatchImage v_train = train.clone();
   MatchPair v_pair(v_query, v_train);

   // Render images for matching
   cv::Mat i_query = v_query.image();
   cv::Mat i_train = v_train.image();

   if ( toBool(m_parameters.get("SaveRenderedImages")) ) {
     QString savepath = m_parameters.get("SavePath");

     FileName qfile(v_query.source().name());
     QString qfout = savepath + "/" + qfile.baseName() + "_query.png";
     FileName oqfile(qfout);
     imwrite( oqfile.expanded().toStdString(), i_query);

     FileName tfile(v_train.source().name());
     QString tfout = savepath + "/" + tfile.baseName() + "_train.png";
     FileName otfile(tfout);
     imwrite( otfile.expanded().toStdString(), i_train);
   }

   if ( isDebug() ) {
     logger() << "**  Query Image:   " << v_query.name() << "\n";
     logger() << "       FullSize:     (" << v_query.source().samples() << ", "
                                          << v_query.source().lines() << ")\n";
     logger() << "       Rendered:     (" << i_query.cols << ", "
                                          << i_query.rows << ")\n";
     logger() << "**  Train Image:   " << v_train.name() << "\n";
     logger() << "       FullSize:     (" << v_train.source().samples() << ", "
                                          << v_train.source().lines() << ")\n";
     logger() << "       Rendered:     (" << i_train.cols << ", "
                                          << i_train.rows << ")\n";
     logger() << "--> Feature detection...\n";
     logger().flush();
  }

   // Do not include in timer up to here
   QTime stime;
   stime.start();

   // 1a. Detection of the features
   detector().algorithm()->detect(i_query, v_query.keypoints());
   detector().algorithm()->detect(i_train, v_train.keypoints());

   int v_query_points = v_query.size();
   int v_train_points = v_train.size();
   int allPoints = v_query_points + v_train_points;

   // Limit keypoints if requested by user
   int v_maxpoints = toInt(m_parameters.get("MaxPoints"));
   if ( v_maxpoints > 0 ) {
     logger() << "  Keypoints restricted by user to " << v_maxpoints << " points...\n";
     logger().flush();
     cv::KeyPointsFilter::retainBest(v_query.keypoints(), v_maxpoints);
     cv::KeyPointsFilter::retainBest(v_train.keypoints(), v_maxpoints);
   }

   double v_time = elapsed(stime);  // Event timing

   // Log results
   if ( isDebug() ) {
     logger() << "  Total Query keypoints:    " << v_query.size()
              << " [" << v_query_points << "]\n";
     logger() << "  Total Trainer keypoints:  " << v_train.size()
              << " [" << v_train_points << "]\n";
     logger() << "  Processing Time:          " << v_time << "\n";
     logger() << "  Processing Keypoints/Sec: "
               << (double) allPoints / v_time << "\n";
     logger() << "--> Extracting descriptors...\n";
     logger().flush();
   }

   // 1b. Extraction of the descriptors
   cv::Mat queryDescriptors, trainerDescriptors;
   extractor().algorithm()->compute(i_query, v_query.keypoints(), v_query.descriptors());
   extractor().algorithm()->compute(i_train, v_train.keypoints(), v_train.descriptors());
   double d_time = elapsed(stime) - v_time;
   v_pair.addTime( v_time + d_time );

   // Do root sift normalization if requested
   bool v_doRootSift = toBool(m_parameters.get("RootSift"));
   if ( v_doRootSift ) {
     if ( isDebug() ) {
       logger() << "  Computing RootSift Descriptors...\n";
     }
     RootSift( v_query.descriptors() );
     RootSift( v_train.descriptors() );
   }

   if ( isDebug() ) {
     logger() <<     "  Processing Time(s):         " << d_time << "\n";
     logger() <<     "  Processing Descriptors/Sec: "
               << (double) allPoints / d_time << "\n";
     logger() << "\n*Removing outliers from image pairs\n";
     logger().flush();
   }


   // OUTLIER DETECTION!!!
   // 2, 3, 4,  5, 6: Apply ratio (2) and symmetric (3) tests, then apply
   // RANSAC homography (4) outlier followed by epipoloar (5) and final
   // homography (6)
    try {
     double mtime;
     cv::Mat homography, fundamental;
     removeOutliers(v_query.descriptors(), v_train.descriptors(),
                    v_query.keypoints(), v_train.keypoints(),
                    v_pair.homography_matches(), v_pair.epipolar_matches(),
                    v_pair.matches(), homography, fundamental,
                    mtime, onErrorThrow);

     v_pair.setFundamental(fundamental);
     v_pair.setHomography(homography);
     v_pair.addTime(mtime);
   }
   catch ( cv::Exception &c ) {
     QString mess = "Outlier removal process failed on image pair: "
                    " Query: " + v_query.name() +
                    ", Train: " + v_train.name() +
                    ".  CV::Error - " + c.what();
     // throw IException(IException::Programmer, mess, _FILEINFO_);
     v_pair.addError(mess);
     if ( isDebug() ) {
       logger() << "  Outlier Error = "
                 << v_pair.getError(v_pair.errorCount()-1) << "\n";
       logger().flush();
     }
   }
   catch ( IException &ie) {
     QString mess = "Outlier removal process failed on Query/Train image pair "
                    " Query: "  + v_query.name() +
                    ", Train: " + v_train.name();
     // throw IException(ie, IException::Programmer, mess, _FILEINFO_);
     v_pair.addError(mess);
     if ( isDebug() ) {
       logger() << "  Outlier Error = "
                 << v_pair.getError(v_pair.errorCount()-1) << "\n";
       logger().flush();
     }
   }

   // All done...
  if ( isDebug() ) {
    logger() << "%% match-pair complete in " << elapsed(stime) << " seconds!\n\n";
    logger().flush();
  }

   // v_pair->addTime(mtime);  // Not added to time unless uncommented
   return ( v_pair );
 }


// Match multiple images to the query image using robust outlier detection.
MatchPairQList RobustMatcher::match(MatchImage &query,
                                    MatchImageQList &trainers) const {

   // Announce our intensions
  if ( isDebug() ) {
     logger() << "\n@@ matcher-multi started on " << Application::DateTime() << "\n";
     logger() << "\n+++++++++++++++++++++++++++++\n";
     logger() << "Entered RobustMatcher::match(MatchImage &query,""MatchImageList &trainer)...\n";
     logger() << "  Specification:   " << name() << "\n";
     logger().flush();
  }

  if (trainers.size() == 0 ) {
    QString mess = "No trainer images provided!!";
    if (isDebug() ) logger() << "  " << mess << "\n";
    throw IException(IException::Programmer, mess, _FILEINFO_);
  }

   bool onErrorThrow = false;  // Conditions for managed matching

   // Setup
   MatchImage &v_query = query;
   MatchImageQList &v_trainers = trainers;


  // Create rendered trainer images for matching
  // Render images for efficiency
   cv::Mat i_query = v_query.image();
   std::vector<cv::Mat> i_trainers;
   bool saveRendered = toBool(m_parameters.get("SaveRenderedImages"));
   QString savepath = m_parameters.get("SavePath");

   if ( true == saveRendered ) {
     // Save the query image first
     FileName qfile(v_query.source().name());
     QString qfout = savepath + "/" + qfile.baseName() + "_query.png";
     FileName oqfile(qfout);
     imwrite( oqfile.expanded().toStdString(), i_query);
   }

   // Now process the rest of the trainer images
   for (int i = 0 ; i < v_trainers.size() ; i++) {
     i_trainers.push_back(v_trainers[i].image());

     if ( true == saveRendered ) {
       FileName tfile(v_trainers[i].source().name());
       QString tfout = savepath + "/" + tfile.baseName() + "_train.png";
       FileName ofile(tfout);
       imwrite( ofile.expanded().toStdString(), i_trainers[i]);
     }
   }

   if ( isDebug() ) {
     logger() << "**  Query Image:   " << v_query.name() << "\n";
     logger() << "       FullSize:     (" << v_query.source().samples() << ", "
                                          << v_query.source().lines() << ")\n";
     logger() << "       Rendered:     (" << i_query.cols << ", "
                                          << i_query.rows << ")\n";

     logger() << "v^v Matching " << v_trainers.size() << " trainer images.\n";
     for (int i = 0 ; i < v_trainers.size() ; i++) {
       logger() << "**  Train Image[" << i <<  "] " <<  v_trainers[i].name()  << "\n";
       logger() << "       FullSize:     (" << v_trainers[i].source().samples() << ", "
                                            << v_trainers[i].source().lines() << ")\n";
       logger() << "       Rendered:     (" << i_trainers[i].cols << ", "
                                              << i_trainers[i].rows << ")\n";
     }
     logger() << "--> Feature detection...\n";
     logger().flush();
   }

   // Start timer
   QTime stime;
   stime.start();

   // 1a. Run detection of features
   std::vector<std::vector<cv::KeyPoint> > trainerKeypoints;
   detector().algorithm()->detect(i_query, v_query.keypoints());
   detector().algorithm()->detect(i_trainers, trainerKeypoints);

   int v_query_points = v_query.size();
   int allPoints = v_query_points;
   QList<int> v_train_points;
   for (unsigned int i = 0; i < trainerKeypoints.size() ; i++) {
     v_train_points.append(trainerKeypoints[i].size());
     allPoints += trainerKeypoints[i].size();
   }

   // Limit keypoints if requested by user
   int v_maxpoints = toInt(m_parameters.get("MaxPoints"));
   if ( v_maxpoints > 0 ) {
     logger() << "  Keypoints restricted by user to " << v_maxpoints << " points...\n";
     logger().flush();
     cv::KeyPointsFilter::retainBest(v_query.keypoints(), v_maxpoints);
     for (unsigned int i = 0 ; i < trainerKeypoints.size() ; i++) {
       cv::KeyPointsFilter::retainBest(trainerKeypoints[i], v_maxpoints);
     }
   }

   double d_time = elapsed(stime);

   // Prep for computing an accurate duration
   double allKeypoints = v_query.size();
   for (unsigned int k = 0 ; k < trainerKeypoints.size() ; k++) {
     allKeypoints += trainerKeypoints[k].size();
   }

   if ( isDebug() ) {
     logger() << "  Total Query keypoints:    " << v_query.size()
              << " [" << v_query_points << "]\n";
     logger() << "  Total Trainer keypoints:  " << trainerKeypoints.size()
               << " @ (";
     QString sep("");
     for ( unsigned int t = 0 ; t < trainerKeypoints.size() ; t++ ) {
       logger() << sep << trainerKeypoints[t].size()
                 << " [" << v_train_points[t] << "]";
       sep = ",";
     }
     logger() << ") = " << allKeypoints - v_query.size() << "\n";
     logger() << "  Total keypoints:          " << allKeypoints << "\n";
     logger() << "  Processing Time:          " << d_time << "\n";
     logger() << "  Processing Keypoints/Sec: "
               << allPoints / d_time  << "\n";
     logger() << "--> Extracting descriptors...\n";
     logger().flush();
   }

   // 1b. Extraction of the descriptors
   std::vector<cv::Mat> trainerDescriptors;
   extractor().algorithm()->compute(i_query, v_query.keypoints(), v_query.descriptors());
   extractor().algorithm()->compute(i_trainers, trainerKeypoints, trainerDescriptors);

    // Record time to detect features and extract descriptors for all images
   double e_time = elapsed(stime) - d_time;

   // Update times for query and train images by distributing the total time
   // by the factor of image keypoints over sum of all keypoints
   v_query.addTime( (d_time  + e_time) * ( v_query.size() / allPoints ) );

   // Do root sift normalization if requested
   bool v_doRootSift = toBool(m_parameters.get("RootSift"));
   if ( v_doRootSift ) {
     if ( isDebug() ) {
       logger() << "  Computing RootSift Descriptors...\n";
     }
     RootSift( v_query.descriptors() );
     for (unsigned int i = 0; i < trainerDescriptors.size() ; i++) {
       RootSift( trainerDescriptors[i] );
     }
   }

   MatchPairQList pairs;
   for ( int i = 0 ; i < v_trainers.size() ; i++) {

     double kpRatio = (trainerKeypoints[i].size() / allPoints);
     MatchImage &v_train = v_trainers[i];

     // Compute the distributed time to train images
     double t_time = (d_time + e_time) * kpRatio;
     v_train.keypoints() = trainerKeypoints[i];
     v_train.setDescriptors(trainerDescriptors[i]);
     v_train.addTime(t_time);
     MatchPair v_pair(v_query, v_train);
     if ( isDebug() ) {
       logger() << "  Processing Time(s):         " << d_time * kpRatio << "\n";
       logger() << "  Processing Descriptors/Sec: "
                << (double) v_pair.keyPointTotal() / (d_time * kpRatio) << "\n";
       logger() << "\n*Removing outliers from image pairs:"
                << "\n *  Query: " << v_query.name()
                << "\n *  Train: " << v_train.name()
                << "\n";
       logger().flush();
     }

     try {
       // OUTLIER DETECTION!!!
       // 2, 3, 4,  5, 6: Apply ratio (2) and symmetric (3) tests, then apply
       // RANSAC homography (4) outlier followed by epipoloar (5) and final
       // homography (6)
       double mtime(0);
       cv::Mat homography, fundamental;
       removeOutliers(v_query.descriptors(), v_train.descriptors(),
                      v_query.keypoints(), v_train.keypoints(),
                      v_pair.homography_matches(), v_pair.epipolar_matches(),
                      v_pair.matches(), homography, fundamental,
                      t_time, onErrorThrow);

       v_pair.setFundamental(fundamental);
       v_pair.setHomography(homography);
       v_pair.addTime(mtime);
       pairs.push_back( v_pair );
     }
     catch ( cv::Exception &c ) {
       QString mess = "Outlier removal process failed on Query/Train image pair "
                      " Query=" + v_query.name() +
                      ", Train[" + QString::number(i) + "]: " + v_train.name() +
                      ".  cv::Error - " + c.what();
       // throw IException(IException::Programmer, mess, _FILEINFO_);
       v_pair.addError(mess);
       pairs.push_back( v_pair ) ;
       if ( isDebug() ) {
         logger() << "  Outlier Error = "
                   << v_pair.getError(v_pair.errorCount()-1) << "\n";
         logger().flush();
       }
     }
     catch ( IException &ie) {
       QString mess = "Outlier removal process failed on Query/Train image pair "
                      " Query=" + v_query.name() +
                      ", Train[" + QString::number(i) + "]: " + v_train.name();
       // throw IException(ie, IException::Programmer, mess, _FILEINFO_);
       v_pair.addError(mess);
       pairs.push_back( v_pair );
       if ( isDebug() ) {
         logger() << "  Outlier Error = "
                   << v_pair.getError(v_pair.errorCount()-1) << "\n";
         logger().flush();
       }
     }
   }

   // All done...
  if ( isDebug() ) {
    logger() << "%% match-multi complete in " << elapsed(stime) << " seconds!\n\n";
    logger().flush();
  }

  // return the match pair
  return ( pairs );

}


/**
 * @brief Apply ratio and symmetric outlier tests
 *
 * @author kbecker (8/21/2015)
 *
 * @param queryDescriptors
 * @param trainDescriptors
 * @param matches
 */
bool RobustMatcher::removeOutliers(const cv::Mat &queryDescriptors,
                                   const cv::Mat &trainDescriptors,
                                   std::vector<cv::KeyPoint>& queryKeypoints,
                                   std::vector<cv::KeyPoint>& trainKeypoints,
                                   std::vector<cv::DMatch> &homography_matches,
                                   std::vector<cv::DMatch> &epipolar_matches,
                                   std::vector<cv::DMatch> &matches,
                                   cv::Mat &homography, cv::Mat &fundamental,
                                   double &mtime, const bool onErrorThrow)
                                   const {

  // Introduce ourselves
  if ( isDebug() ) {
    logger() << "Entered RobustMatcher::removeOutliers(Mat &query, vector<Mat> &trainer)...\n";
    logger() << "--> Matching 2 nearest neighbors for ratio tests..\n";
    logger() << "  Query, Train Descriptors: " << queryDescriptors.rows
              << ", " << trainDescriptors.rows << "\n";
    logger().flush();
  }

  homography_matches.clear();
  epipolar_matches.clear();
  matches.clear();

  // Start the timer
  QTime stime;
  stime.start();
  double v_time;  // Ratio test timing

  // 2) Run two nearest neighbor matches for ratio test from query to train
  //    images based on k nearest neighbours (with k=2)
  std::vector<std::vector<cv::DMatch> > matches1;
  try {
    if ( isDebug() ) {
      logger() << "  Computing query->train Matches...\n";
      logger().flush();
    }
    matcher().algorithm()->knnMatch(queryDescriptors, trainDescriptors,
                                   matches1, // vector of matches (up to 2 per entry)
                                   2); // return 2 nearest neighbours
              }
  catch (cv::Exception &e) {
    QString mess = "RobustMatcher::MatcherFailed: "+ QString(e.what()) +
                   " - if its an assertion failure, you may be enabling "
                   "crosschecking with a BFMatcher. Must use a FlannBased "
                   "matcher if you want inherent ratio testing. "
                   "NOTE a ratio test is implemented in the RobustMatcher "
                   "so its not needed!";
   throw IException(IException::User, mess, _FILEINFO_);
  }

  // Only record the first matching test
  mtime = elapsed(stime);
  v_time = mtime;

  if ( isDebug() ) {
    logger() << "  Total Matches Found:   " << matches1.size() << "\n";
    logger() << "  Processing Time:       " << v_time << "\n";
    logger() << "  Matches/second:        " << (double) matches1.size() / v_time << "\n";
    logger().flush();
  }

  // 2a) Run two nearest neighbor matches for ratio test from train to query
  //    images based on k nearest neighbours (with k=2)
  std::vector<std::vector<cv::DMatch> > matches2;
  if ( isDebug() ) {
    logger() << "  Computing train->query Matches...\n";
    logger().flush();
  }

  matcher().algorithm()->knnMatch(trainDescriptors, queryDescriptors,
                      matches2, // vector of matches (up to 2 per entry)
                      2); // return 2 nearest neighbours
  v_time =  elapsed(stime) - mtime;
  // mtime = elapsed(stime);

  if ( isDebug() ) {
    logger() << "  Total Matches Found:   " << matches2.size() << "\n";
    logger() << "  Processing Time:       " << v_time << " <seconds>\n";
    logger() << "  Matches/second:        " << (double) matches2.size() / v_time << "\n";
    logger().flush();
  }

  // 2b) Remove matches for which NN ratio is than threshold clean image 1 ->
  // image 2 matches

  if ( isDebug() ) {
    logger() << " -Ratio test on query->train matches...\n";
    logger().flush();
  }

  // "removed" is unused variable, so it has been commented out
  /* int removed = */
  ratioTest(matches1,  v_time);
  // mtime += v_time;

  // clean image 2 -> image 1 matches
  if ( isDebug() ) {
    logger() << " -Ratio test on train->query matches...\n";
    logger().flush();
  }

  // "removed" is unused variable, so it has been commented out
  /* removed = */
  ratioTest(matches2, v_time);
  // mtime += v_time;

  // 3) Remove non-symmetrical matches
  std::vector<cv::DMatch> symMatches;
  symmetryTest(matches1, matches2, symMatches, v_time);
  // mtime += v_time;

  // 4) Compute the homography matrix with outlier removal options
  bool refineHomography(true);
  double v_hmgTolerance = toDouble(m_parameters.get("HmgTolerance"));
  homography = computeHomography(queryKeypoints, trainKeypoints,
                                 symMatches, homography_matches,
                                 mtime, cv::FM_RANSAC, v_hmgTolerance,
                                 refineHomography, onErrorThrow);

  // 5) Compute the fundamental matrix with outliers
  fundamental = ransacTest(homography_matches, queryKeypoints, trainKeypoints,
                           epipolar_matches, v_time, onErrorThrow);

  // 6) Compute final homography after all outlier removal has been done
  refineHomography = false;
  homography = computeHomography(queryKeypoints, trainKeypoints,
                                 epipolar_matches, matches,
                                 mtime, cv::FM_RANSAC, v_hmgTolerance,
                                 refineHomography, onErrorThrow);


  return ( matches.size() >  0 );
}

// Clear matches for which NN ratio is > than threshold
// return the number of removed points
// (corresponding entries being cleared,
// i.e. size will be 0)
int RobustMatcher::ratioTest(std::vector<std::vector<cv::DMatch> > &matches,
                             double &mtime) const {

  double v_ratio = toDouble(m_parameters.get("Ratio"));
  if ( isDebug() ) {
    logger() << "Entered RobustMatcher::ratioTest(matches[2]) for 2 NearestNeighbors (NN)...\n";
    logger() << "  RobustMatcher::Ratio:       " << v_ratio << "\n";
    logger().flush();
  }

  QTime stime;
  stime.start();  // Start the timer

  int removed(0);
  int nInput(0);
  int noTwoNN(0);
  int nfailed(0);
  // for all matches
  for (std::vector<std::vector<cv::DMatch> >::iterator matchIterator= matches.begin();
       matchIterator!= matches.end(); ++matchIterator) {

    // if 2 NN has been identified
    if (matchIterator->size() > 1) {
      // check distance ratio
      if ((*matchIterator)[0].distance/
          (*matchIterator)[1].distance > v_ratio) {
         matchIterator->clear(); // remove match
         removed++;
         nfailed++;
      }
    }
    else { // does not have 2 neighbours
      matchIterator->clear(); // remove match
      removed++;
      noTwoNN++;
    }
    nInput++;
  }

  mtime = elapsed(stime);
  if ( isDebug() ) {
    logger() << "  Total Input Matches Tested: " << nInput << "\n";
    logger() << "  Total Passing Ratio Tests:  "
              << matches.size()-removed << "\n";
    logger() << "  Total Matches Removed:      " << removed << "\n";
    logger() << "  Total Failing NN Test:      " << nfailed << "\n";
    logger() << "  Processing Time:            " << mtime << "\n";
    logger().flush();
  }
  return (removed);
}

// Insert symmetrical matches in symMatches vector
void RobustMatcher::symmetryTest(const std::vector<std::vector<cv::DMatch> >& matches1,
                                 const std::vector<std::vector<cv::DMatch> >& matches2,
                                 std::vector<cv::DMatch>& symMatches,
                                 double &mtime) const {

  // Introduce ourselves
  if ( isDebug() ) {
    logger() << "Entered RobustMatcher::symmetryTest(matches1,matches2,symMatches)...\n";
    logger() << " -Running Symmetric Match tests...\n";
    logger().flush();
  }

  symMatches.clear();
  // Do not include in timer

  QTime stime;
  stime.start();  // Start the timer

  // for all matches image 1 -> image 2
  int nInput1(0);
  int nInput2(0);
  for (std::vector<std::vector<cv::DMatch> >::const_iterator matchIterator1= matches1.begin();
       matchIterator1!= matches1.end(); ++matchIterator1) {

    // ignore deleted matches
    if (matchIterator1->size() < 2) continue;
    nInput1++;   // Finally got a good one

   // for all matches image 2 -> image 1
    int ntwo(0);
    for (std::vector<std::vector<cv::DMatch> >::const_iterator matchIterator2= matches2.begin();
         matchIterator2!= matches2.end() ;  ++matchIterator2) {

      // ignore deleted matches
      if (matchIterator2->size() < 2) continue;
      ntwo++;

      // Match symmetry test
      if ((*matchIterator1)[0].queryIdx ==
          (*matchIterator2)[0].trainIdx &&
          (*matchIterator2)[0].queryIdx ==
          (*matchIterator1)[0].trainIdx) {
          // add symmetrical match
        symMatches.push_back(cv::DMatch((*matchIterator1)[0].queryIdx,
                             (*matchIterator1)[0].trainIdx,
                             (*matchIterator1)[0].distance));
        break; // next match in image 1 -> image 2
      }
    }
    nInput2 = qMax(nInput2, ntwo);
  }

  // Return processing time
  mtime = elapsed(stime);

  if ( isDebug() ) {
    logger() << "  Total Input Matches1x2 Tested: " << nInput1 << " x " << nInput2 << "\n";
    logger() << "  Total Passing Symmetric Test:  " << symMatches.size() << "\n";
    logger() << "  Processing Time:               " << mtime << "\n";
    logger().flush();
  }

  return;
}

 // Identify good matches using RANSAC outlier detection in epipolar
 // relationship.
 // Return fundamental matrix
cv::Mat RobustMatcher::ransacTest(const std::vector<cv::DMatch>& matches,
                                  const std::vector<cv::KeyPoint>& keypoints1,
                                  const std::vector<cv::KeyPoint>& keypoints2,
                                  std::vector<cv::DMatch>& outMatches,
                                  double &mtime, const bool onErrorThrow)
                                  const {


  // Introduce ourselves
  double v_epiTolerance = toDouble(m_parameters.get("EpiTolerance"));
  double v_epiConfidence = toDouble(m_parameters.get("EpiConfidence"));
  if ( isDebug() ) {
    logger() << "Entered EpiPolar RobustMatcher::ransacTest(matches, keypoints1/2...)...\n";
    logger() << " -Running EpiPolar Constraints/Fundamental Matrix...\n";
    logger() << "  RobustMatcher::EpiTolerance:    " << v_epiTolerance << "\n";
    logger() << "  RobustMatcher::EpiConfidence:   " << v_epiConfidence << "\n";
    logger() << "  Number Initial Matches:         " << matches.size() << "\n";
    logger().flush();
  }


  outMatches.clear();
  mtime = 0.0;
  cv::Mat fundamental = cv::Mat::eye(3,3,CV_64F);

  // See if enough points to compute fundamental matrix. Minumum number needed
  // for RANSAC is 8 points.
  int v_minEpiPoints = toInt(m_parameters.get("MinimumFundamentalPoints"));
  if ( (unsigned int) v_minEpiPoints > matches.size() ) {
    if ( isDebug() ) {
      logger() << "->ERROR - Not enough points (need at least "
               << v_minEpiPoints << ") to proceed - returning identity!\n";
      logger().flush();
    }
    return ( fundamental );
  }

  // Got some points, get the timer started
  QTime stime;
  stime.start();

  // Convert keypoints into Point2f
  std::vector<cv::Point2f> points1, points2;
  for (std::vector<cv::DMatch>::const_iterator it= matches.begin();
          it!= matches.end(); ++it) {

    // Get the position of left keypoints
    points1.push_back(keypoints1[it->queryIdx].pt);
    // Get the position of right keypoints
    points2.push_back(keypoints2[it->trainIdx].pt);
  }

  // Compute fundamental matrix using RANSAC
  std::vector<uchar> inliers(points1.size(),0);
  try {
    fundamental = cv::findFundamentalMat(cv::Mat(points1),cv::Mat(points2), // matching points
                                         cv::FM_RANSAC, // RANSAC method
                                         v_epiTolerance,   // distance to epipolar line
                                         v_epiConfidence, // confidence probability
                                         inliers);     // match status (inlier or outlier))
  }
  catch ( cv::Exception &e ) {
    QString mess = "1st fundamental (epipolar) test failed!"
                   " QueryPoints=" + QString::number(points1.size()) +
                   ", TrainPoints=" + QString::number(points2.size()) +
                   ".  cv::Error - " + e.what();
    if ( onErrorThrow ) throw IException(IException::Programmer, mess, _FILEINFO_);
    else if ( isDebug()  ) {
      logger() << "->ERROR: " << mess << "return identity matrix for fundamental!\n";
      logger().flush();
    }
    mtime = elapsed(stime);
    return ( fundamental );
  }

  // extract the surviving (inliers) matches (only valid for CV_FM_RANSAC and
  // CV_FM_LMEDS methods)!!!!
  std::vector<uchar>::const_iterator      itIn = inliers.begin();
  std::vector<cv::DMatch>::const_iterator itM  = matches.begin();
  for ( ;itIn != inliers.end(); ++itIn, ++itM) {
    if (*itIn) { // it is a valid match
      outMatches.push_back(*itM);
    }
  }

  if ( isDebug() ) {
    logger() << "  Inliers on 1st Epipolar:        " << outMatches.size() << "\n";
    logger().flush();
  }

  if ( toBool(m_parameters.get("RefineFundamentalMatrix"))  ) {

    // See if enough points to compute fundamental matrix. Minumum number needed
    // for RANSAC is 8 points.
    int v_minEpiPoints = toInt(m_parameters.get("MinimumFundamentalPoints"));
    if ( (unsigned int) v_minEpiPoints <= outMatches.size() ) {

      // Make a copy of the inliers of the first fundamental processing
      std::vector<cv::DMatch> matches1(outMatches);
      outMatches.clear();

      // The F matrix will be recomputed with all accepted matches Convert
      // keypoints into Point2f for final F computation
      points1.clear();
      points2.clear();
      for (std::vector<cv::DMatch>::const_iterator it = matches1.begin();
           it != matches1.end(); ++it) {

        // Get the position of left keypoints
        points1.push_back(keypoints1[it->queryIdx].pt);
        // Get the position of right keypoints
        points2.push_back(keypoints2[it->trainIdx].pt);
      }

      // Compute 8-point F from all accepted matches
      std::vector<uchar> inliers2(points1.size(),0);
      cv::Mat fundamental2 = fundamental;
      try {
        fundamental2 = cv::findFundamentalMat(cv::Mat(points1), cv::Mat(points2), // matches
                                             cv::FM_LMEDS,  // Least squares method
                                             v_epiTolerance,   // distance to epipolar line
                                             v_epiConfidence, // confidence probability
                                             inliers2);   // New set of inliers
        fundamental = fundamental2;
      }
      catch ( cv::Exception &e ) {
        outMatches.clear();
        QString mess = "2st fundamental (epipolar) test failed!"
                       " QueryPoints=" + QString::number(points1.size()) +
                       ", TrainPoints=" + QString::number(points2.size()) +
                       ".  CV::Error - " + e.what();
        if ( onErrorThrow ) throw IException(IException::Programmer, mess, _FILEINFO_);
        else if ( isDebug()  ) {
          logger() << "->Refinement ERROR: " << mess << "return initial matrix for fundamental!\n";
          logger().flush();
        }

        mtime = elapsed(stime);
        return ( fundamental );
      }

      // Now clear the results from the first run and recompute inliers
      std::vector<uchar>::const_iterator      itIn2 = inliers2.begin();
      std::vector<cv::DMatch>::const_iterator itM2  = matches1.begin();
      // for all matches
      for ( ;itIn2 != inliers2.end(); ++itIn2, ++itM2) {
        if (*itIn2) { // it is a valid match
          outMatches.push_back(*itM2);
        }
      }

      if ( isDebug() ) {
        logger() << "  Inliers on 2nd Epipolar:        " << outMatches.size() << "\n";
        logger().flush();
      }
    }
    else {
      if ( isDebug() ) {
        logger() << "  Not enough points (" << outMatches.size()
                 << ", needs " << v_minEpiPoints
                 << ") for 2nd Epipolar - returning current state!!\n";
        logger().flush();
      }
    }
  }

  mtime = elapsed(stime);

  if ( isDebug() ) {
    logger() << "  Total Passing Epipolar:         " << outMatches.size() << "\n";
    logger() << "  Processing Time:                " << mtime << "\n";
    logger().flush();
  }

 return ( fundamental );
}

cv::Mat RobustMatcher::computeHomography(const std::vector<cv::KeyPoint>& query,
                                         const std::vector<cv::KeyPoint>& train,
                                         const std::vector<cv::DMatch> &matches,
                                         std::vector<cv::DMatch> &inliers,
                                         double &mtime, const int method,
                                         const double tolerance,
                                         const bool refine,
                                         const bool onErrorThrow)
                                         const {

  // Introduce ourselves
  double v_hmgTolerance = toDouble(m_parameters.get("HmgTolerance"));
  if ( isDebug() ) {
    logger() << "Entered RobustMatcher::computeHomography(keypoints1/2, matches...)...\n";
    logger() << " -Running RANSAC Constraints/Homography Matrix...\n";
    logger() << "  RobustMatcher::HmgTolerance:  " << v_hmgTolerance << "\n";
    logger() << "  Number Initial Matches:       " << matches.size() << "\n";
    logger().flush();
  }

  // Initialize return parameters
  mtime = 0.0;
  cv::Mat homography = cv::Mat::eye(3,3,CV_64F);

  QTime stime;
  stime.start();  // Start the timer

  // Prepare source and train points
  std::vector<cv::Point2f> srcPoints, dstPoints;
  for (unsigned int i = 0; i < matches.size() ; i++) {
     srcPoints.push_back(query[matches[i].queryIdx].pt);
     dstPoints.push_back(train[matches[i].trainIdx].pt);
  }

  // Test intial return condition
  inliers.clear();
  int v_minHomoPoints = toInt(m_parameters.get("MinimumHomographyPoints"));
  if ( srcPoints.size() < (unsigned int) v_minHomoPoints ) {
    if ( isDebug() ) {
      logger() << "  Not enough points  (" << srcPoints.size()
                << ") to compute initial homography - need at least "
                << v_minHomoPoints << "!\n";
      logger().flush();
    }
    return ( homography );
  }

  try {
    // Find homography using RANSAC algorithm
    cv::Mat h2 = homography;
    h2 = cv::findHomography(srcPoints, dstPoints, method, tolerance);
    homography = h2;

    // Warp dstPoints to srcPoints domain using inverted homography transformation
    std::vector<cv::Point2f> srcReprojected;
    cv::perspectiveTransform(dstPoints, srcReprojected, homography.inv());

    // Pass only matches with low reprojection error (less than tolerance
    // value in pixels)
    double tolSquared = v_hmgTolerance * v_hmgTolerance;
    for (unsigned int i = 0; i < matches.size() ; i++) {
        cv::Point2f actual   = srcPoints[i];
        cv::Point2f expected = srcReprojected[i];
        cv::Point2f v = actual - expected;
        const double distanceSquared = (v.x*v.x + v.y*v.y);

        if ( distanceSquared <= tolSquared ) {
          inliers.push_back(matches[i]);
        }
    }

    if ( isDebug() ) {
      logger() << "  Total 1st Inliers Remaining:  " << inliers.size() << "\n";
      logger().flush();
    }

    if ( true == refine ) {
      // Test for bad case
      if ( inliers.size() <  (unsigned int) v_minHomoPoints ) {
        inliers.clear();
        if ( isDebug() ) {
          logger() << "  Not enough points (" << inliers.size()
                    << ") to compute refined homography - need at least "
                    << v_minHomoPoints << " - failure!\n";
          logger().flush();
        }

        mtime = elapsed(stime);
        return ( homography );
      }

      // Set up return condition for refined homography
      std::vector<cv::DMatch> inliers0(inliers);
      inliers.clear();

      // Now use only good points to find refined homography:
      std::vector<cv::Point2f> refinedSrc, refinedDst;
      for (unsigned int i = 0; i < inliers0.size(); i++) {
          refinedSrc.push_back(query[inliers0[i].queryIdx].pt);
          refinedDst.push_back(train[inliers0[i].trainIdx].pt);
      }

      // Use least squares method to find precise homography
      h2 = cv::findHomography(refinedSrc, refinedDst, 0, tolerance);
      homography = h2;

      // Warp dstPoints to srcPoints domain using inverted homography transformation
      std::vector<cv::Point2f> srcFinal;
      cv::perspectiveTransform(refinedDst, srcFinal, homography.inv());
      // Compute remaining inliers
      for (unsigned int i = 0; i < inliers0.size() ; i++) {
        cv::Point2f actual   = refinedSrc[i];
        cv::Point2f expected = srcFinal[i];
        cv::Point2f v = actual - expected;
        const double distanceSquared = (v.x*v.x + v.y*v.y);

        if ( distanceSquared <= tolSquared ) {
          inliers.push_back(inliers0[i]);
        }
      }
    }
  }
  catch ( cv::Exception &e ) {
    QString mess = "RobustMatcher::HomographyFailed: with cv::Error - "+
                   QString(e.what());
    if ( onErrorThrow ) throw IException(IException::Programmer, mess , _FILEINFO_);
    else if ( isDebug()  ) {
      logger() << "->Homography ERROR: " << mess << "return current state of homography\n";
      logger().flush();
    }
    mtime = elapsed(stime);
    return ( homography );
  }

  mtime = elapsed(stime);
  if ( isDebug() ) {
    logger() << "  Total 2nd Inliers Remaining:  " << inliers.size() << "\n";
    logger() << "  Processing Time:              " << mtime << "\n";
    logger().flush();
  }

  return ( homography );
}


const PvlFlatMap &RobustMatcher::parameters() const {
  return ( m_parameters );
}


PvlObject RobustMatcher::info(const QString &p_name) const {
  PvlObject description = MatcherAlgorithms::info(p_name);
  description.addKeyword(PvlKeyword("OpenCVVersion", CV_VERSION));
  description.addKeyword(PvlKeyword("Name", name()));
  // description.addKeyword(PvlKeyword("Specification", name()));

  PvlObject aparms("Parameters");
  BOOST_FOREACH ( const PvlKeyword &key, m_parameters ) {
    aparms.addKeyword(key);
  }
  description.addObject(aparms);

  return ( description );
}


void RobustMatcher::init(const PvlFlatMap &parameters) {
  m_parameters.clear();
  m_parameters.add("SaveRenderedImages",  "false");
  m_parameters.add("SavePath",  "$PWD");
  m_parameters.add("RootSift",  "false");
  m_parameters.add("Ratio", "0.65");
  m_parameters.add("EpiConfidence",  "0.99");
  m_parameters.add("EpiTolerance",  "3.0");
  m_parameters.add("HmgTolerance",  "3.0");
  m_parameters.add("MaxPoints", "0");
  m_parameters.add("MinimumFundamentalPoints", "8");
  m_parameters.add("RefineFundamentalMatrix",  "true");
  m_parameters.add("MinimumHomographyPoints",  "8");
  m_parameters.merge(parameters);
  return;
}

// Compute RootSift descriptors for better matching potential
void RobustMatcher::RootSift(cv::Mat &descriptors, const float eps) const {
  // Compute sums for L1 Norm
  cv::Mat sums_vec;
  descriptors = cv::abs(descriptors); //otherwise we draw sqrt of negative vals
  cv::reduce(descriptors, sums_vec, 1 /*sum over columns*/, cv::REDUCE_SUM, CV_32FC1);
  for(int row = 0; row < descriptors.rows; row++){
    int offset = row*descriptors.cols;
    for(int col = 0; col < descriptors.cols; col++){
      descriptors.at<float>(offset + col) = sqrt(descriptors.at<float>(offset + col) /
                                            (sums_vec.at<float>(row) + eps) /*L1-Normalize*/);
    }
  }
  return;
}


/** Returns elapsed time since the timer was started in seconds */
double RobustMatcher::elapsed(const QTime &runtime) const {
  return (runtime.elapsed() / 1000.0);
}


}   // namespace Isis
