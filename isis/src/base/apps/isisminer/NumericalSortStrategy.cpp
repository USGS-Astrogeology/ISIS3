/**                                                                       
 * @file                                                                  
 * $Revision: 6218 $
 * $Date: 2015-06-05 10:33:22 -0700 (Fri, 05 Jun 2015) $
 * $Id: NumericalSortStrategy.cpp 6218 2015-06-05 17:33:22Z kbecker@GS.DOI.NET $
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
#include "NumericalSortStrategy.h"

// Qt library
#include <QtAlgorithms>

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
  NumericalSortStrategy::NumericalSortStrategy() : 
                         Strategy("NumericalSort", "NumericalSort"), 
                         m_sortKey(), m_order("ascending") { 
  }
  
  
  /**
   * @brief Constructor loads from a Strategy object NumericalSort definition
   * 
   * This constructor loads and retains processing parameters from the NumericalSort 
   * Strategy object definition as (typically) read from the configuration file. 
   *  
   * @author 2012-07-25 Kris Becker
   * 
   * @param definition NumericalSort Strategy PVL object definition
   * @param globals   List of global keywords to use in argument substitutions
   *  
   * @throw IException::Programmer "Specified sort Order is invalid. 
   *                                Must be Ascending or Descending.";
   */
  NumericalSortStrategy::NumericalSortStrategy(const PvlObject &definition, 
                                               const ResourceList &globals) : 
                                               Strategy(definition, globals), 
                                               m_sortKey(), m_order("ascending") {
  
    PvlFlatMap parms( getDefinitionMap() );
    m_sortKey = parms.get("SortKey");
  
    if ( parms.exists("Order") ) {
      QString order = parms.get("Order");
      m_order = order.toLower();
  
      // check validity of keyword
      if ( (m_order != "ascending") && (m_order != "descending") ) {
        std::string mess = "Specified sort Order [" + order + "] is invalid."
                       " Must be Ascending or Descending.";
        throw IException(IException::Programmer, mess, _FILEINFO_);
      }
    }
  
    return;
  }
  
  
  /** 
   *  Destructor.
   */
  NumericalSortStrategy::~NumericalSortStrategy() { 
  }
  
  
  /**
   * @brief Sort a list of Resources according to a numerical keyword value
   *  
   * This Strategy sorts a list of Resources in ascending or descending order.
   * The sort operates according to the SortKey keyword value provided in the 
   * PVL object definition.
   * 
   * @author 2013-02-19 Kris Becker
   * 
   * @param resources ResourceList containing Resources
   * @param globals   List of global keywords to use in argument substitutions
   * 
   * @return int Returns the number of resources that were sorted.
   */
  int NumericalSortStrategy::apply(ResourceList &resources, 
                                   const ResourceList &globals) { 
    if ( "ascending" == m_order) {
      std::sort(resources.begin(), resources.end(), SortAscending(m_sortKey));
    }
    else {  // ("descending" == m_order) 
      std::sort(resources.begin(), resources.end(), SortDescending(m_sortKey));
    }
  
    //  Keeps the Resource by just running the counter
    return (applyToResources(resources, globals));
  }
  
  
  /** 
   * @param sortKey
   */  
  SortAscending::SortAscending(const QString &sortKey) : m_sortKey(sortKey) { 
  }


  /** 
   *  
   */  
  SortAscending::~SortAscending() { 
  }
  

  /** 
   * @brief Function call operator to compare two SharedResources
   * 
   * Compares two SharedResources using the greater than operator. If the first SharedResource
   * is discarded, then the order is unsatisfied (i.e. returns false).
   * 
   * @param a The SharedResources to compare
   * @param b The SharedResources to compare
   * 
   * @return bool Returns true if the first SharedResource's value
   * is less than the second SharedResource's value
   */
  bool SortAscending::operator()(const SharedResource &a, const SharedResource &b) const {
    if ( a->isDiscarded() ) { 
      return (false); 
    }
    if ( b->isDiscarded() ) { 
      return (true);  
    }
    return (toDouble(a->value(m_sortKey)) < toDouble(b->value(m_sortKey)));
  }
  
    
  /** 
   * @param sortKey 
   */  
  SortDescending::SortDescending(const QString &sortKey) : m_sortKey(sortKey) { 
  }
  
  
  /** 
   *  
   */  
  SortDescending::~SortDescending() { 
  }
  

  /** 
   * @brief Function call operator to compare two SharedResources
   * 
   * Compares two SharedResources using the greater than operator. If the first SharedResource
   * is discarded, then the order is unsatisfied (i.e. returns false).
   * 
   * @param a The SharedResources to compare
   * @param b The SharedResources to compare
   * 
   * @return bool Returns true if the first SharedResource's value
   * is greater than the second SharedResource's values
   */
  bool SortDescending::operator()(const SharedResource &a, const SharedResource &b) const {
    if ( a->isDiscarded() ) { 
      return (false);
    }
    if ( b->isDiscarded() ) { 
      return (true);  
    }
    return (toDouble(a->value(m_sortKey)) > toDouble(b->value(m_sortKey)));
  }  

}  //namespace Isis
