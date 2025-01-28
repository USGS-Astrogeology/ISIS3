#ifndef ResourceManagerStrategy_h
#define ResourceManagerStrategy_h
/**
 * @file                                                                  
 * $Revision: 6187 $ 
 * $Date: 2015-05-11 17:31:51 -0700 (Mon, 11 May 2015) $ 
 * $Id: ResourceManagerStrategy.h 6187 2015-05-12 00:31:51Z kbecker@GS.DOI.NET $
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

// parent class
#include "Strategy.h"

// Qt library
#include <QMap>
#include <QString>
#include <QStringList>

// SharedResource and ResourceList typedefs
#include "Resource.h"

namespace Isis {

  class PvlObject;

  /**
   * @brief ResourceManagerStrategy provides the ability to 
   *        apply simple operations to Resources. 
   *        
   * Here is an example of a ResourceManager definition: 
   *  
   * @code 
   * Object = Strategy 
   *   Name = SwitchActivated
   *   Type = ResourceManger Description = "Swtich activated and
   *   deactivated Resources."
   *   Operations = ToggleDiscard
   *                 
   * EndObject
   * @endcode 
   *  
   * @author 2012-07-15 Kris Becker
   * @internal 
   *   @history 2012-07-15 Kris Becker - Original version.
   *   @history 2015-03-18 Jeannie Backer - Brought class files closer to ISIS coding standards.
   *   @history 2015-03-19 Kristin Berry - Updated documentation.
   *   @history 2015-05-04 Ian Humphrey - Updated tests to test exceptions.
   *   @history 2015-05-08 Kris Becker - Modify constructor to take a global
   *                          resources list; modified apply() method to accept
   *                          a global resource list.
   *   @history 2024-07-15 Ken Edmundson - Fixed minor mispellings in error messages.
   */
  class ResourceManagerStrategy : public Strategy {

    public:
      ResourceManagerStrategy();
      ResourceManagerStrategy(const PvlObject &definition, const ResourceList &globals);
      virtual ~ResourceManagerStrategy();
  
      virtual int apply(ResourceList &resources, const ResourceList &globals);

    private:
      typedef  int (ResourceManagerStrategy::*OperatorFn)(const QString &operation, 
                                                          ResourceList &resources);
      //! Definition for OperationsMap, a map between a string and operator function
      typedef QMap<QString, OperatorFn> OperationsMap;
  
      void initOperations();
      OperatorFn findOperationFn(const QString &op) const;

      int opResetDiscard(const QString &op, ResourceList &resources);
      int opToggleDiscard(const QString &op, ResourceList &resources);
      int opDeleteDiscard(const QString &op, ResourceList &resources);
      int opDeleteAsset(const QString &op, ResourceList &resources);
      int opHasAsset(const QString &op, ResourceList &resources);
  
      QStringList m_operators;     //!< List of operations to perform on the Resource
      OperationsMap m_opFunctions; //!< Map from operator names to operator functions
  
  };

} // Namespace Isis

#endif
