/**                                                                       
 * @file                                                                  
 * $Revision: 6513 $
 * $Date: 2016-01-14 16:04:44 -0700 (Thu, 14 Jan 2016) $
 * $Id: StereoPairStrategy.cpp 6513 2016-01-14 23:04:44Z kbecker@GS.DOI.NET $
 * 
 *   Unless noted otherwise, the portions of Isis written by the USGS are 
 *   public domain. See individual third-party library and package descriptions 
 *   for intellectual property information, user agreements, and related  
 *   information.                                                         
 *                                                                        
 *   Although Isis has been used by the USGS, no warranty, expressed or   
 *   implied, is made by the USGS as to the accuracy and functioning of such 
 *   software and related material nor shall the fact of distribution     
 *   constitute any such warranty, and no responsibility is assumed by the
 *   USGS in connection therewith.                                        
 *                                                                        
 *   For additional information, launch                                   
 *   $ISISROOT/doc//documents/Disclaimers/Disclaimers.html                
 *   in a browser or see the Privacy &amp; Disclaimers page on the Isis website,
 *   http://isis.astrogeology.usgs.gov, and the USGS privacy and disclaimers on
 *   http://www.usgs.gov/privacy.html.                                    
 */ 
#include "StereoPairStrategy.h"

// std library
#include <cmath>

// Naif functions
#include "SpiceUsr.h"

// Other Qt includes
#include <QtGlobal>

// boost library
#include <boost/foreach.hpp>

// other ISIS
#include "CalculatorStrategy.h"
#include "IException.h"
#include "IString.h"
#include "PvlFlatMap.h"
#include "PvlKeyword.h"
#include "PvlObject.h"
#include "Resource.h"
#include "StrategyFactory.h"

using namespace std;

namespace Isis {

  /** 
   *  Default constructor
   */ 
  StereoPairStrategy::StereoPairStrategy() : 
                                GisOverlapStrategy(), 
                                m_imageStrength(), m_stereoStrength(),
                                m_keywordMap(), m_calculator(),
                                m_pixelPrecisionMatch(0.2), 
                                m_useStereoAngle(true), 
                                m_myDebug(false) {
    setName("StereoPair");
    setType("StereoPair");
  }
  
  
/**
 * @brief Constructor using PvlObject definition for StereoPair 
 *  
 * This constructor is invoked when provided a StereoPair object definition. 
 * Note that GisOverlapStrategy is used to determine overlaps as it has all the 
 * features needed to identify overlapping stereo and is optimized for this 
 * funtion. 
 *  
 * @author 2015-04-01 Kris Becker
 * 
 * @param definition PvlObject definition of StereoPair Strategy
 * @param globals    Global parameters
 */
  StereoPairStrategy::StereoPairStrategy(const PvlObject &definition, 
                                         const ResourceList &globals) : 
                                         GisOverlapStrategy(definition, globals), 
                                         m_imageStrength(), m_stereoStrength(),
                                         m_keywordMap(), m_calculator(),
                                         m_pixelPrecisionMatch(0.2), 
                                         m_useStereoAngle(true), 
                                         m_myDebug(false) {
  
    PvlFlatMap parms( getDefinition(), PvlConstraints::withExcludes(QStringList("IsisMiner")) );
    m_pixelPrecisionMatch = parms.get("PixelPrecisionMatch", "2.0").toDouble();
    if ( isDebug() ) {
      cout << "PixelPrecisionMatch = " << m_pixelPrecisionMatch << "\n";
    }

    //  Get thresholds and validate
    if ( definition.hasGroup("ImageStrength") ) {
      m_imageStrength = getConstraints(definition.findGroup("ImageStrength")); 
    }
  
    //  Get stereo strength parameters and validate
    m_stereoStrength = getConstraints(definition.findGroup("StereoStrength"));
  
    //  Get keyword map
    if ( definition.hasGroup("KeywordMap") ) {
      m_keywordMap = PvlFlatMap(definition.findGroup("KeywordMap"));
    }
  
    // Determine what to use for VerticalPrecision
    m_useStereoAngle = toBool( parms.get("UseStereoAngle", "True").toStdString() );

    // Initialize the calculator strategy for ranking purposes
    m_calculator.reset(new CalculatorStrategy(definition, globals));

    return;
  }
  
  
  /** 
   *  Destructor - cleanup is automatic
   */ 
  StereoPairStrategy::~StereoPairStrategy() { }


