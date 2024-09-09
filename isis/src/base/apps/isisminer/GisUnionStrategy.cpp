/**                                                                       
 * @file                                                                  
 * $Revision: 6187 $
 * $Date: 2015-05-11 17:31:51 -0700 (Mon, 11 May 2015) $
 * $Id: GisUnionStrategy.cpp 6187 2015-05-12 00:31:51Z kbecker@GS.DOI.NET $
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
#include "GisUnionStrategy.h"

// other ISIS
#include "GisGeometry.h"
#include "Resource.h"
#include "Strategy.h"

using namespace std;

namespace Isis {

  /** 
   *  
   */  
  GisUnionStrategy::GisUnionStrategy() : Strategy("GisUnion", "GisUnion"),
                          m_overlapMin(0.0), m_overlapMax(100.0),
                          m_ratioKey(""), m_union() { 
  }
  

  /** 
   *  
   * @param definition
   * @param globals   List of global keywords to use in argument substitutions
   */  
  GisUnionStrategy::GisUnionStrategy(const PvlObject &definition, 
                                     const ResourceList &globals) : 
                                     Strategy(definition, globals),
                                     m_overlapMin(0.0), m_overlapMax(100.0),
                                     m_ratioKey(""), m_union(0) {
  
     PvlFlatMap parms( getDefinitionMap() );
     m_overlapMin = parms.get("OverlapMinimum", "0.0").toDouble();
     m_overlapMax = parms.get("OverlapMaximum", "1.0").toDouble();
     m_ratioKey   = parms.get("RatioRef", "UnionOverlapRatio"); 
  }
  

  /** 
   *  
   */  
  GisUnionStrategy::~GisUnionStrategy() { 
  }
  

  /** 
   * Union all geometries that satisfy overlap percentages
   * @param resource
   * @param globals   List of global keywords to use in argument substitutions
   * @return int 
   */
  int GisUnionStrategy::apply(SharedResource &resource,
                              const ResourceList &globals) {
  
    if ( resource->hasValidGeometry() ) {
      SharedGisGeometry geom(resource->geometry());
      if ( !m_union.isNull() ) {
        // Union geometry is present and good
        double ratio = geom->intersectRatio(*m_union);
        resource->add(m_ratioKey, QString::number(ratio));
        if ( (ratio >= m_overlapMin) && (ratio <= m_overlapMax) ) {
           m_union = SharedGisGeometry(m_union->g_union(*geom));
        }
        else {
          // Geometry does not satisfy overlap ratio constraints
          resource->discard();
        }
      }
      else {
        // Union geometry needs initializing or is not good.
        m_union = SharedGisGeometry(geom->clone());
        resource->add(m_ratioKey, "1.0");
      }
    }
    else {
      // Geometry is invalid
      resource->discard();
    }
    return (1);
  }

}  //namespace Isis
