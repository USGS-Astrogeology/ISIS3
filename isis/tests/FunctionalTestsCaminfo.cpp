#include "Cube.h"
#include "CameraFixtures.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "Table.h"
#include "TableField.h"
#include "TableRecord.h"
#include "TestUtilities.h"
#include "CameraStatistics.h"

#include "caminfo.h"

#include "gtest/gtest.h"

using namespace Isis;
using ::testing::HasSubstr;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/caminfo.xml").expanded());


TEST_F(DefaultCube, FunctionalTestCaminfoCsv) {
    QString outFlatFileName = tempDir.path() + "/outTemp.csv";
    QVector<QString> flatArgs = {"from="+ testCube->fileName(),  "to="+outFlatFileName,
        "FORMAT=flat", "APPEND=false", "STATISTICS=true", "CAMSTATS=true",
        "GEOMETRY=true", "spice=true"};

    UserInterface flatOptions(APP_XML, flatArgs);
    try {
       caminfo(flatOptions);
    }
    catch (IException &e) {
        FAIL() << "Failed to run caminfo with flat output: " << e.what() << std::endl;
    }

    QString outPvlFileName = tempDir.path() + "/outTemp.pvl";
    QVector<QString> pvlArgs = {"from="+ testCube->fileName(),  "to="+outPvlFileName,
        "FORMAT=PVL", "APPEND=false", "STATISTICS=true", "CAMSTATS=true",
        "GEOMETRY=true", "spice=true"};

    UserInterface pvlOptions(APP_XML, pvlArgs);
    try {
       caminfo(pvlOptions);
    }
    catch (IException &e) {
        FAIL() << "Failed to run caminfo with PVL output: " << e.what() << std::endl;
    }

    Pvl caminfoPvl(outPvlFileName.toStdString());

    ASSERT_TRUE(caminfoPvl.hasObject("Caminfo"));
    PvlObject camobj = caminfoPvl.findObject("Caminfo");

    ASSERT_TRUE(camobj.hasObject("Parameters"));
    PvlObject parameters = camobj.findObject("Parameters");

    ASSERT_TRUE(camobj.hasObject("Camstats"));
    PvlObject camstats = camobj.findObject("Camstats");

    ASSERT_TRUE(camobj.hasObject("Statistics"));
    PvlObject statistics = camobj.findObject("Statistics");

    ASSERT_TRUE(camobj.hasObject("Geometry"));
    PvlObject geometry = camobj.findObject("Geometry");

    CSVReader caminfoCsv = CSVReader(outFlatFileName,
                                 true, 0, ',', false, true);

    for (int i = 0; i < parameters.keywords(); i++) {
        PvlKeyword currentKey = parameters[i];
        CSVReader::CSVAxis currentColumn = caminfoCsv.getColumn(QString::fromStdString(currentKey.name()));

        // Skip RunDate as it's not in the CSV
        if (currentKey.name() != "RunDate") {
            EXPECT_TRUE(currentColumn.dim1() > 0) << "Failed to find column [" <<
                    currentKey.name() << "] in CSV";
            QString stringCsvValue = currentColumn[0];
            bool isCsvNumeric = false;
            double numericCsvValue = stringCsvValue.toDouble(&isCsvNumeric);
            QString stringPvlValue = QString::fromStdString(currentKey);
            bool isPvlNumeric = false;
            double numericPvlValue = stringPvlValue.toDouble(&isPvlNumeric);
            if (isCsvNumeric && isPvlNumeric) {
                EXPECT_NEAR(numericCsvValue, numericPvlValue, 0.001) <<
                        "Column [" << currentKey.name() << "] value [" <<
                        stringCsvValue.toStdString() << "] does not match Pvl value [" <<
                        stringPvlValue.toStdString() << "] within tolerance [0.001].";
            }
            else {
                EXPECT_EQ(stringCsvValue.toStdString(), stringPvlValue.toStdString()) <<
                        "Column [" << currentKey.name() << "] value [" <<
                        stringCsvValue.toStdString() << "] does not match Pvl value [" <<
                        stringPvlValue.toStdString() << "].";
            }
        }
    }

    for (int i = 0; i < camstats.keywords(); i++) {
        PvlKeyword currentKey = camstats[i];
        QString columnName = "CamStats_" + QString::fromStdString(currentKey.name());
        CSVReader::CSVAxis currentColumn = caminfoCsv.getColumn(columnName);
        EXPECT_TRUE(currentColumn.dim1() > 0) << "Failed to find column [" <<
                                                 columnName.toStdString() << "] in CSV";
        QString stringCsvValue = currentColumn[0];
        bool isCsvNumeric = false;
        double numericCsvValue = stringCsvValue.toDouble(&isCsvNumeric);
        QString stringPvlValue = QString::fromStdString(currentKey);
        bool isPvlNumeric = false;
        double numericPvlValue = stringPvlValue.toDouble(&isPvlNumeric);
        if (isCsvNumeric && isPvlNumeric) {
            EXPECT_NEAR(numericCsvValue, numericPvlValue, 0.001) <<
                    "Column [" << columnName.toStdString() << "] value [" <<
                    stringCsvValue.toStdString() << "] does not match Pvl value [" <<
                    stringPvlValue.toStdString() << "] within tolerance [0.001].";
        }
        else {
            EXPECT_EQ(stringCsvValue.toStdString(), stringPvlValue.toStdString()) <<
                    "Column [" << columnName.toStdString() << "] value [" <<
                    stringCsvValue.toStdString() << "] does not match Pvl value [" <<
                    stringPvlValue.toStdString() << "].";
        }
    }

    for (int i = 0; i < statistics.keywords(); i++) {
        PvlKeyword currentKey = statistics[i];
        QString columnName = "Stats_" + QString::fromStdString(currentKey.name());
        CSVReader::CSVAxis currentColumn = caminfoCsv.getColumn(columnName);
        EXPECT_TRUE(currentColumn.dim1() > 0) << "Failed to find column [" <<
                                                 columnName.toStdString() << "] in CSV";
        QString stringCsvValue = currentColumn[0];
        bool isCsvNumeric = false;
        double numericCsvValue = stringCsvValue.toDouble(&isCsvNumeric);
        QString stringPvlValue = QString::fromStdString(currentKey);
        bool isPvlNumeric = false;
        double numericPvlValue = stringPvlValue.toDouble(&isPvlNumeric);
        if (isCsvNumeric && isPvlNumeric) {
            EXPECT_NEAR(numericCsvValue, numericPvlValue, 0.001) <<
                    "Column [" << columnName.toStdString() << "] value [" <<
                    stringCsvValue.toStdString() << "] does not match Pvl value [" <<
                    stringPvlValue.toStdString() << "] within tolerance [0.001].";
        }
        else {
            EXPECT_EQ(stringCsvValue.toStdString(), stringPvlValue.toStdString()) <<
                    "Column [" << columnName.toStdString() << "] value [" <<
                    stringCsvValue.toStdString() << "] does not match Pvl value [" <<
                    stringPvlValue.toStdString() << "].";
        }
    }

    for (int i = 0; i < geometry.keywords(); i++) {
        PvlKeyword currentKey = geometry[i];
        QString columnName = "Geom_" + QString::fromStdString(currentKey.name());
        CSVReader::CSVAxis currentColumn = caminfoCsv.getColumn(columnName);
        EXPECT_TRUE(currentColumn.dim1() > 0) << "Failed to find column [" <<
                                                 columnName.toStdString() << "] in CSV";
        QString stringCsvValue = currentColumn[0];
        bool isCsvNumeric = false;
        double numericCsvValue = stringCsvValue.toDouble(&isCsvNumeric);
        QString stringPvlValue = QString::fromStdString(currentKey);
        bool isPvlNumeric = false;
        double numericPvlValue = stringPvlValue.toDouble(&isPvlNumeric);
        if (isCsvNumeric && isPvlNumeric) {
            EXPECT_NEAR(numericCsvValue, numericPvlValue, 0.001) <<
                    "Column [" << columnName.toStdString() << "] value [" <<
                    stringCsvValue.toStdString() << "] does not match Pvl value [" <<
                    stringPvlValue.toStdString() << "] within tolerance [0.001].";
        }
        else {
            EXPECT_EQ(stringCsvValue.toStdString(), stringPvlValue.toStdString()) <<
                    "Column [" << columnName.toStdString() << "] value [" <<
                    stringCsvValue.toStdString() << "] does not match Pvl value [" <<
                    stringPvlValue.toStdString() << "].";
        }
    }
}


