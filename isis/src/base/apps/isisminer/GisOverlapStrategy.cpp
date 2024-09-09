/**                                                                       
 * @file                                                                  
 * $Revision: 6109 $
 * $Date: 2015-03-18 17:58:32 -0700 (Wed, 18 Mar 2015) $
 * $Id: GisOverlapStrategy.cpp 6109 2015-03-19 00:58:32Z jwbacker@GS.DOI.NET $
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
#include "GisOverlapStrategy.h"

// std library
#include <cmath>

// boost library
#include <boost/foreach.hpp>

// Qt library
#include <QScopedPointer>
#include <QtGlobal>

// other ISIS
#include "IException.h"
#include "IString.h"
#include "NaifStatus.h"
#include "Progress.h"
#include "PvlFlatMap.h"
#include "PvlKeyword.h"
#include "PvlObject.h"
#include "Resource.h"
#include "Strategy.h"
#include "StrategyFactory.h"

using namespace std;

namespace Isis {

  /** 
   * @brief Default constructor - not very useful. 
   *  
   */ 
  GisOverlapStrategy::GisOverlapStrategy() : 
                                Strategy("GisOverlap", "GisOverlap"), 
                                m_overlapMin(0), m_overlapMax(1.0),
                                m_ratioKey("OverlapRatio"),
                                m_assetName("GisOverlap"),
                                m_merge(None), m_suffixA("A"), m_suffixB("B"),
                                m_pairStrategies(),
                                m_overlapStrategies() {
  }
  
  
  /** 
   * @brief Construct GisOverlap strategy from PVL definition. 
   *  
   * This constructor builds the GisOverlap strategy from parameters read from the
   * (assumed isisminer) configuration object description.  Valdiation of 
   * parameters is also performed in the constructor so a valid strategy is always 
   * assured. 
   *  
   * @param definition PVL Object definition of stereo pair strategy
   * @param globals   List of global keywords to use in argument substitutions
   */ 
  GisOverlapStrategy::GisOverlapStrategy(const PvlObject &definition, 
                                         const ResourceList &globals) : 
                                         Strategy(definition, globals), 
                                         m_overlapMin(0), m_overlapMax(1.0),
                                         m_ratioKey("OverlapRatio"),
                                         m_assetName("GisOverlap"),
                                         m_merge(None), m_suffixA("A"), 
                                         m_suffixB("B"),m_pairStrategies(),
                                         m_overlapStrategies() {
  
    PvlFlatMap parms( getDefinitionMap() );

    m_overlapMin = parms.get("OverlapMinimum", "0.0").toDouble();
    m_overlapMax = parms.get("OverlapMaximum", "1.0").toDouble();
    m_ratioKey   = parms.get("OverlapRatioKey", "OverlapRatio"); 
    m_assetName   = parms.get("Asset", "GisOverlap");
     
    QString mergeOpt = parms.get("OverlapMerge", "none").toLower();
    if ( "intersection" == mergeOpt ) {
      m_merge = Intersection;
    }
    else if ( "union" == mergeOpt ) {
      m_merge = Union;
    }
    else if ( "centroid" == mergeOpt ) {
      m_merge = Centroid;
    }
    else if ( "resourcea" == mergeOpt ) {
      m_merge = ResourceA;
    }
    else if ( "resourceb" == mergeOpt ) {
      m_merge = ResourceB;
    }
    else if ( "none" == mergeOpt ) {
      m_merge = None;
    }
    else {
      std::string mess = "OverlapMerge = " + parms.get("OverlapMerge").toStdString() + 
                     " is not a recognized/valid option";
      throw IException(IException::User, mess, _FILEINFO_);
    }
  
    // User want some special suffixes?
    m_suffixA = parms.get("MergeSuffixA", m_suffixA);
    m_suffixB = parms.get("MergeSuffixB", m_suffixB);

    //  Its not an error if there is no pair selection strategies.  In that case,
    //  the active Resource list is used as overlap candidates.
    m_pairStrategies = getMinerStrategies("Strategy", globals);
    if ( m_pairStrategies.isEmpty() ) {
      m_pairStrategies = getMinerStrategies("Candidate", globals);
    }
    if ( isDebug() ) {
      cout << "CandidateMiner Strategy algorithms loaded: " 
           << m_pairStrategies.size() << ".\n"; 
    }

    // Now deterine if there is overlap processing strategies for each set
    m_overlapStrategies = getMinerStrategies("Overlap", globals);
    if ( isDebug() ) {
      cout << "OverlapMiner Strategy algorithms loaded: " 
           << m_overlapStrategies.size() << ".\n"; 
    }

    return; 
  }
  
  
  /** 
   *  @brief Destructor - cleanup is automatic.
   */ 
  GisOverlapStrategy::~GisOverlapStrategy() { }


  /** 
   * @brief Apply overlap algorithm to a list of Resources. 
   *  
   * This is the main entry algorithm for this strategy which reimplements the 
   * Strategy::apply(ResourceList &resources) method. 
   *  
   * @param resources List of Resources to determine overlapping geometries
   * @param globals   List of global keywords to use in argument substitutions
   * @return int      Number of stereo pairs found
   */ 
  int GisOverlapStrategy::apply(ResourceList &resources, 
                                const ResourceList &globals) {

    if ( isDebug() ) {
      cout << "\n=== Running GisOverlap with " << resources.size() << " total.\n";
    }

    //  Create save points
    ResourceList v_active, v_discard;
    BOOST_FOREACH ( SharedResource resource, resources ) {
      if ( resource->isDiscarded() ) { 
        v_discard.append(resource); 
      }
      else { 
        v_active.append(resource); 
      }
    }
  
    // Don't run it if there are no active Resources
    if ( isDebug() ) { cout << "  Total active resources: " << v_active.size() << "\n"; }
    if ( 0 >= v_active.size() ) { return (0); }

    //  Save list before possible deletions in pair candidate list activities
    ResourceList v_saveall = resources;
  
    //  Now get the pair candidates and find the good ones
    ResourceList candidates = overlapCandidates(resources, globals);
    ResourceList goodones   = activeList(candidates);

    // If we have no good candidates, we are also done
    if ( 0 >= goodones.size() ) {
      // Reset proper state
      activateList(v_active);
      resources = v_saveall;
      return (0);
    }

    if ( isDebug() ) {
      cout << "Creating RTree for " << goodones.size() << " candidates\n";
    }

    // Build the RTree geometry
    int ngeoms = qMax(goodones.size(), 2);
    if ( isDebug() ) {
      cout << "Total Geoms: " << ngeoms << "\n";
    }
    GEOSSTRtree *rtree = GEOSSTRtree_create(ngeoms);
    if ( !rtree ) {
      cout << "Address: " << rtree << "\n";
      std::string mess = "GEOS RTree allocation failed for " +
                      toString(goodones.size()) + " geometries.";
        throw IException(IException::Programmer, mess, _FILEINFO_);
    }

    // Set up progress for building tree
    for ( int i = 0; i < goodones.size(); i++ ) {
      if ( goodones[i]->hasValidGeometry() ) {
        GEOSSTRtree_insert(rtree, goodones[i]->geometry()->geometry(),
                           &goodones[i]); 
      }
    }

    if ( isDebug() ) {
        cout << "Overlap query candidate count: " << goodones.size() << "\n";
      }

    //----------------------------------------------------------
    // For each resource in the active set, run an overlap search
    // and process the results

    // Set up progress for building tree
    initProgress(v_active.size());
    int npaired = 0;
    ResourceList noPairs;
    BOOST_FOREACH ( SharedResource resource, v_active ) {
      if ( isDebug() ) {
        cout << "\n===> Running Overlap query for " << resource->name().toStdString() << "\n";
      }

      // Check for valid geometry
      if ( resource->hasValidGeometry() ) {
        ResourceList overlaps;
        GEOSSTRtree_query(rtree, resource->geometry()->geometry(), 
                          &queryCallback, &overlaps);
  
        if ( isDebug() ) {
          cout << "  Query returned " << overlaps.size()  << " candidates.\n";
        }

        // Process the primary Resource and the resulting overlap Resourcelist.
        // Virtual implementations must confirm addition overlapping conditions 
        // are met.
        ResourceList aList = processOverlaps(resource, overlaps, globals);

        if ( isDebug() ) {
          cout << "  Valid overlaps returned for storage: " << aList.size()  << "\n";
        }

        // Create the asset list if we have any pairs
        if ( aList.size() > 0 ) {
          QVariant v_asset(QVariant::fromValue(aList));
          resource->addAsset(m_assetName, v_asset);
          npaired++;
  
          // If provided, apply strategies for the pair
          ResourceList overlapper;
          overlapper.append(resource);
  
          // If users want to process each overlap immediately, do it here.
          // Note that current resources is added to the global parameter list
          // to resolve keywords from this resource.
          BOOST_FOREACH ( SharedStrategy strategy, m_overlapStrategies  ) {
            strategy->apply(overlapper, getGlobals(resource, globals));
          }
        }
        else {
          noPairs.append(resource);
        }
      }

      processed();
    }
  
    // Done with the query tree
    GEOSSTRtree_destroy(rtree);

   // Restore save states
    activateList(v_active);
    deactivateList(v_discard);
    deactivateList(noPairs);
    resources = v_saveall;
    return (npaired);
  }

  /** Returns GIS merge option as specfied by user */
  GisOverlapStrategy::GisMergeOption GisOverlapStrategy::mergeOption() const{
    return ( m_merge );
  }

  /** Set asset name if it needs to be overridden */
  void GisOverlapStrategy::setAssetName(const QString &assetName) {
    m_assetName = assetName;
    return;
  }
  