  /**
   * @brief Apply method that will screen incoming stereo sources 
   *  
   * This method is only needed to screen out stereo sources that do not meet 
   * initial observation conditions as defined by the user in the 
   * StereoThresholds group that defines general limits on these data. 
   * 
   * @author 2015-05-01 Kris Becker
   * 
   * @param resources Resource list of stereo sources
   * 
   * @return int Number of successful stereo sources that have pairings
   */
  int StereoPairStrategy::apply(ResourceList &resources,
                                const ResourceList &globals) {
    BOOST_FOREACH ( SharedResource resource, resources ) {
      if ( resource->isActive() ) {
        // Check for first level constraints and deactive ones that don't pass
        if ( isDebug() ) {
          cout << "\n===> StereoPair::apply processing to source " << resource->name().toStdString() << "\n";
        }
        if (!passConstraints(resource, m_imageStrength)) {
          resource->discard();
        }
      }
    }

    if ( isDebug() ) {
      cout << "Total stereo sources passing level1 screening: " 
           << countActive(resources) << "\n";
    }

    // Now invoke overlap algorithm in GisOverlapStrategy
    return ( GisOverlapStrategy::apply(resources, globals) );
  }

  /**
   * @brief Screen candidate pairs for level1 constraints
   * 
   * @author 2015-05-01 Kris Becker
   * 
   * @param resources  List of resources that could be used as pair candidates
   * 
   * @return ResourceList Resulting list of stereo level1 screening
   */
  ResourceList StereoPairStrategy::overlapCandidates(ResourceList &resources,
                                                     const ResourceList &globals) {
    ResourceList allCandidates = GisOverlapStrategy::overlapCandidates(resources, globals); 
    ResourceList screened; 
    BOOST_FOREACH ( SharedResource resource, allCandidates ) {
      if ( resource->isActive() ) {
        // Check for first level constraints and don't pass failed resources on
        if ( passConstraints(resource, m_imageStrength) ) {
          screened.append(resource);
        }
      }
    }

    if ( isDebug() ) {
      cout << "Total candidate pairs passing level1 screening: " 
           << screened.size() << "\n";
    }
    return (screened); 
  }

  /**
   * @brief Method to process individual stereo pairings
   *  
   * This method reimplements the processing of individual overlapping pairs 
   * from the results of the GisOverlap strategy. Here, we will be doing 
   * additional stereo analysis on the pairs. 
   *  
   * Note that reimplemented apply() and overlapCandidates() methods ensure we 
   * have passed level1 constraints for both resourceA and resourceB.  
   *  
   * @author 2015-05-01 Kris Becker
   * 
   * @param resourceA Stereo source resource
   * @param resourceB Stereo pair resource
   * @param ovrRatioA Ratio of area coverage for resourceA/resourceB
   * @param ovrRatioB Ratio of area coverage for resourceB/resourceA
   * 
   * @return SharedResource 
   */
  SharedResource StereoPairStrategy::processOverlap(SharedResource &resourceA,
                                                    SharedResource &resourceB,
                                                    const double &ovrRatioA,
                                                    const double &ovrRatioB, 
                                                    const ResourceList &globals) {
  
    //  Ensure we are not matching oneself (also checked in GisOverlap)
    if ( resourceA->isEqual( *resourceB) ) {
      return (SharedResource());
    }
    
    // We will go ahead and merge the two resources
    SharedResource stpair(0);

    try {
      stpair = GisOverlapStrategy::processOverlap(resourceA, resourceB, 
                                                  ovrRatioA, ovrRatioB, 
                                                  globals);
    
      // We know they both pass for level1 constraints. Check level2 (stereo)
      // constraints and rank parameters. stpair automatically deleted upon
      // empty resource returned.
      if ( !computeStereo(resourceA, resourceB, stpair, globals) ) {
        if ( isDebug() ) {
          cout << "StereoPair " << resourceA->name().toStdString() << "/" << resourceB->name().toStdString()
               << " failed!\n";
        }
        return (SharedResource()); 
      }

      if ( isDebug() ) {
        cout << "StereoPair " << resourceA->name().toStdString() << "/" << resourceB->name().toStdString()
             << " is a match!\n";
      }
    }
    catch (IException &ie) {
      if ( isDebug() ) {
        cout << "StereoPair " << resourceA->name().toStdString() << "/" << resourceB->name().toStdString()
             << " incurred an exception! Error = " << ie.what() << "!\n";
      }
      stpair.clear();
    }

    return ( stpair );
  }
  
/**
 * @brief Retreive a value for stereo processing
 *  
 *  This method will get the keyword map value for the specified keyword and
 *  retrieve it from the list of keyword resources.
 * 
 * @author 2015-05-28 Kris Becker
 * 
 * @param key      Keyword value to map and retrieve value for
 * @param globals  List of keyword resources to find keyword value
 * @param defValue Default should the keyword not be found
 * 
 * @return QString The value of the requested keyword
 */
  QString StereoPairStrategy::getStereoValue(const QString &key, 
                                             const ResourceList &globals, 
                                             const QString &defValue) const {
    return (findReplacement(m_keywordMap.get(key, key), globals, 0, defValue)); 
  }


