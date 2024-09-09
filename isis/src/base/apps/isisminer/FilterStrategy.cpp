/**                                                                       
 * @file                                                                  
 * $Revision: 6513 $
 * $Date: 2016-01-14 16:04:44 -0700 (Thu, 14 Jan 2016) $
 * $Id: FilterStrategy.cpp 6513 2016-01-14 23:04:44Z kbecker@GS.DOI.NET $
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
#include "FilterStrategy.h"

// other ISIS
#include "PvlFlatMap.h"
#include "PvlObject.h"
#include "Resource.h"

using namespace std;

namespace Isis {

  /** 
   *  Default constructor.
   */
  FilterStrategy::FilterStrategy() : Strategy("Filter", "Filter"), m_key(), 
                                     m_checkAll(true), m_includes(), 
                                     m_excludes(), m_regexp()  { 
  }
  

  /**
   * @brief Constructor loads from a Strategy object Filter definition
   *  
   * This constructor loads and retains processing parameters from the Filter 
   * Strategy object definition as (typically) read from the configuration file. 
   *  
   * @author 2013-02-19 Kris Becker
   * 
   * @param definition Filter Strategy PVL object definition
   * @param globals   List of global keywords to use in argument substitutions
   */
  FilterStrategy::FilterStrategy(const PvlObject &definition, 
                                 const ResourceList &globals) : 
                                        Strategy(definition, globals), 
                                        m_key(), m_checkAll(true), 
                                        m_includes(), m_excludes(), 
                                        m_regexp() {
  
    // Flatten Filter Strategy object definition
    PvlFlatMap parms( getDefinitionMap() );
    m_key = parms.get("Keyword");
  
    if ( parms.exists("CheckAll") ) {
      m_checkAll = toBool(parms.get("CheckAll").toStdString());
    } 
  
    if ( parms.exists("Include") ) {
      m_includes = parms.allValues("Include");
    } 
  
    if ( parms.exists("Exclude") ) {
      m_excludes = parms.allValues("Exclude");
    } 
  
    if ( parms.exists("RegExp") ) {
      m_regexp.setPattern(parms.get("RegExp"));
      if ( toBool(parms.get("WildCardMode", "false").toStdString()) )  {
        m_regexp.setPatternSyntax(QRegExp::WildcardUnix);
      }
    }

    return; 
  }
  

  /** 
   *  Destructor.
   */
  FilterStrategy::~FilterStrategy() { 
  }
  

  /**
   * @brief Process a single Resource for filter disposition 
   *  
   * This Strategy uses the single Resource processing option to determine the 
   * disposition of a Resource. It will check the named Resource keyword for 
   * inclusion or exclusion of a keyword value as specified in the 
   * Include/Exclude keywords of the strategy definition. 
   * 
   * @author 2013-02-19 Kris Becker
   * 
   * @param resource Resource containing keywords
   * @param globals   List of global keywords to use in argument substitutions
   * 
   * @return int Returns 1 if the Resources is retained, 0 if deactivated
   */
  int FilterStrategy::apply(SharedResource &resource, const ResourceList &globals) { 
    
    //  Determine number of values to check
    int nvals = ( m_checkAll ) ? resource->count(m_key) : 1;
  
    for (int i = 0 ; i < nvals ; i++) {
  
      QString rvalue = resource->value(m_key, i);
  
      // Check include list
      bool hasVal = m_includes.contains(rvalue, Qt::CaseInsensitive);
      if ( hasVal ) { return (1); }  // If in include, keep it - we're done


      // Test if a regular expression is used
      if ( !m_regexp.isEmpty() ) {
        if ( rvalue.contains( m_regexp ) ) {  return (1);  }
      }
  
      // Check exclude list
      hasVal = m_excludes.contains(rvalue, Qt::CaseInsensitive);
      if ( hasVal ) {  // if in the exclude list, discard - we're done
        resource->discard();
        return (0);
      }

    }
  
    // In none of the tests succeeded, return status depends on list population
    // as follows:
    if ( !m_includes.isEmpty() && m_excludes.isEmpty() ) {
      resource->discard();  // Wasn't in the include list - discard it
      return (0);
    }
  
    if ( !m_excludes.isEmpty() ) {
      // not in exclude list or both lists empty so keep it
    }
  
    // If a regular expression was given and it did not find a match, then
    // this Resource must be discarded.
    if ( !m_regexp.isEmpty() ) {
       resource->discard();
       return (0);
    }

    //  Keeps the Resource
    return (1);
  }
  
}  //namespace Isis