/**
 * @brief Determine overlap candidates from resources and strategies 
 *  
 * This method determines overlap candidates. This candidates can come from the 
 * existing list of resources or may be created (or selectively determined) 
 * from a series of strategies. 
 *  
 * If the user did not provide any strategies in the GisOverlap config object, 
 * then the input list is returned as the candidate list. However, users can 
 * add a new IsisMiner object that selectively can manipulate the list or 
 * create an entirely new list to determine the overlap candidates. 
 * 
 * @author 2015-04-30 Kris Becker
 * 
 * @param resources Current resource list that where candidates may come from
 * @param globals   List of global keywords to use in argument substitutions
 * 
 * @return ResourceList List of candidate resources to use as overlap 
 *         candidates
 */
  ResourceList GisOverlapStrategy::overlapCandidates(ResourceList &resources,
                                                     const ResourceList &globals)  {
  
    //  Invoke the pair candidate strategies if specified.  If not, then the
    //  input list is returned as the pair list. Users can delete all the 
    // resources and get a completely different list with available strategies.
    ResourceList candidates = resources;
    BOOST_FOREACH ( SharedStrategy strategy, m_pairStrategies  ) {
      strategy->apply(candidates, globals);
    }
  
    return (candidates);
  }
  

/**
 * @brief Process overlapping Resources 
 *  
 * This method will process a single resource and all its overlapping 
 * candidates. The list is determined by evaluating the GIS footprints of all 
 * candidates. 
 *  
 * Note that the list may not overlap at all because the process used to 
 * determine overlapping geometries (contained in the overlaps list) uses the 
 * bounding box (or envelope) for highly efficient GIS operations. Furthermore, 
 * the overlap ratios of both resources is computed and may eliminate any 
 * resource candidates that do not satisify ratio contraints (if provided by 
 * user). 
 *  
 * For overlapping pairs that statisfy constraints, the actual processing is 
 * dispatched to the virtual processOverlap() method, which may be reimplemented
 * by inheriting classes. 
 *  
 * @author 2015-03-27 Kris Becker
 * 
 * @param resource Current resource for which to determine overlap candidates.
 * @param overlaps List of candidates that may overlap resource.
 * @param globals   List of global keywords to use in argument substitutions
 * 
 * @return ResourceList List of processed overlaps that are found to satisfy 
 *         constraints and additional processing by processOverlap().
 */
  ResourceList GisOverlapStrategy::processOverlaps(SharedResource &resource,
                                                   ResourceList &overlaps,
                                                   const ResourceList &globals)  {
    ResourceList matches;
    SharedGisGeometry rgeom(resource->geometry());
    BOOST_FOREACH ( SharedResource candidate, overlaps ) {

      // Must ensure we do not consider the original resource itself
      if ( !resource->isEqual(*candidate)) {
        double ratioA = rgeom->intersectRatio(*candidate->geometry()); 
        double ratioB = candidate->geometry()->intersectRatio(*rgeom); 
        double EPSILON = 1e-9;

        if ( isDebug() ) {
          cout << "\nSource " << resource->name().toStdString() << " overlaps "
               << candidate->name().toStdString() << " with ratio of " << ratioA << ", " 
               << ratioB << "\n";
        }

        // For efficiency, if 0 is returned it is assumed to not intersect at all
        // no matter what the OverlapMinimum is set to. Note a NULL Resource may
        // be returned by processOverlap() so make sure we check validity.
        if ( ratioA > 0.0 ) {
          if ( (ratioA > m_overlapMin || fabs(ratioA - m_overlapMin) < EPSILON) && (ratioA < m_overlapMax || fabs(ratioA - m_overlapMax) < EPSILON) ) {
            SharedResource composite = processOverlap(resource, candidate, 
                                                      ratioA, ratioB,
                                                      globals);
            if ( !composite.isNull() ) { matches.append(composite); }
          }
        }
      }
    }
  
    return (matches);
  }
  
