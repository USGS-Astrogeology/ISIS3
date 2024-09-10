/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "MaximumLikelihoodWFunctions.h"

#include <QDataStream>
#include <QString>

#include <math.h>
#include <stdio.h>

#include "IException.h"
#include "IString.h"

namespace Isis {
  /**
   * Sets up a maximumlikelihood estimation function with Huber model and default tweaking
   * constant
   */
  MaximumLikelihoodWFunctions::MaximumLikelihoodWFunctions() {
    this->setModel(Huber);
  } // choose Model and define the tweaking constant



  /**
   * Sets up a maximumlikelihood estimation function with specified model and default tweaking
   * constant
   *
   * @param[in] enum Model modelSelection,  the model to be used
   *                                        (see documentation for enum Model)
   */
  MaximumLikelihoodWFunctions::MaximumLikelihoodWFunctions(Model modelSelection) {
    // choose Model and define the tweaking constant
    this->setModel(modelSelection);
  }



  /**
   * Sets up a maximumlikelihood estimation function with specified model and tweaking constant
   *
   * @param[in] enum Model modelSelection, the model to be used
   *                                       (see documentation for enum Model)
   * @param[in] double tweaking constant, exact meaning varies by model, but generally the larger
   *                                      the value the more influence larger resiudals have on
   *                                      the solution. As well as possibly the more measures are
   *                                      included in the solution.
   * @throws IsisProgrammerError if tweakingConstant <= 0.0
   */
  MaximumLikelihoodWFunctions::MaximumLikelihoodWFunctions(Model modelSelection,
                                                           double tweakingConstant) {
    // choose Model and define the tweaking constant
    setModel(modelSelection, tweakingConstant);
  }

  MaximumLikelihoodWFunctions::MaximumLikelihoodWFunctions(
      const MaximumLikelihoodWFunctions &other)
      : m_model(other.m_model),
        m_tweakingConstant(other.m_tweakingConstant) {
  }


  MaximumLikelihoodWFunctions::~MaximumLikelihoodWFunctions() {
  } // empty destructor


  MaximumLikelihoodWFunctions &MaximumLikelihoodWFunctions::operator=(
      const MaximumLikelihoodWFunctions &other) {
    m_model = other.m_model;
    m_tweakingConstant = other.m_tweakingConstant;
    return *this;
  }

  /**
   * Allows the maximum likelihood model to be changed together and the default tweaking constant
   * to be set
   *
   * @Param[in] enum Model modelSelection,  the model to be used
   *                                        (see documentation for enum Model)
   */
  void MaximumLikelihoodWFunctions::setModel(Model modelSelection) {
    // choose Model and use default tweaking constant
    m_model = modelSelection;
    this->setTweakingConstantDefault();
  }



  /**
   * Sets default tweaking constants based on the maximum likelihood estimation model being used.
   */
  void MaximumLikelihoodWFunctions::setTweakingConstantDefault() {
    // default tweaking constants for the various likelihood models
    switch (m_model) {
      case Huber:
        m_tweakingConstant = 1.345; // "95% asymptotic efficiecy on the standard normal distribution"
                                    // is obtained with this constant,
                                    // see Zhang's "Parameter Estimation"
        break;
      case HuberModified:
        m_tweakingConstant = 1.2107;// "95% asymptotic efficiecy on the standard normal distribution"
                                    // is obtained with this constant,
                                    // see Zhang's "Parameter Estimation"
        break;
      case Welsch:
        m_tweakingConstant = 2.9846;// "95% asymptotic efficiecy on the standard normal distribution"
                                    // is obtained with this constant,
                                    // see Zhang's "Parameter Estimation"
        break;
      case Chen:
        m_tweakingConstant = 1;     // This is the constant used by Chen in his paper,
                                    // no specific reason why is stated
        break;
      default:
        m_tweakingConstant = 1;     // default, though at the time of writing this class,
                                    // this value should never actually be used
    }
  }



/**
 * Allows the maximum likelihood model to be changed together with the tweaking constant
 *
 * @param[in] enum Model modelSelection,  the model to be used
 *                                        (see documentation for enum Model)
 * @param[in] tweakingConstant,  exact meaning varies by model, but generally the
 *                                       larger the value the more influence larger resiudals
 *                                       have on the solution. As well as possibly the more
 *                                       measures are included in the solution.
 * @throws IsisProgrammerError if tweakingConstant <= 0.0
 */
  void MaximumLikelihoodWFunctions::setModel(Model modelSelection, double tweakingConstant) {
    // choose Model and define the tweaking constant
    m_model = modelSelection;
    setTweakingConstant(tweakingConstant);
  }



