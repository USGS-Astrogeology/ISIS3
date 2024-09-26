#include "Statistics.h"

#include "IException.h"
#include "Preference.h"
#include "Project.h"
#include "Statistics.h"

#include <QDebug>
#include <QDataStream>
#include <QFile>
#include <QXmlStreamWriter>
#include <QXmlInputSource>
#include <QDomDocument>

#include <float.h>

#include <gtest/gtest.h>

using namespace Isis;
using namespace std;

#define TEST_EPSILON 1e-8

/**
 * @author 2018-11-20 Tyler Wilson
 *
 * @internal
 *   @history 2018-11-20 Tyler Wilson - Coverage of existing
 *            statistical routines using truth data generated
 *            in R.
 *   @history 2019-05-17 Eric Gault - Applied requested changes
 *            in initial pull request. These include:
 *            Changing some of the test values to produce nicer output values.
 *            Splitting the SpecialPixels test into two seperate test cases.
 *            (SpecialPixels and ValidRange)
 *            Changing the XMLReadWrite test to use QString and QDomDocument
 *            instead of opening an XML file and writing to it.
 *
 */

TEST(Statistics, HandCalculations) {

    Statistics s;

    s.AddData(10);
    s.AddData(20);
    s.AddData(30);

    double mu = 20;
    double std = 10;
    double var = 100;

    // Might want make a different test case that produces
    // a nicer RMS.
    double rms = 21.602468994692867;


    EXPECT_DOUBLE_EQ(s.Average(),mu);
    EXPECT_DOUBLE_EQ(s.Variance(),var);
    EXPECT_DOUBLE_EQ(s.StandardDeviation(),std);
    EXPECT_DOUBLE_EQ(s.Sum(), 60.0);
    EXPECT_DOUBLE_EQ(s.SumSquare(), 1400.0);
    EXPECT_DOUBLE_EQ(s.ValidPixels(),3.0);
    EXPECT_DOUBLE_EQ(s.Rms(),rms);
    EXPECT_DOUBLE_EQ(s.Minimum(), 10.0);
    EXPECT_DOUBLE_EQ(s.Maximum(), 30.0);
    EXPECT_DOUBLE_EQ(s.ZScore(1.0),(1-mu)/std);
    double percent = 99.5;
    double k = sqrt(1.0 / (1.0 - percent / 100.0));
    EXPECT_DOUBLE_EQ(s.ChebyshevMinimum(),mu -k*std);
    EXPECT_DOUBLE_EQ(s.ChebyshevMaximum(),mu +k*std);


}

TEST(Statistics,SpecialPixels) {

    Statistics t;

    double a[10];
    a[0] = 1.0;
    a[1] = 2.0;
    a[2] = 3.0;
    a[3] = Null;
    a[4] = Hrs;
    a[5] = Lrs;
    a[6] = His;
    a[7] = Lis;
    a[8] = 10.0;
    a[9] = -1.0;

    //if all pixels (including invalid ones) are considered:
    double mu1 = 3.0;
    double std1 = 4.18330013267038;
    double rms1  = 4.79583152331272;
    double var1 = 17.5;
    t.AddData(a,10);

    EXPECT_DOUBLE_EQ(t.ValidPixels(),5.0);
    EXPECT_DOUBLE_EQ(t.Average(),mu1);
    EXPECT_DOUBLE_EQ(t.Rms(),rms1);
    EXPECT_DOUBLE_EQ(t.Variance(),var1);
    EXPECT_DOUBLE_EQ(t.StandardDeviation(),std1);


    EXPECT_DOUBLE_EQ(t.Minimum(),-1.0);
    EXPECT_DOUBLE_EQ(t.Maximum(),10.0);

    EXPECT_DOUBLE_EQ(t.Sum(),15.0);
    EXPECT_DOUBLE_EQ(t.SumSquare(),115.0);
    double percent = 99.5;
    double k = sqrt(1.0 / (1.0 - percent / 100.0));
    EXPECT_DOUBLE_EQ(t.ChebyshevMinimum(),mu1 -k*std1);
    EXPECT_DOUBLE_EQ(t.ChebyshevMaximum(),mu1 +k*std1);



    EXPECT_DOUBLE_EQ(t.BestMinimum(),-1.0);
    EXPECT_DOUBLE_EQ(t.BestMaximum(),10.0);

    EXPECT_DOUBLE_EQ(t.TotalPixels(),10.0);
    EXPECT_DOUBLE_EQ(t.ValidPixels(),5.0);
    EXPECT_DOUBLE_EQ(t.NullPixels(),1.0);
    EXPECT_DOUBLE_EQ(t.LisPixels(),1.0);
    EXPECT_DOUBLE_EQ(t.LrsPixels(),1.0);
    EXPECT_DOUBLE_EQ(t.HisPixels(),1.0);
    EXPECT_DOUBLE_EQ(t.HrsPixels(),1.0);

    EXPECT_DOUBLE_EQ(t.OutOfRangePixels(),0.0);
    EXPECT_DOUBLE_EQ(t.OverRangePixels(),0.0);
    EXPECT_DOUBLE_EQ(t.UnderRangePixels(),0.0);

}