/**
 * @brief Process a pair of individual overlaps with the provided ratio
 *  
 * This method creates a new Resource by combining the contents of the 
 * overlapping Resource pair keywords. The resourceA keywords have an A 
 * appended to each keyword. resourceB has B appended. A list of keywords to 
 * propagate from each Resource can be provided in the PropagateKeywords 
 * keyword provided by the user in the Strategy object configuration file (the 
 * Strategy class handles this feature when creating the composite). 
 *  
 * The user may also specify how the two GIS geometries can be combined. They 
 * can choose to compute the intersection or union of the two geometries. If 
 * the option is not provided, there will be no geometry created for the 
 * composite Resource. 
 *  
 * Note that other Strategy algorithms that use this class can implement their 
 * own version of this method, but can also call it explicitly to use the 
 * composite function provided by this method. 
 *  
 * @author 2015-04-03 Kris Becker
 * 
 * @param resourceA Primary Resource driving the overlap processing
 * @param resourceB Matched image with validated percentage of overlap
 * @param overlapRatio Ratio of common region of overlap of resourceB with 
 *                     respect to resourceA
 * @param globals   List of global keywords to use in argument substitutions
 * 
 * @return SharedResource Composite resource containing keywords from both 
 *                        resourceA and resourceB including an optional
 *                        composite geoemtery
 */
  SharedResource GisOverlapStrategy::processOverlap(SharedResource &resourceA,
                                                    SharedResource &resourceB,
                                                    const double &ovrRatioA,
                                                    const double &ovrRatioB,
                                                    const ResourceList &globals) {

    // Get the geom
    SharedResource rmerged = composite(resourceA, resourceB);
    rmerged->add(m_ratioKey+m_suffixA, QString::number(ovrRatioA));
    rmerged->add(m_ratioKey+m_suffixB, QString::number(ovrRatioB));
    rmerged->add(m_ratioKey, QString::number(ovrRatioA));

    if ( isDebug() ) {
      cout << "Merging " << resourceA->name().toStdString() << " and "
           << resourceB->name().toStdString() << " creates " << rmerged->keys().size() << " keys.\n";
    }

    // Determine geometry disposition
    if ( Intersection == m_merge ) {
      SharedGisGeometry geom(resourceA->geometry()->intersection(*resourceB->geometry()));
      rmerged->add(geom);
      double x, y;
      if ( geom->centroid(x, y) ) {
        rmerged->add( "GisOverlapCentroidX", QString::number(x));
        rmerged->add( "GisOverlapCentroidY", QString::number(y));
      }
    }
    else if ( Union == m_merge ) {
      SharedGisGeometry geom(resourceA->geometry()->g_union(*resourceB->geometry()));
      rmerged->add(geom);
      double x, y;
      if ( geom->centroid(x, y) ) {
        rmerged->add( "GisOverlapCentroidX", QString::number(x));
        rmerged->add( "GisOverlapCentroidY", QString::number(y));
      }
    }
    else if ( Centroid == m_merge ) {
      QScopedPointer<GisGeometry> geom(resourceA->geometry()->intersection(*resourceB->geometry()));
      double x, y;
      // Only works if the centroid value is defined
      if ( geom->centroid(x, y) ) {
        rmerged->add( "GisOverlapCentroidX", QString::number(x));
        rmerged->add( "GisOverlapCentroidY", QString::number(y));
        SharedGisGeometry centroid(geom->centroid());
        rmerged->add(centroid);
      }
    }
    else if ( ResourceA == m_merge ) {
      SharedGisGeometry geom(resourceA->geometry());
      double x, y;
      if ( geom->centroid(x, y) ) {
        rmerged->add( "GisOverlapCentroidX", QString::number(x));
        rmerged->add( "GisOverlapCentroidY", QString::number(y));
      }
      rmerged->add(geom);
    }
    else if ( ResourceB == m_merge ) {
      SharedGisGeometry geom(resourceB->geometry());
      double x, y;
      if ( geom->centroid(x, y) ) {
        rmerged->add( "GisOverlapCentroidX", QString::number(x));
        rmerged->add( "GisOverlapCentroidY", QString::number(y));
      }
      rmerged->add(geom);
    }

    if ( isDebug() ) {
      if ( rmerged->exists("GisOverlapCentroidX") ) {
        cout << "GisOverlapCentroidX = " << rmerged->value("GisOverlapCentroidX").toStdString() << "\n";
        cout << "GisOverlapCentroidY = " << rmerged->value("GisOverlapCentroidY").toStdString() << "\n";
      }
    }
    return (rmerged);
  }


/**
 * @brief Retrieves strategies from a configuration file or object 
 * 
 * @author  2015-06-12 Kris Becker 
 * 
 * @param minerName Name of Strategy set to retrieve
 * @param globals   Global parameter list
 * 
 * @return StrategyList 
 */
   StrategyList GisOverlapStrategy::getMinerStrategies(const QString &minerName, 
                                             const ResourceList &globals) const {
     StrategyFactory *factory = StrategyFactory::instance();

     StrategyList miner;
     QString config = translateKeywordArgs(minerName+"ConfigFile", globals);
     if ( !config.isEmpty() ) {
       miner = factory->buildRun(config, globals);
     }
     else if ( getDefinition().hasObject(minerName.toStdString() + "Miner") ) {
       miner = factory->buildRun(getDefinition().findObject(minerName.toStdString() + "Miner"), 
                                            globals);
     }
     
     return (miner);
   }


   /** Return the merge keyword suffix for source */
    QString GisOverlapStrategy::suffixA() const {
      return (m_suffixA);
    }

    /** Return merge suffix for match */
    QString GisOverlapStrategy::suffixB() const {
      return (m_suffixB);
    }


}  // namespace Isis
