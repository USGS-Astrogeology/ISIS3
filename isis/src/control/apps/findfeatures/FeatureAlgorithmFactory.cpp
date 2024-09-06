/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <cstdarg>
#include <cstdio>
#include <string>
#include <vector>
#include <numeric>
#include <iostream>
#include <sstream>

#include <QtGlobal>
#include <QCoreApplication>
#include <QRegularExpression>
#include <QScopedPointer>
#include <QString>
#include <QStringList>

#include <boost/foreach.hpp>

#include <opencv2/opencv.hpp>

#include "FileName.h"
#include "IString.h"
#include "IException.h"
#include "FeatureAlgorithmFactory.h"
#include "PvlKeyword.h"
#include "PvlObject.h"
#include "RobustMatcher.h"

// List all the known algorithms
#include "AgastAlgorithm.h"
#include "AKAZEAlgorithm.h"
#include "BlobDetectionAlgorithm.h"
#include "BriefDescriptorAlgorithm.h"
#include "BRISKAlgorithm.h"
#include "DaisyAlgorithm.h"
#include "FASTAlgorithm.h"
#include "FREAKAlgorithm.h"
#include "GFTTAlgorithm.h"
#include "KAZEAlgorithm.h"
#include "LATCHAlgorithm.h"
#include "LUCIDAlgorithm.h"
#include "MSDAlgorithm.h"
#include "MSERAlgorithm.h"
#include "ORBAlgorithm.h"
#include "SIFTAlgorithm.h"
#include "StarAlgorithm.h"
// #include "SURFAlgorithm.h"

#include "BruteForceMatcher.h"
#include "FlannBasedMatcher.h"

using namespace std;

