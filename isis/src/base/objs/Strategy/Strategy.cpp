/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "Strategy.h"

// Qt library
#include <QScopedPointer>
#include <QString>
#include <QStringList>

// boost library
#include <boost/foreach.hpp>

// other ISIS
#include "IException.h"
#include "GisGeometry.h"
#include "Progress.h"
#include "PvlFlatMap.h"
#include "PvlObject.h"
#include "Resource.h"

using namespace std;

namespace Isis {
  
  /** 
   * Constructs default Strategy object of name "Strategy" and type "Counter".
   * Debug and apply discarded are set to false.
   */ 
  Strategy::Strategy() : m_globals(), m_definition(new PvlObject("Strategy")),
                         m_name("Strategy"), m_type("Counter"),
                         m_total(0), m_applyDiscarded(false), m_debug(false),
                         m_progress() { 
  }
  
  
  /** 
   * Constructs a Strategy object from the given name, type, and shared resource
   * globals. Debug and apply discarded are set to false.
   * 
   * @param name A string containing the strategy name.
   * @param type A string containing the strategy type.
   */ 
  Strategy::Strategy(const QString &name, const QString &type) : 
                     m_globals(),
                     m_definition(new PvlObject(name)),
                     m_name(name), m_type(type), m_total(0), 
                     m_applyDiscarded(false), m_debug(false), m_progress() { 
  }
  
  
  /** 
   * Constructs Strategy object from the given definition pvl and shared 
   * resource globals. Strategy name, strategy type, debug, and apply discarded 
   * values are read from the pvl. If not found in the PVL, debug and apply 
   * discarded are set to false. 
   *  
   * Example PVL: 
   * @code 
   * Object = Strategy 
   *   Name = Emission
   *   Type = Calculator
   *   Equation = "((EmissionAngle > 20) && (EmissionAngle < 80))"
   *   Result = EmissionCriteria
   *   ApplyToDiscarded = true
   *   Debug = false
   * EndObject 
   * @endcode 
   * 
   * @param definition A PVL that defines the strategy.
   * @param globals A shared pointer to a global Resource of keywords.
   */ 
  Strategy::Strategy(const PvlObject &definition, const ResourceList &globals) : 
                    m_globals(globals), m_definition(new PvlObject(definition)),
                    m_name("Strategy"), m_type("Unknown"), 
                    m_total(0), m_applyDiscarded(false), m_debug(false), m_progress() {
    PvlFlatMap parms(getDefinitionMap());
    m_name = parms.get("Name");
    m_type = parms.get("Type");
    m_applyDiscarded = toBool(parms.get("ApplyToDiscarded", "false"));
    m_debug = toBool(parms.get("Debug", "false"));
    initProgress();
  }
  
  
  /** 
   * Destroys the Strategy object. 
   */ 
  Strategy::~Strategy() {
  }


  /** 
   * Accessor method to get the name of the strategy. 
   *  
   * @return QString A string containing the strategy's name.
   */ 
  QString Strategy::name() const {
    return (m_name);
  }
  
  
  /** 
   * Accessor method to get the type of strategy. 
   *  
   * @return QString A string containing the strategy's type.
   */ 
  QString Strategy::type() const {
    return (m_type);
  }


  /**
   * Allow derived strategies to reset name (mostly for default
   * constructors)
   * @param   A string containing the new name of the strategy.
   */
  void Strategy::setName(const QString &name) {
    m_name = name;
    return;
  }


  /** Allow derived strategies to reset type (mostly for default
   *  constructors)
   * @param A string containing the new type of the strategy.


   */
  void Strategy::setType(const QString &type) {
    m_type = type;
    return;
  }


  /**
   * Accessor method to get the global defaults
   *
   * @return ResourceList containing the global defaults.
   */
   ResourceList Strategy::getGlobalDefaults() const {
     return (m_globals);
   }


/**
 * 
 * @history kbecker (5/11/2015)
 * 
 * @param myGlobals Additional source for keyword translation  
 * @param globals Pre-exiting global pool to add myGlobals
 * 
 * @return ResourceList 
 */
   ResourceList Strategy::getGlobals(SharedResource &myGlobals, 
                                     const ResourceList &globals) const {

#if 0
     ResourceList v_globals(globals);
     v_globals.append(myGlobals);
#else
     ResourceList v_globals;
     v_globals.push_back(myGlobals);
     v_globals.append(globals);
#endif
     return ( v_globals );
   }


