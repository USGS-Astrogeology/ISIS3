#ifndef MatchPair_h
#define MatchPair_h

/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <QString>
#include <QSharedPointer>

#include <opencv2/opencv.hpp>

#include "FeatureMatcherTypes.h"
#include "MatchImage.h"

namespace Isis {

/**
 * @brief Container for a feature match pair of data sources
 *
 *
 * @author 2015-08-10 Kris Becker
 * @internal
 *   @history 2015-08-10 Kris Becker - Original Version
 */

class MatchPair {
  public:
    MatchPair(const MatchImage &query,
              const MatchImage &train,
              const Matches &matches,
              const cv::Mat &homography,
              const cv::Mat &fundamental,
              const double &matchTime) {
      m_data = new MatchData();
      m_data->m_query = query;
      m_data->m_train = train;
      m_data->m_matches = matches;
      m_data->m_homography = homography;
      m_data->m_epipolar = fundamental;
      m_data->addTime(matchTime);
    }

    MatchPair(const MatchImage &query, const MatchImage &train) {
      m_data = new MatchData();
      m_data->m_query = query;
      m_data->m_train = train;
    }

    virtual ~MatchPair() { }

    inline int size() const {
      return ( matches().size() );
    }

    QString target() const {
      QString v_target = m_data->m_query.target();
      if ( v_target.isEmpty() ) { v_target = m_data->m_train.target(); }
      return ( v_target );
    }

    inline const MatchImage &query() const {
      return ( m_data->m_query );
    }

    inline const MatchImage &train() const {
      return ( m_data->m_train );
    }

    inline int keyPointTotal() const {
      return ( m_data->m_query.size() + m_data->m_train.size() );
    }

    inline Matches &epipolar_matches() {
      return ( m_data->m_epipolar_matches );
    }

    inline const Matches &epipolar_matches() const {
      return ( m_data->m_epipolar_matches );
    }


    inline Matches &homography_matches() {
      return ( m_data->m_homography_matches );
    }

    inline const Matches &homography_matches() const {
      return ( m_data->m_homography_matches );
    }

    inline Matches &matches() {
      return ( m_data->m_matches );
    }

    inline const Matches &matches() const {
      return ( m_data->m_matches );
    }

    inline const cv::DMatch &match(const int &index = 0) const {
      Q_ASSERT ( (index >= 0) && (index < (int) matches().size() ) );
      return ( matches()[index] );
    }

    inline void addTime(const double &mtime) {
      m_data->addTime(mtime);
    }

    inline void setFundamental(const cv::Mat &fundamental) {
      m_data->m_epipolar = fundamental;
      return;
    }

    inline const cv::Mat &fundamental() const {
      return ( m_data->m_epipolar);
    }

    inline void setHomography(const cv::Mat &homography) {
      m_data->m_homography = homography;
      m_data->m_homography_inverse = homography.inv();
      return;
    }

    inline const cv::Mat &homography() const {
      return (m_data->m_homography);
    }

    inline const cv::Mat &homographyInverse() const {
      return (m_data->m_homography_inverse);
    }

    inline int correspondence(double *p_repeatability = 0) const {
      float v_repeatability;
      int   v_correspondence;
      evaluateFeatureDetector(m_data->m_query.image(),
                              m_data->m_train.image(),
                              m_data->m_homography,
                              &m_data->m_query.keypoints(),
                              &m_data->m_train.keypoints(),
                              v_repeatability, v_correspondence);
      if ( p_repeatability != 0 ) {  *p_repeatability =  v_repeatability; }
      return (v_correspondence);
    }

    inline double repeatability() const {
      double v_repeatability;
      (void) correspondence(&v_repeatability);
      // return ( (double) correspondence() / (double) m_data->m_query.size() );
      return ( v_repeatability );
    }

    inline double recall() const {
      return ( (double) matches().size() / (double) correspondence() );
    }

    inline double efficiency() const {
      return ( (double) matches().size() / (double) query().size() );
    }

    inline double time() const {
      return ( m_data->m_duration );
    }

    inline double duration() const {
      return ( query().time() + train().time() + this->time() );
    }

