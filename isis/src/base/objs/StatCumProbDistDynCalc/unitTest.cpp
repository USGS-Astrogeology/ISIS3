/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <QDataStream>
#include <QDebug>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <QFile>

#include <iostream>

#include "FileName.h"
#include "IException.h"
#include "Preference.h"
#include "StatCumProbDistDynCalc.h"
#include "Statistics.h"

using namespace std;
using namespace Isis;

 /** 
  *  
  *  
  * @author 2012-03-23 Orrin Thomas
  *
  * @internal
  *   @history 2012-03-23 Orrin Thomas - Original Version
  *   @history 2014-09-11 Jeannie Backer - Added test for xml read/write. Added
  *                           tests for untested cases. Improved unitTest by 30% in all
  *                           coverage types. Updated truth data.
  *  
  *   @todo Complete test coverage for value(), cumProb(), and addObs(). 
  *
  */

namespace Isis {
  class StatisticsXmlHandlerTester : public StatCumProbDistDynCalc {
    public:
      StatisticsXmlHandlerTester(QXmlStreamReader *reader, FileName xmlFile) : StatCumProbDistDynCalc() {

        QString xmlPath(QString::fromStdString(xmlFile.expanded()));
        QFile file(xmlPath);

        if (!file.open(QFile::ReadOnly) ) {
          throw IException(IException::Io,
                           "Unable to open xml file, ["+xmlPath.toStdString()+"],  with read access",
                           _FILEINFO_);
        }

        if (reader->readNextStartElement()) {
            if (reader->name() == "statCumProbDistDynCalc") {
              readStatistics(reader);
            }
            else {
              reader->raiseError(QObject::tr("Incorrect file"));
            }
          }
        }

      ~StatisticsXmlHandlerTester() {
      }

  };
}

