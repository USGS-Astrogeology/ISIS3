/**                                                                       
 * @file                                                                  
 * $Revision: 6513 $
 * $Date: 2016-01-14 16:04:44 -0700 (Thu, 14 Jan 2016) $
 * $Id: ResourceManagerStrategy.cpp 6513 2016-01-14 23:04:44Z kbecker@GS.DOI.NET $
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
#include "ResourceManagerStrategy.h"

// Qt library
#include <QVector>

// boost library
#include <boost/foreach.hpp>

// other ISIS
#include "IException.h"
#include "PvlFlatMap.h"
#include "PvlObject.h"
#include "Resource.h"
#include "Strategy.h"

// Declare function caller/helper
#define CALL_MEMBER_FN(object,ptrToMember)  ((object).*(ptrToMember))

using namespace std;

namespace Isis {
  
  /**
   * @brief Creates an empty ResourceManagerStrategy object 
   *  
   */
  ResourceManagerStrategy::ResourceManagerStrategy() : 
                                   Strategy("ResourceManager", "ResourceManager"),
                                   m_operators(), m_opFunctions() {
    initOperations();
  }
  

  /**
   * @brief Creates a ResourceManagerStrategy object using its PVL
   *        definition.
   *  
   * @param definition ResourceManager Strategy PVL object 
   *                   definition
   * @param globals   List of global keywords to use in argument substitutions
   * @throw IException::User "Invalid operations requested in ResourceManager."
   */
  ResourceManagerStrategy::ResourceManagerStrategy(const PvlObject &definition, 
                                                   const ResourceList &globals) : 
                                                   Strategy(definition, globals), 
                                                   m_operators(), 
                                                   m_opFunctions() {
  
    initOperations();
    PvlFlatMap parms( getDefinitionMap() );
    m_operators = parms.allValues("Operations");

    // Confirm requested operators are good
    QVector<IException> exceptions;
    BOOST_FOREACH (QString op, m_operators ) {
      ResourceList empty;  //  Use empty list for confirmation
      try {
         OperatorFn fn = findOperationFn(op);
         (void) CALL_MEMBER_FN(*this, fn)(op, empty);
      }
      catch (IException &ie) {
        exceptions.push_back(ie);
      }
    }

    // Handle any errors encountered
    if ( !exceptions.empty() ) {
      IException ie(IException::User,
                     "Invalid operations requested in ResourceManager.",
                    _FILEINFO_); 
      BOOST_FOREACH ( IException e, exceptions ) {
        ie.append(e);
      }
      throw ie;
    }

    return;
  }
  

  /**
   * Destroys the ResourceManagerStrategy object
   */
  ResourceManagerStrategy::~ResourceManagerStrategy() {
  }


  /** 
   * @brief Applies the list of operators to a Resource 
   *  
   * @param resources List of Resources 
   * @param globals   List of global keywords to use in argument substitutions
   *  
   * @return int Total number of Resources affected 
   *  
   */
  int ResourceManagerStrategy::apply(ResourceList &resources,
                                     const ResourceList &globals) {
    if ( isDebug() ) {
      cout << "Running  " << m_operators.size() << " ResourceManager operators...\n";
    }

    int nTotal(0);
    resetProcessed();
    BOOST_FOREACH ( QString op , m_operators ) {
      OperatorFn func = findOperationFn(op);
      if ( isDebug() ) {  cout << "  Running " << op << ":"; }

      int nops = CALL_MEMBER_FN(*this, func)(op, resources);
      
      if ( isDebug() ) {  cout <<  nops << " resources affected\n"; }

      nTotal += nops;
    }

    return (nTotal);
  }


  /**
   * @brief Initializes m_opFunctions, a map from operator names 
   *        to operator functions.
   *  
   */
  void ResourceManagerStrategy::initOperations() {
    m_opFunctions.clear();
    m_opFunctions.insert("resetdiscard", &ResourceManagerStrategy::opResetDiscard);
    m_opFunctions.insert("togglediscard", &ResourceManagerStrategy::opToggleDiscard);
    m_opFunctions.insert("deletediscard", &ResourceManagerStrategy::opDeleteDiscard);
    m_opFunctions.insert("deleteasset", &ResourceManagerStrategy::opDeleteAsset);
    m_opFunctions.insert("hasasset", &ResourceManagerStrategy::opHasAsset);
    return;
  }


  /**
   * @brief Returns an operator function, given an operator name.
   *  
   * @param op Operator name
   *  
   * @return ResourceManagerStrategy A ResourceManagerStrategy Operator function
   * @throw IException::User "ResourceManager operator is ill-formed."
   * @throw IException::User "ResourceManager operator not recognized. 
   *                          Valid are ResetDiscard, ToggleDiscard,
   *                          DeleteDiscard, and DeleteAsset::AssetName."
   *  
                                                                              */
  ResourceManagerStrategy::OperatorFn ResourceManagerStrategy::findOperationFn(
                                                       const QString &op) 
                                                       const {
    QStringList parts = qualifiers(op);
    if ( parts.isEmpty() || (parts.size() > 2)) {
      QString mess = "ResourceManager::Operator [" + op + "] is ill-formed.";
      throw IException(IException::User, mess, _FILEINFO_);
    }

    QString opName = parts[0].toLower();
    if ( !m_opFunctions.contains(opName) ) {
      QString mess = "ResourceManager::Operator [" + op + "] not recognized.";
      mess += "  Valid are ResetDiscard, ToggleDiscard, DeleteDiscard"
              " and DeleteAsset::AssetName.";
      throw IException(IException::User, mess, _FILEINFO_);
    }

    return (m_opFunctions.value(opName));
  }


  /**
   * @brief Restores discarded Resources. 
   *  
   * @param op Operator name
   * @param resources List of resources
   *  
   * @return int The number of un-discarded Resources
   */
  int ResourceManagerStrategy::opResetDiscard(const QString &op, 
                                              ResourceList &resources)  {
    int n(0);
    BOOST_FOREACH ( SharedResource resource, resources ) {
      if ( resource->isDiscarded() ) {
        resource->activate();
        n++;
      }
      processed();
    }
    return (n);
  }


  /**
   * @brief Switches which Resources are activated and 
   *        deactivated.
   *  
   * @param op Operator name
   * @param resources List of Resources
   *  
   * @return int The number of un-discarded Resources
   *  
   */
  //(for consistency, shouldn't this just return the # of resources, since all are affected
  int ResourceManagerStrategy::opToggleDiscard(const QString &op, 
                                               ResourceList &resources)  {
    int n(0);
    BOOST_FOREACH ( SharedResource resource, resources ) {
      if ( resource->isDiscarded() ) {
        resource->activate();
        n++;
      }
      else {
        resource->discard();
      }
      processed();
    }
    return (n);
  }


  /**
   * @brief Deletes all discarded Resources.
   *  
   * @param op Operator name
   * @param resources List of Resources
   *  
   * @return int The number of deleted Resources 
   */
  int ResourceManagerStrategy::opDeleteDiscard(const QString &op, 
                                               ResourceList &resources)  {
    int n(0);
    ResourceList::iterator resource = resources.begin();
    while ( resources.end() != resource ) {
      if ( (*resource)->isDiscarded() ) {
        resource = resources.erase(resource);
        n++;
      }
      else {
        ++resource;
      }
      processed();
    }
    return (n);
  }


  /**
   * @brief Removes a given Asset from all Resources that have it
   *  
   * In the PVL definition of the ResourceManager Strategy, the 
   * AssetName is given in: 
   *  
   * Operations = DeleteAsset::AssetName 
   *  
   * @param op Operator name
   * @param resources List of Resources
   *  
   * @return int The number of deleted assets 
   * @throw IException::User "ResourceManager requires an asset name. 
   *                          Operation is ill-formed."
   *  
   */
  int ResourceManagerStrategy::opDeleteAsset(const QString &op, 
                                             ResourceList &resources)  {
    QStringList parts = qualifiers(op);
    if ( parts.size() != 2) {
      QString mess = "ResourceManager " + parts[0] + " requires an asset name. "
                     " Operation [" + op + "] is ill-formed.";
      throw IException(IException::User, mess, _FILEINFO_);
    }

    QString assetName(parts[1]);
    int n(0);
    BOOST_FOREACH ( SharedResource resource, resources ) {
      if ( resource->hasAsset(assetName) ) {
        resource->removeAsset(assetName);
        n++;
      }
      processed();
    }

    return (n);
  }


  /**
   * @brief Discards Resources without a given asset.
   *  
   * In the PVL definition of the ResourceManager Strategy, the 
   * AssetName is given in: 
   *  
   * Operations = HasAsset::AssetName 
   *  
   * @param op Operator Name
   * @param resources List of Resources
   *  
   * @return int The number of discarded Resources
   * @throw IException::User "ResourceManager requires an asset name. 
   *                          Operation is ill-formed."
   *  
   */
  int ResourceManagerStrategy::opHasAsset(const QString &op, 
                                             ResourceList &resources)  {
    QStringList parts = qualifiers(op);
    if ( parts.size() != 2) {
      QString mess = "ResourceManager " + parts[0] + " requires an asset name. "
                     " Operation [" + op + "] is ill-formed.";
      throw IException(IException::User, mess, _FILEINFO_);
    }

    QString assetName(parts[1]);
    int n(0);
    BOOST_FOREACH ( SharedResource resource, resources ) {
      if ( !resource->hasAsset(assetName) ) {
        resource->discard();
        n++;
      }
      processed();
    }

    return (n);
  }

}  //namespace Isis
