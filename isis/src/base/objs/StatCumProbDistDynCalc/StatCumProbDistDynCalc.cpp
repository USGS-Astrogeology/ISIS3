/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "StatCumProbDistDynCalc.h"

#include <QDataStream>
#include <QDebug>
#include <QList>
#include <QUuid>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>

#include <float.h>
#include <math.h>
#include <stdio.h>

#include "IException.h"
#include "IString.h"
#include "Project.h"

#include "Pvl.h"
#include <iostream>

namespace Isis {


  /** 
   * Construtor sets up the class to start recieving data
   *
   * @param [in] unsigned int nodes -- this is the number of specific evenly spaced quantiles that
   *                 will be dynamically tracked
   */
  StatCumProbDistDynCalc::StatCumProbDistDynCalc(unsigned int nodes, QObject *parent)
      : QObject(parent) {
    initialize();
    setQuantiles(nodes);
  }


  StatCumProbDistDynCalc::StatCumProbDistDynCalc(QXmlStreamReader *xmlReader, QObject *parent) {
    initialize();
    readStatistics(xmlReader);
  }

  void StatCumProbDistDynCalc::readStatistics(QXmlStreamReader *xmlReader) {
    Q_ASSERT(xmlReader->name() == "statCumProbDistDynCalc");
    while (xmlReader->readNextStartElement()) {
      if (xmlReader->qualifiedName() == "numberCells") {
        try {
          m_numberCells = toDouble(xmlReader->readElementText());
        }
        catch (IException &e) {
          m_numberCells = 0.0;
        }
      }
      else if (xmlReader->qualifiedName() == "numberQuantiles") {
        try {
          m_numberQuantiles = toDouble(xmlReader->readElementText());
        }
        catch (IException &e) {
          m_numberQuantiles = 0.0;
        }
      }
      else if (xmlReader->qualifiedName() == "numberObservations") {
        try {
          m_numberObservations = toDouble(xmlReader->readElementText());
        }
        catch (IException &e) {
          m_numberObservations = 0.0;
        }
      }
      else if (xmlReader->qualifiedName() == "distributionData") {
        m_quantiles.clear();
        m_observationValues.clear();
        m_idealNumObsBelowQuantile.clear();
        m_numObsBelowQuantile.clear();
        for (unsigned int i = 0; i < m_numberQuantiles; i++) {
          while (xmlReader->readNextStartElement()) {
            if (xmlReader->qualifiedName() == "quantileInfo") {
              QStringRef quantile = xmlReader->attributes().value("quantile");
              if (!quantile.isEmpty()) {
                m_quantiles.append(quantile.toDouble());
              }
              QStringRef dataValue = xmlReader->attributes().value("dataValue");
              if (!dataValue.isEmpty()) {
                m_observationValues.append(dataValue.toDouble());
              }
              QStringRef idealNumObsBelowQuantile = xmlReader->attributes().value("idealNumObsBelowQuantile");
              if (!idealNumObsBelowQuantile.isEmpty()) {
                m_idealNumObsBelowQuantile.append(idealNumObsBelowQuantile.toDouble());
              }
              QStringRef actualNumObsBelowQuantile = xmlReader->attributes().value("actualNumObsBelowQuantile");
              
              if (!actualNumObsBelowQuantile.isEmpty()) {
                m_numObsBelowQuantile.append(actualNumObsBelowQuantile.toInt());
              }
            }
            else {
              xmlReader->skipCurrentElement();
            }
          }
        }
      }
      else {
        xmlReader->skipCurrentElement();
      }
    }
  }


  StatCumProbDistDynCalc::StatCumProbDistDynCalc(const StatCumProbDistDynCalc &other)
    : m_numberCells(other.m_numberCells),
      m_numberQuantiles(other.m_numberQuantiles),  
      m_numberObservations(other.m_numberObservations),
      m_quantiles(other.m_quantiles),        
      m_observationValues(other.m_observationValues),   
      m_idealNumObsBelowQuantile(other.m_idealNumObsBelowQuantile),         
      m_numObsBelowQuantile(other.m_numObsBelowQuantile) {
  }



  /** 
   * Destroys StatCumProbDistDynCalc object.
   */ 
  StatCumProbDistDynCalc::~StatCumProbDistDynCalc() { 
  }



  StatCumProbDistDynCalc &StatCumProbDistDynCalc::operator=(const StatCumProbDistDynCalc &other) {

    if (&other != this) {
      m_numberCells              = other.m_numberCells;
      m_numberQuantiles          = other.m_numberQuantiles;
      m_numberObservations       = other.m_numberObservations;
      m_quantiles                = other.m_quantiles;
      m_observationValues        = other.m_observationValues;
      m_idealNumObsBelowQuantile = other.m_idealNumObsBelowQuantile;
      m_numObsBelowQuantile      = other.m_numObsBelowQuantile;
    }
    return *this;

  }