int main(int argc, char *argv[]) {
  try {
    Preference::Preferences(true);
    
    StatCumProbDistDynCalc cumDyn(51);  // initize a dynamic probability calculator 
                                        // with 51 quantiles, that way each of the cells/bins is 2%

    // 200 random normal values (mean=4 standard deviation=2)
    double observations[] = { 5.821307885,  1.314169423,  9.003330225,  4.181312434,  4.91643787,
                              2.360563649,  6.245750275,  1.880815167,  3.534559184,  4.995164417,
                              0.692914278,  7.102777599,  4.561705756,  3.453648987,  1.701082078,
                              6.31510756,   4.44349049,   1.522637381,  0.958918639,  1.78311095,
                              4.427988661,  5.123686704,  5.304755981,  2.079683682,  3.055492217,
                              6.620995837,  8.383120856,  2.958927903,  4.292042142,  3.172687697, 
                              5.404910034,  2.261583471,  6.172588512,  2.773482838,  3.870093185, 
                              2.38856928,   3.740220476,  2.089854423,  6.663985277,  5.565883944, 
                              6.563553024,  9.035077934,  4.04322982,   4.635967109,  3.588212299,
                              2.88487091,   3.71676732,   2.859261897,  3.192530305,  3.116589694,
                              5.344490429,  5.097632951,  5.51275054,   4.373746013,  2.664844825,
                              1.192026403,  3.607875904,  4.042762929,  3.647033302,  2.205886273,
                             -0.943039073,  4.224503156,  4.067374855,  3.522392312,  5.993997579,
                              6.287513546,  1.023371357,  4.542656284,  1.377440427,  6.07658523,
                             -1.272533598,  6.01838701,   4.930697642,  2.57971946,   3.448433128,
                              2.46362029,   2.658253653,  4.409753201,  3.923448468,  2.827684129,
                              4.330940469,  6.690561755,  3.895606349,  5.002109825,  5.077037182,
                              3.82274236,   6.93767439,   3.326401835,  0.880302006,  1.125647457,
                              4.599172766,  2.838430634,  5.118943076,  4.626537772,  1.817255397,
                              5.275087862,  1.630645806,  5.720804717,  4.017887697,  5.49518227,
                              0.156432461,  6.506421044,  5.850490023,  4.236432795,  3.581513055,
                              4.865543283,  4.400748655,  4.218135849,  4.985829942,  4.095572264,
                              2.112051223,  5.188320883,  5.290265583,  4.394166581,  5.347183987,
                              5.399505086, -0.252332668,  2.656146694,  4.263043114,  1.603825396,
                              3.673755062,  3.597411671,  4.375176127,  4.09788283,   5.23910596,
                              4.034538196,  6.068227835,  3.379151697,  4.034262304,  2.389285344,
                              5.416421819,  2.93532958,   7.096224204,  1.597166148,  5.329723899,
                              1.349271033,  4.577561902,  5.778512119, -0.604393646,  5.552986971,
                              6.512113196,  3.917619516,  2.795287042, -0.100886591,  4.425956553,
                              1.297191224,  1.454075213,  4.266988275, -0.310655102,  2.826089437,
                              4.608732458,  3.503098424,  4.072084397,  6.92690917,   5.337297637,
                              3.883453359,  8.915929901,  3.754704548,  4.135444297,  7.447021634,
                              5.917394561,  1.845274131,  2.29003513,   3.909303273,  4.3513216,
                              0.992687645,  2.888620032,  3.766291506,  6.938301826,  4.5744484,
                              2.910248634,  5.712411045,  5.195008919,  3.549069934,  7.056521357,
                              5.812731218,  3.785816183,  4.075753871,  5.489282039,  5.64802088,
                              4.561389054,  3.926815005,  4.276043667,  7.311635707,  0.504723206,
                              4.018285671,  4.300326703,  3.312776567,  2.345044423,  6.714209589,
                              4.646002055,  9.07652065,   7.565280691,  3.013286185,  3.820627634,
                              5.656803486,  2.489375047,  0.246631158,  2.844825073,  4.246691574 };

    for (int i = 0; i < 20; i++) cumDyn.addObs(observations[i]);

    qDebug() << "Testing failure modes";
    qDebug() << "Querying minimum before the number of observations is greater than or equal to "
                "the number of quantiles: ";
    try {
      cumDyn.min();
    }
    catch(IException &e) {
      e.print();
    }

    qDebug() << "Querying maximum before the number of observations is greater than or equal to "
                "the number of quantiles: ";
    try {
      cumDyn.max();
    }
    catch(IException &e) {
      e.print();
    }
    
    qDebug() << "Querying a value (as a function of cumulative probability) before "
                "the number of observations is greater than or equal to the number of quantiles: ";
    try {
      cumDyn.value(0.5);
    }
    catch(IException &e) {
      e.print();
    }

    qDebug() << "Querying a cumulative probability (as a function of value) before "
                "the number of observations is greater than or equal to the number of quantiles: ";
    try {
      cumDyn.cumProb(0.0);
    }
    catch(IException &e) {
      e.print();
    }
    
    for (int i = 20; i < 200; i++) cumDyn.addObs(observations[i]);  //add in the rest of the data

    qDebug() << "Querying a nonsense cumulative probability (2.0): ";
    try {
      cumDyn.value(2.0);
    }
    catch(IException &e) {
      e.print();
    }

    qDebug() << "Querying a nonsense cumulative probability (-1.0): ";
    try {
      cumDyn.value(-1.0);
    }
    catch(IException &e) {
      e.print();
    }
    // read xml with no attributes or values
    FileName emptyXmlFile("./unitTest_NoElementValues.xml");
    Project *project = NULL;
    QFile xml(QString::fromStdString(emptyXmlFile.expanded()));
    if(!xml.open(QFile::ReadOnly | QFile::Text)){
      throw IException(IException::Unknown,
                        "Failed to parse xml file, ["+xml.fileName().toStdString()+"]",
                        _FILEINFO_);
    }

    QXmlStreamReader reader(&xml);
    StatisticsXmlHandlerTester statsFromEmptyXml(&reader, emptyXmlFile);
    qDebug() << "Testing XML: read XML with no attributes or values "
                "to StatCumProbDistDynCalc object... Then try to get "
                "min from object with no observations.";
    try {
      qDebug() << QString::number(statsFromEmptyXml.min());
    } 
    catch (IException &e) {
     e.print();
    }


    qDebug() << "";
    qDebug() << "Testing successful construction of StatCumProbDistDynCalc object";
    qDebug() << "Min = " << cumDyn.min();
    qDebug() << "Max = " << cumDyn.max();
    qDebug() << "";
    double temp;
    temp = cumDyn.value(0.0);
    qDebug() << "0.0 approximated Quantile: " << temp;
    qDebug() << "0.0 theoretical Quantile: -1.272533598...";
    qDebug() << "percent error: " << (temp+1.272533598)/-1.272533598*100 << "%";
    qDebug() << "";
    temp = cumDyn.value(0.005);
    qDebug() << "0.005 approximated Quantile: " << temp;
    qDebug() << "";
    temp = cumDyn.value(0.25);
    qDebug() << "0.25 approximated Quantile: " << temp;
    qDebug() << "0.25 theoretical Quantile: 2.6510204996078...";
    qDebug() << "percent error: " << (temp-2.65102049960784000)/2.65102049960784000*100 << "%";
    qDebug() << "";
    temp = cumDyn.value(0.5);
    qDebug() << "0.50 approximated Quantile: " << temp;
    qDebug() << "0.50 theoretical Quantile: 4.0";
    qDebug() << "percent error: " << (temp-4.0)/4.0*100 << "%";
    qDebug() << "";
    temp = cumDyn.value(0.75);
    qDebug() << "0.75 approximated Quantile: " << temp;
    qDebug() << "0.75 theoretical Quantile: 5.34897950039216...";
    qDebug() << "percent error: " << (temp-5.34897950039216)/5.34897950039216*100 << "%";
    qDebug() << "";
    temp = cumDyn.value(0.995);
    qDebug() << "0.995 approximated Quantile: " << temp;
    qDebug() << "";
    temp = cumDyn.value(1.0);
    qDebug() << "0.0 approximated Quantile: " << temp;
    qDebug() << "0.0 theoretical Quantile: 9.07652065...";
    qDebug() << "percent error: " << (temp-9.07652065)/9.07652065*100 << "%";
    qDebug() << "";
    temp = cumDyn.cumProb(-2.0);
    qDebug() << "approximated cumprobabilty [-oo, -2.0]: " << temp;
    qDebug() << "theoretical: 0.0...";
    qDebug() << "percent error: " << (temp)/100 << "%";
    qDebug() << "";
    temp = cumDyn.cumProb(-1.2);
    qDebug() << "approximated cumprobabilty [-oo, -1.2]: " << temp;
    qDebug() << "";
    temp = cumDyn.cumProb(0.0);
    qDebug() << "approximated cumprobabilty [-oo, 0]: " << temp;
    qDebug() << "theoretical: 0.022750131948179...";
    qDebug() << "percent error: " << (temp-0.022750131948179)/0.022750131948179*100 << "%";
    qDebug() << "";
    temp = cumDyn.cumProb(2.0);
    qDebug() << "approximated cumprobabilty [-oo, 2.0]: " << temp;
    qDebug() << "theoretical: 0.158655253931457...";
    qDebug() << "percent error: " << (temp-0.158655253931457)/0.158655253931457*100 << "%";
    qDebug() << "";
    temp = cumDyn.cumProb(5.0);
    qDebug() << "approximate cumprobabilty [-oo, 5.0]: " << temp;
    qDebug() << "theoretical: 0.691462461274013...";
    qDebug() << "percent error: " << (temp-0.691462461274013)/0.691462461274013*100 << "%";
    qDebug() << "";
    temp = cumDyn.cumProb(9.0);
    qDebug() << "approximate cumprobabilty [-oo, 9.0]: " << temp;
    qDebug() << "";
    temp = cumDyn.cumProb(9.07652065);
    qDebug() << "approximate cumprobabilty [-oo, 9.07652065]: " << temp;
    qDebug() << "theoretical: 1.0...";
    qDebug() << "percent error: " << (temp-1.0)/100 << "%";
    qDebug() << "";
    qDebug() << "";
    qDebug() << "";

    qDebug() << "reinitialize the class and redo the tests";
    cumDyn.setQuantiles(51);
    for (int i = 0; i < 200; i++) cumDyn.addObs(observations[i]);
    qDebug() << "Min = " << cumDyn.min();
    qDebug() << "Max = " << cumDyn.max();
    qDebug() << "";
    temp = cumDyn.value(0.0);
    qDebug() << "0.0 approximated Quantile: " << temp;
    qDebug() << "0.0 theoretical Quantile: -1.272533598...";
    qDebug() << "percent error: " << (temp+1.272533598)/-1.272533598*100 << "%";
    qDebug() << "";
    temp = cumDyn.value(0.005);
    qDebug() << "0.005 approximated Quantile: " << temp;
    qDebug() << "";
    temp = cumDyn.value(0.25);
    qDebug() << "0.25 approximated Quantile: " << temp;
    qDebug() << "0.25 theoretical Quantile: 2.6510204996078...";
    qDebug() << "percent error: " << (temp-2.65102049960784000)/2.65102049960784000*100 << "%";
    qDebug() << "";
    temp = cumDyn.value(0.5);
    qDebug() << "0.50 approximated Quantile: " << temp;
    qDebug() << "0.50 theoretical Quantile: 4.0";
    qDebug() << "percent error: " << (temp-4.0)/4.0*100 << "%";
    qDebug() << "";
    temp = cumDyn.value(0.75);
    qDebug() << "0.75 approximated Quantile: " << temp;
    qDebug() << "0.75 theoretical Quantile: 5.34897950039216...";
    qDebug() << "percent error: " << (temp-5.34897950039216)/5.34897950039216*100 << "%";
    qDebug() << "";
    temp = cumDyn.value(0.995);
    qDebug() << "0.995 approximated Quantile: " << temp;
    qDebug() << "";
    temp = cumDyn.value(1.0);
    qDebug() << "0.0 approximated Quantile: " << temp;
    qDebug() << "0.0 theoretical Quantile: 9.07652065...";
    qDebug() << "percent error: " << (temp-9.07652065)/9.07652065*100 << "%";
    qDebug() << "";
    temp = cumDyn.cumProb(-2.0);
    qDebug() << "approximated cumprobabilty [-oo, -2.0]: " << temp;
    qDebug() << "theoretical: 0.0...";
    qDebug() << "percent error: " << (temp)/100 << "%";
    qDebug() << "";
    temp = cumDyn.cumProb(-1.2);
    qDebug() << "approximated cumprobabilty [-oo, -1.2]: " << temp;
    qDebug() << "";
    temp = cumDyn.cumProb(0.0);
    qDebug() << "approximated cumprobabilty [-oo, 0]: " << temp;
    qDebug() << "theoretical: 0.022750131948179...";
    qDebug() << "percent error: " << (temp-0.022750131948179)/0.022750131948179*100 << "%";
    qDebug() << "";
    temp = cumDyn.cumProb(2.0);
    qDebug() << "approximated cumprobabilty [-oo, 2.0]: " << temp;
    qDebug() << "theoretical: 0.158655253931457...";
    qDebug() << "percent error: " << (temp-0.158655253931457)/0.158655253931457*100 << "%";
    qDebug() << "";
    temp = cumDyn.cumProb(5.0);
    qDebug() << "approximate cumprobabilty [-oo, 5.0]: " << temp;
    qDebug() << "theoretical: 0.691462461274013...";
    qDebug() << "percent error: " << (temp-0.691462461274013)/0.691462461274013*100 << "%";
    qDebug() << "";
    temp = cumDyn.cumProb(9.0);
    qDebug() << "approximate cumprobabilty [-oo, 9.0]: " << temp;
    qDebug() << "";
    temp = cumDyn.cumProb(9.07652065);
    qDebug() << "approximate cumprobabilty [-oo, 9.07652065]: " << temp;
    qDebug() << "theoretical: 1.0...";
    qDebug() << "percent error: " << (temp-1.0)/100 << "%";
    qDebug() << "";
    qDebug() << "";
    qDebug() << "";

    qDebug() << "Testing copy constructor...";
    StatCumProbDistDynCalc copyStats(cumDyn);
    qDebug() << "Min = " << copyStats.min();
    qDebug() << "Max = " << copyStats.max();
    qDebug() << "";
    temp = copyStats.value(0.0);
    qDebug() << "0.0 approximated Quantile: " << temp;
    qDebug() << "0.0 theoretical Quantile: -1.272533598...";
    qDebug() << "percent error: " << (temp+1.272533598)/-1.272533598*100 << "%";
    qDebug() << "";
    temp = copyStats.value(0.005);
    qDebug() << "0.005 approximated Quantile: " << temp;
    qDebug() << "";
    temp = copyStats.value(0.25);
    qDebug() << "0.25 approximated Quantile: " << temp;
    qDebug() << "0.25 theoretical Quantile: 2.6510204996078...";
    qDebug() << "percent error: " << (temp-2.65102049960784000)/2.65102049960784000*100 << "%";
    qDebug() << "";
    temp = copyStats.value(0.5);
    qDebug() << "0.50 approximated Quantile: " << temp;
    qDebug() << "0.50 theoretical Quantile: 4.0";
    qDebug() << "percent error: " << (temp-4.0)/4.0*100 << "%";
    qDebug() << "";
    temp = copyStats.value(0.75);
    qDebug() << "0.75 approximated Quantile: " << temp;
    qDebug() << "0.75 theoretical Quantile: 5.34897950039216...";
    qDebug() << "percent error: " << (temp-5.34897950039216)/5.34897950039216*100 << "%";
    qDebug() << "";
    temp = copyStats.value(0.995);
    qDebug() << "0.995 approximated Quantile: " << temp;
    qDebug() << "";
    temp = copyStats.value(1.0);
    qDebug() << "0.0 approximated Quantile: " << temp;
    qDebug() << "0.0 theoretical Quantile: 9.07652065...";
    qDebug() << "percent error: " << (temp-9.07652065)/9.07652065*100 << "%";
    qDebug() << "";
    temp = copyStats.cumProb(-2.0);
    qDebug() << "approximated cumprobabilty [-oo, -2.0]: " << temp;
    qDebug() << "theoretical: 0.0...";
    qDebug() << "percent error: " << (temp)/100 << "%";
    qDebug() << "";
    temp = copyStats.cumProb(-1.2);
    qDebug() << "approximated cumprobabilty [-oo, -1.2]: " << temp;
    qDebug() << "";
    temp = copyStats.cumProb(0.0);
    qDebug() << "approximated cumprobabilty [-oo, 0]: " << temp;
    qDebug() << "theoretical: 0.022750131948179...";
    qDebug() << "percent error: " << (temp-0.022750131948179)/0.022750131948179*100 << "%";
    qDebug() << "";
    temp = copyStats.cumProb(2.0);
    qDebug() << "approximated cumprobabilty [-oo, 2.0]: " << temp;
    qDebug() << "theoretical: 0.158655253931457...";
    qDebug() << "percent error: " << (temp-0.158655253931457)/0.158655253931457*100 << "%";
    qDebug() << "";
    temp = copyStats.cumProb(5.0);
    qDebug() << "approximate cumprobabilty [-oo, 5.0]: " << temp;
    qDebug() << "theoretical: 0.691462461274013...";
    qDebug() << "percent error: " << (temp-0.691462461274013)/0.691462461274013*100 << "%";
    qDebug() << "";
    temp = copyStats.cumProb(9.0);
    qDebug() << "approximate cumprobabilty [-oo, 9.0]: " << temp;
    qDebug() << "";
    temp = copyStats.cumProb(9.07652065);
    qDebug() << "approximate cumprobabilty [-oo, 9.07652065]: " << temp;
    qDebug() << "theoretical: 1.0...";
    qDebug() << "percent error: " << (temp-1.0)/100 << "%";
    qDebug() << "";
    qDebug() << "";
    qDebug() << "";

    qDebug() << "Testing assignment operator=...";
    {
      StatCumProbDistDynCalc &c = copyStats;
      copyStats = c;
    }
    qDebug() << "Min = " << copyStats.min();
    qDebug() << "Max = " << copyStats.max();
    qDebug() << "";
    temp = copyStats.value(0.0);
    qDebug() << "0.0 approximated Quantile: " << temp;
    qDebug() << "0.0 theoretical Quantile: -1.272533598...";
    qDebug() << "percent error: " << (temp+1.272533598)/-1.272533598*100 << "%";
    qDebug() << "";
    temp = copyStats.value(0.005);
    qDebug() << "0.005 approximated Quantile: " << temp;
    qDebug() << "";
    temp = copyStats.value(0.25);
    qDebug() << "0.25 approximated Quantile: " << temp;
    qDebug() << "0.25 theoretical Quantile: 2.6510204996078...";
    qDebug() << "percent error: " << (temp-2.65102049960784000)/2.65102049960784000*100 << "%";
    qDebug() << "";
    temp = copyStats.value(0.5);
    qDebug() << "0.50 approximated Quantile: " << temp;
    qDebug() << "0.50 theoretical Quantile: 4.0";
    qDebug() << "percent error: " << (temp-4.0)/4.0*100 << "%";
    qDebug() << "";
    temp = copyStats.value(0.75);
    qDebug() << "0.75 approximated Quantile: " << temp;
    qDebug() << "0.75 theoretical Quantile: 5.34897950039216...";
    qDebug() << "percent error: " << (temp-5.34897950039216)/5.34897950039216*100 << "%";
    qDebug() << "";
    temp = copyStats.value(0.995);
    qDebug() << "0.995 approximated Quantile: " << temp;
    qDebug() << "";
    temp = copyStats.value(1.0);
    qDebug() << "0.0 approximated Quantile: " << temp;
    qDebug() << "0.0 theoretical Quantile: 9.07652065...";
    qDebug() << "percent error: " << (temp-9.07652065)/9.07652065*100 << "%";
    qDebug() << "";
    temp = copyStats.cumProb(-2.0);
    qDebug() << "approximated cumprobabilty [-oo, -2.0]: " << temp;
    qDebug() << "theoretical: 0.0...";
    qDebug() << "percent error: " << (temp)/100 << "%";
    qDebug() << "";
    temp = copyStats.cumProb(-1.2);
    qDebug() << "approximated cumprobabilty [-oo, -1.2]: " << temp;
    qDebug() << "";
    temp = copyStats.cumProb(0.0);
    qDebug() << "approximated cumprobabilty [-oo, 0]: " << temp;
    qDebug() << "theoretical: 0.022750131948179...";
    qDebug() << "percent error: " << (temp-0.022750131948179)/0.022750131948179*100 << "%";
    qDebug() << "";
    temp = copyStats.cumProb(2.0);
    qDebug() << "approximated cumprobabilty [-oo, 2.0]: " << temp;
    qDebug() << "theoretical: 0.158655253931457...";
    qDebug() << "percent error: " << (temp-0.158655253931457)/0.158655253931457*100 << "%";
    qDebug() << "";
    temp = copyStats.cumProb(5.0);
    qDebug() << "approximate cumprobabilty [-oo, 5.0]: " << temp;
    qDebug() << "theoretical: 0.691462461274013...";
    qDebug() << "percent error: " << (temp-0.691462461274013)/0.691462461274013*100 << "%";
    qDebug() << "";
    temp = copyStats.cumProb(9.0);
    qDebug() << "approximate cumprobabilty [-oo, 9.0]: " << temp;
    qDebug() << "";
    temp = copyStats.cumProb(9.07652065);
    qDebug() << "approximate cumprobabilty [-oo, 9.07652065]: " << temp;
    qDebug() << "theoretical: 1.0...";
    qDebug() << "percent error: " << (temp-1.0)/100 << "%";
    qDebug() << "";
    qDebug() << "";
    StatCumProbDistDynCalc assignedStats;
    assignedStats = copyStats;
    qDebug() << "Min = " << assignedStats.min();
    qDebug() << "Max = " << assignedStats.max();
    qDebug() << "";
    temp = assignedStats.value(0.0);
    qDebug() << "0.0 approximated Quantile: " << temp;
    qDebug() << "0.0 theoretical Quantile: -1.272533598...";
    qDebug() << "percent error: " << (temp+1.272533598)/-1.272533598*100 << "%";
    qDebug() << "";
    temp = assignedStats.value(0.005);
    qDebug() << "0.005 approximated Quantile: " << temp;
    qDebug() << "";
    temp = assignedStats.value(0.25);
    qDebug() << "0.25 approximated Quantile: " << temp;
    qDebug() << "0.25 theoretical Quantile: 2.6510204996078...";
    qDebug() << "percent error: " << (temp-2.65102049960784000)/2.65102049960784000*100 << "%";
    qDebug() << "";
    temp = assignedStats.value(0.5);
    qDebug() << "0.50 approximated Quantile: " << temp;
    qDebug() << "0.50 theoretical Quantile: 4.0";
    qDebug() << "percent error: " << (temp-4.0)/4.0*100 << "%";
    qDebug() << "";
    temp = assignedStats.value(0.75);
    qDebug() << "0.75 approximated Quantile: " << temp;
    qDebug() << "0.75 theoretical Quantile: 5.34897950039216...";
    qDebug() << "percent error: " << (temp-5.34897950039216)/5.34897950039216*100 << "%";
    qDebug() << "";
    temp = assignedStats.value(0.995);
    qDebug() << "0.995 approximated Quantile: " << temp;
    qDebug() << "";
    temp = assignedStats.value(1.0);
    qDebug() << "0.0 approximated Quantile: " << temp;
    qDebug() << "0.0 theoretical Quantile: 9.07652065...";
    qDebug() << "percent error: " << (temp-9.07652065)/9.07652065*100 << "%";
    qDebug() << "";
    temp = assignedStats.cumProb(-2.0);
    qDebug() << "approximated cumprobabilty [-oo, -2.0]: " << temp;
    qDebug() << "theoretical: 0.0...";
    qDebug() << "percent error: " << (temp)/100 << "%";
    qDebug() << "";
    temp = assignedStats.cumProb(-1.2);
    qDebug() << "approximated cumprobabilty [-oo, -1.2]: " << temp;
    qDebug() << "";
    temp = assignedStats.cumProb(0.0);
    qDebug() << "approximated cumprobabilty [-oo, 0]: " << temp;
    qDebug() << "theoretical: 0.022750131948179...";
    qDebug() << "percent error: " << (temp-0.022750131948179)/0.022750131948179*100 << "%";
    qDebug() << "";
    temp = assignedStats.cumProb(2.0);
    qDebug() << "approximated cumprobabilty [-oo, 2.0]: " << temp;
    qDebug() << "theoretical: 0.158655253931457...";
    qDebug() << "percent error: " << (temp-0.158655253931457)/0.158655253931457*100 << "%";
    qDebug() << "";
    temp = assignedStats.cumProb(5.0);
    qDebug() << "approximate cumprobabilty [-oo, 5.0]: " << temp;
    qDebug() << "theoretical: 0.691462461274013...";
    qDebug() << "percent error: " << (temp-0.691462461274013)/0.691462461274013*100 << "%";
    qDebug() << "";
    temp = assignedStats.cumProb(9.0);
    qDebug() << "approximate cumprobabilty [-oo, 9.0]: " << temp;
    qDebug() << "";
    temp = assignedStats.cumProb(9.07652065);
    qDebug() << "approximate cumprobabilty [-oo, 9.07652065]: " << temp;
    qDebug() << "theoretical: 1.0...";
    qDebug() << "percent error: " << (temp-1.0)/100 << "%";
    qDebug() << "";
    qDebug() << "";
    qDebug() << "";

    qDebug() << "Testing serialization...";
    QByteArray byteArray;
    QDataStream outputData(&byteArray, QIODevice::WriteOnly);
    outputData << cumDyn;
    QDataStream inputData(byteArray);
    StatCumProbDistDynCalc newCumDyn;
    inputData >> newCumDyn;
    qDebug() << "Min = " << newCumDyn.min();
    qDebug() << "Max = " << newCumDyn.max();
    qDebug() << "";
    temp = newCumDyn.value(0.0);
    qDebug() << "0.0 approximated Quantile: " << temp;
    qDebug() << "0.0 theoretical Quantile: -1.272533598...";
    qDebug() << "percent error: " << (temp+1.272533598)/-1.272533598*100 << "%";
    qDebug() << "";
    temp = newCumDyn.value(0.005);
    qDebug() << "0.005 approximated Quantile: " << temp;
    qDebug() << "";
    temp = newCumDyn.value(0.25);
    qDebug() << "0.25 approximated Quantile: " << temp;
    qDebug() << "0.25 theoretical Quantile: 2.6510204996078...";
    qDebug() << "percent error: " << (temp-2.65102049960784000)/2.65102049960784000*100 << "%";
    qDebug() << "";
    temp = newCumDyn.value(0.5);
    qDebug() << "0.50 approximated Quantile: " << temp;
    qDebug() << "0.50 theoretical Quantile: 4.0";
    qDebug() << "percent error: " << (temp-4.0)/4.0*100 << "%";
    qDebug() << "";
    temp = newCumDyn.value(0.75);
    qDebug() << "0.75 approximated Quantile: " << temp;
    qDebug() << "0.75 theoretical Quantile: 5.34897950039216...";
    qDebug() << "percent error: " << (temp-5.34897950039216)/5.34897950039216*100 << "%";
    qDebug() << "";
    temp = newCumDyn.value(0.995);
    qDebug() << "0.995 approximated Quantile: " << temp;
    qDebug() << "";
    temp = newCumDyn.value(1.0);
    qDebug() << "0.0 approximated Quantile: " << temp;
    qDebug() << "0.0 theoretical Quantile: 9.07652065...";
    qDebug() << "percent error: " << (temp-9.07652065)/9.07652065*100 << "%";
    qDebug() << "";
    temp = newCumDyn.cumProb(-2.0);
    qDebug() << "approximated cumprobabilty [-oo, -2.0]: " << temp;
    qDebug() << "theoretical: 0.0...";
    qDebug() << "percent error: " << (temp)/100 << "%";
    qDebug() << "";
    temp = newCumDyn.cumProb(-1.2);
    qDebug() << "approximated cumprobabilty [-oo, -1.2]: " << temp;
    qDebug() << "";
    temp = newCumDyn.cumProb(0.0);
    qDebug() << "approximated cumprobabilty [-oo, 0]: " << temp;
    qDebug() << "theoretical: 0.022750131948179...";
    qDebug() << "percent error: " << (temp-0.022750131948179)/0.022750131948179*100 << "%";
    qDebug() << "";
    temp = newCumDyn.cumProb(2.0);
    qDebug() << "approximated cumprobabilty [-oo, 2.0]: " << temp;
    qDebug() << "theoretical: 0.158655253931457...";
    qDebug() << "percent error: " << (temp-0.158655253931457)/0.158655253931457*100 << "%";
    qDebug() << "";
    temp = newCumDyn.cumProb(5.0);
    qDebug() << "approximate cumprobabilty [-oo, 5.0]: " << temp;
    qDebug() << "theoretical: 0.691462461274013...";
    qDebug() << "percent error: " << (temp-0.691462461274013)/0.691462461274013*100 << "%";
    qDebug() << "";
    temp = newCumDyn.cumProb(9.0);
    qDebug() << "approximate cumprobabilty [-oo, 9.0]: " << temp;
    qDebug() << "";
    temp = newCumDyn.cumProb(9.07652065);
    qDebug() << "approximate cumprobabilty [-oo, 9.07652065]: " << temp;
    qDebug() << "theoretical: 1.0...";
    qDebug() << "percent error: " << (temp-1.0)/100 << "%";
    qDebug() << "";
    qDebug() << "";
    qDebug() << "";

    qDebug() << "Testing XML: write XML from StatCumProbDistDynCalc object...";
    // write xml 
    FileName xmlFile("./StatCumProbDistDynCalc.xml");
    QString xmlPath = QString::fromStdString(xmlFile.expanded());
    QFile qXmlFile(xmlPath);
    if (!qXmlFile.open(QIODevice::WriteOnly|QIODevice::Text)) {
      throw IException(IException::Io,
                       "Unable to open xml file, ["+xmlPath.toStdString()+"],  with write access",
                       _FILEINFO_);
    }
    QXmlStreamWriter writer(&qXmlFile);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    cumDyn.save(writer, project);
    writer.writeEndDocument();
    qXmlFile.close();
    // read xml    
    qDebug() << "Testing XML: read XML to StatCumProbDistDynCalc object...";
    QFile xml2(QString::fromStdString(xmlFile.expanded()));
    if(!xml2.open(QFile::ReadOnly | QFile::Text)){
      throw IException(IException::Unknown,
                        "Failed to parse xml file, ["+xml2.fileName().toStdString()+"]",
                        _FILEINFO_);
    }

    QXmlStreamReader reader2(&xml2);
    StatisticsXmlHandlerTester statsFromXml(&reader2, xmlFile);
    qDebug() << "Min = " << statsFromXml.min();
    qDebug() << "Max = " << statsFromXml.max();
    qDebug() << "";
    temp = statsFromXml.value(0.0);
    qDebug() << "0.0 approximated Quantile: " << temp;
    qDebug() << "0.0 theoretical Quantile: -1.272533598...";
    qDebug() << "percent error: " << (temp+1.272533598)/-1.272533598*100 << "%";
    qDebug() << "";
    temp = statsFromXml.value(0.005);
    qDebug() << "0.005 approximated Quantile: " << temp;
    qDebug() << "";
    temp = statsFromXml.value(0.25);
    qDebug() << "0.25 approximated Quantile: " << temp;
    qDebug() << "0.25 theoretical Quantile: 2.6510204996078...";
    qDebug() << "percent error: " << (temp-2.65102049960784000)/2.65102049960784000*100 << "%";
    qDebug() << "";
    temp = statsFromXml.value(0.5);
    qDebug() << "0.50 approximated Quantile: " << temp;
    qDebug() << "0.50 theoretical Quantile: 4.0";
    qDebug() << "percent error: " << (temp-4.0)/4.0*100 << "%";
    qDebug() << "";
    temp = statsFromXml.value(0.75);
    qDebug() << "0.75 approximated Quantile: " << temp;
    qDebug() << "0.75 theoretical Quantile: 5.34897950039216...";
    qDebug() << "percent error: " << (temp-5.34897950039216)/5.34897950039216*100 << "%";
    qDebug() << "";
    temp = statsFromXml.value(0.995);
    qDebug() << "0.995 approximated Quantile: " << temp;
    qDebug() << "";
    temp = statsFromXml.value(1.0);
    qDebug() << "0.0 approximated Quantile: " << temp;
    qDebug() << "0.0 theoretical Quantile: 9.07652065...";
    qDebug() << "percent error: " << (temp-9.07652065)/9.07652065*100 << "%";
    qDebug() << "";
    temp = statsFromXml.cumProb(-2.0);
    qDebug() << "approximated cumprobabilty [-oo, -2.0]: " << temp;
    qDebug() << "theoretical: 0.0...";
    qDebug() << "percent error: " << (temp)/100 << "%";
    qDebug() << "";
    temp = statsFromXml.cumProb(-1.2);
    qDebug() << "approximated cumprobabilty [-oo, -1.2]: " << temp;
    qDebug() << "";
    temp = statsFromXml.cumProb(0.0);
    qDebug() << "approximated cumprobabilty [-oo, 0]: " << temp;
    qDebug() << "theoretical: 0.022750131948179...";
    qDebug() << "percent error: " << (temp-0.022750131948179)/0.022750131948179*100 << "%";
    qDebug() << "";
    temp = statsFromXml.cumProb(2.0);
    qDebug() << "approximated cumprobabilty [-oo, 2.0]: " << temp;
    qDebug() << "theoretical: 0.158655253931457...";
    qDebug() << "percent error: " << (temp-0.158655253931457)/0.158655253931457*100 << "%";
    qDebug() << "";
    temp = statsFromXml.cumProb(5.0);
    qDebug() << "approximate cumprobabilty [-oo, 5.0]: " << temp;
    qDebug() << "theoretical: 0.691462461274013...";
    qDebug() << "percent error: " << (temp-0.691462461274013)/0.691462461274013*100 << "%";
    qDebug() << "";
    temp = statsFromXml.cumProb(9.0);
    qDebug() << "approximate cumprobabilty [-oo, 9.0]: " << temp;
    qDebug() << "";
    temp = statsFromXml.cumProb(9.07652065);
    qDebug() << "approximate cumprobabilty [-oo, 9.07652065]: " << temp;
    qDebug() << "theoretical: 1.0...";
    qDebug() << "percent error: " << (temp-1.0)/100 << "%";
    qDebug() << "";

    bool deleted = qXmlFile.remove();
    if (!deleted) {
      std::string msg = "Unit Test failed. XML file [" + xmlPath.toStdString() + "not deleted.";
      throw IException(IException::Io, msg, _FILEINFO_);
    }
  }
  catch (IException &e) {
    e.print();
  }
}

