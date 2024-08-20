#include <QTemporaryDir>
#include <QFile>

#include "hyb2onc2isis.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"
#include "Histogram.h"
#include "OriginalLabel.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using namespace Isis;
using ::testing::HasSubstr;

static QString APP_XML = FileName("$ISISROOT/bin/xml/hyb2onc2isis.xml").expanded();

TEST(Hyb2onc2isis, Hyb2onc2isisTestDefault) {
    QTemporaryDir prefix;
    QString cubeFileName = prefix.path() + "/hyb2onc2isisTEMP.cub";
    QVector<QString> args = {"from=data/hyb2onc2isis/hyb2_onc_20151203_000006_w2f_l2a.fit",
                            "to=" + cubeFileName };

    UserInterface options(APP_XML, args);
    try {
        hyb2onc2isis(options);
    }
    catch (IException &e) {
        FAIL() << "Unable to ingest HYB2ONC image: " << e.toString().toStdString().c_str() << std::endl;
    }
    Cube cube(cubeFileName);
    Pvl *isisLabel = cube.label();

    // Dimensions Group
    ASSERT_EQ(cube.sampleCount(), 1024);
    ASSERT_EQ(cube.lineCount(), 1024);
    ASSERT_EQ(cube.bandCount(), 1);

    // Pixels Group
    ASSERT_EQ(PixelTypeName(cube.pixelType()), "Real");
    ASSERT_EQ(ByteOrderName(cube.byteOrder()), "Lsb");
    ASSERT_DOUBLE_EQ(cube.base(), 0.0);
    ASSERT_DOUBLE_EQ(cube.multiplier(), 1.0);

    // Instrument Group
    PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);
    ASSERT_EQ(inst["SpacecraftName"][0], "HAYABUSA-2");
    ASSERT_EQ(inst["InstrumentId"][0], "ONC-W2" );
    ASSERT_EQ(inst["StartTime"][0], "2015-12-03T00:00:06.637" );
    ASSERT_EQ(inst["StopTime"][0], "2015-12-03T00:00:06.641" );
    ASSERT_EQ(inst["SpacecraftClockStartCount"][0], "1/1009473117" );
    ASSERT_EQ(inst["TargetName"][0], "Earth" );

    // Archive Group
    PvlGroup &archive = isisLabel->findGroup("Archive", Pvl::Traverse);
    ASSERT_EQ(archive["ProducerId"][0], "ISAS/JAXA" );
    ASSERT_EQ(archive["FormatType"][0], "HAYABUSA2 IMAGE ONC L2a" );
    ASSERT_EQ(archive["Contenttype"][0], "ONC-W2 NON SMEARCORRECTED");
    ASSERT_EQ(archive["SourceProductId"][0], "hyb2_onc_20151203_000006_w2f_l2a" );

    // Kernels Group
    PvlGroup &kernel = isisLabel->findGroup("Kernels", Pvl::Traverse);
    ASSERT_EQ(int(kernel["NaifFrameCode"]), -37120);

    std::unique_ptr<Histogram> hist (cube.histogram());

    ASSERT_NEAR(hist->Average(), 297.8918, .0001);
    ASSERT_DOUBLE_EQ(hist->Sum(), 312362230);
    ASSERT_EQ(hist->ValidPixels(), 1048576);
    ASSERT_NEAR(hist->StandardDeviation(), 65.75840, .00001);

    // check original label exists
    Pvl ogLabel = cube.readOriginalLabel().ReturnLabels();
    PvlGroup &fitsLabel = ogLabel.findGroup("FitsLabels", Pvl::Traverse);
    ASSERT_EQ(fitsLabel["SPCECRFT"][0], "HAYABUSA2" );
}