  /** 
   * Inializer, resets the class to start its dynamic calculation anew
   *
   * @param [in] unsigned int nodes -- this is the number of specific evenly spaced quantiles that 
   *                 will be dynamically tracked
   */
  void StatCumProbDistDynCalc::initialize() {
    m_numberCells = m_numberQuantiles = m_numberObservations = 0;
    m_quantiles.clear();
    m_observationValues.clear();
    m_idealNumObsBelowQuantile.clear();
    m_numObsBelowQuantile.clear();
  }



  void StatCumProbDistDynCalc::setQuantiles(unsigned int nodes) {
    initialize();
    if (nodes < 3) {
      m_numberQuantiles = 3; 
    }
    else {
      m_numberQuantiles = nodes;
    }

    m_numberCells = m_numberQuantiles - 1; // there is one more border value then there are cells

    double stepSize = 1.0 / (double)m_numberCells;
    for (unsigned int i = 0; i < m_numberQuantiles; i++) {
      if (i == 0) {
        m_quantiles.append( 0.0 );
      }
      else {
        m_quantiles.append( m_quantiles[i-1] + stepSize );
      } // essentially the same as i/numCells without having to cast i every time...
      m_idealNumObsBelowQuantile.append(i+1);
      m_numObsBelowQuantile.append(i+1);
    }

  }



  /** 
   * Returns the maximum observation so far included in the dynamic calculation
   *
   * @return double -- the maximum observation so far included in the dynamic calculation
   * @throw  IsisProgrammerError -- StatCumProbDistDynCalc will return 
   *             no data until the number of observations added matches
   *             the number of quantiles (i.e. number of nodes)
   *             selected.
   */
   double StatCumProbDistDynCalc::max() {
     validate(); 
     //return the largest value so far
     return m_observationValues[m_numberCells];
   }



  /** 
   * Returns the maximum observation so far included in the dynamic calculation
   *
   * @return double -- the maximum observation so far included in the dynamic calculation
   * @throw  IsisProgrammerError -- StatCumProbDistDynCalc will return 
   *             no data until the number of observations added matches
   *             the number of quantiles (i.e. number of nodes)
   *             selected.
   */
   double StatCumProbDistDynCalc::min() { 
     validate();
     //return the smallest value so far
     return m_observationValues[0];
   }