TEST_F(DefaultCube, FunctionalTestCaminfoDefault) {
    CameraStatistics camStats(testCube->camera(), 100, 100, testCube->fileName());

    Pvl statsPvl = camStats.toPvl();
    TableField fname("Name", Isis::TableField::Text, 45);
    TableField fmin("Minimum", Isis::TableField::Double);
    TableField fmax("Maximum", Isis::TableField::Double);
    TableField favg("Average", Isis::TableField::Double);
    TableField fstd("StandardDeviation", Isis::TableField::Double);

    TableRecord record;
    record += fname;
    record += fmin;
    record += fmax;
    record += favg;
    record += fstd;

    Table table("CameraStatistics", record);

    for (int i = 1; i < statsPvl.groups(); i++) {
      PvlGroup &group = statsPvl.group(i);

      int entry = 0;
      record[entry] = group.name();
      entry++;
      for (int j = 0; j < group.keywords(); j++) {
        record[entry] = std::stod(group[j][0]);
        entry++;
      }
      table += record;
    }
    testCube->write(table);

    QString outFileName = tempDir.path() + "/outTemp.csv";
    QVector<QString> args = {"to="+outFileName,
        "ISISLABEL=true", "STATISTICS=true", "CAMSTATS=true",
        "POLYGON=true", "polysinc=100", "polylinc=100"};

    UserInterface options(APP_XML, args);
    try {
        caminfo(testCube, options);
    }
    catch (IException &e) {
        FAIL() << "Unable to open image: " << e.what() << std::endl;
    }

    Pvl pvlobject = Pvl(outFileName.toStdString());

    ASSERT_TRUE(pvlobject.hasObject("Caminfo"));
    PvlObject camobj = pvlobject.findObject("Caminfo");
    ASSERT_TRUE(camobj.hasObject("Camstats"));
    PvlObject camstats = camobj.findObject("Camstats");

    EXPECT_NEAR(camstats.findKeyword("LatitudeMinimum"), 9.9286479874788, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("LatitudeMaximum"), 10.434709753119, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("LatitudeAverage"), 10.181983206084, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("LatitudeStandardDeviation"), 0.11084102743244, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("LongitudeMinimum"), 255.64554871862, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("LongitudeMaximum"), 256.14606952525, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("LongitudeAverage"), 255.89390491018, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("LongitudeStandardDeviation"), 0.10658330458136, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("SampleResolutionMinimum"), 18.840683425668, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("SampleResolutionMaximum"), 18.985953877822, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("SampleResolutionAverage"), 18.90816559308, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("SampleResolutionStandardDeviation"), 0.038060007171614, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("LineResolutionMinimum"), 18.840683425668, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("LineResolutionMaximum"), 18.985953877822, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("LineResolutionAverage"), 18.90816559308, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("LineResolutionStandardDeviation"), 0.038060007171614, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("ResolutionMinimum"), 18.840683425668, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("ResolutionMaximum"), 18.985953877822, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("ResolutionAverage"), 18.90816559308, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("ResolutionStandardDeviation"), 0.038060007171614, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("ObliqueSampleResolutionMinimum"), 18.967781671350998, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("ObliqueSampleResolutionMaximum"), 21.179434547755999, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("ObliqueSampleResolutionAverage"), 19.550786846366002, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("ObliqueSampleResolutionStandardDeviation"), 0.21126188466418, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("ObliqueLineResolutionMinimum"), 18.967781671350998, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("ObliqueLineResolutionMaximum"), 21.179434547755999, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("ObliqueLineResolutionAverage"), 19.550786846366002, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("ObliqueLineResolutionStandardDeviation"), 0.21126188466418, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("ObliqueResolutionMinimum"), 18.967781671350998, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("ObliqueResolutionMaximum"), 21.179434547755999, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("ObliqueResolutionAverage"), 19.550786846366002, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("ObliqueResolutionStandardDeviation"), 0.21126188466418, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("AspectRatioMinimum"), 1.0, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("AspectRatioAverage"), 1.0, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("AspectRatioStandardDeviation"), 0.0, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("PhaseMinimum"), 79.756143590222, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("PhaseMaximum"), 81.304900313013, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("PhaseAverage"), 80.529097153288, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("PhaseStandardDeviation"), 0.44420861263609, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("EmissionMinimum"), 10.798462835458, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("EmissionMaximum"), 13.502630463571, 0.001);
    EXPECT_NEAR(camstats.findKeyword("EmissionAverage"), 12.15148695101, 0.001);
    EXPECT_NEAR(camstats.findKeyword("EmissionStandardDeviation"), 0.56543791358689, 0.001);
    EXPECT_NEAR(camstats.findKeyword("IncidenceMinimum"), 69.941096124192, 0.001);
    EXPECT_NEAR(camstats.findKeyword("IncidenceMaximum"), 70.311944975377, 0.001);
    EXPECT_NEAR(camstats.findKeyword("IncidenceAverage"), 70.127459134075, 0.001);
    EXPECT_NEAR(camstats.findKeyword("IncidenceStandardDeviation"), 0.10249039125851, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("LocalSolarTimeMinimum"), 7.7698055422189, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("LocalSolarTimeMaximum"), 7.8031735959943, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("LocalSolarTimeAverage"), 7.7863626216564, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("LocalSolarTimeStandardDeviation"), 0.0071055546198845, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("LocalRadiusMinimum"), 3410663.3374636, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("LocalRadiusMaximum"), 3413492.0662692, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("LocalRadiusAverage"), 3412205.8144925, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("LocalRadiusStandardDeviation"), 648.57630914361, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("NorthAzimuthMinimum"), 312.35223008738, 0.001);
    EXPECT_NEAR(camstats.findKeyword("NorthAzimuthMaximum"), 350.57900431316, 0.001);
    EXPECT_NEAR(camstats.findKeyword("NorthAzimuthAverage"), 332.96766151042, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("NorthAzimuthStandardDeviation"), 0.67037977027338, 0.001);

    EXPECT_TRUE(camobj.hasObject("IsisLabel"));
    EXPECT_TRUE(camobj.hasObject("Parameters"));
    EXPECT_FALSE(camobj.hasObject("OriginalLabel"));

    ASSERT_TRUE(camobj.hasObject("Statistics"));
    PvlObject statistics = camobj.findObject("Statistics");

    EXPECT_NEAR(statistics.findKeyword("MeanValue"), 127.49950846428, 0.001);
    EXPECT_NEAR(statistics.findKeyword("StandardDeviation"), 73.322672255332, 0.001 );
    EXPECT_NEAR(statistics.findKeyword("MinimumValue"), 1.0, 0.001 );
    EXPECT_NEAR(statistics.findKeyword("MaximumValue"), 254, 0.001 );
    EXPECT_NEAR(statistics.findKeyword("PercentHIS"), 0, 0.001 );
    EXPECT_NEAR(statistics.findKeyword("PercentHRS"), 0, 0.001 );
    EXPECT_NEAR(statistics.findKeyword("PercentLIS"), 0, 0.001 );
    EXPECT_NEAR(statistics.findKeyword("PercentLRS"), 0, 0.001 );
    EXPECT_NEAR(statistics.findKeyword("PercentNull"), 0.39208006141146, 0.001 );
    EXPECT_NEAR(statistics.findKeyword("TotalPixels"), 1271424, 0.001 );

    ASSERT_TRUE(camobj.hasObject("Geometry"));
    PvlObject geometry = camobj.findObject("Geometry");

    EXPECT_DOUBLE_EQ(geometry.findKeyword("BandsUsed"), 1);
    EXPECT_DOUBLE_EQ(geometry.findKeyword("ReferenceBand"), 1);
    EXPECT_DOUBLE_EQ(geometry.findKeyword("OriginalBand"), 1);
    EXPECT_EQ(geometry.findKeyword("Target")[0], "MARS");
    EXPECT_EQ(geometry.findKeyword("StartTime")[0], "1977-07-09T20:05:51.5549999");
    EXPECT_EQ(geometry.findKeyword("EndTime")[0], "1977-07-09T20:05:51.5549999");
    EXPECT_DOUBLE_EQ(geometry.findKeyword("CenterLine"), 528.0);
    EXPECT_DOUBLE_EQ(geometry.findKeyword("CenterSample"), 602.0);
    EXPECT_NEAR(geometry.findKeyword("CenterLatitude"), 10.181441241544, 0.0001);
    EXPECT_NEAR(geometry.findKeyword("CenterLongitude"), 255.89292858176, 0.0001);
    EXPECT_NEAR(geometry.findKeyword("CenterRadius"), 3412288.6569794999, 0.0001);
    EXPECT_NEAR(geometry.findKeyword("RightAscension"), 310.20703346939001, 0.0001);
    EXPECT_NEAR(geometry.findKeyword("Declination"), -46.327247017379, 0.0001);
    EXPECT_NEAR(geometry.findKeyword("UpperLeftLongitude"), 255.64554860056, 0.0001);
    EXPECT_NEAR(geometry.findKeyword("UpperLeftLatitude"), 10.086794148631, 0.0001);
    EXPECT_NEAR(geometry.findKeyword("LowerLeftLongitude"), 255.96651410281, 0.0001);
    EXPECT_NEAR(geometry.findKeyword("LowerLeftLatitude"), 9.928647808629, 0.0001);
    EXPECT_NEAR(geometry.findKeyword("LowerRightLongitude"), 256.14606965798, 0.0001);
    EXPECT_NEAR(geometry.findKeyword("LowerRightLatitude"), 10.279980555851, 0.0001);
    EXPECT_NEAR(geometry.findKeyword("UpperRightLongitude"), 255.82316032959, 0.0001);
    EXPECT_NEAR(geometry.findKeyword("UpperRightLatitude"), 10.434709827388, 0.0001);
    EXPECT_NEAR(geometry.findKeyword("PhaseAngle"), 80.528382053153, 0.0001);
    EXPECT_NEAR(geometry.findKeyword("EmissionAngle"), 12.13356433166, 0.0001);
    EXPECT_NEAR(geometry.findKeyword("IncidenceAngle"), 70.127983086993, 0.0001);
    EXPECT_NEAR(geometry.findKeyword("NorthAzimuth"), 332.65918485196, 0.0001);
    EXPECT_NEAR(geometry.findKeyword("OffNadir"), 9.9273765164008, 0.0001);
    EXPECT_NEAR(geometry.findKeyword("SolarLongitude"), 294.73518830594998, 0.0001);
    EXPECT_NEAR(geometry.findKeyword("LocalTime"), 7.7862975334032, 0.0001);
    EXPECT_NEAR(geometry.findKeyword("TargetCenterDistance"), 4160.7294345949, 0.0001);
    EXPECT_NEAR(geometry.findKeyword("SlantDistance"), 762.37204489156, 0.0001);
    EXPECT_NEAR(geometry.findKeyword("SampleResolution"), 18.904248476287, 0.0001);
    EXPECT_NEAR(geometry.findKeyword("LineResolution"), 18.904248476287, 0.0001);
    EXPECT_NEAR(geometry.findKeyword("PixelResolution"), 18.904248476287, 0.0001);
    EXPECT_NEAR(geometry.findKeyword("MeanGroundResolution"), 18.913336801664, 0.0001);
    EXPECT_NEAR(geometry.findKeyword("SubSolarAzimuth"), 92.033828011827, 0.0001);
    EXPECT_NEAR(geometry.findKeyword("SubSolarGroundAzimuth"), 118.87356332432, 0.0001);
    EXPECT_NEAR(geometry.findKeyword("SubSolarLatitude"), -22.740326163641, 0.0001);
    EXPECT_NEAR(geometry.findKeyword("SubSolarLongitude"), 319.09846558533, 0.0001);
    EXPECT_NEAR(geometry.findKeyword("SubSpacecraftAzimuth"), 240.08514371127, 0.0001);
    EXPECT_NEAR(geometry.findKeyword("SubSpacecraftGroundAzimuth"), 267.53187323573, 0.0001);
    EXPECT_NEAR(geometry.findKeyword("SubSpacecraftLatitude"), 10.078847382918, 0.0001);
    EXPECT_NEAR(geometry.findKeyword("SubSpacecraftLongitude"), 253.65422317887, 0.0001);
    EXPECT_NEAR(geometry.findKeyword("ParallaxX"), 0.0092584293412006, 0.0001);
    EXPECT_NEAR(geometry.findKeyword("ParallaxY"), -0.21479478952768, 0.0001);
    EXPECT_NEAR(geometry.findKeyword("ShadowX"), 1.3359751259293, 0.0001);
    EXPECT_NEAR(geometry.findKeyword("ShadowY"), 2.4227562244446, 0.0001);
    EXPECT_EQ(geometry.findKeyword("HasLongitudeBoundary")[0], "FALSE");
    EXPECT_EQ(geometry.findKeyword("HasNorthPole")[0], "FALSE");
    EXPECT_EQ(geometry.findKeyword("HasSouthPole")[0], "FALSE");
    EXPECT_NEAR(geometry.findKeyword("ObliqueSampleResolution"), 19.589652452595999, 0.0001);
    EXPECT_NEAR(geometry.findKeyword("ObliqueLineResolution"), 19.589652452595999, 0.0001);
    EXPECT_NEAR(geometry.findKeyword("ObliquePixelResolution"), 19.589652452595999, 0.0001);
    EXPECT_NEAR(geometry.findKeyword("ObliqueDetectorResolution"), 19.589652452595999, 0.0001);
}


