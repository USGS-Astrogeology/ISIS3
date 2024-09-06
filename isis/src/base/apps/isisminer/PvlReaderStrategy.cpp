/**                                                                       
 * @file                                                                  
 * $Revision: 6513 $
 * $Date: 2016-01-14 16:04:44 -0700 (Thu, 14 Jan 2016) $
 * $Id: PvlReaderStrategy.cpp 6513 2016-01-14 23:04:44Z kbecker@GS.DOI.NET $
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
#include "PvlReaderStrategy.h"

// boost library
#include <boost/foreach.hpp>

// other ISIS
#include "FileList.h"
#include "FileName.h"
#include "IException.h"
#include "Pvl.h"
#include "PvlFlatMap.h"
#include "PvlObject.h"
#include "Resource.h"
#include "Strategy.h"

using namespace std;

namespace Isis {

  /** 
   * Default constructor
   */
  PvlReaderStrategy::PvlReaderStrategy() : Strategy("PvlReader", "PvlReader"),
                                           m_pvlfile(), m_identity(), 
                                           m_pvlparms() { 
  }
  
    
  /** 
   * @brief Constructor loads from a Strategy object PvlReader definition
   * 
   * This constructor loads and retains processing parameters from the PvlReader
   * Strategy object definition as (typically) read from the configuration file.
   * 
   * @author 2012-07-15 Kris Becker
   * 
   * @param definition PvlReader Strategy PVL object definition
   * @param globals   List of global keywords to use in argument substitutions
   */
  PvlReaderStrategy::PvlReaderStrategy(const PvlObject &definition, 
                                       const ResourceList &globals) : 
                                       Strategy(definition, globals),
                                       m_pvlfile(), m_identity(), m_pvlparms() {
  
    PvlFlatMap parms( getDefinitionMap() );
    // m_pvlfile  = parms.get("FromList", "");
    m_identity = parms.get("Identity", "");
   
    if ( parms.exists("Includes") ) { 
      m_pvlparms.addInclude(parms.allValues("Includes"));
    }
  
    if ( parms.exists("Excludes") ) { 
      m_pvlparms.addExclude(parms.allValues("Excludes"));
    }
  
    // allows for using KeyListFileArgs
    QString keyfile = translateKeywordArgs("KeyListFile", globals);
    if ( !keyfile.isEmpty() ) { 
      m_pvlparms.addKeyToList(FileName(keyfile.toStdString()));
    }
  
  }
  
    
  /** 
   * Destructor
   */
  PvlReaderStrategy::~PvlReaderStrategy() {
  }


  /** 
   * @brief Obtains the Resources from a list of Pvl files
   * 
   * Creates Resources from each Pvl file provided in the FromList. Each 
   * Resource will have columns corresponding to the keywords in the Pvl file. 
   * 
   * @author 2012-07-15 Kris Becker
   * 
   * @param resources A list of Resources to add the created Resources to
   * @param globals   List of global keywords to use in argument substitutions
   *
   * @return (int) The number of Resources created from the Pvl files in 
   *                 FromList
   */
  int PvlReaderStrategy::apply(ResourceList &resources, const ResourceList &globals) { 
  
    m_pvlfile = translateKeywordArgs("FromList", globals);
    FileName fromlistFile(m_pvlfile.toStdString());
    FileList fromlist(fromlistFile);
  
    int npvls(0);
    BOOST_FOREACH ( FileName from, fromlist ) {
      resources.push_back( pvlResource(QString::fromStdString(from.expanded()), globals, npvls) );
      npvls++;
    }
  
    return (npvls);
  }
  
    
  /** 
   * @brief Creates a Resource from a Pvl file
   * 
   * Creates a Resource from a Pvl file contained in the FromList. The nth 
   * parameter specifies which Pvl file to create a Resource from, with 0 
   * indicating the first Pvl file in the list. 
   * 
   * If the Identity keyword is not provided or if its value is an empty 
   * string, the Identity of the Resource will be set to PvlN, with N being the 
   * number corresponding to the index of Pvl file in the FromList. The 
   * basename, Pvl, can be changed by providing the keyword PvlBaseName with a 
   * different value. 
   * 
   * For example, if the FromList contains a list of files pvlA.pvl, pvlB.pvl, 
   * and pvlC.pvl, their default Identities will be Pvl0, Pvl1, and Pvl2. 
   * 
   * Resource is created according to any constraints provided (see PvlFlatMap 
   * for more info). 
   * 
   * @author 2012-07-15 Kris Becker
   * 
   * @param pvlfile The Pvl file to create the Resource from
   * @param globals   List of global keywords to use in argument substitutions
   * @param nth The nth (0-indexed) filename in the FromList
   * 
   * @return (SharedResource) The Resource created from the nth Pvl file
   * 
   * @throws IException::User "Geometry conversion failed for Resource"
   */
  SharedResource PvlReaderStrategy::pvlResource(const QString &pvlfile,
                                                const ResourceList &globals, 
                                                int nth) const { 
  
    // Make assets out of them
    PvlFlatMap keys(getDefinitionMap());
    QString rowBase(keys.get("PvlBaseName","Pvl"));
    QString rowId = rowBase + QString::number(nth);
  
    Pvl pvl(pvlfile.toStdString());
    PvlFlatMap pvlImports(pvl, m_pvlparms);
    SharedResource pvlsrc(new Resource(rowId, pvlImports));
  
    // Determine identity
    QString identity = translateKeywordArgs("Identity", getGlobals(pvlsrc, globals));;
    if ( identity.isEmpty() ) {
      identity = rowId;
    }
    
    pvlsrc->setName(identity);
    
    if ( isDebug() ) { 
      cout << "  PvlReader::Resource::" << rowId 
           << "::Identity = " << identity << "\n"; 
    }
  
    // Import geometry w/exception handling
    //  Check for Geometry.  May want to remove it after parsing/conversion.  
    //  These text geometries tend to be huge and consume lots of memory.
    try {
      importGeometry(pvlsrc, globals);
    }
    catch (IException &ie) {
      std::string mess = "Geometry conversion failed horribly for Resource(" + 
                     identity + ")";
      throw IException(ie, IException::User, mess, _FILEINFO_);
    }
  
    return (pvlsrc);
  }

}  //namespace Isis
