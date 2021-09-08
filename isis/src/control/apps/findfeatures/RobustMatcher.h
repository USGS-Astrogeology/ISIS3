#ifndef RobustMatcher_h
#define RobustMatcher_h

/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */


#include <QString>
#include <QSharedPointer>
#include <QTime>

#include <opencv2/opencv.hpp>

#include "FeatureMatcherTypes.h"
#include "MatcherAlgorithms.h"
#include "MatchImage.h"
#include "MatchPair.h"
#include "PvlFlatMap.h"
#include "PvlObject.h"
#include "QDebugLogger.h"

namespace Isis {

class QDebug;

/**
 * @brief Container for a feature match pair of data sources
 *
 *
 * @author 2015-08-18 Kris Becker
 * @internal
 *   @history 2015-08-18 Kris Becker - Original Version
 *   @history 2016-10-05 Ian Humphrey & Makayla Shepherd - Changed headers to OpenCV2.
 */
  class RobustMatcher : public MatcherAlgorithms, public QLogger {

    public:
      RobustMatcher();
      RobustMatcher(const QString &name);
      RobustMatcher(const QString &name,
                    const MatcherAlgorithms &algorithms,
                    const PvlFlatMap &parameters = PvlFlatMap(),
                    const QLogger &logger = QLogger());
      virtual ~RobustMatcher();

      void setName(const QString &name);
      inline QString name() const {  return ( m_name );  }

      // For just images, MatchImage objects are created generically using the
      // other match interfaces
      MatchPair      match(cv::Mat& query, cv::Mat& trainer) const;
      MatchPairQList match(cv::Mat& query, std::vector<cv::Mat>& trainers) const;

      // Formal matching methods
      MatchPair      match(MatchImage &query, MatchImage &train) const;
      MatchPairQList match(MatchImage &query, MatchImageQList &trainers) const;

      // Robust outlier removal
      bool removeOutliers(const cv::Mat &queryDescriptors,
                          const cv::Mat &trainDescriptors,
                          std::vector<cv::KeyPoint>& queryKeypoints,
                          std::vector<cv::KeyPoint>& trainKeypoints,
                          std::vector<cv::DMatch> &homography_matches,
                          std::vector<cv::DMatch> &epipolar_matches,
                          std::vector<cv::DMatch> &matches,
                          cv::Mat &homography, cv::Mat &fundamental,
                          double &mtime, const bool onErrorThrow = true)
                          const;

      int ratioTest( std::vector<std::vector<cv::DMatch> > &matches,
                     double &mtime) const;

      void symmetryTest(const std::vector<std::vector<cv::DMatch> >& matches1,
                        const std::vector<std::vector<cv::DMatch> >& matches2,
                        std::vector<cv::DMatch>& symMatches, double &mtime) const;

      cv::Mat ransacTest(const std::vector<cv::DMatch>& matches,
                         const std::vector<cv::KeyPoint>& keypoints1,
                         const std::vector<cv::KeyPoint>& keypoints2,
                         std::vector<cv::DMatch>& outMatches, double &mtime,
                         const bool onErrorThrow = true) const;

      cv::Mat computeHomography(const std::vector<cv::KeyPoint>& query,
                                const std::vector<cv::KeyPoint>& train,
                                const std::vector<cv::DMatch> &matches,
                                std::vector<cv::DMatch> &inliers,
                                double &mtime,
                                const int method = cv::FM_RANSAC,
                                const double tolerance = 3.0,
                                const bool refine = true,
                                const bool onErrorThrow = true)
                                const;

      const PvlFlatMap &parameters() const;

      // This hides the MatcherAlgorithm version which will be augmented
      PvlObject info(const QString &p_name = "RobustMatcher") const;

    private:
      QString      m_name;        // Name of matcher
      PvlFlatMap   m_parameters;  // Parameters for matcher

      void init(const PvlFlatMap &parameters = PvlFlatMap());
      void RootSift(cv::Mat &descriptors, const float eps = 1.0E-7) const;
      double elapsed(const QTime &runtime) const;  // returns seconds

  };

  ///!<   Shared FeatureAlgorithm pointer that everyone can use
  typedef QSharedPointer<RobustMatcher> SharedRobustMatcher;
  typedef QList<SharedRobustMatcher>    RobustMatcherList;


}  // namespace Isis

#endif