TEST_F(DefaultCube, FunctionalTestCaminfoPoly) {
    QString outFileName = tempDir.path() + "/outTemp.pvl";
    QVector<QString> args = {"from="+ testCube->fileName(),  "to="+outFileName,
        "ISISLABEL=false", "STATISTICS=false", "CAMSTATS=false",
        "POLYGON=true", "inctype=vertices", "numvertices=3"};

    UserInterface options(APP_XML, args);
    try {
        caminfo(options);
    }
    catch (IException &e) {
        FAIL() << "Unable to open image: " << e.what() << std::endl;
    }

    Pvl pvlobject = Pvl(outFileName.toStdString());
    PvlObject camobj = pvlobject.findObject("Caminfo");
    PvlObject poly = camobj.findObject("Polygon");

    EXPECT_NEAR(poly.findKeyword("CentroidLine"), 533.58306993138, 0.001 );
    EXPECT_NEAR(poly.findKeyword("CentroidSample"), 608.16401376754, 0.001 );
    EXPECT_NEAR(poly.findKeyword("CentroidLatitude"), 10.182403056571, 0.001 );
    EXPECT_NEAR(poly.findKeyword("CentroidLongitude"), 255.8955754569, 0.001 );
    EXPECT_NEAR(poly.findKeyword("CentroidRadius"), 3412286.6660398, 0.001 );
    EXPECT_NEAR(poly.findKeyword("SurfaceArea"), 486.66203306014, 0.001 );
    EXPECT_NEAR(poly.findKeyword("GlobalCoverage"), 3.33e-04, 0.001 );
    EXPECT_NEAR(poly.findKeyword("SampleIncrement"), 1506, 0.001 );
    EXPECT_NEAR(poly.findKeyword("LineIncrement"),1506, 0.001 );
    EXPECT_TRUE(poly.hasKeyword("GisFootprint"));
}