  /**
   * Allows the tweaking constant to be changed without changing the maximum likelihood function
   *
   * @param[in] tweakingConstant,  exact meaning varies by model, but generally the larger
   * the value the more influence larger resiudals have on the solution. As well as possiblly the
   * more measures are included in the solution.
   *
   * @throws IsisProgrammerError if tweakingConstant <= 0.0
   */
  void MaximumLikelihoodWFunctions::setTweakingConstant(double tweakingConstant) {
    // leave model type unaltered and change tweaking constant
    if (tweakingConstant <= 0.0) {
      IString msg = "Maximum likelihood estimation tweaking constants must be > 0.0";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    m_tweakingConstant = tweakingConstant;
  }



  /**
   *  Returns the current tweaking constant
   */
  double MaximumLikelihoodWFunctions::tweakingConstant() const {
    return m_tweakingConstant;
  }



  /**
   * This provides the scalar for the weight (not the scaler for the square
   * root of the weight, which is generally more useful)
   *
   * @param[in] double residualZScore, this the residual of a particulare measure in a particular
   *                                   iteration divided by the standard deviation (sigma) of that
   *                                   measure
   * @return double the scaler adjustment to the weight for the measure
   *                nominal weight = 1 /sigma/sigma and weight' = scaler/sigma/sigma
   */
  double MaximumLikelihoodWFunctions::weightScaler(double residualZScore) {
    // this is likely the least usefull of the scaler functions but it is provided for completness.
    // This directly provides the scaler for the weight (instead of the radical weight), thus it
    // provides sqrtWeightScaler^2
    switch(m_model) {
    case Huber:
      return this->huber(residualZScore);
    case HuberModified:
      return this->huberModified(residualZScore);
    case Welsch:
      return this->welsch(residualZScore);
    case Chen:
      return this->chen(residualZScore);
    default:
      return 1.0;  // default to prevent nonsense from being returned,
                   // but the program should never reach this line
    }
  }



  /**
   * This provides the scaler to the sqrt of the weight, which is very useful for building normal
   * equations.
   *
   * @param[in] double residualZScore, this the residual of a particulare measure in a particular
   *                                   iteration divided by the standard deviation (sigma) of that
   *                                   measure
   * @return double the scaler adjustment to the sqrt of the weight for the measure
   *         nominal sqrt(weight) = 1 /sigma and sqrt(weight') = scaler/sigma
   */
  double MaximumLikelihoodWFunctions::sqrtWeightScaler(double residualZScore) {
    // it is often convient to use square roots of weights when building normals, this function
    // provides the scaler for the square root of the weight directly
    double scaler = this->weightScaler(residualZScore);
    if (scaler <= 0.0) {
      return 0.0;  // <0 should never happen, but 0.0 may be quite frequent
                   //    (thus this saves some time)
    }
    return sqrt(scaler);
  }



  /**
   * Huber maximum likelihood estimation function evaluation.  For details, see documentation of the
   * enum, MaximumLikelihoodWFunctions::Model
   *
   * @param[in] double residualZScore, this the residual of a particulare measure in a particular
   *                                   iteration divided by the standard deviation (sigma) of that
   *                                   measure
   * @return double the scaler adjustment to the weight for the measure
   *         nominal weight = 1 /sigma/sigma and weight' = scaler/sigma/sigma
   */
  double MaximumLikelihoodWFunctions::huber(double residualZScore) {
    // huber weight function
    if ( fabs(residualZScore) < m_tweakingConstant) {
      return 1.0;
    }
    else {
      return m_tweakingConstant/fabs(residualZScore);
    }
  }



  /**
   * Modified Huber maximum likelihood estimation function evaluation.  For
   * details see documentation of enum Model
   *
   * @param[in] double residualZScore, this the residual of a particulare measure in a particular
   *                                   iteration divided by the standard deviation (sigma) of that
   *                                   measure
   * @return double the scaler adjustment to the weight for the measure
   *         nominal weight = 1 /sigma/sigma and weight' = scaler/sigma/sigma
   */
  double MaximumLikelihoodWFunctions::huberModified(double residualZScore) {
    // huber modified weight function
    if ( fabs(residualZScore)/m_tweakingConstant < Isis::HALFPI) {
      return m_tweakingConstant*(sin(residualZScore/m_tweakingConstant)/residualZScore);
    }
    else {
      return m_tweakingConstant / fabs(residualZScore);
    }
  }



  /**
   * Modified Huber maximum likelihood estimation function evaluation.  For
   * details see documentation of enum Model
   *
   * @param[in] double residualZScore, this the residual of a particulare measure in a particular
   *                                   iteration divided by the standard deviation (sigma) of that
   *                                   measure
   * @return double the scaler adjustment to the weight for the measure
   *         nominal weight = 1 /sigma/sigma and weight' = scaler/sigma/sigma
   */
  double MaximumLikelihoodWFunctions::welsch(double residualZScore) {
    // welsch weight function
    double weightFactor = residualZScore / m_tweakingConstant;
    return exp(-(weightFactor)*(weightFactor));
  }



  /**
   * Modified Huber maximum likelihood estimation function evaluation.
   * For details, see documentation of enum Model.
   *
   * @param[in] double residualZScore, this the residual of a particulare measure in a particular
   *                                   iteration divided by the standard deviation (sigma) of that
   *                                   measure
   * @return double the scaler adjustment to the weight for the measure
   *         nominal weight = 1 /sigma/sigma and weight' = scaler/sigma/sigma
   */
  double MaximumLikelihoodWFunctions::chen(double residualZScore) {
    // chen weight function
    if ( fabs(residualZScore) <= m_tweakingConstant) {
      double weightFactor = m_tweakingConstant * m_tweakingConstant
                            - residualZScore * residualZScore;
      return 6 * weightFactor * weightFactor;  // use of weight factor variable reduces number of
                                               // operations from 7 to 4
    }
    else {
      return 0.0;
    }
  }



  /**
   * Suggest a quantile of the probility distribution of the residuals to use as the tweaking
   * constants based on the maximum likelihood estimation model being used.
   *
   * @return double quantile [0,1] the value pretaining to this quantile (in the probility
   *         distribution of the residuals) should be used as the tweaking constant.
   */
  double MaximumLikelihoodWFunctions::tweakingConstantQuantile() {
    // returns which quantile of the sqaured residuals is recommended to use as the tweaking
    // constants, this varies as a function of the model being employed desired quantiles for
    // various models,  these parameters are estimated based on inspection of the fucntions and
    // should be tested and revised with experience
    switch(m_model) {
    case Huber:
      return 0.5;  // In this model m_tweakingConstant determines the point at which residuals stop having
                   // increased influence on the solution, so after the median all the measures will
                   // have the same effect on the solution regardless of magnitude
    case HuberModified:
      return 0.4; // In this model after residualZScore >= m_tweakingConstant*Isis::HALFPI the residuals have the same
                  // influence on the solution,
    case Welsch:
      return 0.7; // at about double m_tweakingConstant the residuals have very little influence
    case Chen:
      return 0.98; // after r > m_tweakingConstant residuals have no influence
    default:
      return 0.5; // default, though at the time of writing this should never actually be used
    }
  }



  /**
   * Static method to return a string represtentation for a given MaximumLikelihoodWFunctions::Model
   * enum.
   *
   * @param model Enumerated value for a MaximumLikelihoodWFunctions model.
   * @return QString label for the enumeration.
   */
  QString MaximumLikelihoodWFunctions::modelToString(MaximumLikelihoodWFunctions::Model model) {
    if (model == MaximumLikelihoodWFunctions::Huber)              return "Huber";
    else if (model == MaximumLikelihoodWFunctions::HuberModified) return "HuberModified";
    else if (model == MaximumLikelihoodWFunctions::Welsch)        return "Welsch";
    else if (model == MaximumLikelihoodWFunctions::Chen)          return "Chen";
    else throw IException(IException::Programmer,
                          "Unknown estimation model enum [" + toString(model) + "].",
                          _FILEINFO_);
  }



  MaximumLikelihoodWFunctions::Model MaximumLikelihoodWFunctions::stringToModel(QString modelName) {
    if (modelName.compare("HUBER", Qt::CaseInsensitive) == 0) {
      return Huber;
    }
    else if (modelName.compare("HUBER_MODIFIED", Qt::CaseInsensitive) == 0 ||
             modelName.compare("HUBERMODIFIED", Qt::CaseInsensitive) == 0 ||
             modelName.compare("HUBER MODIFIED", Qt::CaseInsensitive) == 0) {
      return HuberModified;
    }
    else if (modelName.compare("WELSCH", Qt::CaseInsensitive) == 0) {
      return Welsch;
    }
    else if (modelName.compare("CHEN", Qt::CaseInsensitive) == 0) {
      return Chen;
    }
    else {
      throw IException(IException::Programmer,
                       "Unknown maximum likelihood model name " + modelName.toStdString() + ".",
                       _FILEINFO_);
    }
  }



  /**
   * Method to return a string represtentation of the weighted residual cutoff (if it exists) for
   * the MaximumLikelihoodWFunctions::Model.  If no cutoff exists, the string "N/A" is returned.
   *
   * @return QString label for the weighted residual cut off of the maximum likelihood estimation
   *         model.
   * @throw  "Estimation model has not been set."
   */
  QString MaximumLikelihoodWFunctions::weightedResidualCutoff() {
    if (m_model == Huber || m_model == HuberModified) return "N/A";
    else if (m_model == Welsch) return QString::number(m_tweakingConstant * 1.5);
    else if (m_model == Chen) return QString::number(m_tweakingConstant);
    else throw IException(IException::Programmer, "Estimation model has not been set.", _FILEINFO_);
  }



  /**
   * Accessor method to return the MaximumLikelihoodWFunctions::Model enumeration.
   *
   * @return enum for the maximum likelihood estimation model.
   */
  MaximumLikelihoodWFunctions::Model MaximumLikelihoodWFunctions::model() const {
    return m_model;
  }



  QDataStream &MaximumLikelihoodWFunctions::write(QDataStream &stream) const {
    stream << (qint32)m_model
           << m_tweakingConstant;
    return stream;
  }



  QDataStream &MaximumLikelihoodWFunctions::read(QDataStream &stream) {
    qint32 model;
    stream >> model >> m_tweakingConstant;
    m_model = (MaximumLikelihoodWFunctions::Model)model;
    return stream;
  }



  QDataStream &operator<<(QDataStream &stream, const MaximumLikelihoodWFunctions &mlwf) {
    return mlwf.write(stream);
  }



  QDataStream &operator>>(QDataStream &stream,  MaximumLikelihoodWFunctions &mlwf) {
    return mlwf.read(stream);
  }



  QDataStream &operator<<(QDataStream &stream, const MaximumLikelihoodWFunctions::Model &modelEnum) {
    stream << (qint32)modelEnum;
    return stream;
  }



  QDataStream &operator>>(QDataStream &stream, MaximumLikelihoodWFunctions::Model &modelEnum) {
    qint32 modelInteger;
    stream >> modelInteger;
    modelEnum = (MaximumLikelihoodWFunctions::Model)modelInteger;
    return stream;
  }

}// end namespace Isis
