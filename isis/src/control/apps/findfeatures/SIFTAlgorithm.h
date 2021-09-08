#ifndef SIFTAlgorithm_h
#define SIFTAlgorithm_h

/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "FeatureAlgorithm.h"
#include "opencv2/xfeatures2d.hpp"

#include <QVariant>

namespace Isis {

/**
 * @brief SIFT Feature matcher algorithm
 *
 * This class provides the OpenCV3 SIFT Feature2D algortithm. Only the
 * necesary methods are implemented here.
 *
 * @author  2016-11-30 Kris Becker
 *
 * @internal
 *   @history 2016-11-30 Kris Becker - Original Version
 *   @history 2016-12-06 Kristin Berry - Updates for OpenCV3
 */

class SIFTAlgorithm : public Feature2DAlgorithm {  // See FeatureAlgorithm.h
  //  OpenCV 3 API
  public:
    SIFTAlgorithm();
    SIFTAlgorithm( const PvlFlatMap &cvars, const QString &config = QString() );

    virtual ~SIFTAlgorithm();

    QString description() const;

    // Required for all algorithms
    static Feature2DAlgorithm *create(const PvlFlatMap &vars,
                                      const QString &config = QString());

    virtual bool hasDetector() const;
    virtual bool hasExtractor() const;
    virtual bool hasMatcher() const;

  protected:
    virtual PvlFlatMap setupParameters();
    virtual PvlFlatMap getAlgorithmVariables() const;
    virtual int setAlgorithmVariables(const PvlFlatMap &variables);

  private:
    typedef cv::SIFT    SIFTType;
    typedef cv::Ptr<SIFTType>        SIFTPtr;
};

};  // namespace Isis
#endif
