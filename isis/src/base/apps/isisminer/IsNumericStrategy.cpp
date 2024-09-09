/**                                                                       
 * @file                                                                  
 * $Revision: 6187 $
 * $Date: 2015-05-11 17:31:51 -0700 (Mon, 11 May 2015) $
 * $Id: IsNumericStrategy.cpp 6187 2015-05-12 00:31:51Z kbecker@GS.DOI.NET $
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
#include "IsNumericStrategy.h"

// boost library
#include <boost/foreach.hpp>

// other ISIS
#include "IException.h"
#include "PvlFlatMap.h"
#include "PvlObject.h"
#include "Resource.h"
#include "Strategy.h"

using namespace std;

namespace Isis {

  /** 
   *  Default constructor.
   */
  IsNumericStrategy::IsNumericStrategy() : Strategy("IsNumeric", "IsNumeric"), 
                                           m_keys()  { 
  }
  

  /**
   * @brief Constructor loads from a Strategy object IsNumeric definition
   *  
   * This constructor loads and retains processing parameters from the IsNumeric 
   * Strategy object definition as (typically) read from the configuration file. 
   *  
   * @author 2013-02-19 Kris Becker
   * 
   * @param definition IsNumeric Strategy PVL object definition
   * @param globals   List of global keywords to use in argument substitutions
   */
  IsNumericStrategy::IsNumericStrategy(const PvlObject &definition, 
                                       const ResourceList &globals) : 
                                       Strategy(definition, globals), m_keys() {
  
    PvlFlatMap parms( getDefinitionMap() );
    m_keys = parms.allValues("Keyword");
    return;
  }
  

  /** 
   *  Destructor.
   */
  IsNumericStrategy::~IsNumericStrategy() { 
  }
  

  /**
   * @brief Test a single Resource for a numerical keyword value 
   *  
   * This Strategy retains a single resource if its keyword value
   * is numeric. The value is considered numeric if it can be implicitly
   * converted into a floating point number. If the keyword does not exist
   * it's value is not considered numeric.
   * 
   * @author 2013-02-19 Kris Becker
   * 
   * @param resource Resource containing keywords
   * @param globals   List of global keywords to use in argument substitutions
   * 
   * @return int Returns 1 if the Resources is retained, 0 if deactivated
   */
  int IsNumericStrategy::apply(SharedResource &resource, 
                               const ResourceList &globals) { 
   
    QStringList badkeys;
    BOOST_FOREACH ( QString key, m_keys ) {
      if ( !resource->exists(key) ) {
         badkeys.push_back(key);
      }
      else {
        try {
          resource->value(key).toDouble();
        }
        catch (IException &ie) {
          badkeys.push_back(key);
        }
      }
    }
  
    //  If any keys are in the bad list, those are not numeric
    if ( !badkeys.isEmpty() ) {
      resource->discard();
      return (0);
    }
  
    //  All good!
    return (1);
  }

}  //namespace Isis
