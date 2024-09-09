/**                                                                       
 * @file                                                                  
 * $Revision: 6513 $
 * $Date: 2016-01-14 16:04:44 -0700 (Thu, 14 Jan 2016) $
 * $Id: LimitStrategy.cpp 6513 2016-01-14 23:04:44Z kbecker@GS.DOI.NET $
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
#include "LimitStrategy.h"

// other ISIS
#include "PvlObject.h"
#include "Resource.h"

using namespace std;

namespace Isis {

  /** 
   *  Default constructor.
   */
  LimitStrategy::LimitStrategy() : CalculatorStrategy() { 
    setName("Limit");
    setType("Limit");
  }
  

  /**
   * @brief Constructor loads from a Strategy object Limit 
   *        definition
   *  
   * This constructor loads and retains processing parameters from 
   * the LimitStrategy Strategy object definition as (typically) 
   * read from the configuration file. 
   * 
   * @author 2013-02-19 Kris Becker
   * 
   * @param definition LimitStrategy Strategy PVL object definition
   * @param globals   List of global keywords to use in argument substitutions
   */
  LimitStrategy::LimitStrategy(const PvlObject &definition, 
                               const ResourceList &globals) : 
                               CalculatorStrategy(definition, globals) { 
  }
  

  /** 
   *  Destructor.
   */
  LimitStrategy::~LimitStrategy() { 
  }
  

  /**
   * @brief Checks if Resource keyword values are in numeric 
   *        limits, if not the keyword is deactivated
   *  
   * This Strategy computes a single Resource keyword and checks 
   * if the keyword is within the limits. If the keyword is within
   * the limits, the keyword is retained. If the keyword is 
   * outside of the limits, it is deactivated. 
   *  
   * @author 2015-02-24 Kris Becker 
   *  
   * @param resource Resource containing keywords 
   * @param globals   List of global keywords to use in argument substitutions
   *  
   * @return int Returns 1 if the Resource is within the limits, 
   *         0 if out of bounds
   */
  int LimitStrategy::apply(SharedResource &resource, const ResourceList &globals) { 
    int n = CalculatorStrategy::apply(resource, globals); 
    if ( (0 == n) || (resource->isDiscarded()) ) return (0);

    if ( isDebug() ) {
      cout << "Limit::Resource " << resource->name().toStdString() << " result = " << result() << "\n";
    }

    if (!(result() != 0.0)) {
      resource->discard();
      return (0);
    }
    return (1);
  }

}  //namespace Isis