  /**
   * @brief Compute stereo paremters with constraint checking
   * 
   * This method computes all the stereo components for a pair of images. The
   * input is assumed to pass overlap constraints as defined by the
   * GisOverlap::candidateOverlaps() method. The stereo Resource is the
   * composite of the two stereo pair candidates with keywords from each
   * resource added with the suffixA() appended to the keywords in resourceA and
   * suffixB() appended to keywords in resourceB (originating from 
   * GisOverlapStrategy). 
   * 
   * @history 2015-05-03 Kris Becker
   * 
   * @param resourceA Stereo source resource
   * @param resourceB Stereo match resource
   * @param stereo    Stereo composite resource
   * 
   * @return bool Returns true if all operations were successful
   */
  bool StereoPairStrategy::computeStereo(const SharedResource &resourceA, 
                                         const SharedResource &resourceB, 
                                         SharedResource &stereo,
                                         const ResourceList &globals) const {

    // Evaluate stereo constraints 
    double rankA, rankB;
    (void) rankConstraints(resourceA, m_imageStrength, rankA, stereo, "Rank"+suffixA());
    (void) rankConstraints(resourceB, m_imageStrength, rankB, stereo, "Rank"+suffixB());

    stereo->add("ImageStrengthRank"+suffixA(), QString::number(rankA));
    stereo->add("ImageStrengthRank"+suffixB(), QString::number(rankB));
  
    //  Level1 constraints rank is the average of the two level1 constraints
    double rank = (rankA + rankB) / 2.0;
    stereo->add("ImageStrengthRank", QString::number(rank));
  
    // Now compute parallax height ratio
    QString plx = m_keywordMap.get("ParallaxX", "ParallaxX");
    QString ply = m_keywordMap.get("ParallaxY", "ParallaxY");
    double px1 = resourceA->value(plx).toDouble();
    double py1 = resourceA->value(ply).toDouble();
    double px2 = resourceB->value(plx).toDouble();
    double py2 = resourceB->value(ply).toDouble();
    double pxdiff = px1 - px2;
    double pydiff = py1 - py2;
    double dp = sqrt( pxdiff*pxdiff + pydiff*pydiff );
    stereo->add("ParallaxHeightRatio", QString::number(dp));
  
  
    // Now compute shadow tip distance 
    QString shx = m_keywordMap.get("ShadowX", "ShadowX");
    QString shy = m_keywordMap.get("ShadowY", "ShadowY");
    double shx1 = resourceA->value(shx).toDouble();
    double shy1 = resourceA->value(shy).toDouble();
    double shx2 = resourceB->value(shx).toDouble();
    double shy2 = resourceB->value(shy).toDouble();
    double shxdiff = shx1 - shx2;
    double shydiff = shy1 - shy2;
    double dsh = sqrt( shxdiff*shxdiff + shydiff*shydiff );
    stereo->add("ShadowTipDistance", QString::number(dsh));
  
  
    // Now compute Resolution 
    QString reskey = m_keywordMap.get("Resolution", "Resolution");
    double pxlresA = resourceA->value(reskey).toDouble();
    double pxlresB = resourceB->value(reskey).toDouble();
    double resratio = pxlresA / pxlresB;
    if ( resratio < 1.0 ) { resratio = 1.0 / resratio; }
    stereo->add("ResolutionRatio", QString::number(resratio));
  
    // Now compute DeltaSunAzimuth
    (void) computeDelta(resourceA, resourceB, "DeltaSolarAzimuth", 
                        "SubSolarGroundAzimuth", stereo);
  
    // Now compute DeltaSpacecraftAzimuth
    (void) computeDelta(resourceA, resourceB, "DeltaSpacecraftAzimuth", 
                        "SubSpacecraftGroundAzimuth", stereo);
  
    // Compute StereoAngle that will contribute to the vertical precision if
    // it is successfully computed.
    double stAngle(dp);  // ParallelHeightRatio is the default
    if ( computeStereoAngle(resourceA, resourceB, stereo, globals) ) {
      if ( m_useStereoAngle ) {
        stAngle = tan(stereo->value("StereoAngle").toDouble() * rpd_c()); 
      }
    }

    //  Compute the bonus expected vertical precision
    double gsd = qMax(pxlresA, pxlresB);
    double rho = m_pixelPrecisionMatch;

    // Ensure we have a valid stereo angle
    if ( qFuzzyCompare(stAngle+1.0, 1.0) ) { stAngle = 0.1E-6; }
    double evp = rho * gsd / stAngle; 
    stereo->add("VerticalPrecision", QString::number(evp));
  
    //  Now compute the stereo rank
    // m_myDebug = true;
    if ( !passConstraints(stereo, m_stereoStrength) ) {
      if ( isDebug() ) { cout << "Stereo Strength Failed!\n"; }
      m_myDebug = false; 
      return (false);
    }
    // m_myDebug = false;

    // Good, compute rank values
    double rankS;
    (void) rankConstraints(stereo, m_stereoStrength, rankS, stereo);
    stereo->add("StereoStrengthRank", QString::number(rankS));


    // Evaluate the rank
    m_calculator->apply(stereo);
    return (true);
  }