namespace Isis {

/** Initialize the singleton factory pointer */
FeatureAlgorithmFactory *FeatureAlgorithmFactory::m_maker = 0;


/**
 * @brief Constructor
 *
 * This constructor will initialize the OpenCV interface within the ISIS
 * environment by setting necessary parsing elements, setting the OpenCV error
 * handler to an ISIS handler method, and enabling all OpenCV algorithms for
 * parsing and resolving algorithms.
 */
FeatureAlgorithmFactory::FeatureAlgorithmFactory()  {
//  This ensures this singleton is shut down when the application exists
   qAddPostRoutine(DieAtExit);
   m_nMade = 0;

   // Disable OpenCV's error handler by default to suppress text on screen
   disableOpenCVErrorHandler();

   // Collection of global parameters
   m_globalParameters = PvlFlatMap();

   // Initialize algorithms
   (void) initialize();
   return;
}


/**
 * @brief Destructor
 *
 * All formal shutdown processing is handled by the DieAtExit() method that is
 * envoked when Qt exists.
 *
 */
FeatureAlgorithmFactory::~FeatureAlgorithmFactory() { }


/**
 * @brief Retrieve reference to Singleton instance of this object
 *
 * The only access provided for Singleton instance of this object. All method
 * access is made through the pointer returned by this method. The object is
 * created upon the first call to this method. The object is deleted when Qt
 * shuts down.
 *
 * @return FeatureAlgorithmFactory* Pointer to Singleton instance of this object
 */
FeatureAlgorithmFactory *FeatureAlgorithmFactory::getInstance() {
  if (!m_maker) {
    m_maker = new FeatureAlgorithmFactory();
  }
  return (m_maker);
}


int FeatureAlgorithmFactory::initialize() {

  int numAliases(0);

  // The detector/extractor Feature2D
  numAliases += m_algorithmInventory.addFeatureAlgorithm<AgastAlgorithm>();
  numAliases += m_algorithmInventory.addFeatureAlgorithm<AKAZEAlgorithm>();
  numAliases += m_algorithmInventory.addFeatureAlgorithm<BlobDetectionAlgorithm>();
  numAliases += m_algorithmInventory.addFeatureAlgorithm<BriefDescriptorAlgorithm>();
  numAliases += m_algorithmInventory.addFeatureAlgorithm<BRISKAlgorithm>();
  numAliases += m_algorithmInventory.addFeatureAlgorithm<DaisyAlgorithm>();
  numAliases += m_algorithmInventory.addFeatureAlgorithm<FASTAlgorithm>();
  numAliases += m_algorithmInventory.add("FASTX", FASTAlgorithm::create);
  numAliases += m_algorithmInventory.addFeatureAlgorithm<FREAKAlgorithm>();
  numAliases += m_algorithmInventory.addFeatureAlgorithm<GFTTAlgorithm>();
  numAliases += m_algorithmInventory.addFeatureAlgorithm<KAZEAlgorithm>();
  numAliases += m_algorithmInventory.addFeatureAlgorithm<LATCHAlgorithm>();
  numAliases += m_algorithmInventory.addFeatureAlgorithm<LUCIDAlgorithm>();
  numAliases += m_algorithmInventory.addFeatureAlgorithm<MSDAlgorithm>();
  numAliases += m_algorithmInventory.addFeatureAlgorithm<MSERAlgorithm>();
  numAliases += m_algorithmInventory.addFeatureAlgorithm<ORBAlgorithm>();
  numAliases += m_algorithmInventory.addFeatureAlgorithm<SIFTAlgorithm>();
  numAliases += m_algorithmInventory.addFeatureAlgorithm<StarAlgorithm>();
  // numAliases += m_algorithmInventory.addFeatureAlgorithm<SURFAlgorithm>();

  // The matchers
  numAliases += m_algorithmInventory.addMatcherAlgorithm<BruteForceMatcher>();
  numAliases += m_algorithmInventory.addMatcherAlgorithm<FlannBasedMatcher>();

  return ( numAliases );
}


/**
 * @brief Disable the OpenCV error handler and replace with ISIS exceptions
 *
 * This method can be called at any time to reestablish ISIS exception handling
 * on behalf of OpenCV errors when they occur. This method is called when the
 * object is first created via the getInstance() method. Users can establish
 * their own handler by reenabling OpenCV handling calling the
 * enableOpenCVErrors() method and using the OpenCV API to establish their own
 * handler.
 */
void FeatureAlgorithmFactory::disableOpenCVErrorHandler() {
   // Set up a CV error handler to suppress text on screen
   cv::redirectError(handleOpenCVErrors);
   return;
}


/**
 * @brief Re-enable OpenCV error handling
 *
 * This method will terminate ISIS error handling of OpenCV errors and return
 * handling to OpenCV.
 */
void FeatureAlgorithmFactory::enableOpenCVErrorHandler() {
   // Enable OpenCV error handling
   cv::redirectError(0);
   return;
}


/**
 * @brief Retrieve a list of all available algorithms from OpenCV
 *
 * This method returns a vector of strings that provide the names of all OpenCV
 * algorithms available for use in this class for feature detection activities.
 *
 * @return QStringList List of all OpenCV algorithms
 */
QStringList FeatureAlgorithmFactory::getListAll() const {
  return ( m_algorithmInventory.allNames() );
}


/**
 * @brief Return reference to global matcher parameters
 *
 * This method returns a reference to all the parameters that will be passed on
 * to the RobustMatcher algorithms.
 *
 * @return const PvlFlatMap&  Reference to matcher algorithms
 */
const PvlFlatMap &FeatureAlgorithmFactory::globalParameters() const {
  return ( m_globalParameters );
}


/**
 * @brief Parse global parameters into a Pvl flat map strucure
 *
 * This method will accept a string that contain variables structured according
 * to specifications used in the algorithm string. It only recognizes
 * keyword/variable structures of the form "keyword1:value1@keyword2:value2".
 *
 * @param globals String of global parameters
 */
PvlFlatMap FeatureAlgorithmFactory::parseGlobalParameters(const QString &globals) {
  PvlFlatMap pvlmap;
  QStringList parms = globals.split("@", Qt::SkipEmptyParts);
  for (int i = 0 ; i < parms.size() ; i++ ) {

    // Only parse substrings that have 2 distinct parts separated by :
    QStringList parts = parms[i].split(":", Qt::SkipEmptyParts);
    if ( parts.size() == 2 ) {
      pvlmap.add(parts[0], parts[1]);
    }
  }

  return ( pvlmap );
}

/**
 * @brief Set the global parameters to use in all matchers created
 *
 * This method will accept a set of global paramters that will be applied to
 * all RobustMatcher algorithms created by this factory.  It will replace any
 * existing set of parameters with the contents of the globals parameter.
 *
 * @param globals Global parameters to set for all RobustMatcher algorithms
 */
void FeatureAlgorithmFactory::setGlobalParameters(const PvlFlatMap &globals) {
  m_globalParameters = globals;
}


/**
 * @brief Add parameters to the global set of RobustMatcher parameters
 *
 * This method will be added to the existing set of global parameters for
 * creation of all RobustMatcher algorithms. It will replace/overried any
 * existing parameters of the same name and add any new ones.
 *
 * @param globals Set of new parameters to add to exist global parameters
 */
void FeatureAlgorithmFactory::addGlobalParameters(const PvlFlatMap &globals) {
  m_globalParameters = PvlFlatMap(m_globalParameters, globals);
}


/**
 * @brief Add a single parameter to the global RobustMatcher parameters
 *
 * This method is used to add a new or replace an exist parameter with value
 * provided in the arguemts.
 *
 * @param name  Name of paramter to add/replace
 * @param value Value of the parameter
 */
void FeatureAlgorithmFactory::addParameter(const QString &name,
                                           const QString &value) {
  m_globalParameters.add(name, value);
}


/**
 * @brief Provide Pvl Object describing parameters of an OpenCV algorithm
 *
 * The complete parameter list of the specified OpenCV algorithm is retrieved
 * and a Pvl Object is created that contains this description.
 *
 * @param name Name of the OpenCV algorithm to return parameters for
 *
 * @return PvlObject Contains all parameters and values for the named algorithm
 */
PvlObject FeatureAlgorithmFactory::info(const QString &name) const {
  return ( m_algorithmInventory.info( name ) );
}


/**
 * @brief Create Pvl Objects with parameters for list of OpenCV algorithms
 *
 * This method creates objects for each algorithm that contains parameters and
 * current default values. Each algorithm object is contained in the top level
 * object named "Algorithms".
 *
 * If an error should occur, the algorithm object is created but the name and a
 * description of the error (in the Error keyword) is recorded instead of
 * parameter/value objects.
 *
 * The OpenCV version is recorded in the "OpenCVVersion" keyword in the
 * Algorithms object.
 *
 * @param algorithms List of algorithms to create parameter/value objects
 *
 * @return PvlObject Top level Algorithms object contain algorithm
 *                   parameter/value objects
 */
PvlObject FeatureAlgorithmFactory::info(const QStringList &algorithms) const {
  return ( m_algorithmInventory.info( algorithms, "Algorithms" ) );
}


/**
 * @brief Create Pvl object of parameter/values for RobustMatcher algorithms
 *
 * This method generates PvlObjects containing the actual parameter/values for
 * each RobustMatcher algorithm contained in the list. The parameters for each
 * RobustMatcher algorithm consists of three individual OpenCV algorithms.
 * For each RobustMatcher algorithm, there will be objects named "Detector",
 * "Extractor" and "Matcher" containing the parameter/values for each one. These
 * three objects will be contained in a PvlObject named "FeatureAlgorithm". In
 * addition, keywords with the name (Name), version of OpenCV being used
 * (OpenCVVersion) and the string specification used to create the RobustMatcher
 * algorithm (Specification) are also added to the object. These algorithms are
 * added to a top level object called "FeatureAlgorithms".
 *
 * @param algorithmList List of RobustMatcher algorithms to generate info for
 *
 * @return PvlObject Top level FeatureAlgorithms object containing current
 *                   parameter/values for each RobustMatcher algorithm
 */
PvlObject FeatureAlgorithmFactory::info(const RobustMatcherList &algorithmList)
                                        const {
  PvlObject listPvl("FeatureAlgorithms");

  BOOST_FOREACH(SharedRobustMatcher algorithms, algorithmList) {
    listPvl += algorithms->info();
  }

  return ( listPvl );
}


/**
 * @brief Create a series of feature-based RobustMatcher algorithms
 *
 * This method will create a list of individual OpenCV feature-based
 * RobustMatcher algorithms from a strings specification. The specification of
 * the string cantains a series of individual matcher specifications separated
 * by a "|" character.
 *
 * A single feature matcher algorithm specification is of the basic form:
 * "/detector/extractor/matcher". Multiple algorithms can be provided in the
 * specification string where each feature matcher algorithm spec is separated
 * by a "|", e.g.:
 *
 *   "/detector1/extractor1/matcher1[/parameters@name:value...] |
 *    /detector2/extractor2/matcher2[/parameters@name:value...]"
 *
 * The input string is parsed and validated by instantiating algorithms using
 * the OpenCV API. In the former case, a single RobustMatcher algorithm is
 * instantiated. The latter form returns a list of RobustMatcher algorithms.
 *
 * The "/parameters@name:value" portion specifies parameter for the
 * RobustMatcher algorithm. Many of these parameters are described in the
 * RobustMatcher class documentation. Each individual algorithm can also have a
 * parameters modified by adding a series of "@name:value@name:value..." for
 * each algorithm specified.
 *
 * @param specs         String specification of RobustMatcher algorithms
 * @param errorIfEmpty  If the parser finds no valid algorithm specs and this
 *                      parameter is true, and error is thrown, otherwise an
 *                      empty list is returned
 *
 * @return RobustMatcherList List of OpenCV feature-based RobustMatcher
 *                           algorithms
 */
RobustMatcherList FeatureAlgorithmFactory::create(const QString &specs,
                                                      const bool &errorIfEmpty)
                                                      const {
  RobustMatcherList algoList;

  // Individual algorithm specifications are separated by vertical bars
  QStringList algorithms = specs.split("|", Qt::SkipEmptyParts);
  if ( algorithms.size() == 0 ) {
    if ( errorIfEmpty ) {
      std::string mess = "No feature matcher algorithms provided!";
      throw IException(IException::User, mess, _FILEINFO_);
    }
    return (algoList);
  }

  // Make all feature algorithms
  BOOST_FOREACH ( QString matcherSpec, algorithms ) {
    SharedRobustMatcher algo( make(matcherSpec) );
    if ( algo.isNull() ) {
      std::string mess = "Failed to create feature matcher from spec " + matcherSpec + ".";
      throw IException(IException::User, mess, _FILEINFO_);
    }
    algoList.append(algo);
  }
  return (algoList);
}


/**
 * @brief Create OpenCV feature-based RobustMatcher algorithm from string spec
 *
 *  This method creates a single OpenCV feature-based RobustMatcher class from a
 *  string specfication containing a detector, extractor and optionally a
 *  matcher. If a matcher is not provided, the RobustMatcher class will create
 *  one that is best suited for the detector/extractor combination.
 *
 *  In addition, parameters that alter the behavior of the outlier detection
 *  processing, among oother things, in the RobustMatcher can be specified as an
 *  additional part of the string using the "/parameters@name:value..."
 *  specification.
 *
 * @param definition A single string specification for an OpencCV feature-based
 *                   algorithm from which to generate a RobustMatcher algorithm
 *
 * @return RobustMatcher* Pointer to a RobustMatcher algorithm
 */
SharedRobustMatcher FeatureAlgorithmFactory::make(const QString &definition) const {

  // std::cout << "FeatureAlgorithmFactorty::make - Creating feature algorithms from spec: " << definition << "\n";
  FeatureAlgorithmPtr detector;
  FeatureAlgorithmPtr extractor;
  MatcherAlgorithmPtr matcher;

  QString fullspec(definition);
  // Split the full config string up and check formatting
  // Output order is always detector, extractor, matcher, parameters
  QStringList formattedSpecs = formatSpecifications(fullspec);

  // Merge parameters with global parameters
  PvlFlatMap matcherParameters = globalParameters();
  if ( !formattedSpecs[3].isEmpty() ) {
    QStringList parametersList = m_algorithmInventory.parse(formattedSpecs[3], "@");
    parametersList.removeFirst();
    matcherParameters.merge( m_algorithmInventory.parameters( parametersList ) );
  }

  // Construct the detector and extractor
  try {
    if ( !formattedSpecs[0].isEmpty() ) {
      // std::cout << "Creating detector with " <<  formattedSpecs[0]<< "\n";
      detector = m_algorithmInventory.getDetector(formattedSpecs[0]);
      if ( formattedSpecs[1].isEmpty() ) {
        // std::cout << "Equating extractor to detector\n";
        extractor = detector;
      }
    }

    if ( !formattedSpecs[1].isEmpty() ) {
      // std::cout << "Creating extractor with " <<  formattedSpecs[1]<< "\n";
      extractor = m_algorithmInventory.getExtractor(formattedSpecs[1]);
      if ( formattedSpecs[0].isEmpty() ) {
        // std::cout << "Equating detector to extractor\n";
        detector = extractor;
      }
    }

    if ( !formattedSpecs[2].isEmpty() ) {
      // std::cout << "Creating matcher with " <<  formattedSpecs[2]<< "\n";
      matcher = m_algorithmInventory.getMatcher( formattedSpecs[2] );
    }
  }
  catch (IException &e) {
    std::string mess = "Failed to create algorithms for config:\n" + definition;
    throw IException(e, IException::User, mess, _FILEINFO_);
  }

  // If no matcher was entered use a BruteForceMatcher
  if ( formattedSpecs[2].isEmpty() ) {
    // std::cout << "Creating matcher from extractor with " <<  formattedSpecs[1]<< "\n";
    matcher = createMatcher(extractor);
    fullspec += ("/" + matcher->config());
  }

  // Make the algorithms container
  MatcherAlgorithms algos(detector, extractor, matcher, matcherParameters);
  SharedRobustMatcher falgo( new RobustMatcher(fullspec, algos, matcherParameters) );

  // Check for validity of matcher that has just been created
  try {
   falgo->validate(true);
  }
  catch (IException &ie) {
    std::string mess = "MatcherAlgorithms were not created successfully!";
    ie.append(IException(IException::User, mess, _FILEINFO_));
    throw ie;
  }

  m_nMade++;
  return ( falgo );
}


/**
 * @brief Parses a full specification string for a set of algorithms.
 *
 * Parses a full specification string for a set of algorithms and checks formatting.
 * The specifications will be returned as a QStringList containing, in order: the detector
 * specification, the extractor specification, the matcher specification, and the
 * parameters specification.  If the matcher and/or parameters are not specified they
 * will be a null QString.
 *
 * Upon return, you can expect the following conditions:
 *
 *   1) All strings have content meaning the specification contained all
 *      four (detector, extractor, matcher, parameters) algorithm specifications
 *   2) Only one of detector *or* extractor was specified and it is a fully
 *     capable Feature2D algorithm (i.e., it has both valid detector and
 *     extractor capabilities).
 *   3) May not have a matcher, so it must be allocated from the type of the
 *      extractor which must be determined after it has been initialized (see
 *      createMatcher(extractor))..
 */
QStringList FeatureAlgorithmFactory::formatSpecifications(QString specification) const {
  QStringList parts;
  if ( specification.contains(QRegularExpression("@savepath", QRegularExpression::CaseInsensitiveOption)) ) {
    QRegularExpression sep("/(?=(.*(@savepath)))",QRegularExpression::CaseInsensitiveOption);
    parts = specification.split(sep, Qt::SkipEmptyParts);
  }
  else {
    parts = specification.split("/", Qt::SkipEmptyParts);
  }
  // Componenet specifications
  QString feature2dSpec;
  QString detectorSpec;
  QString extractorSpec;
  QString matcherSpec;
  QString parametersSpec;

  QStringList remains;
  // Within each part are the values between /'s (i.e., detector, extractor,
  // matcher and parameters.
  for (int i = 0 ; i < parts.size() ; i++) {
    QString part = parts[i].trimmed();  // Remove leading/trailing whitespace
    if ( part.contains(QRegularExpression("^feature2d\\.*", QRegularExpression::CaseInsensitiveOption)) ) {
      // std::cout << "Setting feature2d: " << part << "\n";
      // If we have a detector and extractor, this is an error
      if ( !detectorSpec.isEmpty() && !extractorSpec.isEmpty() ) {
        std::string mess = "Too many Feature2Ds specified at " + part + " in specification " +
                       specification;
        throw IException(IException::User, mess, _FILEINFO_);
      }

      // Check detector situation
      if ( detectorSpec.isEmpty() ) {
        detectorSpec = part;
      }
      else {
        // Already have a detector so this must be extractor
        extractorSpec = part;
      }
      // Also sets this for testing purposes
      feature2dSpec = part;
    }
    // Check for explicit settin of detector
    else if ( part.contains(QRegularExpression("^detector\\.*", QRegularExpression::CaseInsensitiveOption)) ) {
      // std::cout << "Setting detector: " << part << "\n";
      if ( !detectorSpec.isEmpty() ) {
        std::string mess = "Multiple Detector specs found - have \"" + detectorSpec + "\", but found \"" +
                        part + "\" in specification: " + specification;
        throw IException(IException::User, mess, _FILEINFO_);
      }
      detectorSpec = part;
    }
    // Check for explicit setting of the extractor
    else if ( part.contains(QRegularExpression("^extractor\\.*", QRegularExpression::CaseInsensitiveOption)) ) {
      // std::cout << "Setting extractor: " << part << "\n";
      if ( !extractorSpec.isEmpty() ) {
        std::string mess = "Multiple Extractor specs found - have \"" + extractorSpec + "\", but found \"" +
                        part + "\" in specification: " + specification;
        throw IException(IException::User, mess, _FILEINFO_);
      }
      extractorSpec = part;
    }
    // Find matcher alorithm string specification
    else if ( part.contains(QRegularExpression("^matcher\\.*", QRegularExpression::CaseInsensitiveOption)) ) {
      // std::cout << "Setting matcher: " << part << "\n";
      if ( !matcherSpec.isEmpty() ) {
        std::string mess = "Multiple Matcher specs found - have \"" + matcherSpec + "\", but found  \"" +
                        part + "\" in specification: " + specification;
        throw IException(IException::User, mess, _FILEINFO_);
      }
      matcherSpec = part;
    }
    else if ( part.contains(QRegularExpression("^parameters*", QRegularExpression::CaseInsensitiveOption)) ) {
      // std::cout << "Setting paramters: " << part << "\n";
      if ( !parametersSpec.isEmpty() ) {
        std::string mess = "Multiple Parameter specs found - have \"" + parametersSpec + "\", but found \"" +
                        part + "\" in specification: " + specification;
        throw IException(IException::User, mess, _FILEINFO_);
      }
      parametersSpec = part;
    }
    else {
      // std::cout << "Setting algorithm[" << i << "] to " << part << "\n";
      // Can't safely check for ill-formed qualifier, so let the allocation
      if ( detectorSpec.isEmpty() ) {
        detectorSpec = part;
      }
      else if ( extractorSpec.isEmpty() ) {
        extractorSpec = part;
      }
      else if ( matcherSpec.isEmpty() ) {
        matcherSpec = part;
      }
      else {
        // Should have had parameter already so there are too many parts of
        // the specification and its invalid.
        std::string mess = "Invalid algorithm/part at or near \"" + part +
                       "\" - too many or invalid algorithm specs detected in specification: "
                       + specification;
        throw IException(IException::User, mess, _FILEINFO_);
      }
    }
  }

  // If a parameter specification was found get it
  if ( !parametersSpec.isEmpty() ) {
    if ( parametersSpec.split("@", Qt::SkipEmptyParts).takeFirst().toLower() !=
         "parameters" ) {
      std::string mess = "Invalid specification:\n" +
                     specification + "\n" +
                     "Invalid parameters specification:\n" +
                     parametersSpec;
      throw IException(IException::User, mess, _FILEINFO_);
    }
  }

  QStringList formattedSpecs;
  formattedSpecs.append(detectorSpec);
  formattedSpecs.append(extractorSpec);
  formattedSpecs.append(matcherSpec);
  formattedSpecs.append(parametersSpec);

  return ( formattedSpecs );
}


/**
 * @brief Allocate a BruteForceMatcher algorithm based upon descriptor extractor
 *
 * See the OpenCV 3.1 BFMatcher documentation
 * http://docs.opencv.org/3.1.0/d3/da1/classcv_1_1BFMatcher.html
 * for details on this implementation.
 *
 * @author 2016-12-19 Jesse Mapel
 * @internal
 *   @history 2016-12-19 Jesse Mapel - Adapted from Kris Becker's RobustMatcher::allocateMatcher
 */
MatcherAlgorithmPtr FeatureAlgorithmFactory::createMatcher(FeatureAlgorithmPtr extractor,
                                                           const QString &normalize,
                                                           const QString &crossCheck)
                                                           const {

  QString name( extractor->name().toLower() );

  QString normType(normalize);
  if ( name.contains("SURF", Qt::CaseInsensitive) ) {
    normType = "NORM_L2";
  }
  else if ( name.contains("SIFT", Qt::CaseInsensitive)  ) {
    normType = "NORM_L2";
  }
  else if (name.contains("ORB", Qt::CaseInsensitive) ) {
    normType = "NORM_HAMMING";
    try {
      if ( toInt( extractor->getVariable("WTA_K") ) > 2 ) {
        normType = "NORM_HAMMING2";
      }
    }
    catch ( cv::Exception &e ) {
      //  NOOP - use existing value
    }
  }
  else if ( name.contains("BRISK", Qt::CaseInsensitive) ) {
    normType = "NORM_HAMMING";
  }
  else if ( name.contains("BRIEF", Qt::CaseInsensitive) ) {
    normType = "NORM_HAMMING";
  }

  // Create the matcher and return
  QString matcherSpecs = "BFMatcher";
  matcherSpecs += ("@NormType:" + normType);
  matcherSpecs += ("@CrossCheck:" + crossCheck);
  MatcherAlgorithmPtr matcher = m_algorithmInventory.getMatcher(matcherSpecs);
  return ( matcher );
}


/** Returns the number of algorithms created by this object */
unsigned int FeatureAlgorithmFactory::manufactured() const {
  return (m_nMade);
}


/**
 * @brief Method to capture OpenCV exceptions/errors and suppress them
 *
 * This allows us to control the output of the errors. OpenCV outputs them to
 * stderr.
 *
 * If, after you use this function in the call (as in findHomography and
 * computeReprojectionError):
 *
 *      cv::redirectError(handleOpenCVErrors);
 *
 * you can revert back to the openCV default behavior with the following
 * function call:
 *
 *      cv::redirectError(nullptr);
 *
 * @param status    Status of OpenCV error
 * @param func_name Name of function
 * @param err_msg   Error message from OpenCV
 * @param file_name Name of source file
 * @param line      Line of source file
 * @param userdata  Optional user data assocaited with error
 *
 * @return int
 */
int FeatureAlgorithmFactory::handleOpenCVErrors( int status,
                                                 const char* func_name,
                                                 const char* err_msg,
                                                 const char* file_name,
                                                 int line, void* userdata ) {
  //Do nothing -- will suppress console output
  return 0;   //Return value is not used
}


/**
 * @brief Exit termination routine
 *
 * This (static) method ensure this object is destroyed when Qt exits.
 *
 * Note that it is error to add this to the system _atexit() routine because
 * this object utilizes Qt classes.  At the time the atexit call stack is
 * executed, Qt is long gone resulting in Very Bad Things.  Fortunately, Qt has
 * an exit stack function as well.  This method is added to the Qt exit call
 * stack.
 */
void FeatureAlgorithmFactory::DieAtExit() {
  delete  m_maker;
  m_maker = 0;
  return;
}

} // namespace Isis