TEST(Statistics,ValidRange) {

    Statistics t;

    double a[5];
    a[0] = 1.0;
    a[1] = 2.0;
    a[2] = 3.0;
    a[3] = 10.0;
    a[4] = -1.0;

    //if only pixels in the valid range are considered:
    double mu = 2.0;
    double std = 1.0;
    double rms = 2.1602468994692869;
    double var = 1.0;

    t.SetValidRange(1.0,6.0);

    t.AddData(a,5);

    EXPECT_EQ(t.InRange(0.0),false);
    EXPECT_EQ(t.InRange(2.0),true);
    EXPECT_EQ(t.AboveRange(7.0),true);
    EXPECT_EQ(t.AboveRange(6.0),false);
    EXPECT_EQ(t.BelowRange(0.0),true);
    EXPECT_EQ(t.BelowRange(1.0),false);

    EXPECT_DOUBLE_EQ(t.ValidPixels(),3.0);
    EXPECT_DOUBLE_EQ(t.Average(),mu);
    EXPECT_DOUBLE_EQ(t.Rms(),rms);
    EXPECT_DOUBLE_EQ(t.Variance(),var);
    EXPECT_DOUBLE_EQ(t.StandardDeviation(),std);


    EXPECT_DOUBLE_EQ(t.Minimum(),1.0);
    EXPECT_DOUBLE_EQ(t.Maximum(),3.0);

    EXPECT_DOUBLE_EQ(t.Sum(),6.0);
    EXPECT_DOUBLE_EQ(t.SumSquare(),14.0);
    double percent = 99.5;
    double k = sqrt(1.0 / (1.0 - percent / 100.0));
    EXPECT_DOUBLE_EQ(t.ChebyshevMinimum(),mu -k*std);
    EXPECT_DOUBLE_EQ(t.ChebyshevMaximum(),mu +k*std);



    EXPECT_DOUBLE_EQ(t.BestMinimum(),1.0);
    EXPECT_DOUBLE_EQ(t.BestMaximum(),3.0);

    EXPECT_DOUBLE_EQ(t.ValidMinimum(),1.0);
    EXPECT_DOUBLE_EQ(t.ValidMaximum(),6.0);

    EXPECT_DOUBLE_EQ(t.TotalPixels(),5.0);
    EXPECT_DOUBLE_EQ(t.ValidPixels(),3.0);
    EXPECT_DOUBLE_EQ(t.NullPixels(),0.0);
    EXPECT_DOUBLE_EQ(t.LisPixels(),0.0);
    EXPECT_DOUBLE_EQ(t.LrsPixels(),0.0);
    EXPECT_DOUBLE_EQ(t.HisPixels(),0.0);
    EXPECT_DOUBLE_EQ(t.HrsPixels(),0.0);

    EXPECT_DOUBLE_EQ(t.OutOfRangePixels(),2.0);
    EXPECT_DOUBLE_EQ(t.OverRangePixels(),1.0);
    EXPECT_DOUBLE_EQ(t.UnderRangePixels(),1.0);

}