  /** 
   * Provides the value of the variable that has the given cumulative probility
   * (according the current estimate of cumulative probility function)
   *
   * @param [in] cumProb -- cumlative probability, domain [0, 1]
   * @return double -- the vaule of the variable that has the cumulative probility (according the 
   *             current estimate of cumulative probility function)
   * @throw  IsisProgrammerError -- StatCumProbDistDynCalc will return no data until 
   *             the number of observations added matches the number of
   *             quantiles (i.e. number of nodes) selected.
   * @throw  IsisProgrammerError -- Invalid cumulative probability passed in to 
   *             StatCumProbDistDynCalc::value(double cumProb). Must be on
   *             the domain [0, 1].
   */
   double StatCumProbDistDynCalc::value(double cumProb) {
     validate(); 
     //given a cumProbability return the associate value

     //if the requested cumProb is outside [0,1] return DBL_MAX
     if (cumProb < 0.0 || cumProb > 1.0) {
       IString msg = "Invalid cumulative probability [" + toString(cumProb) + "] passed in to "
                     "StatCumProbDistDynCalc::value(double cumProb). Must be on the domain [0, 1].";
       throw IException(IException::Programmer, msg, _FILEINFO_);
     }

     //if the requested quantile is 0.0 return the lowest value
     if (cumProb == 0.0) {
       return m_observationValues[0];
     }

     //if the requested quantile is 1.0 return the largest value
     if (cumProb == 1.0) {
       return m_observationValues[m_numberCells];
     }

     // otherwise find the the node nearest the value
     double minDistance = fabs(m_quantiles[0] - cumProb); // distance from first quantile 
                                                          // to the given probability
     unsigned int index = 0;
     for (int i = 1; i < int(m_numberQuantiles); i++) {
        // if the given probability is closer to this quantile than the currently saved 
        // min distance, then replace
       double dist = fabs(m_quantiles[i] - cumProb);
//       if ( dist < minDistance || qFuzzyCompare(dist, minDistance)) { 
       if ( dist < minDistance ) { 
         minDistance = fabs(m_quantiles[i] - cumProb);
         index = i;
       }
       else { 
         break; // we are getting farther from the given value
       }
     }// TODO: must be a better way to write this???

     //get the coordinates of the three closest nodes, value as a function of cumProb
     double x[3]; // m_observationValues values
     double y[3]; // cumlative probilities

     if (index == 0) { // the given prob is closest to the first quantile
       y[0] = m_observationValues[0];
       y[1] = m_observationValues[1];
       y[2] = m_observationValues[2];

       x[0] = m_quantiles[0];
       x[1] = m_quantiles[1];
       x[2] = m_quantiles[2];
     }
     else if (index == m_numberCells) { // the given prob is closest to the last quantile
       y[0] = m_observationValues[index-2];
       y[1] = m_observationValues[index-1];
       y[2] = m_observationValues[index  ];

       x[0] = m_quantiles[index-2];
       x[1] = m_quantiles[index-1];
       x[2] = m_quantiles[index  ];
     }
     else {
       y[0] = m_observationValues[index-1];
       y[1] = m_observationValues[index  ];
       y[2] = m_observationValues[index+1];

       x[0] = m_quantiles[index-1];
       x[1] = m_quantiles[index  ];
       x[2] = m_quantiles[index+1];
     }

     if ( x[0] == x[1] || x[0] == x[2] || x[1] == x[2]) {
       return m_observationValues[index];  // this should never happen,
                                        // but just in case return the value of the nearest node
     }

     // try quadratic interpolation
     double interp =   (cumProb-x[1]) * (cumProb-x[2]) / (x[0]-x[1]) / (x[0]-x[2]) * y[0]
                     + (cumProb-x[0]) * (cumProb-x[2]) / (x[1]-x[0]) / (x[1]-x[2]) * y[1]
                     + (cumProb-x[0]) * (cumProb-x[1]) / (x[2]-x[0]) / (x[2]-x[1]) * y[2];

     // check for reasonability of the quadratic interpolation

     // TODO: better way to write this???
     int i;
     for (i = 0; i < 3; i++) {
       if ( x[i] <= cumProb && x[i+1] >= cumProb) { // find the nodes on both sides of cumProb
         break;
       }
     }

     // make sure things are strictly increasing
     if (y[i] <= interp && y[i+1] >= interp) {
       return interp;
     }
     else {
       // if the quadratic wasn't reasonable return the linear interpolation
       return ( (x[i] - cumProb) * y[i+1] + (x[i+1] - cumProb) * y[i] ) / (x[i] - x[i+1]);
     }

   }



  /** 
   * Provides the cumulative probility, that is, the proportion of the
   * distribution that is less than or equal to the value given (according the
   * current estimate of cumulative probility function).
   *
   * @param [in] value -- the upper bound of values considered in the cumlative 
   *                probility calculation
   * @return    double -- the cumulative probility, that is, the proportion of the distribution 
   *                that is less than or equal to the value given (according the current
   *                estimate of cumulative probility function).
   * @throw     IsisProgrammerError -- StatCumProbDistDynCalc will return no data until there has 
   *                been at least m_numberQuantiles observations added
   */
   double StatCumProbDistDynCalc::cumProb(double value) {
     validate(); 
     //given a value return the cumulative probility

     if (value <= m_observationValues[0]) {
       return 0.0; // if the value is less than or equal to the the current min, 
                   // the best estimate is 0.0
     }
     else if (value >= m_observationValues[m_numberCells]) {
       return 1.0;  // if the value is greater than or equal to the current max,
                    // the best estimate is 1.0
     }

     // otherwise, find the the node nearest to the given value
     double minDistance = fabs(m_observationValues[0]-value);
     unsigned int index = 0;
     for (unsigned int i = 1; i < m_numberQuantiles; i++) {
       double dist = fabs(m_observationValues[i]-value);
       if ( dist < minDistance) {
         minDistance = fabs(m_observationValues[i]-value);
         index = i;
       }
       else { 
         break; // we are getting farther from the given value
       }
     }// TODO: must be a better way to write this???

     //get the coordinates of the three closest nodes cumProb as a function of value
     double x[3]; // m_observationValues values
     double y[3]; // cumlative probilities

     if (index == 0) {
       x[0] = m_observationValues[0];
       x[1] = m_observationValues[1];
       x[2] = m_observationValues[2];

       y[0] = m_quantiles[0];
       y[1] = m_quantiles[1];
       y[2] = m_quantiles[2];
     }
     else if (index == m_numberCells) {
       x[0] = m_observationValues[index-2];
       x[1] = m_observationValues[index-1];
       x[2] = m_observationValues[index  ];

       y[0] = m_quantiles[index-2];
       y[1] = m_quantiles[index-1];
       y[2] = m_quantiles[index  ];
     }
     else {
       x[0] = m_observationValues[index-1];
       x[1] = m_observationValues[index  ];
       x[2] = m_observationValues[index+1];

       y[0] = m_quantiles[index-1];
       y[1] = m_quantiles[index  ];
       y[2] = m_quantiles[index+1];
     }

     if ( x[0] == x[1] || x[0] == x[2] || x[1] == x[2]) {
       return m_quantiles[index];  // this should never happen, 
                                   // but just in case return the cumProb of the nearest node
     }

     //do a parabolic interpolation to find the probability at value
     double interp =   (value-x[1]) * (value-x[2]) / (x[0]-x[1]) / (x[0]-x[2]) * y[0]
                     + (value-x[0]) * (value-x[2]) / (x[1]-x[0]) / (x[1]-x[2]) * y[1]
                     + (value-x[0]) * (value-x[1]) / (x[2]-x[0]) / (x[2]-x[1]) * y[2];

     //check for reasonability of the quadratic interpolation
     // TODO: better way to write this???
     int i;
     for (i = 0; i < 3; i++) {
       if ( x[i] <= value && x[i+1] >= value) //find the nodes on both sides of cumProb
         break;
     }

     if (y[i] <= interp && y[i+1] >= interp) //make sure things are strictly increasing
       return interp;
     //if the quadratic wasn't reasonable return the linear interpolation
     else {
       return ((x[i] - value) * y[i+1] + (x[i+1] - value) * y[i]) / (x[i] - x[i+1]);
     }

   }