TEST_F(DefaultCube, FunctionalTestCaminfoBoundary) {
    QString outFileName = tempDir.path() + "/outTemp.cub";
    QVector<QString> args = {"from="+ testCube->fileName(),  "to="+outFileName,
        "ISISLABEL=false", "STATISTICS=true", "CAMSTATS=true",
        "POLYGON=true", "LINC=25", "SINC=25", "POLYSINC=100", "POLYLINC=100"};

    UserInterface options(APP_XML, args);
    try {
        caminfo(options);
    }
    catch (IException &e) {
        FAIL() << "Unable to open image: " << e.what() << std::endl;
    }

    Pvl pvlobject = Pvl(outFileName.toStdString());
    PvlObject camobj = pvlobject.findObject("Caminfo");
    PvlObject poly = camobj.findObject("Polygon");

    EXPECT_NEAR(poly.findKeyword("CentroidLine"), 532.66229285950999, 0.001 );
    EXPECT_NEAR(poly.findKeyword("CentroidSample"), 607.53672501072003, 0.001 );
    EXPECT_NEAR(poly.findKeyword("CentroidLatitude"), 10.182356969859001, 0.001 );
    EXPECT_NEAR(poly.findKeyword("CentroidLongitude"), 255.89519621875999, 0.001 );
    EXPECT_NEAR(poly.findKeyword("CentroidRadius"), 3412287.9074047999, 0.001 );
    EXPECT_NEAR(poly.findKeyword("SurfaceArea"), 488.62348528983, 0.001 );
    EXPECT_NEAR(poly.findKeyword("GlobalCoverage"), 3.33e-04, 0.001 );
    EXPECT_NEAR(poly.findKeyword("SampleIncrement"), 100, 0.001 );
    EXPECT_NEAR(poly.findKeyword("LineIncrement"), 100, 0.001 );
    EXPECT_TRUE(poly.hasKeyword("GisFootprint"));

    PvlObject camstats = camobj.findObject("Camstats");

    EXPECT_NEAR(camstats.findKeyword("LatitudeMinimum"), 9.9286479874788, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("LatitudeMaximum"), 10.434709753119, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("LongitudeMinimum"), 255.64554871862, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("LongitudeMaximum"), 256.14606952525, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("ResolutionMaximum"), 18.985953877821999, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("PhaseMinimum"), 79.756145388578005, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("PhaseMaximum"), 81.304900313013, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("EmissionMinimum"), 10.798462835458, 0.001);
    EXPECT_NEAR(camstats.findKeyword("EmissionMaximum"), 13.502630463571, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("IncidenceMinimum"), 69.941096124192, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("IncidenceMaximum"), 70.311944975377, 0.001);
    EXPECT_NEAR(camstats.findKeyword("LocalSolarTimeMinimum"), 7.7698055422189, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("LocalSolarTimeMaximum"), 7.8031735959943, 0.001 );
}