   const PvlObject &Strategy::getDefinition() const {
     return (*m_definition);
   }


/**
 * @brief Returns the keyword definitions found in the Strategy object 
 * 
 * @author 2015-06-12 Kris Becker 
 * 
 * @return PvlFlatMap Set of keywords found in strategy object
 */
   PvlFlatMap Strategy::getDefinitionMap() const {
   return ( PvlFlatMap(*m_definition, PvlConstraints::withExcludes(getObjectList(*m_definition))) );
   }


/**
 * @brief Return description for the strategy. 
 *  
 * If a Description keyword exists in the PvlObject definition of the strategy, 
 * this is returned to the caller. If it doesn't exist, it will provide a simple 
 * description made up of its name and type. 
 * 
 * @author 2015-03-28 Kris Becker
 * 
 * @return QString Description for this strategy
 */
  QString Strategy::description() const {
    if ( m_definition->hasKeyword("Description") ) {
      return (m_definition->findKeyword("Description")[0]);
    }
    else {
      QString descr = "Strategy::" + name() + " is running a " + type() +
                      " algorithm.";
      return (descr);
    }
  }

  
/**
 * @brief Apply algorithm to resource list 
 *  
 * This method is used to initiate the Strategy algorithm implemented by 
 * deriving Strategy for a list of Resources. 
 * 
 * @history 2015-05-27 Kris Becker
 * 
 * @param resources List of resources to be processed by the Strategy algorithm
 * 
 * @return int Number of Resources processed by the algorithm
 */
  int Strategy::apply(ResourceList &resources) {
    return ( apply(resources, getGlobalDefaults()) );
  }

  
/**
 * @brief Apply algorithm to Resource
 *  
 * This method is used to initiate the Strategy algorithm implemented by 
 * deriving Strategy for a Resources. 
 * 
 * @history 2015-05-27 Kris Becker
 * 
 * @param resource Resource to be processed by the Strategy algorithm
 * 
 * @return int Number of Resources processed by the algorithm
 */
  int Strategy::apply(SharedResource &resource) {
    return ( apply( resource, getGlobalDefaults()) );
  }

  
  /**
   * @brief Apply strategy algorithms to list of Resources 
   *  
   * This method iterates once through all Resources contained in the list. 
   * Resources that have been discarded are filtered out unless users/strategies 
   * opt to process all resources. 
   *  
   * By making the actual iteration of Resources a separate method, this allows 
   * other strategies to derive this virtual return to give them a change to do 
   * something meaninful prior to processing the Resource list, but retain 
   * behavior in counts and calls to the Resource applicator. 
   *  
   * @see applyToResources(ResourceList &resources, ResourceList &globals) 
   * 
   * @author 2013-01-30 Kris Becker 
   *  
   * @param resources A list of resources to which the strategy will be applied.
   *  
   * @return int The number of resources processed.
   *  
   */
  int Strategy::apply(ResourceList &resources, const ResourceList &globals) { 
    return ( applyToResources(resources, globals) );
  }
  
  
  /** 
   * This method applies the strategy algorithms to the data stored in the 
   * given shared resource. 
   *  
   * @param resource A resource to which the strategy will be applied.
   *  
   * @return int The number of resources processed, i.e. 1.
   */
  int Strategy::apply(SharedResource &resource, const ResourceList &globals) {
    if ( isDebug() ) {
      cout << "Empty apply is called...\n";
    }
    return (1); 
  }
  
    
  /** 
   * Accessor for the total number of resources processed. 
   *  
   * @return unsigned int The number of resources processed.
   */ 
  unsigned int Strategy::totalProcessed() const {
    return (m_total);
  }
  
  
  /** 
   * Sets Resource as discarded. This mutator tells the object to not apply
   * strategy algorithms to discarded resources.
   */ 
  void Strategy::setApplyToDiscarded() {
    m_applyDiscarded = true;
    return;
  }
  
  
  /** 
   * Accessor for the apply discarded variable. 
   *  
   * @return bool Indicates whether the object will apply strategy algorithms to
   *              discarded resources.
   */ 
  bool Strategy::isApplyToDiscarded() const {
    return (m_applyDiscarded);
  }
  
  
  /** 
   *  Disables the general application of Strategy algorithm for all Resources
   *  regardless of state
   */ 
  void Strategy::setDoNotApplyToDiscarded() {
    m_applyDiscarded = false;
    return;
  }
  
  
  /** 
   * Applies the strategy algorithms to the resources in the given list. 
   *  
   * @param resources A list of resources to which the strategy will be applied.
   * @param globals A list of global resources.
   * @return int The number of resources processed.
   */ 
  int Strategy::applyToResources(ResourceList &resources, const ResourceList &globals) { 
   
    initProgress( ( isApplyToDiscarded() ) ? resources.size() : countActive(resources) );

    int result = 0;
    BOOST_FOREACH ( SharedResource resource, resources ) {
      if ( resource->isDiscarded() ) {
        if (true == isApplyToDiscarded() ) {
          result += apply(resource, globals);
          processed();
        }
      }
      else {
        result += apply(resource, globals);
        processed();
      }
    }
    return (result);
  }
  
  
  /** 
   * Increments the total number of resources processed and returns the 
   * incremented value. 
   *  
   * @return int The number of resources processed.
   */ 
  unsigned int Strategy::processed() {
    m_total++;
    if ( doShowProgress() && !m_progress.isNull() ) { 
      m_progress->CheckStatus(); 
    }

    return (m_total);
  }
  
  
  /** 
   * Resets the total number of processed resources to zero. 
   */ 
  void Strategy::resetProcessed() {
    m_total = 0;
    return;
  }
  
  
  /** 
   * Counts the number of active (i.e. not discarded) resources in the given 
   * list. 
   *  
   * @param resources A list of resources to count.
   * 
   * @return int The number of resources in the given list that are not set to 
   *             discarded.
   */ 
  int Strategy::countActive(const ResourceList &resources) const {
    return ( resources.size() - countDiscarded(resources) );
  }
  
  
  /** 
   * Counts the number of non-active (i.e. discarded) resources in the given
   * list. 
   *  
   * @param resources A list of resources to count.
   * 
   * @return int The number of resources in the given list that are set to 
   *             discarded.
   */ 
  int Strategy::countDiscarded(const ResourceList &resources) const {
    int n = 0;
    BOOST_FOREACH ( SharedResource resource, resources ) {
      if ( resource->isDiscarded() ) { n++; }
    }
    return (n);
  }
  
  
  /** 
   * Searches the given resource for an asset with the given name and converts 
   * it to a ResourceList, if possible. 
   *  
   * @param resource The resource to be searched
   * @param name The name of the asset being searched for
   * @return ResourceList If the asset can be converted to a ResourceList, it
   * is returned.  Otherwise an empty ResourceList is returned.
   */ 
  ResourceList Strategy::assetResourceList(const SharedResource &resource, 
                                           const QString &name) const {
    ResourceList alist;
    if ( resource->hasAsset(name) ) {
      QVariant v_asset = resource->asset(name);
      if ( v_asset.canConvert<ResourceList>() ) {
         alist = v_asset.value<ResourceList>();
      }
    }
  
    return (alist);
  }

  
/**
 * @brief Find keyword replacement value in globals list 
 * 
 * @author 2015-05-07 Kris Becker
 * 
 * @param globals ResourceList A list of resources to be searched.
 * @param target QString The keyword of the target resource.
 * @param index int The index of the last resource in the list with keyword == target.
 * @param defValue QString The value to return in the event the resource is not found.
 * 
 * @return QString 
 */
   QString Strategy::findReplacement(const QString &target,
                                     const ResourceList &globals, 
                                     const int &index,
                                     const QString &defValue) const {
     BOOST_FOREACH ( const SharedResource keystore, globals ) {
       if ( keystore->exists(target) ) {
         if ( keystore->count(target) > index ) {
           return (keystore->value(target, index)); 
         }
       }
     }

     return ( defValue );
   }

  
  /** 
   * Splits the given keyspec string into a list using the given delimiter 
   * string. 
   *  
   * @param keyspec The string to be split.
   * @param delimiter A string containing the characters used to specify the 
   *                  boundary by which to split the string.
   *  
   * @return QStringList The list of strings that were split from the given 
   *         keyspec string using the given delimiter string.
   */ 
  QStringList Strategy::qualifiers(const QString &keyspec,
                                   const QString &delimiter) const {
    QStringList parts = keyspec.split(delimiter);
    return (parts);
  }

  
  /** 
   * Performs a case insensitive scan of the input string for a substring 
   * matching the target string and replaces the target substring with the 
   * replacement string. 
   *  
   * @param input The input string to be modified.
   * @param target The string pattern to find in the input string.
   * @param replacement The string pattern that will replace the target string 
   *                    pattern.
   *  
   * @return QString The modified string.
   */ 
  QString Strategy::scanAndReplace(const QString &input, const QString &target, 
                                   const QString &replacement) const {
    QString str(input);
    str.replace(target, replacement, Qt::CaseInsensitive);
    return (str);
  }
  
  
  /** 
   * Translates the arguments of the PVL keyword in the PVL definition object. 
   *  
   * This method uses the keyBase string to get the keyword value and populate 
   * the argument replacement list. The keyword value is then modified by 
   * passing it to processArgs() along with using the argument list, resource,
   * and default resource pointers. 
   *  
   * If a keyword does not exist, the string provided by defValue is returned.
   * The empty string is returned in the event that defValue is not given an argument.
   *  
   * @param keyBase A string containing the base of the keyword name.
   * @param globals A list of resources that will be searched.
   * @param defValue The default value to return if the keyBase does not exist
   *  
   * @return QString A modified keyword value with appropriate arguments 
   *                 inserted into the appropriate locations.
   */ 
  QString Strategy::translateKeywordArgs(const QString &keyBase,
                                         const ResourceList &globals,
                                         const QString &defValue) const {
  
    // Get keyword configuration from definition 
    PvlFlatMap keys( getDefinitionMap() );
  
    QStringList idArgs;  // Argument replacement list
    QString value;
    
    QString keyword = keyBase + "Keyword";
    if ( keys.exists(keyword) ) {
      idArgs.push_back(keys.get(keyword));
      value = keys.get(keyBase, "%1");
    }
    else {
      if ( keys.exists(keyBase + "Args") ) {
        idArgs = keys.allValues(keyBase + "Args");
      }
      value = keys.get(keyBase, defValue);
    }
  
    return (processArgs(value, idArgs, globals, defValue));
  }
  
  
  /** 
   * Processes the given string value using the given argument list, resource 
   * and default resource. 
   *  
   * For each argument in the given list, the target string "%i" (where i is the
   * argument number) is replaced with the resource's keyword value 
   * corresponding to the argument.  If this value doesn't exist, then the 
   * default resource's keyword value is used.  If both fail, then the target 
   * string is replaced with "NULL" 
   *  
   * @param value A keyword value to modified using the given argument list and 
   *              resources.
   * @param argKeys A list of string arguments representing the resource values
   *                to be found in the resource's PVL flat map.
   * @param globals  A list of resources that will be searched.
   *
   * @param defValue A pointer to the default resource whose PVL flat map will
   *                 be searched for the arguments if they are not found in the
   *                 main resource PVL flat map.
   *  
   * @return QString The modified string value.
   */ 
  QString Strategy::processArgs(const QString &value, const QStringList &argKeys,
                                const ResourceList &globals,
                                const QString &defValue) const {
  
    QString result = value;
    int i = argKeys.size();
    BOOST_REVERSE_FOREACH ( QString arg, argKeys ) {
      QString target("%"+QString::number(i));
      result = scanAndReplace(result, target, findReplacement(arg,globals,0,defValue));
      i--;
    } 
    return (result);
  }
  
  
  /** 
   * Adds the PVL definition keywords from the source to the target. This 
   * creates a PVL flat map from the definition pvl. All of the keywords in the 
   * map that are also in the source resource are added to the target resource. 
   *  
   * @param source A reference to the resource to be copied from.
   * @param target A reference to the resource to be added to.
   */ 
  void Strategy::propagateKeys(SharedResource &source, SharedResource &target) 
                               const {
    PvlFlatMap keys( getDefinitionMap() );
    QStringList keySources = keys.allValues("PropagateKeywords");
    BOOST_FOREACH ( QString key, keySources ) {
      if ( source->exists(key) ) {
        target->add(source->keyword(key));
      }
    }
    return;
  }

  
/**
 * @brief Create a composite Resource from a pair by merging keywords 
 *  
 * This method will create a composite Resource from two resources my merging 
 * the keywords from both resources. Users can provide a list of keywords they 
 * would like to merge by providing the PropogateKeywords Strategy keyword in 
 * the definition section. If not provided or empty, all keywords in each 
 * Resource is propagated. 
 *  
 * Keywords from each resource are distinguished by appending a suffix to the 
 * name of each keyword propagated. For example, the default suffix for 
 * resourceA is "A" and resourceB is "B". If the keyword named "Emission" exists 
 * in both resourceA and resourceB then two keywords wind up in the composite 
 * resource as "EmissionA" and "EmissionB". 
 *  
 * The name of the composite resource is the name of resourceA and resourceB 
 * separated by an "_". 
 * 
 * @author 2015-03-28 Kris Becker
 * 
 * @param resourceA First resource
 * @param resourceB Second resource
 * @param keySuffix String pair specifying the suffix to append to the keywords 
 *                  originating from each Resource Defaults: ("A", "B")
 * 
 * @return SharedResource Composite Resource of combined keywords
 */
  SharedResource Strategy::composite(SharedResource &resourceA, 
                                     SharedResource &resourceB,
                                     const QPair<QString, QString> &keySuffix) 
                                     const {

    // Create the new resource
    QString id = resourceA->name() + "_" + resourceB->name();
    SharedResource composite(new Resource(id));
    
    // See if user wants to restrict keyword propagation to a list 
    QStringList keySources = PvlFlatMap( getDefinitionMap() ).allValues("PropagateKeywords");
    if ( !keySources.isEmpty() ) {
      // Propogate only specified keywords from each Resource
      BOOST_FOREACH ( QString key, keySources ) {
        if ( resourceA->exists(key) ) {
          PvlKeyword keyword(resourceA->keyword(key));
          keyword.setName(keyword.name().append(keySuffix.first));
          composite->add(keyword);
        }

        if ( resourceB->exists(key) ) {
          PvlKeyword keyword(resourceB->keyword(key));
          keyword.setName(keyword.name().append(keySuffix.second));
          composite->add(keyword);
        }
      }
    }
    else {
      // Propagate all keys from each source to the composite Resource
      const PvlFlatMap &rkeysA = resourceA->keys();
      PvlFlatMap::ConstPvlFlatMapIterator pkeys = rkeysA.begin();
      while ( rkeysA.constEnd() != pkeys) {
        PvlKeyword keyword(pkeys.value());
        keyword.setName(keyword.name().append(keySuffix.first));
        composite->add(keyword);
        ++pkeys;
      }

      const PvlFlatMap &rkeysB = resourceB->keys();
      pkeys = rkeysB.begin();
      while ( rkeysB.constEnd() != pkeys) {
        PvlKeyword keyword(pkeys.value());
        keyword.setName(keyword.name().append(keySuffix.second));
        composite->add(keyword);
        ++pkeys;
      }
    }
    return (composite);
  }
  
  
  /** 
   * Imports a geometry from the given resource. 
   *  
   * By default, this method searches the definition PVL object for a 
   * GisGeometry keyword. If the PVL contains either the GisGeometryRef or 
   * the GisGeometryKey keywords, then the value is used to search the 
   * resource's flat map for the geometry. Then, if the 
   * RemoveGisKeywordAfterImport keyword is set to true in the PVL, the 
   * resource erases the keyword from the flat map. 
   *  
   * If a geometry is found, the PVL is searched for a GisGeometryArgs 
   * keyword and the arguments are processed, if they exist. Next, the 
   * GisType is read from the PVL. The geometry and type are used to 
   * construct a GIS geometry and add it to the resource. 
   *  
   * @param resource A shared pointer to the resource from which the geometry 
   *                 will be imported.
   * @param globals  The resource list to be searched.
   * @return bool Indicates whether the geometry was successfully imported.
   */ 
  bool Strategy::importGeometry(SharedResource &resource, 
                                const ResourceList &globals) const {
    PvlFlatMap keys( getDefinitionMap() );
  
    //  Assume a specific geometry is present
    QString geom(keys.get("GisGeometry", ""));

    QString giskey;
    if ( keys.exists("GisGeometryRef") ) { giskey = keys.get("GisGeometryRef"); }
    if ( keys.exists("GisGeometryKey") ) { giskey = keys.get("GisGeometryKey"); }
  
    if ( !giskey.isEmpty() ) {
      if ( !resource->isNull(giskey) ) {
        geom = resource->value(giskey);
        
        // Erase key if requested
        if ( toBool(keys.get("RemoveGisKeywordAfterImport", "false")) ) { 
          resource->erase(giskey); 
        }
      }
    }
  
    // Got a geometry.
    if ( !geom.isEmpty() ) {

        // Get decision keys
        bool repairGeom = toBool(keys.get("RepairInvalidGeometry", "true"));

        QString geomAction = keys.get("InvalidGeometryAction", "disable").toLower();
        if ( !QString("disable error continue").contains(geomAction) ) {
            if ( isDebug() ) {
              cout << "  Invalid value for InvalidGeometryAction (" << geomAction 
                   << ") - set to disable!\n";
            }
            geomAction = "disable";
        }
        
      //  Process arguments  Allows creation specialized geometry as well
      if ( keys.exists("GisGeometryArgs") ) {
        QStringList args = keys.allValues("GisGeometryArgs");
        geom = processArgs(geom, args, getGlobals(resource, globals));
      }
  
      // Get the type
      QString gisType = keys.get("GisType");
      int npoints(0);
      int npointsOrg(0);
      double tolerance(0.0);
  
      //  Check for Geometry.  May want to remove it after parsing/conversion.  
      //  These text geometries tend to be huge and consume lots of memory.
      if ( !geom.isEmpty() ) {
        
        QScopedPointer<GisGeometry> geosgeom(new GisGeometry(geom, GisGeometry::type(gisType)));
        if ( geosgeom.isNull() ) {
            if ( isDebug() ) {
                cout << resource->name() << " geometry failed to construct\n";
            }
            if ("continue" == geomAction) return (false);
            if ( "disable" == geomAction ) {
                resource->discard();
                return ( false );
            }

            // Throw an error
            QString mess = resource->name() + " failed to construct geometry!";
            throw IException(IException::Programmer, mess, _FILEINFO_);
        }
            
        // Check validity and take appropriate action
        if ( !geosgeom->isValid() ) {
          
          QString geomError = geosgeom->isValidReason();
          if ( isDebug() ) {
              cout << "  Geometry error: " << geomError << "\n";
          }
          
          // Attempt repair if requested
          if ( repairGeom ) {
            if (isDebug()) {
              cout << "  " << resource->name() << " geometry is invalid..."
                    << "attempting buffer(0) to fix it!\n";
            }
            geosgeom.reset( geosgeom->buffer(0) );
            if ( isDebug() ) {
              if (geosgeom.isNull() || !geosgeom->isValid() ) {
                 cout << "  Geometry could not be repaired!\n";
              }
              else {
                cout << "  Geometry was successfully repaired!\n";
              }
            }
          }

          // Now check state and take final action regarding a failed
          // geometry 

          if (geosgeom.isNull() || !geosgeom->isValid() ) {
            if ( isDebug() ) {
                cout << "  All efforts to convert geometry failed!\n";
            }
            if ("continue" == geomAction) return (false);
            if ( "disable" == geomAction ) {
              resource->discard();
              return ( false );
            }

            // Throw an error
            QString mess = resource->name() + " failed to construct geometry - Error: " +
                            geomError;
            throw IException(IException::Programmer, mess, _FILEINFO_);
          }
        }

        npointsOrg =  npoints = geosgeom->points();
        QString gisTolerance  = translateKeywordArgs("GisSimplifyTolerance", 
                                                     getGlobals(resource, globals), ""); 

        if ( !gisTolerance.isEmpty() ) {
          tolerance = toDouble(gisTolerance);
          GisGeometry *simple = geosgeom->simplify(tolerance);
          if ( 0 != simple ) geosgeom.reset(simple);
          npoints = geosgeom->points();
        }

        resource->add(geosgeom.take());

        // Add the geometry point count to the resource if requested
        QString pointsKey = translateKeywordArgs("GisGeometryPointsKey", 
                                                 getGlobals(resource, globals), 
                                                 ""); 
        if ( !pointsKey.isEmpty() ) {
          resource->add(pointsKey, toString(npoints));
          resource->add(pointsKey+"Original", toString(npointsOrg));
          resource->add(pointsKey+"Tolerance", toString(tolerance));
        }

        //  Status if requested
        if ( isDebug() ) {
          cout << "  " << type() << ":" << name() << " has a geometry with "
               << npoints << " points!\n";
          if ( npoints != npointsOrg ) {
            cout << "  Geometry has been simplified/reduced from original " 
                 << npointsOrg << " points.\n";
          }
        }
        return (true);
      }
    }
  
    // Report geometry status
    if ( isDebug() ) {
      cout << "  " << type() << ":" << name() << " does not have a geometry!\n";
    }
  
    return (false);
  }
  
  
  /** 
   * Get list of all active Resources only - no discarded Resources
   * 
   * @param resources A list of resources to check. 
   *  
   * @return ResourceList A list of active resouces from the given list.
   */
  ResourceList Strategy::activeList(ResourceList &resources) const {
    //  Create a list of active resources
    ResourceList v_active;
    BOOST_FOREACH ( SharedResource resource, resources ) {
      if ( !resource->isDiscarded() ) { v_active.append(resource); }
    }
   return (v_active);
  }
  
  
  /** 
   * Activate all resources contained in the resource list
   * 
   * @param resources A list of resources to activate.
   */
  void Strategy::activateList(ResourceList &resources) const {
    BOOST_FOREACH ( SharedResource resource, resources ) {
      resource->activate();
    }
  }
  
  
  /** 
   * Deactivate all resources contained in the resource list
   * 
   * @param resources A list of resources to discard.
   */
  void Strategy::deactivateList(ResourceList &resources) const {
    BOOST_FOREACH ( SharedResource resource, resources ) {
      resource->discard();
    }
  }

  
/**
 * @brief Make a copy of the resource list that is independently managed
 *  
 * This method creates a copy of the resources but provides independent 
 * management of its status from its parent. All Resource data, name, keywords,
 * assets and geometry, are copied to the new Resource list created. Its 
 * current status is also copied. 
 *  
 * This allows the same data for all copies to be shared but the 
 * active/discard status to be managed in each copy independently of the parent 
 * for each resource. 
 *  
 * This is really useful for strategies such as AssetSideBar. 
 * 
 * @author 2015-09-27 Kris Becker
 * 
 * @param resources Input list of Resources to copy
 * 
 * @return resources of copied (data) Resources
 */
  ResourceList Strategy::copyList(const ResourceList &resources) const {
    ResourceList v_copy;
    BOOST_FOREACH ( SharedResource resource, resources ) {
      v_copy.append(SharedResource(resource->copy()));
    }
    return ( v_copy);
  }


/**
 *  @brief Create a clone of a Resource list
 *  
 * This method creates a clone of the resources where an independent copy of 
 * the Resource data is created from its parent. Assets are optional created as 
 * well, but can be retained if desired. Any shared components are simply 
 * copied as well and can be individually managed if desired. 
 *  
 * This creates new data for all copies the parent for each resource. All 
 * Resources are set to an active state upon cloning. 
 * 
 * @author 2015-09-27 Kris Becker
 * 
 * @param resources  List of resources to clone
 * @param withAssets If true, assets are retained, otherwise they are deleted 
 *                   from the cloned Resource.
 * 
 * @return ResourceList List of cloned Resources
 */
  ResourceList Strategy::cloneList(const ResourceList &resources,
                                   const bool &withAssets) const {
    ResourceList v_clone;
    BOOST_FOREACH ( SharedResource resource, resources ) {
      v_clone.append(SharedResource(resource->clone(resource->name(), withAssets))); 
    }
    return ( v_clone );
  }