  /**
   * @brief Parse list of threshold values, convert to double, add to map 
   *  
   * This method will insert every keyword in the container into a named map 
   * where all keyword values are converted to double values. 
   * 
   * @author 2015-05-02 Kris Becker
   * 
   * @param constraints Pvl keyword container to create double valued map for
   * 
   * @return StereoPairStrategy::ConstraintList Map containing vector of 
   *           doubles for every keyword
   */
  StereoPairStrategy::ConstraintList StereoPairStrategy::getConstraints(
                                      const PvlContainer &constraints) const {
    ConstraintList cmap;
    for (int i = 0 ; i < constraints.keywords() ; i++) {
      int nval = 0; 
      PvlKeyword key = constraints[i];

      QVector<double> threshold;
      try {
        for ( nval = 0 ; nval < key.size() ; nval++ ) {
          threshold.push_back(std::stod(key[nval]));
        }
      }
      catch (IException &ie) {
        // Traps float conversion errors
        std::string mess = "Error converting value [" + std::to_string(nval+1) + 
                       "] in keyword [" + key.name() + "] to double.";
        throw IException(ie, IException::User, mess, _FILEINFO_);
      }

      // Verify proper number values
      if ( (threshold.size() >= 3)  ) {
        // Check for valid pivot value
        if ( (threshold[2] < threshold[0]) || (threshold[2] > threshold[1]) ) {
          std::string mess = "Criteria keyword [" + key.name() + "] pivot value [" +
                         key[2] + "] exceeds min/max ranges [" +
                         key[0] + "," + key[1] + "].";
          throw IException(IException::User, mess, _FILEINFO_);
        }
      }
      cmap.insert(QString::fromStdString(key.name()).toLower(), threshold);
    }

    return (cmap);

  }