TEST_F(DefaultCube, FunctionalTestCaminfoCamStatsTable) {

    CameraStatistics camStats(testCube->camera(), 100, 100, testCube->fileName());

    Pvl statsPvl = camStats.toPvl();

    TableField fname("Name", Isis::TableField::Text, 45);
    TableField fmin("Minimum", Isis::TableField::Double);
    TableField fmax("Maximum", Isis::TableField::Double);
    TableField favg("Average", Isis::TableField::Double);
    TableField fstd("StandardDeviation", Isis::TableField::Double);

    TableRecord record;
    record += fname;
    record += fmin;
    record += fmax;
    record += favg;
    record += fstd;

    Table table("CameraStatistics", record);

    for (int i = 1; i < statsPvl.groups(); i++) {
      PvlGroup &group = statsPvl.group(i);

      int entry = 0;
      record[entry] = group.name();
      entry++;
      for (int j = 0; j < group.keywords(); j++) {
        record[entry] = std::stod(group[j][0]);
        entry++;
      }
      table += record;
    }
    testCube->write(table);

    QString outFileName = tempDir.path() + "/outTemp.csv";
    QVector<QString> args = {"to="+outFileName,
                             "USECAMSTATSTBL=true",
                             "CAMSTATS=true"};

    UserInterface options(APP_XML, args);
    try {
        caminfo(testCube, options);
    }
    catch (IException &e) {
        FAIL() << "Unable to open image: " << e.what() << std::endl;
    }

    Pvl pvlobject = Pvl(outFileName.toStdString());

    ASSERT_TRUE(pvlobject.hasObject("Caminfo"));
    PvlObject camobj = pvlobject.findObject("Caminfo");
    ASSERT_TRUE(camobj.hasObject("Camstats"));
    PvlObject camstats = camobj.findObject("Camstats");

    EXPECT_NEAR(camstats.findKeyword("MinimumLatitude"), 9.9286479874788, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("MaximumLatitude"), 10.434709753119, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("MinimumLongitude"), 255.64554871862, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("MaximumLongitude"), 256.14606952525, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("MaximumResolution"), 18.985953877821999, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("MinimumPhase"), 79.756145388578005, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("MaximumPhase"), 81.304900313013, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("MinimumEmission"), 10.798462835458, 0.001);
    EXPECT_NEAR(camstats.findKeyword("MaximumEmission"), 13.502630463571, 0.001);
    EXPECT_NEAR(camstats.findKeyword("MinimumIncidence"), 69.941096124192, 0.001);
    EXPECT_NEAR(camstats.findKeyword("MaximumIncidence"), 70.311944975377, 0.001);
    EXPECT_NEAR(camstats.findKeyword("LocalTimeMinimum"), 7.7698055422189, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("LocalTimeMaximum"), 7.8031735959943, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("ObliqueResolutionMinimum"), 19.183652922680999, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("ObliqueResolutionMaximum"), 20.152531403933999, 0.001 );

    EXPECT_NEAR(camstats.findKeyword("LatitudeMinimum"), 9.9286479874788, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("LatitudeMaximum"), 10.434709753119, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("LatitudeAverage"), 10.191400154932, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("LatitudeStandardDeviation"), 0.12530865607936, 0.001 );

    EXPECT_NEAR(camstats.findKeyword("LongitudeMinimum"), 255.64554871862, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("LongitudeMaximum"), 256.14606952525, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("LongitudeAverage"), 255.90602354651, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("LongitudeStandardDeviation"), 0.11949570208348, 0.001 );

    EXPECT_NEAR(camstats.findKeyword("SampleResolutionMinimum"), 18.840699663038, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("SampleResolutionMaximum"), 18.985953877822, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("SampleResolutionAverage"), 18.911994840355, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("SampleResolutionStandardDeviation"), 0.042469580108781, 0.001 );

    EXPECT_NEAR(camstats.findKeyword("LineResolutionMinimum"), 18.840699663038, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("LineResolutionMaximum"), 18.985953877822, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("LineResolutionAverage"), 18.911994840355, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("LineResolutionStandardDeviation"), 0.042469580108781, 0.001 );

    EXPECT_NEAR(camstats.findKeyword("ResolutionMinimum"), 18.840699663038, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("ResolutionMaximum"), 18.985953877822, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("ResolutionAverage"), 18.911994840355, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("ResolutionStandardDeviation"), 0.042469580108781, 0.001 );

    EXPECT_NEAR(camstats.findKeyword("ObliqueSampleResolutionMinimum"), 19.183652922680999, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("ObliqueSampleResolutionMaximum"), 20.152531403933999, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("ObliqueSampleResolutionAverage"), 19.559780980294999, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("ObliqueSampleResolutionStandardDeviation"), 0.21057982709442, 0.001 );

    EXPECT_NEAR(camstats.findKeyword("ObliqueLineResolutionMinimum"), 19.183652922680999, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("ObliqueLineResolutionMaximum"), 20.152531403933999, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("ObliqueLineResolutionAverage"), 19.559780980294999, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("ObliqueLineResolutionStandardDeviation"), 0.21057982709442, 0.001 );

    EXPECT_NEAR(camstats.findKeyword("ObliqueResolutionMinimum"), 19.183652922680999, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("ObliqueResolutionMaximum"), 20.152531403933999, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("ObliqueResolutionAverage"), 19.559780980294999, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("ObliqueResolutionStandardDeviation"), 0.21057982709442, 0.001 );

    EXPECT_NEAR(camstats.findKeyword("AspectRatioMinimum"), 1.0, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("AspectRatioMaximum"), 1.0, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("AspectRatioAverage"), 1.0, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("AspectRatioStandardDeviation"), 0.0, 0.001 );

    EXPECT_NEAR(camstats.findKeyword("PhaseAngleMinimum"), 79.756386363556, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("PhaseAngleMaximum"), 81.304900313013, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("PhaseAngleAverage"), 80.556249549336, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("PhaseAngleStandardDeviation"), 0.496128069014, 0.001 );

    EXPECT_NEAR(camstats.findKeyword("EmissionAngleMinimum"), 10.798462835458, 0.001);
    EXPECT_NEAR(camstats.findKeyword("EmissionAngleMaximum"), 13.502630463571, 0.001);
    EXPECT_NEAR(camstats.findKeyword("EmissionAngleAverage"), 12.222241624466, 0.001);
    EXPECT_NEAR(camstats.findKeyword("EmissionAngleStandardDeviation"), 0.63393199155819, 0.001);

    EXPECT_NEAR(camstats.findKeyword("IncidenceAngleMinimum"), 69.941096124192, 0.001);
    EXPECT_NEAR(camstats.findKeyword("IncidenceAngleMaximum"), 70.311944975377, 0.001);
    EXPECT_NEAR(camstats.findKeyword("IncidenceAngleAverage"), 70.121640864056, 0.001);
    EXPECT_NEAR(camstats.findKeyword("IncidenceAngleStandardDeviation"), 0.1144744474437, 0.001);

    EXPECT_NEAR(camstats.findKeyword("LocalSolarTimeMinimum"), 7.7698055422189, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("LocalSolarTimeMaximum"), 7.8031735959943, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("LocalSolarTimeAverage"), 7.7871705307454, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("LocalSolarTimeStandardDeviation"), 0.0079663801248517, 0.001 );

    EXPECT_NEAR(camstats.findKeyword("LocalRadiusMinimum"), 3410663.3374636, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("LocalRadiusMaximum"), 3413492.0662692, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("LocalRadiusAverage"), 3412223.5305052, 0.001 );
    EXPECT_NEAR(camstats.findKeyword("LocalRadiusStandardDeviation"), 719.07504507167, 0.001 );

    EXPECT_NEAR(camstats.findKeyword("NorthAzimuthMinimum"), 331.73654530385, 0.001);
    EXPECT_NEAR(camstats.findKeyword("NorthAzimuthMaximum"), 334.60868272768, 0.001);
    EXPECT_NEAR(camstats.findKeyword("NorthAzimuthAverage"), 332.98305534536, 0.001);
    EXPECT_NEAR(camstats.findKeyword("NorthAzimuthStandardDeviation"), 0.4825660916106, 0.001);
}