  /**
   * @brief Identify and apply Strategy to Resources that intersect a geometry
   *  
   * This method applies the Strategy::apply(SharedResource &) method for all 
   * Resources whose geometry intersect the given geometry.  This method uses a 
   * very efficient RTree implementation in GEOS to determine geometries that 
   * intersect each others envelope (a minimal boundary for a geometry).  There 
   * is a significant performance increase (up to 30% or more) achieved by 
   * using this method over direct intersection and operation techniques. 
   *  
   * However, since this is typically done once, it is not advantageous to 
   * create an index and then search it doing essentially the same operation as 
   * the evaluation of the RTree. Therefore a direct intersection loop is 
   * available and applied by default. The use of the RTree method can be 
   * selected by a user preferenced keyword. Set GisMethod = Indexed.  The 
   * default is GisMethod = Direct. 
   *  
   * This method only acts upon active Resources, so be sure to provide the 
   * appropriate state of Resources in the list provided. 
   *  
   * It will discard all Resources prior to call the GEOS intersection query. 
   * When a Resources is identified to intersect, it makes in active again prior 
   * to calling the Strategy::apply(SharedResource &) method.  Note that the 
   * implementation of the apply(SharedResource &) should further check for false 
   * postives as the envelope used in the RTree method may not be as accurate 
   * as is required for a robust intersection of GIS geometries. 
   *  
   * @author 2013-02-23  Kris Becker
   * 
   *  
   * @param resources A list of resources to which the strategy will be applied.
   * @param geom A reference to the GIS geometry to be intersected.
   * @param globals Global parameter pool for keyword translation
   * 
   * @return int The number of resources processed.
   * @throw IException::Programmer "Cannot apply RTree search to bad geometry."
   */
  int Strategy::applyToIntersectedGeometry(ResourceList &resources, 
                                           GisGeometry &geom,
                                           const ResourceList &globals) {
  
    QString method = getDefinitionMap().get("GisMethod", "direct").toLower();  

    // The programmer is required to ensure a valid geometry provides the
    // source of the intersection operation.
    if ( !geom.isValid() ) {
      QString mess = type() + ":" + name() + 
                     "Cannot apply RTree search to bad geometry."; 
      throw IException(IException::Programmer, mess, _FILEINFO_);
    }
  
    //  Create a list of active resources only
    ResourceList v_active = activeList(resources);

    // Use direct computation method 
    ResourceList overlaps;
    if ( "direct" == method ) {
      if ( isDebug() ) {
        cout << "Using direct Geom intersects for " << v_active.size() << " geometries...\n";
      }

      // Direct intersect computations of overlaps has about the same overhead 
      // of constructing an RTree index and running queries.
      initProgress(v_active.size());
      BOOST_FOREACH ( SharedResource resource, v_active ) {
        if ( resource->hasValidGeometry() ) {
          if ( geom.intersects( *resource->geometry()) ) {
            overlaps.append(resource);
          }
        }
        processed();
      }
    }
    else {

      //  Use RTree method to get potenital overlappers...
      if ( isDebug() ) {
        cout << "Allocating " << v_active.size() << " RTree geometries.\n";
      }
      GEOSSTRtree *rtree = GEOSSTRtree_create(v_active.size());
      if ( !rtree ) {
        QString mess = "GEOS RTree allocation failed for " +
                        toString(v_active.size()) + " geometries.";
          throw IException(IException::Programmer, mess, _FILEINFO_);
        }

      if ( isDebug() ) {
        cout << "GEOS RTree allocated " << v_active.size() << " geometry entries.\n";
      }
  
      // Create and query the RTree geometries calling once for each 
      // strategy::appy(SharedResource &) geometry that intersect the envelope.
      initProgress(v_active.size());
      int nvalid = 0;
      for ( int i = 0 ; i < v_active.size() ; i++ ) {
        if ( v_active[i]->hasValidGeometry() ) {
          GEOSSTRtree_insert(rtree, v_active[i]->geometry()->geometry(),
                             &v_active[i]); 
          nvalid++;
        }
        processed();
      }

      // Report number found...
      if ( isDebug() ) {
        cout << "Valid Geometries found: " << nvalid << " - running query...\n";
      }
  
      // Run the query
      GEOSSTRtree_query(rtree, geom.geometry(), &queryCallback, &overlaps);
      GEOSSTRtree_destroy(rtree);
    }

    if ( isDebug() ) {
      cout << "Potential Intersections Found: " << overlaps.size() << "\n";
    }


    // Now apply all intersected geometries to calling strategy. Note to 
    // properly reflect the intersected list, we must deactivate the entire
    // active list after the query search and activate each one prior to
    // applying it in the strategy method, which has the responsibility to
    // determine validity, and return the proper Resource list status to the
    // next strategy.
    deactivateList(v_active);

    initProgress(overlaps.size());
    int n = 0;  // Result count
    BOOST_FOREACH ( SharedResource resource, overlaps ) {
      resource->activate();
      n += apply(resource, globals);
      processed();
    }


    if ( isDebug() ) {
      cout << "Total valid Intersections Found: " << n << "\n"; 
    }
  
    return (n);
  }
  
  
  /**
   * @brief Important GEOS query callback for class and overlap geometry 
   *  
   * This method accumulates the resources that are determined to potentially 
   * overlap the resource geometry provided in the GEOS query. 
   *  
   * This implementation may not be portable or safe. It most certainly is not 
   * threadable.  (I am open for suggestions on how to do this better...) 
   * 
   * @author 2015-03-27 Kris Becker 
   * 
   * @param item      Resource to add to overlap list
   * @param userdata  ResourceList to contain overlapping geoemetries
   */
  void Strategy::queryCallback(void *item, void *userdata) {
    SharedResource *resource = (SharedResource *) item;  // Maybe fancy casts?
    ResourceList *rlist = (ResourceList *) userdata;
    rlist->append(*resource);
    return;
  }
  
  
  /** 
   * An accessor method so that inherited classes can determine whether to print
   * debug messages for this object. 
   *  
   * @return bool Indicates whether to print debug messages for this object.
   */ 
  bool Strategy::isDebug() const {
    return ( m_debug );
  }


