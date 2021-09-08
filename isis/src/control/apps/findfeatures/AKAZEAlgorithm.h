#ifndef AKAZEAlgorithm_h
#define AKAZEAlgorithm_h
/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */


#include <boost/bimap.hpp>
#include "FeatureAlgorithm.h"
#include "opencv2/features2d.hpp"

#include <QMap>
#include <QVariant>

namespace Isis {

/**
 * @brief AKAZE Feature matcher algorithm
 *
 * This class provides the OpenCV3 AKAZE Feature2D algortithm. Only the
 * necesary methods are implemented here.
 *
 * @author  2016-12-07 Jesse Mapel
 *
 * @internal
 *   @history 2016-12-07 Jesse Mapel - Original Version
 */

class AKAZEAlgorithm : public Feature2DAlgorithm {  // See FeatureAlgorithm.h
  //  OpenCV 3 API
  public:
    AKAZEAlgorithm();
    AKAZEAlgorithm(const PvlFlatMap &cvars, const QString &config = QString());
    virtual ~AKAZEAlgorithm();

    QString description() const;

    // Required for all algorithms
    static Feature2DAlgorithm *create(const PvlFlatMap &vars,
                                     const QString &config = QString());

    // Provide information about the abilities of the algorithm
    virtual bool hasDetector() const;
    virtual bool hasExtractor() const;
    virtual bool hasMatcher() const;

  protected:
    virtual PvlFlatMap getAlgorithmVariables() const;
    virtual int setAlgorithmVariables(const PvlFlatMap &variables);
    void setupMaps();

    boost::bimap<QString, int> m_descriptorTypeMap; /**!< Bi-directional map for converting
                                                          DescriptorType values.*/
    boost::bimap<QString, int> m_diffusivityMap;    /**!< Bi-directional map for converting
                                                          Diffusivity values.*/


  private:
    typedef cv::AKAZE          AKAZEType;
    typedef cv::KAZE           KAZEType;
    typedef cv::Ptr<AKAZEType> AKAZEPtr;
};

};  // namespace Isis
#endif