  /** 
   * Values for the estimated quantile positions are update as observations are
   * added
   *
   * @param [in] double obs -- the individual observation to be used to 
   *                dynamically readjust the cumulative probility distribution
   */
  void StatCumProbDistDynCalc::addObs(double obs) {
    double temp = 0.0;

    if (m_numberObservations < m_numberQuantiles) {
      // in this phase, the algorithm is getting initial values
      m_observationValues.append(obs);
      m_numberObservations++;

      // if there are now m_numberQuantiles observations, then sort them from smallest to greatest
      if (m_numberObservations == m_numberQuantiles) {
        std::sort(m_observationValues.begin(),  m_observationValues.end(),  std::less<double>());
      }
    }
    else { // m_numberObservations >= m_numberQuantiles 

      //incrementing the number of observations
      m_numberObservations++;
      m_numObsBelowQuantile[m_numberQuantiles-1] = m_numberObservations;

      // replace min or max with this observation, if appropriate
      if (obs > m_observationValues[m_numberQuantiles-1]) {
        m_observationValues[m_numberQuantiles-1] = obs;
      }
      if (obs < m_observationValues[0]) {
        m_observationValues[0] = obs;
        temp = 1.0;
      }

      //estimated quantile positions are increased if obs <= estimated quantile value
      for (int i = 1; i < (int)m_numberQuantiles-1; i++) {
        if (obs <= m_observationValues[i]) {
          for (; i < (int)m_numberQuantiles-1; i++) {
            m_numObsBelowQuantile[i]++; // all m_n's above the first >= obs get incremented
          }
        }
      }

      for (int i = 1; i < (int)m_numberQuantiles; i++) {
        m_idealNumObsBelowQuantile[i] += m_quantiles[i];
      }

      //calculate corrections to node positions (m_n) and values (m_observationValues)
      int d;
      // note that the loop does not edit the first or last positions
      for (int i = 1; i < (int)m_numberCells; i++) {
        //calculation of d[i] it is either 1, -1, or 0
        temp = m_idealNumObsBelowQuantile[i] - m_numObsBelowQuantile[i];
        if (fabs(temp)>1 && temp > 0.0) {
          d = 1;
        }
        else if (fabs(temp)>1 && temp < 0.0) {
          d = -1;
        } 
        else {
          continue;
        }

         // if the node can be moved by d without stepping on another node
        if ( ( (d == 1) 
                && (m_numObsBelowQuantile[i+1] - m_numObsBelowQuantile[i] > 1) )
             || ( (d == -1)
                   && (m_numObsBelowQuantile[i-1] - m_numObsBelowQuantile[i] < -1) ) ) {
          //calculate a new quantile value for the node from the parabolic formula
          temp = m_observationValues[i]
                 + double(d) / (m_numObsBelowQuantile[i+1] - m_numObsBelowQuantile[i-1])
                   * ( (m_numObsBelowQuantile[i] - m_numObsBelowQuantile[i-1] + d)
                       * (m_observationValues[i+1] - m_observationValues[i])
                       / (m_numObsBelowQuantile[i+1] - m_numObsBelowQuantile[i])
                       + (m_numObsBelowQuantile[i+1] - m_numObsBelowQuantile[i] - d)
                       * (m_observationValues[i] - m_observationValues[i-1])
                       / (m_numObsBelowQuantile[i] - m_numObsBelowQuantile[i-1]) );

          // it is necessary that the values of the quantiles be strictly increasing,
          // if the parabolic formula perserves this so be it
          if ( m_observationValues[i-1] < temp && temp < m_observationValues[i+1]) {// use qSort???
            m_observationValues[i] = temp;
          }
          else {
             // if the parabolic formula does not maintain
             //  that the values of the quantiles be strictly increasing,
             //  then use linear interpolation
            m_observationValues[i] = m_observationValues[i]
                                  + double(d)
                                  * (m_observationValues[i+d] - m_observationValues[i]) 
                                  / (m_numObsBelowQuantile[i+d] - m_numObsBelowQuantile[i]);
          }

          // the position of the quantile 
          // (in terms of the number of observations <= quantile) is also adjusted
          m_numObsBelowQuantile[i] += d;
        }
      }
    }
  }