TEST(Statistics,XMLReadWrite) {

    Statistics s;

    s.SetValidRange(0.0, 40.0);

    s.AddData(10.0);
    s.AddData(20.0);
    s.AddData(30.0);

    QString xmlOutput;

    QXmlStreamWriter writer(&xmlOutput);

    writer.setAutoFormatting(true);
    writer.setCodec("UTF-8");
    writer.writeStartDocument();
    Project *project = NULL;
    s.save(writer, project);
    writer.writeEndDocument();

    // Save the QString to a QDomDocument.
    QDomDocument xmlDoc("xml_doc");
    xmlDoc.setContent(xmlOutput);

    QDomElement root = xmlDoc.documentElement();
    QDomElement sum = root.firstChildElement("sum");
    QDomElement sumSquares = root.firstChildElement("sumSquares");

    QDomElement range = root.firstChildElement("range");
    QDomElement minimum = range.firstChildElement("minimum");
    QDomElement maximum = range.firstChildElement("maximum");
    QDomElement validMinimum = range.firstChildElement("validMinimum");
    QDomElement validMaximum = range.firstChildElement("validMaximum");

    QDomElement pixelCounts = root.firstChildElement("pixelCounts");
    QDomElement totalPixels = pixelCounts.firstChildElement("totalPixels");
    QDomElement validPixels = pixelCounts.firstChildElement("validPixels");
    QDomElement nullPixels = pixelCounts.firstChildElement("nullPixels");
    QDomElement lisPixels = pixelCounts.firstChildElement("lisPixels");
    QDomElement lrsPixels = pixelCounts.firstChildElement("lrsPixels");
    QDomElement hisPixels = pixelCounts.firstChildElement("hisPixels");
    QDomElement hrsPixels = pixelCounts.firstChildElement("hrsPixels");
    QDomElement underRangePixels = pixelCounts.firstChildElement("underRangePixels");
    QDomElement overRangePixels = pixelCounts.firstChildElement("overRangePixels");

    QDomElement removedData = root.firstChildElement("removedData");

    EXPECT_DOUBLE_EQ(sum.text().toDouble(), 60.0);
    EXPECT_DOUBLE_EQ(sumSquares.text().toDouble(), 1400.0);
    EXPECT_DOUBLE_EQ(minimum.text().toDouble(), 10.0);
    EXPECT_DOUBLE_EQ(maximum.text().toDouble(), 30.0);
    EXPECT_DOUBLE_EQ(validMinimum.text().toDouble(), 0.0);
    EXPECT_DOUBLE_EQ(validMaximum.text().toDouble(), 40.0);
    EXPECT_DOUBLE_EQ(totalPixels.text().toDouble(), 3.0);
    EXPECT_DOUBLE_EQ(validPixels.text().toDouble(), 3.0);
    EXPECT_DOUBLE_EQ(nullPixels.text().toDouble(), 0.0);
    EXPECT_DOUBLE_EQ(lisPixels.text().toDouble(), 0.0);
    EXPECT_DOUBLE_EQ(lrsPixels.text().toDouble(), 0.0);
    EXPECT_DOUBLE_EQ(hisPixels.text().toDouble(), 0.0);
    EXPECT_DOUBLE_EQ(hrsPixels.text().toDouble(), 0.0);
    EXPECT_DOUBLE_EQ(underRangePixels.text().toDouble(), 0.0);
    EXPECT_DOUBLE_EQ(overRangePixels.text().toDouble(), 0.0);
    EXPECT_STREQ(removedData.text().toStdString().c_str(), "0");

}
