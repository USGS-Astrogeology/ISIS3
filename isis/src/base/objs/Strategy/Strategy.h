#ifndef Strategy_h
#define Strategy_h
/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

// Qt library
#include <QList>
#include <QPair>
#include <QScopedPointer>
#include <QSharedPointer>
#include <QString>

// SharedResource and ResourceList typedefs
#include "GisGeometry.h"
#include "Resource.h"
#include "PvlObject.h"

class QStringList;

namespace Isis {

  class GisGeometry;
  class Progress;


  /**
   * @brief Strategy - Supports algorithm development
   *  
   * This base class provides a framework to develop strategic 
   * planning tools used to process and/or rank ISIS resources. 
   *  
   * The ISIS resource can be cubes, PVLs (e.g., PDS/EDRs, etc), 
   * CSV files or virtually any other resource type simply by 
   * adding support in the Resources type class.
   *  
   * This default implementation is simply a LessThan function for real data types
   * - likely on to the more heavily used function evaluators. 
   *  
   * The Users may provide values for the following influential keywords in the 
   * Pvl Stategy configuration: 
   *  
   * @code 
   * Object = Strategy 
   *   Name = Emission
   *   Type = Calculator
   *   Equation = "((EmissionAngle > 20) && (EmissionAngle < 80))"
   *   Result = EmissionCriteria
   * EndObject 
   * @endcode 
   *  
   * @author 2012-07-15 Kris Becker 
   * @internal 
   *   @history 2012-07-15 Kris Becker - Original version.
   *   @history 2015-03-18 Jeannie Backer - Brought class files closer to ISIS coding standards.
   *   @history 2015-04-07 Kristin Berry - Modified applytoIntersectedGeometry to deal
   *   @history 2015-03-26 Jeannie Backer - Updated documentation.
   *   @history 2015-03-28 Kris Becker - Added the composite(Resource, Resource)
   *                          method
   *   @history 2015-04-30 Kris Becker - Added setName() and setType() methods
   *   @history 2015-05-08 Kris Becker - Added new methods to provide better
   *                          support for keyword argument replacement with
   *                          global parameters: getGlobalDefaults(),
   *                          getGlobals(), getDefinition() and
   *                          getDefinitionMap(). Removed virtual designation
   *                          for apply() methods that do not take a globals
   *                          resource list.
   *  @history 2015-06-17 Kris Becker - Added ability for users to simplify
   *                          geometry such that topology is preserved. Keywords
   *                          that provide numbers of original points, points in
   *                          geoemtry size and tolerance used can optionally be
   *                          created in Resources that apply this feature.
   *  @history 2015-09-27 Kris Becker - Added copyList() and cloneList()
   *                          methods.
   *  @history 2015-10-12 Kris Becker - Fixed bug in how arguments are scanned
   *                          and replaced. Once the count of arguments reached
   *                          above 9, %1 was replaced for %10. Scan for
   *                          replacements in reverse order fixes this issue.
   * @history 2016-03-07  Tyler Wilson - Corrected documentation, and created a
   *                          unit test to test most of this classe's methods.
   * @history 2018-07-29 Kris Becker - Fixed errors in importGeometry() where an 
   *                          exception was being thrown when checking for valid
   *                          Geometry. It now checks for this case and runs
   *                          buffer(0) to attempt to fix the geometry. This is
   *                          typically a self-intersection error that can be
   *                          repaired with a buffer(0) operator.
   */

  class Strategy {
  
    public:
      Strategy();
      Strategy(const PvlObject &definition, const ResourceList &globals);
      virtual ~Strategy();
  
      QString name() const;
      QString type() const;
      QString description() const;
  
      // Not intended to be reimplemented but available for direct calls
      int apply(ResourceList &resources);
      int apply(SharedResource &resource);

      // Inheriting strategies must reimplement either one or both of these
      virtual int apply(ResourceList &resources, const ResourceList &globals);
      virtual int apply(SharedResource &resource, const ResourceList &globals);
  
      unsigned int totalProcessed() const;
  
   protected:
     //! Definition for a SharedPvlObject, a shared pointer to a PvlObject 
     typedef QSharedPointer<PvlObject> SharedPvlObject;
  
     // For default constructions
     Strategy(const QString &name, const QString &type);

     void setName(const QString &name);
     void setType(const QString &type);

     ResourceList getGlobalDefaults() const;
     ResourceList getGlobals(SharedResource &myGlobals, 
                             const ResourceList &globals) const;
     const PvlObject &getDefinition() const;
     PvlFlatMap getDefinitionMap() const;

  
     void setApplyToDiscarded();
     bool isApplyToDiscarded() const;
     void setDoNotApplyToDiscarded();  // default
  