  void StatCumProbDistDynCalc::save(QXmlStreamWriter &stream, const Project *project) const {   // TODO: does xml stuff need project???

    stream.writeStartElement("statCumProbDistDynCalc");
    stream.writeTextElement("numberCells", toString(m_numberCells));
    stream.writeTextElement("numberQuantiles", toString(m_numberQuantiles));
    stream.writeTextElement("numberObservations", toString(m_numberObservations));

    stream.writeStartElement("distributionData");
    for (unsigned int i = 0; i < m_numberQuantiles; i++) {
      stream.writeStartElement("quantileInfo");
       // we need to write out high precision for minDistance calculations in value() and cumProb()
      stream.writeAttribute("quantile", toString(m_quantiles[i], 17));
      stream.writeAttribute("dataValue", toString(m_observationValues[i], 17));
      stream.writeAttribute("idealNumObsBelowQuantile", 
                            toString(m_idealNumObsBelowQuantile[i]));
      stream.writeAttribute("actualNumObsBelowQuantile", toString(m_numObsBelowQuantile[i]));
      stream.writeEndElement(); // end observation
    }
    stream.writeEndElement(); // end observationData
    stream.writeEndElement(); // end statCumProbDistDynCalc

  }

  QDataStream &StatCumProbDistDynCalc::write(QDataStream &stream) const {
    stream << (qint32)m_numberCells
           << (qint32)m_numberQuantiles
           << (qint32)m_numberObservations
           << m_quantiles
           << m_observationValues
           << m_idealNumObsBelowQuantile
           << m_numObsBelowQuantile;
    return stream;
  }

  QDataStream &StatCumProbDistDynCalc::read(QDataStream &stream) {
    QString id;
    qint32 numCells, numQuantiles, numObservations;
    stream >> numCells
           >> numQuantiles
           >> numObservations
           >> m_quantiles
           >> m_observationValues
           >> m_idealNumObsBelowQuantile
           >> m_numObsBelowQuantile;

    m_numberCells = (unsigned int)numCells;
    m_numberQuantiles  = (unsigned int)numQuantiles;
    m_numberObservations   = (unsigned int)numObservations;
    return stream;
  }



  QDataStream &operator<<(QDataStream &stream, const StatCumProbDistDynCalc &scpddc) {
    return scpddc.write(stream);
  }



  QDataStream &operator>>(QDataStream &stream, StatCumProbDistDynCalc &scpddc) {
    return scpddc.read(stream);
  }

  void StatCumProbDistDynCalc::validate() {
    // if quantiles have not been set
    if (m_numberQuantiles == 0) {
      std::string msg = "StatCumProbDistDynCalc will return no data until the quantiles have been set. "
                    "Number of cells = [" + toString(m_numberCells) + "].";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    } 

    //if there isn't even as much data as there are quantiles to track
    if (m_numberObservations < m_numberQuantiles) {
      std::string msg = "StatCumProbDistDynCalc will return no data until the number of observations "
                    "added [" + toString(m_numberObservations) + "] matches the number of "
                    "quantiles [" + toString(m_numberQuantiles)
                    + "] (i.e. number of nodes) selected.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    } 

  }
}// end namespace Isis
