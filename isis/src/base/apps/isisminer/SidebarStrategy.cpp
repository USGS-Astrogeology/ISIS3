/**                                                                       
 * @file                                                                  
 * $Revision: 6513 $
 * $Date: 2016-01-14 16:04:44 -0700 (Thu, 14 Jan 2016) $
 * $Id: SidebarStrategy.cpp 6513 2016-01-14 23:04:44Z kbecker@GS.DOI.NET $
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
#include "SidebarStrategy.h"

// boost library
#include <boost/foreach.hpp>

// other ISIS
#include "IException.h"
#include "PvlFlatMap.h"
#include "PvlObject.h"
#include "Resource.h"
#include "Strategy.h"
#include "StrategyFactory.h"

using namespace std;

namespace Isis {

  /** 
   * Default constructor.
   *
   * Note: This method is not called by isisminer
   * or any of it's support classes so it is not covered
   * by appTests.  It is not possible to get full test
   * coverage unless isisminer implementation changes
   * or this class is moved to an objs directory with a unitTest. 
   */
  SidebarStrategy::SidebarStrategy() : Strategy("SideBar", "Sidebar"),  
                                       m_savepoint(true), m_savedeletes(true),
                                       m_strategies() { 
  }
  
  
  /** 
   * @brief Constructor loads from a Strategy object Sidebar definition.
   *
   * This constructor loads and retains processing parameters from the Sidebar
   * Strategy object definition as typically read from the configuration file.
   *
   * @param definition PvlObject of the strategy object definition
   * @param globals   List of global keywords to use in argument substitutions

   */
  SidebarStrategy::SidebarStrategy(const PvlObject &definition, 
                                   const ResourceList &globals) : 
                                   Strategy(definition, globals), 
                                   m_savepoint(true), m_savedeletes(true),
                                   m_strategies()  {
  
    PvlFlatMap parms(getDefinitionMap());
    m_savepoint  = toBool(parms.get("SavePoint", "True"));
    m_savedeletes = toBool(parms.get("SaveDelete", parms.get("SavePoint", "True")));
    
    StrategyFactory *factory = StrategyFactory::instance();
    QString config = translateKeywordArgs("StrategyConfigFile", globals);
    if ( !config.isEmpty() ) {
      if ( isDebug() ) cout << "Loading IsisMiner Objects from external config file " << config << "\n";
      m_strategies = factory->buildRun(config);
    }
    else if ( getDefinition().hasObject("IsisMiner") ) {
      if ( isDebug() ) cout << "Loading IsisMiner Objects from input CONFIG file...\n";
      m_strategies = factory->buildRun(getDefinition().findObject("IsisMiner"));
    }
    else {
      std::string mess = "No IsisMiner strategies found in " + name() + " Sidebar.";
      throw IException(IException::User, mess, _FILEINFO_);
    }
  
    if ( isDebug() ) {
      cout << type() << "::" << name() << "::StrategiesLoaded = "
           << m_strategies.size() << "\n";
    }
  
    return;
  }
  
  
  /** 
   * Destructor.
   */
  SidebarStrategy::~SidebarStrategy() {  }

  
  /** 
   * @brief Applies the strategies to the resources.
   *
   * Applies the strategies to the resources while maintaining activeness if
   * a save point is set. In addition it will restore any resources deleted
   * by the strategy if save deletes is set. Otherwise it applies the
   * strategies like normal configuration.
   *
   * Note: BOOST_FOREACH aren't tested (where there isn't anything to iterate over)
   * 
   * @param resources ResourceList of the resources that the Strategies are
   *                  applied to.
   * @param globals   List of global keywords to use in argument substitutions
   *
   * @return int The number of active resources after applying the strategies.
   */
  int SidebarStrategy::apply(ResourceList &resources, const ResourceList &globals) { 
  
    ResourceList v_active, v_discard, v_saveall;
  //  int inTotal(resources.size());
  
    //  Create save points
    if ( m_savepoint ) {
      BOOST_FOREACH ( SharedResource resource, resources ) {
        if ( resource->isDiscarded() ) { 
          v_discard.append(resource); 
        }
        else {
          v_active.append(resource);  
        }
      }
    }
  
    //  Save list before possible deletions
    if ( m_savedeletes ) {  
      v_saveall = resources;  
    }
  
  
    //------------------------------------------------------------------------
    // Apply resources
    //------------------------------------------------------------------------ 
    (void) preRunProcess(resources, globals); 
    int nth(0);
    BOOST_FOREACH ( SharedStrategy strategy, m_strategies ) {
      if  ( isDebug() ) cout << "\nRunning SideBar::" << strategy->type() << "::" 
                             << strategy->name() << "\n"; 
      preStrategyProcess(nth, strategy.data(), resources, globals);
      int n = apply(strategy, resources, globals);
      postStrategyProcess(nth, strategy.data(), resources, globals);
      unsigned int ntotal = strategy->totalProcessed();
      if ( isDebug() ) {
        cout << n << " of " << ntotal << " processed in " << strategy->type() << "::"
             << strategy->name() << "\n";
      }
      nth++;
    }
    int nvalid = postRunProcess(resources, globals); 

    // Restore save states
    if ( m_savepoint  ) {
      BOOST_FOREACH ( SharedResource resource, v_active ) { 
        resource->activate(); 
      }
      BOOST_FOREACH ( SharedResource resource, v_discard ) { 
        resource->discard(); 
      }
    }
    
    
    // Restore deleted state
    if ( m_savedeletes ) {  
      resources = v_saveall; 
    }
    return ( nvalid );
  }
  

  /** Gives deriving classes an opportunity to set up prior to each strategy
   *  ran. When nthStrategy == 0, the first strategy is about to be run. */
  int SidebarStrategy::preRunProcess(ResourceList &resources, 
                                     const ResourceList &globals) {
    //  Count current active ones whilst incrementing the counter
    BOOST_FOREACH ( SharedResource resource, resources ) {
      if ( !resource->isDiscarded() ) { 
        processed(); 
      }
    }

    return (totalProcessed());
  }
  

  int SidebarStrategy::preStrategyProcess(const int &nth, const Strategy *strategy,
                                          ResourceList &resources, 
                                          const ResourceList &globals) {
    return (0);
  }

  /** 
   * Applies a single strategy to the resources.
   *
   * @param strategy SharedStrategy to be applied to the resources.
   * @param resources ResourceList of the resources that the strategy will be
   *                  applied to.
   * @param globals   List of global keywords to use in argument substitutions
   *
   * @return int The return value of applying the strategy, which depends on
   *             the strategy applied.
   */
  int SidebarStrategy::apply(SharedStrategy &strategy, ResourceList &resources,
                             const ResourceList &globals) {
    return (strategy->apply(resources, globals));
  }


  int SidebarStrategy::postStrategyProcess(const int &nth, const Strategy *strategy,
                                           ResourceList &resources, 
                                           const ResourceList &globals) {
    return ( 1 );
  }

  /** Gives deriving classes chance to run clean up processing after all
   *  strategies have been executed */
  int SidebarStrategy::postRunProcess(ResourceList &resources, 
                                      const ResourceList &globals) {
    return ( countActive(resources) );
  }

}  //namespace Isis