     int applyToResources(ResourceList &resources, const ResourceList &globals);
     unsigned int processed();
     void resetProcessed();
  
     int countActive(const ResourceList &resources) const;
     int countDiscarded(const ResourceList &resources) const;
     ResourceList assetResourceList(const SharedResource &resource, 
                                       const QString &name) const;
  
     QString findReplacement(const QString &target, const ResourceList &globals,
                             const int &index = 0, 
                             const QString &defValue = "") const;
     QStringList qualifiers(const QString &keyspec, 
                               const QString &delimiter = "::") const;
     QString scanAndReplace(const QString &input, const QString &target, 
                            const QString &replacement) const;
     QString translateKeywordArgs(const QString &value,
                                  const ResourceList &globals,
                                  const QString &defValue = "") const; 
     QString processArgs(const QString &value, const QStringList &argKeys,
                         const ResourceList &globals,
                         const QString &defValue = "") const;
  
     void propagateKeys(SharedResource &source, SharedResource &target) const;
     SharedResource composite(SharedResource &resourceA, SharedResource &resourceB,
        const QPair<QString, QString> &keySuffix = qMakePair(QString("A"),QString("B"))) const;
     bool importGeometry(SharedResource &resource, 
                         const ResourceList &globals) const;
  
     ResourceList activeList(ResourceList &resources) const;
     void activateList(ResourceList &resources) const;
     void deactivateList(ResourceList &resources) const;
     ResourceList copyList(const ResourceList &resources) const;
     ResourceList cloneList(const ResourceList &resources,
                            const bool &withAssets = false) const;

     int applyToIntersectedGeometry(ResourceList &resources, GisGeometry &geom,
                                    const ResourceList &globals);
     bool isDebug() const;

     bool doShowProgress() const;
     bool initProgress(const int &nsteps = 0, const QString &text = "");

     static void queryCallback(void *item, void *userdata);

     QStringList getObjectList(const PvlObject &object) const;

     template <class STRATEGYLIST, class STRATEGYFACTORY> 
       STRATEGYLIST LoadMinerStrategies(const QString &minerName, 
                                        const ResourceList &globals) const {
         STRATEGYFACTORY *factory = STRATEGYFACTORY::instance();

         STRATEGYLIST miner;
         QString config = translateKeywordArgs(minerName+"ConfigFile", globals);
         if ( !config.isEmpty() ) {
           miner = factory->buildRun(config, globals);
         }
         else if ( getDefinition().hasObject(minerName+"Miner") ) {
           miner = factory->buildRun(getDefinition().findObject(minerName+"Miner"), 
                                                globals);
       }
       
       return (miner);
     }

  private:
     ResourceList   m_globals;    /**< A shared pointer to the global Resource of keywords. 
                                        If not provided upon construction, it will be set to NULL.
                                        If a SharedResource is passed into the apply() method, 
                                        this member is temporarily set to the given SharedResource
                                        and then restored.  The apply() method can not be used as a 
                                        mutator for this variable.*/
     SharedPvlObject m_definition; /**< A shared pointer to the PvlObject that defines the strategy.
                                        If a definition PvlObject is not passed in on construction,
                                        then a pointer to an empty PvlObject is created using the
                                        strategy name. See class documentation for an example.*/

      QString      m_name;           /**< A string containing the name of the strategy. This name is
                                          passed into the constructor directly or found in the 
                                          Pvl definition object. If none is provided, the default
                                          name 'Strategy' is set.*/
      QString      m_type;           /**< A string containing the type of strategy. This type is
                                          passed into the constructor directly or found in the 
                                          Pvl definition object. If none is provided, the default
                                          type 'Counter' is set.*/
      unsigned int m_total;          /**< The total number of resources processed. This value is 
                                          initialized to zero, incremented by the applyToResources()
                                          or processed() methods, and can be reset to zero by
                                          calling resetProcessed().*/ 
      bool         m_applyDiscarded; /**< Indicates whether to apply strategy to discarded 
                                          resources. This value is initialized to false if not found
                                          in the Pvl definition object.*/
      bool         m_debug;          /**< Indicates whether to print debug messages. This value is
                                          initialized to false if not found in the Pvl definition
                                          object.*/
      QScopedPointer<Progress> m_progress; //!< Progress percentage monitor
  
  };
  
  //! Definition for a SharedStrategy, a shared pointer to a Strategy
  typedef QSharedPointer<Strategy> SharedStrategy;
  
  //! Definition for a StrategyList, a list of SharedStrategy types
  typedef QList<SharedStrategy> StrategyList;

} // Namespace Isis

Q_DECLARE_METATYPE(Isis::SharedStrategy);

#endif