  /**
   * @brief Evaluate a keyword value based upon specified (stereo) criteria 
   *  
   * This method computes a rank value assumed to originate from a Resource.  The 
   * specification of this value is a (required) 3-vector set with an optional 
   * fourth value that acts as a scaler (1 is the default if a 3-vector is 
   * specified).  We refer to this concept as a "ranked threshold".
   *  
   * How The  rank value is computed is best described by an example.  The 
   * following example provides a resolution limit and ranking criteria.  The 
   * keyword in the StereoPair Strategy is: 
   *  
   *     Resolution      = ( 100.0, 500.0, 200.0 )
   *  
   *  This specifies a minimum pixel resolution of 100 pixels/degree (t0), a
   *  maximum of 500 pixels/degree (t1) with 200 pixels/degree (t2) being the
   *  nominal resolution for our desired use.  The 4th element, the weight, (t3)
   *  is not provided which means it is 1.0 by default.
   *
   *  If the "out" parameter is valid, then the rank computed for every
   *  constraint is stored in a keyword with "Rank" appended to. For the example
   *  above using "Resolution", the rank value computed for it is stored in a
   *  keyword named "ResolutionRank" in the Resource "out" if provided.
   *  
   *  The value parameter is the current resolution to threshold and it must be
   *  between the minimum and maximum values (t0 <= value <= t1).  If this test
   *  passes, then the following equations are used to compute the rank values:
   *  
   *  range = max(abs(t2-t0), abs(t1-t2))
   *  rank = 1 - abs(t2-value)/range
   *  rank *= t3
   *  
   *  These set of equations ensure the values is between 0.0 and 1.0 where 1.0 is
   *  the highest rank closest to the nominal value of 200.0.
   * 
   * @author 2013-03-27 Kris Becker
   * 
   * @param value Value to apply thresholding to
   * @param key   Name of keyword with threshold specification (e.g., Resolution 
   *              in the above example)
   * @param rank  Returns the computed rank from the equation given above
   * 
   * @return bool Returns true if the value passed the threshold criteria
   */
  bool StereoPairStrategy::rankConstraints(const SharedResource &resource, 
                                           const ConstraintList &constraints, 
                                           double &rank,
                                           const SharedResource &out,
                                           const QString &suffix) 
                                           const {

    // Translate each keyword criteria that is mapped to the keywords 
    // in resource. Rank is computed and accumulated for all constraits that 
    // have 3 or more values.
    rank = 0.0;
    ConstraintList::const_iterator constraint = constraints.constBegin(); 
    for ( ; constraint != constraints.constEnd() ; ++constraint ) {
      QString key = m_keywordMap.get(constraint.key(), constraint.key()); 
      if ( resource->exists(key) ) {
        double value = resource->value(key).toDouble(); 
        QVector<double> thresholds = constraint.value();
        double myrank = computeRank(value, thresholds);
        if ( !out.isNull() ) { 
          out->add(constraint.key()+suffix, QString::number(myrank)); 
        }
        rank += myrank;
      }
    }
    return (true);
  }
  

/**
 * @brief Determine if constraints are satisfied 
 * 
 * @history 2015-05-03 Kris Becker
 * 
 * @param resource    Resource to check for constraints
 * @param constraints List of constraints to check
 * 
 * @return bool True if all constraints pass
 */
  bool StereoPairStrategy::passConstraints(const SharedResource &resource, 
                                           const StereoPairStrategy::ConstraintList &constraints) 
                                           const {

  // Count missing keywords and passing is required that all exist
    int nbad(0);
    if ( isDebug()  ) { cout << "Running passConstraints on " << resource->name().toStdString() << "...\n";}
    ConstraintList::const_iterator constraint = constraints.constBegin(); 
    for ( ; constraint != constraints.constEnd() ; ++constraint) {
      QString key = m_keywordMap.get(constraint.key(), constraint.key()); 
      if ( resource->exists(key) ) {
        double value = resource->value(key).toDouble(); 
        QVector<double> thresholds = constraint.value();
        Q_ASSERT( thresholds.size() >= 3 );
        if ( (value < thresholds[0]) || (value > thresholds[1]) ) {
          nbad++;
          if ( isDebug() ) { 
            cout << constraint.key().toStdString() << "::(" << key.toStdString() << ") = " << value 
                 << " is out of constraints boundaries!\n"; 
          }
        }
      }
      else {
        nbad++;
        if ( isDebug()  ) {
          cout << constraint.key().toStdString() << "::(" << key.toStdString() 
               << ") does not exist in resource " << resource->name().toStdString() << "\n";
        }
      }
    }

    if ( isDebug() ) { 
      cout <<  "BadConstraintCount = " << nbad << "\n"; 
    }

    return ( nbad == 0 );
}
  

/**
 * @brief Compute angular difference values for given keyword parameter 
 *  
 * The delta angle difference parameter is computed from a common keyword in 
 * both resources.  If either keyword does not exist in both resources, then 
 * false is returned.  Both values of the keywords are required to be in 
 * degrees, 
 *  
 * The computed angular difference is  stored in the composite resource in the 
 * keyword named parameter if successful. If for some reason the angle cannot 
 * be computed, then no keyword is created in the composite resource. 
 * 
 * @author 2015-05-29 Kris Becker
 * 
 * @param resourceA First resource
 * @param resourceB Second resource
 * @param parameter Name of parameter to compute and store in composite 
 *                  resource
 * @param keysrc    Name of keyword in each resource to use as the angle values
 * @param composite Resource to record angular difference in
 * 
 * @return bool True if the value is computed, false otherwise
 */
bool StereoPairStrategy::computeDelta(const SharedResource &resourceA, 
                                        const SharedResource &resourceB, 
                                        const QString &parameter, 
                                        const QString &keysrc,
                                        SharedResource &composite) const {
  // Now compute requested delta
  QString dsckey = m_keywordMap.get(keysrc, keysrc); 
  if ( resourceA->exists(dsckey) && resourceB->exists(dsckey) ) {
    double dsc1 = resourceA->value(dsckey).toDouble(); 
    double dsc2 = resourceB->value(dsckey).toDouble();
    double dscaz = acos( cos((dsc2 - dsc1) * rpd_c()) ) * dpr_c();
    composite->add(parameter, QString::number(dscaz));
    return (true);
  }

  return (false);
}

/**
 * @brief Compute stereo seperation angle if keywords are present 
 *  
 * This method will compute the stereo angle or separation angle between two 
 * image sets as long as the required keywords are present. If one of the 
 * required keywords is missing, this method will return with mno action. 
 *  
 * The required keywords that must exist in each Resource to compute the 
 * separation angle is: CenterRadius, CenterLatitude, CenterLongitude, 
 * TargetCenterDistance, SubspacecraftLatitude and SubspacecraftLongitude. 
 * Alternative keywords can be specified in the KeywordMap group to choose a 
 * differently named keyword for any of these keywords. This is useful for 
 * specifying the centroid latitude/longitude values of the intersection 
 * geometry (GisOverlapCentroidX and GisOverlapCentroidY) of the two resources 
 * to get a more accurate angle. 
 * 
 * @author 2015-05-27 Kris Becker
 * 
 * @param resourceA Primary geometry resource
 * @param resourceB Matching geometry resource
 * @param composite Composite resource of the stereo pair resources
 * @param globals   Global parameter substitution pool
 * 
 * @return bool      True if the angle was successfully computed
 */
bool StereoPairStrategy::computeStereoAngle(const SharedResource &resourceA, 
                                            const SharedResource &resourceB, 
                                            SharedResource &stereo,
                                            const ResourceList &globals) 
                                            const {

  // Wrap in try clause to catch any errors and return without passing the 
  // error up the chain
  QString keyword;
  try {
    // Construct keyword substitution mapping. Individual resources take
    // precedence over stereo parameters
    ResourceList globalA = ResourceList(getGlobals(stereo, globals));
    ResourceList globalB = ResourceList(getGlobals(stereo, globals));
    globalA.prepend(resourceA);
    globalB.prepend(resourceB);


    keyword = "CenterRadius";
    double radiusA = (getStereoValue(keyword, globalA).toDouble()); 
    double radiusB = (getStereoValue(keyword, globalB).toDouble()); 

    keyword = "CenterLatitude";
    double latA = (getStereoValue(keyword, globalA).toDouble()); 
    double latB = (getStereoValue(keyword, globalB).toDouble()); 

    keyword = "CenterLongitude";
    double lonA = (getStereoValue(keyword, globalA).toDouble()); 
    double lonB = (getStereoValue(keyword, globalB).toDouble()); 

    keyword = "TargetCenterDistance";
    double tcentA = (getStereoValue(keyword, globalA).toDouble());
    double tcentB = (getStereoValue(keyword, globalB).toDouble());

    keyword = "SubspacecraftLatitude";
    double sclatA = (getStereoValue(keyword, globalA).toDouble()); 
    double sclatB = (getStereoValue(keyword, globalB).toDouble());

    keyword = "SubspacecraftLongitude";
    double sclonA = (getStereoValue(keyword, globalA).toDouble()); 
    double sclonB = (getStereoValue(keyword, globalB).toDouble()); 

    keyword = "AllDone...";

    double pxA = radiusA/1000.0 * cos(lonA * rpd_c()) * cos(latA * rpd_c());
    double pyA = radiusA/1000.0 * sin(lonA * rpd_c()) * cos(latA * rpd_c());
    double pzA = radiusA/1000.0 * sin(latA * rpd_c());

    double pxB = radiusB/1000.0 * cos(lonB * rpd_c()) * cos(latB * rpd_c());
    double pyB = radiusB/1000.0 * sin(lonB * rpd_c()) * cos(latB * rpd_c());
    double pzB = radiusB/1000.0 * sin(latB * rpd_c());

    double sxA = tcentA * cos(sclonA * rpd_c()) * cos(sclatA * rpd_c());
    double syA = tcentA * sin(sclonA * rpd_c()) * cos(sclatA * rpd_c());
    double szA = tcentA * sin(sclatA * rpd_c());

    double sxB = tcentB * cos(sclonB * rpd_c()) * cos(sclatB * rpd_c());
    double syB = tcentB * sin(sclonB * rpd_c()) * cos(sclatB * rpd_c()); 
    double szB = tcentB * sin(sclatB * rpd_c());

    double vxA = sxA - pxA;
    double vyA = syA - pyA;
    double vzA = szA - pzA;

    double vxB = sxB - pxB;
    double vyB = syB - pyB;
    double vzB = szB - pzB;

    // Compute convergence angle
    double convang = (vxA*vxB + vyA*vyB + vzA*vzB) /
                     ( sqrt(vxA*vxA + vyA*vyA + vzA*vzA) * 
                       sqrt(vxB*vxB + vyB*vyB + vzB*vzB) );


    // Convert to degrees
    convang = acos ( convang ) * dpr_c();
    stereo->add("StereoAngle", QString::number(convang));

    if ( isDebug() ) {
      cout << "StereoAngle = " << convang << "\n";
    }
  } catch (IException &ie) { 
    if ( isDebug() ) {
      cout << "Error computing StereoAngle [Keyword = " << keyword.toStdString() <<  "]:" 
        << ie.what() << "\n"; 
    }
    return (false);
  }

  return (true);
}


/**
 * @brief Compute rank for a set of constraints in a Resource
 * 
 * @history 2015-05-03 Kris Becker
 * 
 * @param value       Value to rank
 * @param thresholds  Rank parameters - must be at least three values
 * 
 * @return double     Rank. If 0, then parameters may not have proper number of 
 *                    values in thresholds or thresholds[3] = 0.
 */
double StereoPairStrategy::computeRank(const double &value,
                                       const QVector<double> &thresholds) 
                                       const {
  double rank = 0;
  Q_ASSERT( thresholds.size() >= 3 );
  if ( thresholds.size()  > 2 ) {
    double range = max(fabs(thresholds[2] - thresholds[0]), 
                       fabs(thresholds[1]-thresholds[2]) );
    rank =  1.0 - (fabs(thresholds[2] - value ) / range); 
    if ( thresholds.size() == 4) { 
      rank *= thresholds[3]; 
    }
  }
  return (rank);
}

}  //namespace Isis
