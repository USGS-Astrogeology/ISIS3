/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */


#include <QList>

#include <boost/assert.hpp>

#include "IException.h"
#include "MatcherAlgorithms.h"

namespace Isis {

  MatcherAlgorithms::MatcherAlgorithms() { }


  MatcherAlgorithms::MatcherAlgorithms(FeatureAlgorithmPtr &detector,
                                       FeatureAlgorithmPtr &extractor,
                                       MatcherAlgorithmPtr &matcher,
                                       PvlFlatMap parameters)  :
                                       m_detector(detector),
                                       m_extractor(extractor),
                                       m_matcher(matcher),
                                       m_parameters(parameters) {
  }


  MatcherAlgorithms::~MatcherAlgorithms() { }


  /** Determine validity of the algorithms with robust validation   */
  bool MatcherAlgorithms::isValid() const {
    return ( validate(false) );
  }


/**
 * @brief This method throughly vets the matcher algorithms for validity
 *
 * This method insures the contents of this container is valid for its intend
 * operations. This includes ensuring that the detector, extractor and matcher
 * algorithms have all been allocated. If this is valid
 *
 * @author 2016-12-23 Kris Becker
 *
 * @param throwOnErrors Throw a cummulative exception reporting errors if true
 *
 * @return bool Returns true if no errors are found. If errors are found and
 *              the throwOnErrors parameters is false, it will return false.
 */
  bool MatcherAlgorithms::validate(const bool &throwOnErrors) const {
    // Accumulate errors for a comprehensive list of issues
    IException ie;
    int nerrors(0);

    // Check the detector algorithm to ensure all are allocated properly
    // and it contains the require functionality
    if ( m_detector.empty() ){
      ie.append(IException(IException::Programmer,
                           "Required detector algorithm has not been allocated",
                           _FILEINFO_));
      nerrors++;
    }
    else {
      if ( !m_detector->isValid() )  {
        ie.append(IException(IException::Programmer,
                             "Required detector algorithm is not present/valid",
                             _FILEINFO_));
        nerrors++;
      }
      else {
        if ( !m_detector->hasDetector() ) {
          std::string mess = "Detector " + detector().name().toStdString() + " does not possess detector capabilities";
          ie.append(IException(IException::Programmer, mess,_FILEINFO_));
          nerrors++;
        }
      }
    }

    // Check the extractor algorithm to ensure all are allocated properly and it
    // contains the require functionality
    if ( m_extractor.empty() ) {
      ie.append(IException(IException::Programmer,
                           "Required extractor algorithm has not been allocated",
                           _FILEINFO_));
      nerrors++;
    }
    else {
      if ( !m_extractor->isValid() ) {
        ie.append(IException(IException::Programmer,
                             "Required extractor algorithm is not present",
                             _FILEINFO_));
        nerrors++;
      }
      else {
        if ( !m_extractor->hasExtractor() ) {
          std::string mess = "Extractor " + extractor().name().toStdString() + " does not possess extractor capabilities";
          ie.append(IException(IException::Programmer, mess,_FILEINFO_));
          nerrors++;
        }
      }
    }

    // Check the matcher algorithm to ensure all are allocated properly and it
    // contains the require functionality
    if ( m_matcher.empty() )  {
      ie.append(IException(IException::Programmer,
                           "Required matcher algorithm has not been allocated",
                           _FILEINFO_));
      nerrors++;
    }
    else {
      if ( !m_matcher->isValid() ) {
        ie.append(IException(IException::Programmer,
                             "Required matcher algorithm is not present",
                             _FILEINFO_));
        nerrors++;
      }
      else {
        if ( !m_matcher->hasMatcher() ) {
          std::string mess = "Matcher " + matcher().name().toStdString() + " does not possess matcher capabilities";
          ie.append(IException(IException::Programmer, mess,_FILEINFO_));
          nerrors++;
        }
      }
    }

    //  Shall we throw an exception to report the issues?
    if ( (0 < nerrors) && (true == throwOnErrors)) {
      std::string mess = "There were " + toString(nerrors) +
                     " errors found in this matcher algorithm set";
      ie.append(IException(IException::Programmer, mess, _FILEINFO_));
      throw ie;
    }

    return ( 0 == nerrors );
  }


  /** Return the detector algorithm    */
  Feature2DAlgorithm &MatcherAlgorithms::detector() const {
    BOOST_ASSERT ( m_detector.empty() == false);
    BOOST_ASSERT ( m_detector->isValid() == true );
    return ( *m_detector );
  }

  /** Return a reference to the OpenCV extractor algorithm   */
  Feature2DAlgorithm &MatcherAlgorithms::extractor() const {
     BOOST_ASSERT ( m_extractor.empty() == false);
     BOOST_ASSERT ( m_extractor->isValid() == true );
    return ( *m_extractor );
  }

  /** Return a reference to the OpenCV matcher algorithm   */
  DescriptorMatcherAlgorithm &MatcherAlgorithms::matcher() const {
    BOOST_ASSERT ( m_matcher.empty() == false);
    BOOST_ASSERT ( m_matcher->isValid() == true );
    return ( *m_matcher );
  }


  /** Return a const reference to the merge of the RobustMatcher and global
   *  parameters */
  const PvlFlatMap &MatcherAlgorithms::parameters() const {
    return ( m_parameters );
  }

  /** Return a PvlObject containing the chain of algorithm information   */
  PvlObject MatcherAlgorithms::info(const QString &name) const {
    PvlObject data(name.toStdString());
    data += m_detector->info("Detector");
    data += m_extractor->info("Extractor");
    data += m_matcher->info("Matcher");
    return ( data );
  }


};  // namespace Isis
