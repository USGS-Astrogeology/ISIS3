#ifndef FeatureAlgorithm_h
#define FeatureAlgorithm_h

/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */


#include "IException.h"
#include "PvlFlatMap.h"
#include "PvlKeyword.h"
#include "PvlObject.h"

#include <QString>
#include <QVariant>

// boost library
#include <boost/foreach.hpp>

#include "opencv2/core.hpp"
#include "opencv2/features2d.hpp"

namespace Isis {

/**
 * @brief Wrapper for generic algorithm OpenCV3 feature matcher algorithms
 *
 * This class provides a generic wrapper that is intended to restore some of the
 * functionality for these algorithms that were lost in the upgrade from version
 * 2. Hence, the class T is intended to be an OpenCV Ptr type that contains
 * either a Feature2D or DescriptorMatcher algorithm pointer.
 *
 * However, the design does lend itself to overide some the OpenCV specifics
 * if needed.
 *
 * The OpenCV implementation can simply extract and populate the
 * the PvlFlatMap variable structure to allow the default
 * implementation to serve up the algorithm variables to the
 * calling environment - or provide a specific implementation if
 * inadequate.
 *
 * The config variable provides a direct way to store how the
 * invocation string in the form of
 * "algorithmname@var:value@var:value...".
 *
 * @author  2016-11-30 Kris Becker
 *
 * @internal
 *   @history 2016-11-30 Kris Becker - Original Version
 */

template <class T> class FeatureAlgorithm {
  public:
    FeatureAlgorithm();
    FeatureAlgorithm(const QString &name, const QString &type);
    FeatureAlgorithm(const QString &name, const QString &type,
                     const T algorithm,
                     const PvlFlatMap &variables = PvlFlatMap());
    virtual ~FeatureAlgorithm();

    virtual bool isValid() const;

    QString name() const;
    QString type() const;

    /** Deriving classes must provide a description of the algorithm */
    virtual QString description() const;

    // Setter/getter for config string
    void    setConfig(const QString &config);
    QString config() const;

    // Setter/getter for algorithm variable
    virtual bool setVariable(const QString &var, const QString &value);
    virtual QString getVariable(const QString &var,
                                const QString &separator = ",");

    // Setter/getter for specialized variant variable
    virtual bool setVariable(const QString &name, const QVariant &var);
    virtual bool getVariable(const QString &var, QVariant &value);

    const PvlFlatMap &variables() const;

    /** Pointer operator - designed for a T shared pointer thus the oddness */
    T algorithm()  { return ( m_algorithm ); }
    T algorithm() const { return ( m_algorithm ); }

    // Return algorithm information in PVL form
    virtual PvlObject info(const QString &objname = "Algorithm") const;

    // Provide information about the abilities of the algorithm
    virtual bool hasDetector() const { return false; }
    virtual bool hasExtractor() const { return false; }
    virtual bool hasMatcher() const { return false; }

  protected:
    QString    m_name;
    QString    m_type;
    QString    m_config;
    T          m_algorithm;
    PvlFlatMap m_variables;

    /** Inheritor must provide implementation of variable
     *  manipulators */
    virtual PvlFlatMap getAlgorithmVariables() const;
    virtual int        setAlgorithmVariables(const PvlFlatMap &variables);
};



//-----------------------------------------------------------------------------
//  Implementation of template methods
//-----------------------------------------------------------------------------

/**
 * @brief Default constructor, which is likely needed but useless
 *
 * @author 2016-12-07 Kris Becker
 */
template <class T> FeatureAlgorithm<T>::FeatureAlgorithm() :
                                         m_name("FeatureAlgorithm"),
                                         m_type("Algorithm"), m_config(),
                                         m_algorithm(), m_variables() {  }


/**
 * @brief Construct a named feature algorithm
 *
 * This constructor is partially valid but assumes the deriving class defines
 * the algoritm after construction
 *
 * @author 2016-12-07 Kris Becker
 *
 * @param name  Name of algorithm
 * @param type  Type of Algorithm, typically "Feature2D" or "DecriptorMatcher"
 */
template <class T> FeatureAlgorithm<T>::FeatureAlgorithm(const QString &name,
                                                         const QString &type) :
                                                         m_name(name),
                                                         m_type(type),
                                                         m_config(),
                                                         m_algorithm(),
                                                         m_variables() {  }

/**
 * @brief Construct a complete algorithm
 *
 * This constructor will provide a complete algorithm with the parameters
 * provided.
 *
 * @author 2016-12-07 Kris Becker
 *
 * @param name       Name of algorithm
 * @param type       Type of Algorithm, typically "Feature2D" or "DecriptorMatcher"
 * @param algorithm  Algorithm (assumed to be a cv::Ptr-type parameter
 * @param variables  Optional list of variables that accompany the algorithm
 */
template <class T> FeatureAlgorithm<T>::FeatureAlgorithm(const QString &name,
                                                         const QString &type,
                                                         const T algorithm,
                                                         const PvlFlatMap &variables) :
                                                         m_name(name),
                                                         m_type(type),
                                                         m_config(),
                                                         m_algorithm(algorithm),
                                                         m_variables(variables) { }

/** Destructor */
template <class T> FeatureAlgorithm<T>::~FeatureAlgorithm() { }

/**
 * @brief Check validity of algorithm
 *
 * The default implementation is to simply test if the cv::Ptr is valid.
 * Derived classes can reimplement this method if needed.
 *
 * @author 2016-12-07 Kris Becker
 *
 * @return bool Returns true if all is well, false if not
 */
template <class T> bool FeatureAlgorithm<T>::isValid() const {
  return ( !m_algorithm.empty() );
}

/** Return the name of the algorithm */
template <class T> QString FeatureAlgorithm<T>::name() const {
  return ( m_name );
}

/** Returns the type of the algorithm   */
template <class T> QString FeatureAlgorithm<T>::type() const {
  return ( m_type );
}

/** Returns the description of the algorithm   */
template <class T> QString FeatureAlgorithm<T>::description() const {
  return ( QString("-- Description not provided ---") );
}

/** Set the config string if appropriate (typically seen at higher level) */
template <class T> void FeatureAlgorithm<T>::setConfig(const QString &config) {
   m_config = config;
}

/** Return the config string */
template <class T> QString FeatureAlgorithm<T>::config() const {
  return ( m_config );
}

/**
 * @brief Set an algorithm variable to a specific value
 *
 * The parameters provided in this method are deferred to the
 * setAlgorithmVariables() method that derived classes must implement. A
 * PvlFlatMap of the value is constructed and passed into the derived routine
 * that implenents the setting of all variables. The derived class must return
 * the number of valid variables set, which in this case is expected to be 1,
 * which is tested for success. If it fails, an exception is thrown. The
 * deriving class can also throw an exception if indicated, which is trapped
 * and propagated up the call chain.
 *
 * If the setting succeeds the value is added to the existing Pvl container to
 * maintain consistent states of the algorithm variable settings.
 *
 * If the needed, the deriving class can reimplement this method if the
 * behavior is not acceptable.
 *
 * @author 2016-12-07 Kris Becker
 *
 * @param var   Name of variable to set. Deriving classes should not expect
 *              case insensitive variable names. Note PvlFlatMap is case
 *              insensitive.
 * @param value String value to set
 *
 * @return bool True if the variable was successfully set. A false condition
 *              will never be returned as an exception is thrown on failure.
 */
template <class T> bool FeatureAlgorithm<T>::setVariable(const QString &var,
                                                      const QString &value) {
      PvlFlatMap variable;
      variable.add(var, value);
      try {
        if (0 >= setAlgorithmVariables(variable)) {
          QString mess = "Setting of variable " + var + " failed in " + name();
          throw IException(IException::Programmer, mess, _FILEINFO_);
        }
      }
      catch (IException &ie) {
        QString mess = "Error setting variable " + var + " in " + name();
        throw IException(ie, IException::Programmer, mess, _FILEINFO_);
      }

      m_variables.merge(variable);
      return (true);
    }

/**
 * @brief Return the values of an algorithm parameter
 *
 * This method returns the value of an algorithm as it is currently set.
 *
 * @author 2016-12-12 Kris Becker
 *
 * @param var  Name of variable to retrieve
 *
 * @return QString The value of the variable. Multiple values are separateed by
 *         commas.
 */
template <class T> QString FeatureAlgorithm<T>::getVariable(const QString &var,
                                                            const QString &separator) {
      if ( m_variables.exists(var) ) {
        return (m_variables.allValues(var).join(separator));
      }
      return (QString());
    }

/**
 * @brief Set variable with value contained in a QVariant
 *
 * This method is for those algorithm uncommon variable types that are not
 * suitable to be represented as a string or may require special consideration
 * (such as higher precision of doubles). The default implementation, however,
 * attempts to convert the QVariant content to a string and apply it using the
 * setAlgorithmnVariables() method.
 *
 * Deriving classes can reimplement this method should it need specialized
 * value treatment.
 *
 * @author 2016-12-07 Kris Becker
 *
 * @param name Name of variable to set
 * @param var  QVariant containing the value to appy in the algorithm
 *
 * @return bool True if successful, however failures result in a thrown
 *              exception
 */
template <class T> bool FeatureAlgorithm<T>::setVariable(const QString &vname,
                                                         const QVariant &var) {
      QString value = var.toString();
      if ( value.isEmpty() ) {
        QString mess = "Variant/variable " + vname +
                       " cannot be converted in " + name();
        throw IException(IException::Programmer, mess, _FILEINFO_);
      }

      PvlFlatMap variable;
      variable.add(vname, value);
      if ( 0 >= setAlgorithmVariables(variable) ) {
        QString mess = "Setting of variable " + vname + " failed in " + name();
        throw IException(IException::Programmer, mess, _FILEINFO_);
      }

      m_variables.merge(variable);
      return (true);
    }

/**
 * @brief Retreive an algorithm variable in a QVariant
 *
 * This method is provided for algorithm variable types that are not well
 * suited for representation in strings. One such example may be a cv::Mat
 * variable type.
 *
 * This method should be reimplemented by deriving classes with special
 * variable types if needed. The deffault behavior is to convert the parameter
 * data to a QStringList.
 *
 * @author 2016-12-07 Kris Becker
 *
 * @param var   Name of variable to retrieve its value from
 * @param value A QVariont to return the value in
 *
 * @return bool True if successful, however failures result in a thrown
 *              exception
 */
template <class T> bool FeatureAlgorithm<T>::getVariable(const QString &var,
                                                         QVariant &value) {
      value = QVariant(QStringList());
      if ( m_variables.exists(var) ) {
        if ( m_variables.count(var) > 0 ) {
          value = QVariant(m_variables.allValues(var));
        }
        return (true);
      }
      return (false);
    }

/** Returns the PvlFlatMap containing all the variable/values as
 *  keyword/values */
template <class T> const PvlFlatMap &FeatureAlgorithm<T>::variables() const {
  return ( m_variables );
}

/**
 * @brief Create an object structure of algorithm definition
 *
 * @author 2016-12-07 Kris Becker
 *
 * @param objname   Optional name for the PvlObject of the description
 *
 * @return PvlObject PvlObject containing a description of the algorithm
 */
template <class T> PvlObject FeatureAlgorithm<T>::info(const QString &objname) const {
    PvlObject data(objname.toStdString());
    data.addKeyword(PvlKeyword("CVVersion", CV_VERSION));
    data.addKeyword(PvlKeyword("Name", name().toStdString()));
    data.addKeyword(PvlKeyword("Type", type().toStdString()));

    PvlKeyword options("Features");
    if ( hasDetector() ) { options += "Detector"; }
    if ( hasExtractor() ) { options += "Extractor"; }
    if ( hasMatcher() ) { options += "Matcher"; }
    data.addKeyword(options);

    data.addKeyword(PvlKeyword("Description", description().toStdString()));
    data.addKeyword(PvlKeyword("CreatedUsing", config().toStdString()));

    QList<PvlKeyword> values = m_variables.values();
    PvlGroup parameters("Parameters");
    BOOST_FOREACH(PvlKeyword &key, values ) {
      parameters.addKeyword(key);
    }
    data.addGroup(parameters);
    return (data);
  }

template <class T> PvlFlatMap FeatureAlgorithm<T>::getAlgorithmVariables() const {
  return ( m_variables );
}


template <class T> int FeatureAlgorithm<T>::setAlgorithmVariables(const PvlFlatMap &variables) {
  throw IException(IException::Programmer, "Derived classes must reimplement this method",
                   _FILEINFO_);

  return (0);
}



// A couple of convenient typedefs for types we know will be need
typedef FeatureAlgorithm<cv::Ptr<cv::Feature2D> >         Feature2DAlgorithm;
typedef FeatureAlgorithm<cv::Ptr<cv::DescriptorMatcher> > DescriptorMatcherAlgorithm;

// Shared pointers for convenience
typedef cv::Ptr<Feature2DAlgorithm>         FeatureAlgorithmPtr;
typedef cv::Ptr<DescriptorMatcherAlgorithm> MatcherAlgorithmPtr;
};  // namespace Isis
#endif
