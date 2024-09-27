/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <QDebug>
#include <QDataStream>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <QFile>

#include <iostream>

#include "FileName.h"
#include "IException.h"
#include "Preference.h"
#include "Statistics.h"

using namespace std;
using namespace Isis;

/**
 * @author 2002-05-06 Jeff Anderson
 *
 * @internal
 *   @history 2014-09-05 Jeannie Backer - Added xml read/write tests. Improved coverage of
 *                           existing code.
 *   @history 2016-07-15 Ian Humphrey - Added toPvl() and new constructor that takes a PvlGroup
 *                           to test pvl serialization methods. References #2282.
 *
 */
namespace Isis {
  class StatisticsXmlHandlerTester : public Statistics {
    public:
      StatisticsXmlHandlerTester(QXmlStreamReader *reader, FileName xmlFile) : Statistics() {

        QString xmlPath(QString::fromStdString(xmlFile.expanded()));
        QFile file(xmlPath);

        if (!file.open(QFile::ReadOnly) ) {
          throw IException(IException::Io,
                           "Unable to open xml file, ["+xmlPath.toStdString()+"],  with read access",
                           _FILEINFO_);
        }

        if (reader->readNextStartElement()) {
            if (reader->name() == "statistics") {
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
    qDebug() << "Unit test for Statistics...";

    Statistics s;
    s.AddData(5.0);
    s.AddData(5.0);
    s.Average();
    qDebug() << "For data set:    5, 5";
    qDebug() << "Average:        " << s.Average();
    qDebug() << "ZScore(5.0):    " << s.ZScore(5.0);
    qDebug() << "";

    qDebug() << "Testing Reset...";
    s.Reset();
    qDebug() << "Average:             " << s.Average();
    qDebug() << "Variance:            " << s.Variance();
    qDebug() << "Rms:                 " << s.Rms();
    qDebug() << "Std Deviation:       " << s.StandardDeviation();
    qDebug() << "Minimum:             " << s.Minimum();
    qDebug() << "Maximum:             " << s.Maximum();
    qDebug() << "ChebyShev Min:       " << s.ChebyshevMinimum();
    qDebug() << "ChebyShev Max:       " << s.ChebyshevMaximum();
    qDebug() << "Best Minimum:        " << s.BestMinimum();
    qDebug() << "Best Maximum:        " << s.BestMaximum();
    qDebug() << "Valid Minimum:       " << QString::fromStdString(Isis::toString(s.ValidMinimum()));
    qDebug() << "Valid Maximum:       " << QString::fromStdString(Isis::toString(s.ValidMaximum()));
    qDebug() << "Total Pixels:        " << s.TotalPixels();
    qDebug() << "Valid Pixels:        " << s.ValidPixels();
    qDebug() << "Null Pixels:         " << s.NullPixels();
    qDebug() << "Lis Pixels:          " << s.LisPixels();
    qDebug() << "Lrs Pixels:          " << s.LrsPixels();
    qDebug() << "His Pixels:          " << s.HisPixels();
    qDebug() << "Hrs Pixels:          " << s.HrsPixels();
    qDebug() << "Out of Range Pixels: " << s.OutOfRangePixels();
    qDebug() << "Over Range Pixels:   " << s.OverRangePixels();
    qDebug() << "Under Range Pixels:  " << s.UnderRangePixels();
    qDebug() << "Sum:                 " << s.Sum();
    qDebug() << "SumSquare:           " << s.SumSquare();
    qDebug() << "Removed Data?        " << s.RemovedData();
    qDebug() << "Z-Score at 1.0       " << s.ZScore(1.0);
    qDebug() << "";

    double a[10];
    a[0] = 1.0;
    a[1] = 2.0;
    a[2] = 3.0;
    a[3] = NULL8;
    a[4] = HIGH_REPR_SAT8;
    a[5] = LOW_REPR_SAT8;
    a[6] = HIGH_INSTR_SAT8;
    a[7] = LOW_INSTR_SAT8;
    a[8] = 10.0;
    a[9] = -1.0;

    qDebug() << "SetValidRange(1, 6) and AddData( 1, 2, 3, Null, HRS, LRS, HIS, LIS, 10, -1)";
    s.SetValidRange(1.0, 6.0);
    s.AddData(a, 10);
    qDebug() << "InRange(0.0)?        " << s.InRange(0.0);
    qDebug() << "InRange(2.0)?        " << s.InRange(2.0);
    qDebug() << "InRange(7.0)?        " << s.InRange(7.0);
    qDebug() << "Average:             " << s.Average();
    qDebug() << "Variance:            " << s.Variance();
    qDebug() << "Rms:                 " << s.Rms();
    qDebug() << "Std Deviation:       " << s.StandardDeviation();
    qDebug() << "Minimum:             " << s.Minimum();
    qDebug() << "Maximum:             " << s.Maximum();
    qDebug() << "ChebyShev Min:       " << s.ChebyshevMinimum();
    qDebug() << "ChebyShev Max:       " << s.ChebyshevMaximum();
    qDebug() << "Best Minimum:        " << s.BestMinimum();
    qDebug() << "Best Maximum:        " << s.BestMaximum();
    qDebug() << "Valid Minimum:       " << s.ValidMinimum();
    qDebug() << "Valid Maximum:       " << s.ValidMaximum();
    qDebug() << "Total Pixels:        " << s.TotalPixels();
    qDebug() << "Valid Pixels:        " << s.ValidPixels();
    qDebug() << "Null Pixels:         " << s.NullPixels();
    qDebug() << "Lis Pixels:          " << s.LisPixels();
    qDebug() << "Lrs Pixels:          " << s.LrsPixels();
    qDebug() << "His Pixels:          " << s.HisPixels();
    qDebug() << "Hrs Pixels:          " << s.HrsPixels();
    qDebug() << "Out of Range Pixels: " << s.OutOfRangePixels();
    qDebug() << "Over Range Pixels:   " << s.OverRangePixels();
    qDebug() << "Under Range Pixels:  " << s.UnderRangePixels();
    qDebug() << "Sum:                 " << s.Sum();
    qDebug() << "SumSquare:           " << s.SumSquare();
    qDebug() << "Removed Data?        " << s.RemovedData();
    qDebug() << "Z-Score at 1.0       " << s.ZScore(1.0);
    qDebug() << "";

    double b[4];
    b[0] = 4.0;
    b[1] = 5.0;
    b[2] = 6.0;
    b[3] = NULL8;

    qDebug() << "AddData( 4, 5, 6, Null)";
    s.AddData(b, 4);
    qDebug() << "Average:             " << s.Average();
    qDebug() << "Variance:            " << s.Variance();
    qDebug() << "Rms:                 " << s.Rms();
    qDebug() << "Std Deviation:       " << s.StandardDeviation();
    qDebug() << "Minimum:             " << s.Minimum();
    qDebug() << "Maximum:             " << s.Maximum();
    qDebug() << "ChebyShev Min:       " << s.ChebyshevMinimum();
    qDebug() << "ChebyShev Max:       " << s.ChebyshevMaximum();
    qDebug() << "Best Minimum:        " << s.BestMinimum();
    qDebug() << "Best Maximum:        " << s.BestMaximum();
    qDebug() << "Valid Minimum:       " << s.ValidMinimum();
    qDebug() << "Valid Maximum:       " << s.ValidMaximum();
    qDebug() << "Total Pixels:        " << s.TotalPixels();
    qDebug() << "Valid Pixels:        " << s.ValidPixels();
    qDebug() << "Null Pixels:         " << s.NullPixels();
    qDebug() << "Lis Pixels:          " << s.LisPixels();
    qDebug() << "Lrs Pixels:          " << s.LrsPixels();
    qDebug() << "His Pixels:          " << s.HisPixels();
    qDebug() << "Hrs Pixels:          " << s.HrsPixels();
    qDebug() << "Out of Range Pixels: " << s.OutOfRangePixels();
    qDebug() << "Over Range Pixels:   " << s.OverRangePixels();
    qDebug() << "Under Range Pixels:  " << s.UnderRangePixels();
    qDebug() << "Sum:                 " << s.Sum();
    qDebug() << "SumSquare:           " << s.SumSquare();
    qDebug() << "Removed Data?        " << s.RemovedData();
    qDebug() << "Z-Score at 1.0       " << s.ZScore(1.0);
    qDebug() << "";

    qDebug() << "Testing copy constructor...";
    Statistics copyStats(s);
    qDebug() << "Average:             " << copyStats.Average();
    qDebug() << "Variance:            " << copyStats.Variance();
    qDebug() << "Rms:                 " << copyStats.Rms();
    qDebug() << "Std Deviation:       " << copyStats.StandardDeviation();
    qDebug() << "Minimum:             " << copyStats.Minimum();
    qDebug() << "Maximum:             " << copyStats.Maximum();
    qDebug() << "ChebyShev Min:       " << copyStats.ChebyshevMinimum();
    qDebug() << "ChebyShev Max:       " << copyStats.ChebyshevMaximum();
    qDebug() << "Best Minimum:        " << copyStats.BestMinimum();
    qDebug() << "Best Maximum:        " << copyStats.BestMaximum();
    qDebug() << "Valid Minimum:       " << copyStats.ValidMinimum();
    qDebug() << "Valid Maximum:       " << copyStats.ValidMaximum();
    qDebug() << "Total Pixels:        " << copyStats.TotalPixels();
    qDebug() << "Valid Pixels:        " << copyStats.ValidPixels();
    qDebug() << "Null Pixels:         " << copyStats.NullPixels();
    qDebug() << "Lis Pixels:          " << copyStats.LisPixels();
    qDebug() << "Lrs Pixels:          " << copyStats.LrsPixels();
    qDebug() << "His Pixels:          " << copyStats.HisPixels();
    qDebug() << "Hrs Pixels:          " << copyStats.HrsPixels();
    qDebug() << "Out of Range Pixels: " << copyStats.OutOfRangePixels();
    qDebug() << "Over Range Pixels:   " << copyStats.OverRangePixels();
    qDebug() << "Under Range Pixels:  " << copyStats.UnderRangePixels();
    qDebug() << "Sum:                 " << copyStats.Sum();
    qDebug() << "SumSquare:           " << copyStats.SumSquare();
    qDebug() << "Removed Data?        " << copyStats.RemovedData();
    qDebug() << "Z-Score at 1.0       " << copyStats.ZScore(1.0);
    qDebug() << "";

    qDebug() << "Testing assignment operator=...";
    {
      Statistics &tstats = s;
      s = tstats;
    }
    qDebug() << "Average:             " << s.Average();
    qDebug() << "Variance:            " << s.Variance();
    qDebug() << "Rms:                 " << s.Rms();
    qDebug() << "Std Deviation:       " << s.StandardDeviation();
    qDebug() << "Minimum:             " << s.Minimum();
    qDebug() << "Maximum:             " << s.Maximum();
    qDebug() << "ChebyShev Min:       " << s.ChebyshevMinimum();
    qDebug() << "ChebyShev Max:       " << s.ChebyshevMaximum();
    qDebug() << "Best Minimum:        " << s.BestMinimum();
    qDebug() << "Best Maximum:        " << s.BestMaximum();
    qDebug() << "Valid Minimum:       " << s.ValidMinimum();
    qDebug() << "Valid Maximum:       " << s.ValidMaximum();
    qDebug() << "Total Pixels:        " << s.TotalPixels();
    qDebug() << "Valid Pixels:        " << s.ValidPixels();
    qDebug() << "Null Pixels:         " << s.NullPixels();
    qDebug() << "Lis Pixels:          " << s.LisPixels();
    qDebug() << "Lrs Pixels:          " << s.LrsPixels();
    qDebug() << "His Pixels:          " << s.HisPixels();
    qDebug() << "Hrs Pixels:          " << s.HrsPixels();
    qDebug() << "Out of Range Pixels: " << s.OutOfRangePixels();
    qDebug() << "Over Range Pixels:   " << s.OverRangePixels();
    qDebug() << "Under Range Pixels:  " << s.UnderRangePixels();
    qDebug() << "Sum:                 " << s.Sum();
    qDebug() << "SumSquare:           " << s.SumSquare();
    qDebug() << "Removed Data?        " << s.RemovedData();
    qDebug() << "Z-Score at 1.0       " << s.ZScore(1.0);
    qDebug() << "";
    Statistics assignedStats;
    assignedStats = s;
    qDebug() << "Average:             " << assignedStats.Average();
    qDebug() << "Variance:            " << assignedStats.Variance();
    qDebug() << "Rms:                 " << assignedStats.Rms();
    qDebug() << "Std Deviation:       " << assignedStats.StandardDeviation();
    qDebug() << "Minimum:             " << assignedStats.Minimum();
    qDebug() << "Maximum:             " << assignedStats.Maximum();
    qDebug() << "ChebyShev Min:       " << assignedStats.ChebyshevMinimum();
    qDebug() << "ChebyShev Max:       " << assignedStats.ChebyshevMaximum();
    qDebug() << "Best Minimum:        " << assignedStats.BestMinimum();
    qDebug() << "Best Maximum:        " << assignedStats.BestMaximum();
    qDebug() << "Valid Minimum:       " << assignedStats.ValidMinimum();
    qDebug() << "Valid Maximum:       " << assignedStats.ValidMaximum();
    qDebug() << "Total Pixels:        " << assignedStats.TotalPixels();
    qDebug() << "Valid Pixels:        " << assignedStats.ValidPixels();
    qDebug() << "Null Pixels:         " << assignedStats.NullPixels();
    qDebug() << "Lis Pixels:          " << assignedStats.LisPixels();
    qDebug() << "Lrs Pixels:          " << assignedStats.LrsPixels();
    qDebug() << "His Pixels:          " << assignedStats.HisPixels();
    qDebug() << "Hrs Pixels:          " << assignedStats.HrsPixels();
    qDebug() << "Out of Range Pixels: " << assignedStats.OutOfRangePixels();
    qDebug() << "Over Range Pixels:   " << assignedStats.OverRangePixels();
    qDebug() << "Under Range Pixels:  " << assignedStats.UnderRangePixels();
    qDebug() << "Sum:                 " << assignedStats.Sum();
    qDebug() << "SumSquare:           " << assignedStats.SumSquare();
    qDebug() << "Removed Data?        " << assignedStats.RemovedData();
    qDebug() << "Z-Score at 1.0       " << assignedStats.ZScore(1.0);
    qDebug() << "";

    qDebug() << "Testing Pvl serialization methods";
    PvlGroup toStats = s.toPvl();
    Statistics fromStats(toStats);
    qDebug() << "Average:             " << fromStats.Average();
    qDebug() << "Variance:            " << fromStats.Variance();
    qDebug() << "Rms:                 " << fromStats.Rms();
    qDebug() << "Std Deviation:       " << fromStats.StandardDeviation();
    qDebug() << "Minimum:             " << fromStats.Minimum();
    qDebug() << "Maximum:             " << fromStats.Maximum();
    qDebug() << "ChebyShev Min:       " << fromStats.ChebyshevMinimum();
    qDebug() << "ChebyShev Max:       " << fromStats.ChebyshevMaximum();
    qDebug() << "Best Minimum:        " << fromStats.BestMinimum();
    qDebug() << "Best Maximum:        " << fromStats.BestMaximum();
    qDebug() << "Valid Minimum:       " << fromStats.ValidMinimum();
    qDebug() << "Valid Maximum:       " << fromStats.ValidMaximum();
    qDebug() << "Total Pixels:        " << fromStats.TotalPixels();
    qDebug() << "Valid Pixels:        " << fromStats.ValidPixels();
    qDebug() << "Null Pixels:         " << fromStats.NullPixels();
    qDebug() << "Lis Pixels:          " << fromStats.LisPixels();
    qDebug() << "Lrs Pixels:          " << fromStats.LrsPixels();
    qDebug() << "His Pixels:          " << fromStats.HisPixels();
    qDebug() << "Hrs Pixels:          " << fromStats.HrsPixels();
    qDebug() << "Out of Range Pixels: " << fromStats.OutOfRangePixels();
    qDebug() << "Over Range Pixels:   " << fromStats.OverRangePixels();
    qDebug() << "Under Range Pixels:  " << fromStats.UnderRangePixels();
    qDebug() << "Sum:                 " << fromStats.Sum();
    qDebug() << "SumSquare:           " << fromStats.SumSquare();
    qDebug() << "Removed Data?        " << fromStats.RemovedData();
    qDebug() << "Z-Score at 1.0       " << fromStats.ZScore(1.0);
    qDebug() << "";

    qDebug() << "Testing RemoveData(3, Null, HRS, LRS, HIS, LIS, 10, -1)";
    s.RemoveData(&a[2], 8);
    qDebug() << "Average:             " << s.Average();
    qDebug() << "Variance:            " << s.Variance();
    qDebug() << "Rms:                 " << s.Rms();
    qDebug() << "Std Deviation:       " << s.StandardDeviation();
    qDebug() << "ChebyShev Min:       " << s.ChebyshevMinimum();
    qDebug() << "ChebyShev Max:       " << s.ChebyshevMaximum();
    qDebug() << "Total Pixels:        " << s.TotalPixels();
    qDebug() << "Valid Pixels:        " << s.ValidPixels();
    qDebug() << "Null Pixels:         " << s.NullPixels();
    qDebug() << "Lis Pixels:          " << s.LisPixels();
    qDebug() << "Lrs Pixels:          " << s.LrsPixels();
    qDebug() << "His Pixels:          " << s.HisPixels();
    qDebug() << "Hrs Pixels:          " << s.HrsPixels();
    qDebug() << "Out of Range Pixels: " << s.OutOfRangePixels();
    qDebug() << "Over Range Pixels:   " << s.OverRangePixels();
    qDebug() << "Under Range Pixels:  " << s.UnderRangePixels();
    qDebug() << "Sum:                 " << s.Sum();
    qDebug() << "SumSquare:           " << s.SumSquare();
    qDebug() << "Removed Data?        " << s.RemovedData();
    qDebug() << "Z-Score at 1.0       " << s.ZScore(1.0);
    qDebug() << "";

    qDebug() << "Testing serialization from copy constructor object...";
    QByteArray byteArray;
    QDataStream outputData(&byteArray, QIODevice::WriteOnly);
    outputData << copyStats;
    QDataStream inputData(byteArray);
    Statistics serializedStats;
    inputData >> serializedStats;
    qDebug() << "Average:             " << serializedStats.Average();
    qDebug() << "Variance:            " << serializedStats.Variance();
    qDebug() << "Rms:                 " << serializedStats.Rms();
    qDebug() << "Std Deviation:       " << serializedStats.StandardDeviation();
    qDebug() << "Minimum:             " << serializedStats.Minimum();
    qDebug() << "Maximum:             " << serializedStats.Maximum();
    qDebug() << "ChebyShev Min:       " << serializedStats.ChebyshevMinimum();
    qDebug() << "ChebyShev Max:       " << serializedStats.ChebyshevMaximum();
    qDebug() << "Best Minimum:        " << serializedStats.BestMinimum();
    qDebug() << "Best Maximum:        " << serializedStats.BestMaximum();
    qDebug() << "Valid Minimum:       " << serializedStats.ValidMinimum();
    qDebug() << "Valid Maximum:       " << serializedStats.ValidMaximum();
    qDebug() << "Total Pixels:        " << serializedStats.TotalPixels();
    qDebug() << "Valid Pixels:        " << serializedStats.ValidPixels();
    qDebug() << "Null Pixels:         " << serializedStats.NullPixels();
    qDebug() << "Lis Pixels:          " << serializedStats.LisPixels();
    qDebug() << "Lrs Pixels:          " << serializedStats.LrsPixels();
    qDebug() << "His Pixels:          " << serializedStats.HisPixels();
    qDebug() << "Hrs Pixels:          " << serializedStats.HrsPixels();
    qDebug() << "Out of Range Pixels: " << serializedStats.OutOfRangePixels();
    qDebug() << "Over Range Pixels:   " << serializedStats.OverRangePixels();
    qDebug() << "Under Range Pixels:  " << serializedStats.UnderRangePixels();
    qDebug() << "Sum:                 " << serializedStats.Sum();
    qDebug() << "SumSquare:           " << serializedStats.SumSquare();
    qDebug() << "Removed Data?        " << serializedStats.RemovedData();
    qDebug() << "Z-Score at 1.0       " << serializedStats.ZScore(1.0);
    qDebug() << "";

    // write xml 
    qDebug() << "Testing XML: write XML from Statistics object...";
    FileName xmlFile("./Statistics.xml");
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
    Project *project = NULL;
    fromStats.save(writer, project);
    writer.writeEndDocument();
    qXmlFile.close();
    // read xml    
    qDebug() << "Testing XML: read XML to Statistics object...";

    if(!qXmlFile.open(QFile::ReadOnly | QFile::Text)){
      throw IException(IException::Unknown,
                        "Failed to parse xml file, ["+qXmlFile.fileName().toStdString()+"]",
                        _FILEINFO_);
    }

    QXmlStreamReader reader(&qXmlFile);
    StatisticsXmlHandlerTester statsFromXml(&reader, xmlFile);
    qDebug() << "Average:             " << statsFromXml.Average();
    qDebug() << "Variance:            " << statsFromXml.Variance();
    qDebug() << "Rms:                 " << statsFromXml.Rms();
    qDebug() << "Std Deviation:       " << statsFromXml.StandardDeviation();
    qDebug() << "Minimum:             " << statsFromXml.Minimum();
    qDebug() << "Maximum:             " << statsFromXml.Maximum();
    qDebug() << "ChebyShev Min:       " << statsFromXml.ChebyshevMinimum();
    qDebug() << "ChebyShev Max:       " << statsFromXml.ChebyshevMaximum();
    qDebug() << "Best Minimum:        " << statsFromXml.BestMinimum();
    qDebug() << "Best Maximum:        " << statsFromXml.BestMaximum();
    qDebug() << "Valid Minimum:       " << statsFromXml.ValidMinimum();
    qDebug() << "Valid Maximum:       " << statsFromXml.ValidMaximum();
    qDebug() << "Total Pixels:        " << statsFromXml.TotalPixels();
    qDebug() << "Valid Pixels:        " << statsFromXml.ValidPixels();
    qDebug() << "Null Pixels:         " << statsFromXml.NullPixels();
    qDebug() << "Lis Pixels:          " << statsFromXml.LisPixels();
    qDebug() << "Lrs Pixels:          " << statsFromXml.LrsPixels();
    qDebug() << "His Pixels:          " << statsFromXml.HisPixels();
    qDebug() << "Hrs Pixels:          " << statsFromXml.HrsPixels();
    qDebug() << "Out of Range Pixels: " << statsFromXml.OutOfRangePixels();
    qDebug() << "Over Range Pixels:   " << statsFromXml.OverRangePixels();
    qDebug() << "Under Range Pixels:  " << statsFromXml.UnderRangePixels();
    qDebug() << "Sum:                 " << statsFromXml.Sum();
    qDebug() << "SumSquare:           " << statsFromXml.SumSquare();
    qDebug() << "Removed Data?        " << statsFromXml.RemovedData();
    qDebug() << "Z-Score at 1.0       " << statsFromXml.ZScore(1.0);
    qDebug() << "";

    // read xml with no attributes or values
    qDebug() << "Testing XML: read XML with no attributes or values to Statistics object...";
    FileName emptyXmlFile("./unitTest_NoElementValues.xml");
    
    QFile xml(QString::fromStdString(emptyXmlFile.expanded()));
    if(!xml.open(QFile::ReadOnly | QFile::Text)){
      throw IException(IException::Unknown,
                       "Failed to parse xml file, ["+xml.fileName().toStdString()+"]",
                        _FILEINFO_);
    }
    QXmlStreamReader reader2(&xml);
    StatisticsXmlHandlerTester statsFromEmptyXml(&reader2, emptyXmlFile);
    qDebug() << "Average:             " << statsFromEmptyXml.Average();
    qDebug() << "Variance:            " << statsFromEmptyXml.Variance();
    qDebug() << "Rms:                 " << statsFromEmptyXml.Rms();
    qDebug() << "Std Deviation:       " << statsFromEmptyXml.StandardDeviation();
    qDebug() << "Minimum:             " << statsFromEmptyXml.Minimum();
    qDebug() << "Maximum:             " << statsFromEmptyXml.Maximum();
    qDebug() << "ChebyShev Min:       " << statsFromEmptyXml.ChebyshevMinimum();
    qDebug() << "ChebyShev Max:       " << statsFromEmptyXml.ChebyshevMaximum();
    qDebug() << "Best Minimum:        " << statsFromEmptyXml.BestMinimum();
    qDebug() << "Best Maximum:        " << statsFromEmptyXml.BestMaximum();
    qDebug() << "Valid Minimum:       " << statsFromEmptyXml.ValidMinimum();
    qDebug() << "Valid Maximum:       " << statsFromEmptyXml.ValidMaximum();
    qDebug() << "Total Pixels:        " << statsFromEmptyXml.TotalPixels();
    qDebug() << "Valid Pixels:        " << statsFromEmptyXml.ValidPixels();
    qDebug() << "Null Pixels:         " << statsFromEmptyXml.NullPixels();
    qDebug() << "Lis Pixels:          " << statsFromEmptyXml.LisPixels();
    qDebug() << "Lrs Pixels:          " << statsFromEmptyXml.LrsPixels();
    qDebug() << "His Pixels:          " << statsFromEmptyXml.HisPixels();
    qDebug() << "Hrs Pixels:          " << statsFromEmptyXml.HrsPixels();
    qDebug() << "Out of Range Pixels: " << statsFromEmptyXml.OutOfRangePixels();
    qDebug() << "Over Range Pixels:   " << statsFromEmptyXml.OverRangePixels();
    qDebug() << "Under Range Pixels:  " << statsFromEmptyXml.UnderRangePixels();
    qDebug() << "Sum:                 " << statsFromEmptyXml.Sum();
    qDebug() << "SumSquare:           " << statsFromEmptyXml.SumSquare();
    qDebug() << "Removed Data?        " << statsFromEmptyXml.RemovedData();
    qDebug() << "Z-Score at 1.0       " << statsFromEmptyXml.ZScore(1.0);
    qDebug() << "";

    qDebug() << "Testing error throws...";
    try {
      // You are removing non-existant data in [Statistics::RemoveData]
      s.RemoveData(a, 8);
    }
    catch(IException &e) {
      e.print();
    }

    try {
      // Minimum is invalid since you removed data
      s.Minimum();
    }
    catch(IException &e) {
      e.print();
    }

    try {
      // Maximum is invalid since you removed data
      s.Maximum();
    }
    catch(IException &e) {
      e.print();
    }

    try {
      // Invalid value for percent
      s.ChebyshevMinimum(0.0);
    }
    catch(IException &e) {
      e.print();
    }
    try {
      // Invalid value for percent
      s.ChebyshevMinimum(100.0);
    }
    catch(IException &e) {
      e.print();
    }

    try {
      // Invalid value for percent
      s.ChebyshevMaximum(0.0);
    }
    catch(IException &e) {
      e.print();
    }
    try {
      // Invalid value for percent
      s.ChebyshevMaximum(100.0);
    }
    catch(IException &e) {
      e.print();
    }

    try {
      // Invalid range
      s.SetValidRange(100.0, 0.0);
    }
    catch(IException &e) {
      e.print();
    }

    try {
      // Invalid z value
      s.Reset();
      s.SetValidRange(1, 100);
      s.AddData(5.0);
      s.AddData(5.0);
      s.ZScore(0.0);
    }
    catch(IException &e) {
      e.print();
    }

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

#if 0
need test coverage for
BelowRange(value)
#endif