  /** 
   *  
   * @return bool 
   */ 
  bool Strategy::doShowProgress() const {
    return ( !m_progress.isNull() );
  }



  /**
   * @brief Initializes strategy progress monitor if requested by user
   *  
   * This method will set up progress monitoring for the user if the 
   * "ShowProgress" keyword exists and is set to "True" in any strategy Pvl 
   * configuration. 
   *  
   * The processed() method will apply status checking to measure progress. For 
   * consistency purposes, the resetProcessed() method is always called which 
   * resets the internal resource process count to 0. 
   *  
   * Strategy implementors must initialize using this method if they rederive 
   * apply(ResourceList &resources) or they do not call 
   * applyToResources(ResourceList &resources) or 
   * appyToIntersectedGeoemtry(ResourceList &resources). 
   *  
   * @author 2015-04-02 Kris Becker
   * 
   * @param nsteps Number of steps to set for progress 
   * @param text   Optional text to set progress message to
   * 
   * @return bool  True if progress is initialized based upon user preference
   */
  bool Strategy::initProgress(const int &nsteps, const QString &text) {
    
    QString p_text = text;
    resetProcessed();
    bool status = false;

    //  Check for initialization 
    if ( !doShowProgress()  ) {
      PvlFlatMap p_var( getDefinitionMap() );
      if ( toBool(p_var.get("ShowProgress", "false")) ) {
        m_progress.reset( new Progress() );
        if ( p_text.isEmpty() ) { 
           p_text = type() + "::" + name();
        }
      }
    }


    // No check if progress is requested
    if ( doShowProgress() ) {
      if ( !p_text.isEmpty() ) m_progress->SetText(p_text);
      m_progress->SetMaximumSteps(nsteps);
      if ( nsteps > 0 ) m_progress->CheckStatus();
      status = true;
    }

    // Set up for systematic processing
    return (status);
  }


  QStringList Strategy::getObjectList(const PvlObject &source) const {
    QStringList objList;
    PvlObject::ConstPvlObjectIterator object = source.beginObject();
    while ( object != source.endObject() ) {
      objList << object->name();
      ++object;
    }
    return (objList);
  }


}  //namespace Isis
