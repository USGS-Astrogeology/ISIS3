/**                                                                       
 * @file                                                                  
 * $Revision: 6513 $
 * $Date: 2016-01-14 16:04:44 -0700 (Thu, 14 Jan 2016) $
 * $Id: CalculatorStrategy.cpp 6513 2016-01-14 23:04:44Z kbecker@GS.DOI.NET $
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
#include "CalculatorStrategy.h"

// boost library
#include <boost/foreach.hpp>

// other ISIS
#include "IException.h"
#include "InlineCalculator.h"
#include "PvlFlatMap.h"
#include "PvlObject.h"
#include "Resource.h"
#include "SpecialPixel.h"

using namespace std;

namespace Isis {
  
  /** 
   * Default constructor. 
   */  
  CalculatorStrategy::CalculatorStrategy() : 
                                         Strategy("Calculator", "Calculator"), 
                                         m_equations(), m_initializers(0), 
                                         m_initArgs(), m_calculator(0), 
                                         m_result(0.0) { }
  
  /**
   * @brief Constructor loads from a Strategy object CalculatorStrategy
   *        definition
   *
   * This constructor loads and retains processing parameters form the
   * Calculator Strategy object definition as (typically) read form the
   * configuration file.
   *
   * @param definitnion Calculator Strategy PVL definition
   * @param globals Global Resource of keywords
   */ 
  CalculatorStrategy::CalculatorStrategy(const PvlObject &definition, 
                                         const ResourceList &globals) : 
                                         Strategy(definition, globals), 
                                         m_equations(), m_initializers(0), 
                                         m_initArgs(), m_calculator(0),
                                         m_result(0.0) {

    if ( getDefinition().hasGroup("Initializers") ) {
      const PvlObject &keydefs = getDefinition();
      PvlFlatMap *vars = new PvlFlatMap(keydefs.findGroup("Initializers"));
      m_initializers = QSharedPointer<PvlFlatMap> (vars);

      // Get initializer argument keyword replacement values
      if ( keydefs.hasKeyword("InitializersArgs") ) {
        PvlFlatMap args;
        args.add(keydefs["InitializersArgs"]);
        m_initArgs = args.allValues("InitializersArgs");
      }
    }

    PvlFlatMap parms( getDefinitionMap() );

    if ( parms.exists("Equation") ) {
      Equation eqn;
      eqn.equation = parms.get("Equation");
      InlineCalculator *calc = new InlineCalculator(eqn.equation);
      eqn.calculator = QSharedPointer<InlineCalculator> (calc);
      if ( parms.exists("Result") ) {
        eqn.store = parms.get("Result");
      }
      m_equations.append(eqn);
    }

    if ( definition.hasGroup("Equations") ) {
      PvlGroup eqns = getDefinition().findGroup("Equations");
      PvlGroup::PvlKeywordIterator i;
      for (i = eqns.begin(); i != eqns.end(); i++) {      
        Equation eqn;
        eqn.equation = QString::fromStdString(*i);
        InlineCalculator *calc = new InlineCalculator(eqn.equation);
        eqn.calculator = QSharedPointer<InlineCalculator> (calc);
        eqn.store = QString::fromStdString(i->name());
        m_equations.append(eqn);
      }

    }

    return;
  }
  
  
  /** 
   * Destructor 
   */  
  CalculatorStrategy::~CalculatorStrategy() {
  }
  
  
  /** 
   * @brief Calculates the result for a resoruce and stores it in the resource
   *
   * Calculates the result of the calculation for a resource and stores it in
   * the specified key in the resource. If there is an error in the calculation,
   * such as if the value of a variable is invalid, then the resource is
   * discarded.
   *
   * @param resource SharedResource to be written to
   * @param globals Global Resource of keywords 
   *  
   * @return int The number of keys written to the resource
   */ 
  int CalculatorStrategy::apply(SharedResource &resource, const ResourceList &globals) {  

    int nInits = initialize(resource, globals); 
    ResourceList variables = getGlobals(resource, globals);

    int ntotal = 0;
    BOOST_FOREACH ( Equation eqn, m_equations) {
      try {
        m_calculator = eqn.calculator;
        m_result = calculate(variables);
        if ( !eqn.store.isEmpty() ) {
          resource->add(eqn.store, QString::fromStdString(toString(m_result)));
        }
        ntotal++;
      }
      // Discard on error
      catch (IException &ie) {
        if ( isDebug() ) {
         cout << "Calculator error on " << resource->name().toStdString() << " with equation "
              << eqn.equation.toStdString() << ": " << ie.what() << "\n"; 
        }
        m_result = Null;
        resource->discard();
        ntotal = 0;
        break;
      }
    }

    // Check for initialization only and return success
    if ( (0 == m_equations.size()) && (nInits > 0) ) { return (1); }
    return ((ntotal == 0) ? 0 : 1); 
  }
  
  
  /** 
   * Returns the result of the calculation.
   *
   * @return double The result of the calculation
   */  
  double CalculatorStrategy::result() const {
    return (m_result);
  }
  
  
  /** 
   * @brief Initializes a Resource.
   *
   * Initializes a resource based on the keys in the list of initializers.
   * Each key in the list of initializers is added to the resource. Returns the
   * number of keys added.
   *
   * @param resource SharedResource to be initialized
   *
   * @return int The number of keys added to the resource
   */   
  int CalculatorStrategy::initialize(SharedResource &resource, 
                                     const ResourceList &globals) {
    int ntotal(0);
    if ( !m_initializers.isNull() ) {
      ResourceList myGlobals = getGlobals(resource, globals);
      BOOST_FOREACH ( PvlKeyword key, *m_initializers) {
        PvlKeyword newkey(key.name());
        for (int i = 0 ; i < key.size() ; i++) {
          QString value = processArgs(QString::fromStdString(key[i]), m_initArgs, myGlobals);
          newkey.addValue(value.toStdString());
          if ( isDebug() ) {
            cout << "Initializing " << key.name() << "[" << i << "] = " 
                  << value.toStdString() << "\n"; 
          }
        }
        resource->add(newkey); 
        ntotal++;
      }
    }
    return (ntotal);
  }
  
  
  /** 
   * Calculates the result of the equation for a resource and returns the
   * result.
   *
   * @param resource SharedResource to be used for the calculation.
   *
   * @return double The result of the calculation.
   */  
  double CalculatorStrategy::calculate(SharedResource &resource) {
    ResourceCalculatorVariablePool vars(resource);
    QVector<double> values = m_calculator->evaluate(&vars);
    return (values[0]);
  }
  

  /** 
   * Calculates the result of the equation for a list of resource and returns 
   * the result. 
   *
   * @param resources List of Resources to be used for the calculation.
   *
   * @return double The result of the calculation.
   */  
  double CalculatorStrategy::calculate(ResourceList &resources) {
    ResourceCalculatorVariablePool vars(resources);
    QVector<double> values = m_calculator->evaluate(&vars);
    return (values[0]);
  }

  
  /** 
   * @brief Initializes a PvlFlatMap.
   *
   * Initializes a PvlFlatMap based on the keys in the list of initializers.
   * Each key in the list of initializers is added to the PvlFlatMap. Returns
   * the number of keys added.
   *
   * @param pvl PvlFlatMap to be initialized
   *
   * @return int The number of keys added to the PvlFlatMap
   */ 
  int CalculatorStrategy::initialize(PvlFlatMap &pvl) {
    int ntotal(0);
    if ( !m_initializers.isNull() ) {
      BOOST_FOREACH ( PvlKeyword key, *m_initializers) {
        pvl.add(key);
        ntotal++;
      }
    }
    return (ntotal);
  }
  

  /** 
   * Calculates the result of the equation for a PvlFlatMap and returns the
   * result.
   *
   * @param pvl PvlFlatMap to be used for the calculation.
   *
   * @return double The result of the calculation.
   */ 
  double CalculatorStrategy::calculate(PvlFlatMap &pvl) {
    PvlFlatMapCalculatorVariablePool vars(pvl);
    QVector<double> values = m_calculator->evaluate(&vars);
    return (values[0]);
  }


  /** 
   * Construct ResourceCalculatorVariablePool with a SharedResource.
   *
   * @param resource SharedResource of the resource
   */
  ResourceCalculatorVariablePool::ResourceCalculatorVariablePool(
      SharedResource &resource) {
    m_resources.push_back(resource);
  }

  /** 
   * Construct ResourceCalculatorVariablePool with a SharedResource.
   *
   * @param resource SharedResource of the resource
   */
  ResourceCalculatorVariablePool::ResourceCalculatorVariablePool(
      ResourceList &resources) {
    m_resources.append(resources);
  }


  /** 
   * Destruct ResourceCalculatorVariablePool.
   */
  ResourceCalculatorVariablePool::~ResourceCalculatorVariablePool() { 
  }
  

  /** 
   * Check existance of variable.
   *
   * @param variable QString of the variable.
   */
  bool ResourceCalculatorVariablePool::exists(const QString &variable) const {
    BOOST_FOREACH ( SharedResource resource, m_resources ) {
      if ( resource->exists(variable) ) {  
            return (true);
      }
    }
    return (false);
  }
  

  /** 
   * Returns a vector associated to a variable request from the
   * calculator.
   *
   * @param variable QString of the variable
   *
   * @param index int of the index
   */
  QVector<double> ResourceCalculatorVariablePool::value(const QString &variable, 
                                                        const int &index) const {
    QVector<double> vars;
    BOOST_FOREACH ( SharedResource resource, m_resources ) {
      if ( resource->exists(variable) ) {  
         vars.push_back(resource->value(variable, index).toDouble());
         break;
      }
    }
    
    return (vars);
  }


  /** 
   * Construct PvlFlatMapCalculatorVariablePool with a PvlFlatMap.
   *
   * @param pvl PvlFlatMap
   */
  PvlFlatMapCalculatorVariablePool::PvlFlatMapCalculatorVariablePool(
      PvlFlatMap &pvl) : m_pvl(pvl) { 
  }


  /** 
   * Destruct PvlFlatMapCalculatorVariablePool.
   */
  PvlFlatMapCalculatorVariablePool::~PvlFlatMapCalculatorVariablePool() { 
  }
  

  /** 
   * Check existance of variable.
   *
   * @param variable QString of the variable.
   */
  bool PvlFlatMapCalculatorVariablePool::exists(const QString &variable) const {
    return (m_pvl.exists(variable));
  }
  

  /** 
   * Returns a vector associated to a variable request from the
   * calculator.
   *
   * @param variable QString of the variable
   * @param index int of the index
   */
  QVector<double> PvlFlatMapCalculatorVariablePool::value(const QString &variable, 
                                                          const int &index) const {
    QVector<double> vars;
    vars.push_back(m_pvl.get(variable, index).toDouble());
    return (vars);
  }

} // namespace Isis