    inline double speed() const {
      return ( duration() / (double) (query().size() + train().size()) );
    }

    inline double distance() const {
      std::vector<cv::Point2f> points;
      for (unsigned int i = 0 ; i < matches().size() ; i++) {
        points.push_back(query().keypoint(match(i).queryIdx).pt);
      }

      std::vector<cv::Point2f> projected;
      // cv::perspectiveTransform(points, projected, m_epipolar);
      cv::perspectiveTransform(points, projected, m_data->m_homography);

      double sumdist(0.0);
      for (unsigned int i = 0 ; i < projected.size() ; i++) {
        double xdiff = projected[i].x - train().keypoint(match(i).trainIdx).pt.x;
        double ydiff = projected[i].y - train().keypoint(match(i).trainIdx).pt.y;
        sumdist += std::sqrt(xdiff*xdiff + ydiff*ydiff);
      }
      return ( sumdist / (double) matches().size() );
    }

    bool operator<(const MatchPair &other) const {
      return ( efficiency() < other.efficiency() );
    }

    bool operator<(const MatchPair *other) const {
      return ( efficiency() < other->efficiency() );
    }

    cv::Point2f forward(const cv::Point2f &point) const {
      std::vector<cv::Point2f> points, ptsProjected;
      points.push_back(point);
      cv::perspectiveTransform(points, ptsProjected, homography());
      return (ptsProjected[0]);
    }

    cv::Point2f inverse(const cv::Point2f &point) const {
      std::vector<cv::Point2f> points, ptsProjected;
      points.push_back(point);
      cv::perspectiveTransform(points, ptsProjected, homographyInverse());
      return (ptsProjected[0]);
    }

    int errorCount() const {
      return ( m_data->m_errors.size() );
    }

    void addError(const QString &error) {
      m_data->addError(error);
    }

    QString getError(const int &index) const {
      Q_ASSERT ( index >= 0 );
      Q_ASSERT ( index < m_data->m_errors.size() );
      return ( m_data->m_errors.at(index) );
    }

    QString errors(const QString &separator = "\n") {
      return ( m_data->m_errors.join(separator));
    }

  private:
    /**
     *  Shared Image data pointer
     *
     * @author 2015-08-10 Kris Becker
     * @internal
     *   @history 2015-08-10 Kris Becker - Original Version
     */
    class MatchData : public QSharedData {
      public:
        MatchData() : QSharedData(), m_query(), m_train(),
                      m_epipolar_matches(), m_homography_matches(), m_matches(),
                      m_duration(0), m_homography(cv::Mat::eye(3,3,CV_64F)),
                      m_homography_inverse(cv::Mat::eye(3,3,CV_64F)),
                      m_epipolar(cv::Mat::eye(3,3,CV_64F)), m_errors() { }
        MatchData(const MatchData &other) : QSharedData(other),
                                            m_query(other.m_query),
                                            m_train(other.m_train),
                                            m_epipolar_matches(),
                                            m_homography_matches(),
                                            m_matches(),
                                            m_duration(0),
                                            m_homography(cv::Mat::eye(3,3,CV_64F)),
                                            m_homography_inverse(cv::Mat::eye(3,3,CV_64F)),
                                            m_epipolar(cv::Mat::eye(3,3,CV_64F)),
                                            m_errors() { }
        ~MatchData() { }

        inline void addTime(const double &delta) {
          m_duration += delta;
        }

        inline void addError(const QString &error) {
          m_errors.append(error);
        }

          // Data....
        MatchImage  m_query;
        MatchImage  m_train;
        Matches     m_epipolar_matches;
        Matches     m_homography_matches;
        Matches     m_matches;

        double      m_duration;

        cv::Mat     m_homography;
        cv::Mat     m_homography_inverse;
        cv::Mat     m_epipolar;

        QStringList m_errors;
    };

    // Managed point to match data
    QExplicitlySharedDataPointer<MatchData> m_data;

};

// Define a list of match pairs
typedef QList<MatchPair>    MatchPairQList;

}  // namespace Isis
#endif
