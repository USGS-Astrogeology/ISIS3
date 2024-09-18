#include <QTemporaryDir>

#include "marciflip.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"
#include "Histogram.h"
#include "Portal.h"

#include "gtest/gtest.h"
#include "gmock/gmock.h"

using namespace Isis;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/marciflip.xml").expanded());

TEST(Marciflip, MarciflipTestDefault) {
  QTemporaryDir prefix;
  QString outCubeFn = prefix.path() + "/marciflip_out.cub";
  QString inCubeFn = "data/marciflip/T02_001002_1200_MC_00N284W_cropped.cub";
  QVector<QString> args = {"from=" + inCubeFn, "to=" + outCubeFn };
  UserInterface options(APP_XML, args);
  marciflip(options);

  Cube inCube(inCubeFn.toStdString());
  Cube outCube(outCubeFn.toStdString());
  Pvl *label = outCube.label();

  PvlGroup &dims = label->findGroup("Dimensions", Pvl::Traverse);
  EXPECT_EQ( (int)dims["Lines"], 48 );
  EXPECT_EQ( (int)dims["Samples"], 2 );
  EXPECT_EQ( (int)dims["Bands"], 3 );

  PvlGroup &inst = label->findGroup("Instrument", Pvl::Traverse);
  EXPECT_EQ( (int)inst.findKeyword("DataFlipped"), 0 );

  // check that the cubes have equal histograms
  // since pixels should only be moved, not changed
  std::unique_ptr<Histogram> outHist (outCube.histogram());
  std::unique_ptr<Histogram> inHist (inCube.histogram());
  EXPECT_EQ( outHist->Average(), outHist->Average() );
  EXPECT_EQ( outHist->Sum(), outHist->Sum() );
  EXPECT_EQ( outHist->ValidPixels(), outHist->ValidPixels() );
  EXPECT_EQ( outHist->StandardDeviation(), outHist->StandardDeviation() );


  // check that first 8 lines of input cube
  // are equal to last 8 lines of output cube
  Portal iPortal( 2, 1, inCube.pixelType() );
  Portal oPortal( 2, 1, outCube.pixelType() );

  for( int lines = 0; lines < 8; lines++ ) {
    iPortal.SetPosition(1, 1 + lines, 1);
    inCube.read(iPortal);

    oPortal.SetPosition(1, 41 + lines, 1);
    outCube.read(oPortal);

    EXPECT_DOUBLE_EQ(iPortal[0], oPortal[0]);
    EXPECT_DOUBLE_EQ(iPortal[1], oPortal[1]);
  }
}
