/**                                                                       
 * @file                                                                  
 * $Revision: 6513 $
 * $Date: 2016-01-14 16:04:44 -0700 (Thu, 14 Jan 2016) $
 * $Id: AssetSidebarStrategy.cpp 6513 2016-01-14 23:04:44Z kbecker@GS.DOI.NET $
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
#include "AssetSidebarStrategy.h"

// boost library
#include <boost/foreach.hpp>

// other ISIS
#include "Application.h"
#include "PvlFlatMap.h"
#include "PvlObject.h"
#include "Resource.h"

using namespace std;

namespace Isis {

  /** 
   * Default constructor
   * 
   * Note: This method is not called by isisminer
   * or any of it's support classes so it is not covered
   * by appTests.  It is not possible to get full test
   * coverage unless isisminer implementation changes
   * or this class is moved to an objs directory with a unitTest.
   */  
  AssetSidebarStrategy::AssetSidebarStrategy() : SidebarStrategy(), 
                        m_asset(), m_create(false), m_removeEmpties(true),
                        m_source(FromCopy) {
    setName("AssetSidebar");
    setType("AssetSidebar");
  }
  

  /** 
   * @brief Constructor loads from a Strategy object AssetSidebar definition
   * 
   * This constructor loads and retains processing parameters from the AssetSidebar
   * Strategy object definition as (typically) read from the configuration file.
   * 
   * @author 2012-07-15 Kris Becker
   * 
   * @param definition AssetSidebar Strategy PVL object definition
   * @param globals Global Resource of keywords
   */  
  AssetSidebarStrategy::AssetSidebarStrategy(const PvlObject &definition, 
                                             const ResourceList &globals) : 
                                             SidebarStrategy(definition, globals), 
                                             m_asset(), m_create(false),
                                             m_removeEmpties(true),
                                             m_source(FromCopy) {
  
    PvlFlatMap parms(getDefinitionMap());
    m_asset         = parms.get("Asset");
    m_create        = ("create" == parms.get("Operation", "process").toLower());
    m_removeEmpties = toBool(parms.get("ClearOnEmpty", "true"));

    QString source = parms.get("CreateSource", "copy").toLower();
    if ( "clone" == source ) { m_source = FromClone; }
    else if ( "none" == source ) { m_source = FromNone; }
    else { m_source = FromCopy; }

    return; 
  }
  
  
  /**
   * Destructor
   */
  AssetSidebarStrategy::~AssetSidebarStrategy() { }



  /** Remove existing asset if create option is invoked */
  int AssetSidebarStrategy::preRunProcess(ResourceList &resources, 
                                     const ResourceList &globals) {

    //  Count current active ones whilst incrementing the counter
    BOOST_FOREACH ( SharedResource resource, resources ) {
      if ( !resource->isDiscarded() ) { 
        processed(); 
        // If creating a an existing one of the complete resource list by
        // default. Either a copy or a clone.
        if ( true == m_create ) {
          if ( FromCopy == m_source ) {
            QVariant asset(QVariant::fromValue(copyList(resources))); 
            resource->addAsset(m_asset, asset);
          }
          else if ( FromClone == m_source ) {
            QVariant asset(QVariant::fromValue(cloneList(resources))); 
            resource->addAsset(m_asset, asset);
          }
          else {
            resource->removeAsset( m_asset );
          }
        }
      }
    }

    return (totalProcessed());
  }


  /** 
   * @brief Applies a strategy to assets of active resources.
   * 
   * This method applies another strategy to active resources' assets.
   * 
   * When the Operation keyword has a value of "create", this method will not try to obtain
   * any assets contained in active resources. This is in order to create new assets to 
   * add to active resources. If this operation is not "create", then this method will
   * attempt to obtain assets that are contained in active resources, apply the strategy to
   * these assets, then add the processed assets to the active resources.
   * 
   * @author 2012-07-15 Kris Becker
   * 
   * @param strategy The strategy to apply to assets
   * @param resources The active resources
   * @param globals Global Resource of keywords
   * @return int The total number of assets processed in the strategy
   */  
  int AssetSidebarStrategy::apply(SharedStrategy &strategy, 
                                  ResourceList &resources, 
                                  const ResourceList &globals) { 
    int nassets = 0;
    BOOST_FOREACH ( SharedResource resource, resources ) {
      if ( resource->isActive() ) {
        ResourceList assetList;

        // Obtain assets from the current resource if they exist
        if ( resource->hasAsset(m_asset) ) {
          QVariant asset = resource->asset(m_asset);
          // Note: this conditional will not evaluate as False
          // ResourceList is declared as a Q_METATYPE (QVariant) in Resource.h
          // Thus, the QVariant asset will be converted to ResourceList, another QVariant
          if ( asset.canConvert<ResourceList>() ) {
            assetList = asset.value<ResourceList>();
          }
        }
        
        //  Apply the strategy to the list of assets
        nassets += strategy->apply( assetList, getGlobals(resource, globals) ); 
        
        //  Add the list to the asset
        QVariant v_asset(QVariant::fromValue(assetList));
        resource->addAsset(m_asset, v_asset);
      }
    }
  
    return (nassets);
  }

/**
 * @brief Post run processing after execution of all strategies 
 *  
 * This method will be invoked by SidebarStrategy after all the strategies have 
 * been ran. This implementation will check for the existance of the named 
 * asset and determine if empty lists exists. If the user requests, removed 
 * these assets. 
 *  
 * Note this will only operate on active Resources. 
 *  
 * The active status of the Resources is unaltered (users can use the 
 * ResourceManager to do that). 
 * 
 * @author 2015-09-15 Kris Becker
 * 
 * @param resources List of Resources to check for assets
 * @param globals   Global parameter pool
 * 
 * @return int Number of empty Asset resource lists found and removed
 */
  int AssetSidebarStrategy::postRunProcess(ResourceList &resources, 
                                          const ResourceList &globals) {

    // Set up incoming asset handling conditions
    if ( m_removeEmpties ) {
      BOOST_FOREACH ( SharedResource resource, resources ) {
        if ( resource->isActive() ) {
          if ( resource->hasAsset(m_asset) ) {
            QVariant asset = resource->asset(m_asset);
            // Note: this conditional will not evaluate as False
            // ResourceList is declared as a Q_METATYPE (QVariant) in Resource.h
            // Thus, the QVariant asset will be converted to ResourceList, another QVariant
            if ( asset.canConvert<ResourceList>() ) {
              ResourceList assetList = asset.value<ResourceList>();
              if ( assetList.size() == 0) {
                resource->removeAsset(m_asset);
              }
            }
          }
        }
      }
    }
    return ( SidebarStrategy::postRunProcess(resources, globals) );
  }


}  //namespace Isis
