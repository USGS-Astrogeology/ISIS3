#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"
#include <QTemporaryDir>

#include "kerneldbgen.h"

#include "gtest/gtest.h"

using namespace Isis;
using ::testing::HasSubstr;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/kerneldbgen.xml").expanded());

TEST(Kerneldbgen, FunctionalTestKerneldbgenCk) {
  QTemporaryDir prefix;
  QVector<QString> args = {"to="+ prefix.path() + "/kernel.db.pvl",
                           "type=CK",
                           "recondir=data/kerneldbgen",
                           "reconfilter=mro_sc_2005-12-*.bc",
                           "sclk=$mro/kernels/sclk/MRO_SCLKSCET.00006.tsc",
                           "lsk=$base/kernels/lsk/naif0008.tls"};

  UserInterface options(APP_XML, args);
  try {
    kerneldbgen(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to generate kernel db: " << e.what() << std::endl;
  }

  Pvl kerneldbPvl(options.GetFileName("TO").toStdString());

  EXPECT_TRUE(kerneldbPvl.hasObject("SpacecraftPointing"));
  PvlObject &scPointing = kerneldbPvl.findObject("SpacecraftPointing");

  PvlGroup depend = kerneldbPvl.findGroup("Dependencies", Pvl::Traverse);
  EXPECT_PRED_FORMAT2(AssertStringsEqual, depend.findKeyword("SpacecraftClockKernel"), "$mro/kernels/sclk/MRO_SCLKSCET.00006.tsc");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, depend.findKeyword("LeapsecondKernel"), "$base/kernels/lsk/naif0008.tls");


  PvlGroup select = scPointing.group(1);
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select.findKeyword("Time")[0], "2005 DEC 13 00:01:04.662071 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select.findKeyword("Time")[1], "2005 DEC 14 00:01:03.933358 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select.findKeyword("File"), "data/kerneldbgen/mro_sc_2005-12-13.bc");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select.findKeyword("Type"), "Reconstructed");

  PvlGroup select2 = scPointing.group(2);
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select2.findKeyword("Time")[0], "2005 DEC 15 00:01:04.290582 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select2.findKeyword("Time")[1], "2005 DEC 16 00:01:03.241556 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select2.findKeyword("File"), "data/kerneldbgen/mro_sc_2005-12-15.bc");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select2.findKeyword("Type"), "Reconstructed");
}

 /**
  * Coverage Level Test for kerneldbgen
  *
  * This test creates an output database file from the kernel in the input file
  * that follow the given filter for reconstructed ck file name patterns. A database
  * is output with time coverage at the SPICE segment level and at the SPICE interval
  * level. (There will be one output entry for the spice segment, and several for the
  * SPICE interval because a SPICE segment is composed of SPICE intervals.)
  *
  * After the output PVL is created, when compared, the DIFF file indicates to
  * ignore RunTime and File.  The File keyword is ignored since, depending on
  * where the test is run, files may have different paths. These paths can not be
  * removed since they may be long enough to take up multiple lines.
  *
  * This test uses files from the TGO CaSSIS mission, as this is where the problem
  * was identified.
  *
  * history 2018-05-09 Kristin Berry - Added test for newly added time coverage
  *                                 LEVEL=(SEGMENT*, INTERVAL) option. See #5410
 **/
TEST(Kerneldbgen, FunctionalTestKerneldbgenCoverageLevelDefault) {
  QTemporaryDir prefix;
  QVector<QString> args = {"to="+ prefix.path() + "/kernel_segment.db.pvl",
                           "type=CK",
                           "recondir=data/kerneldbgen",
                           "reconfilter=em16_tgo_sc_??m_*.bc",
                           "sclk=$tgo/kernels/sclk/em16_tgo_step_????????.tsc",
                           "lsk=$base/kernels/lsk/naif0012.tls"};

  UserInterface options(APP_XML, args);
  try {
    kerneldbgen(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to generate kernel db: " << e.what() << std::endl;
  }
  Pvl kerneldbPvl(options.GetFileName("TO").toStdString());

  EXPECT_TRUE(kerneldbPvl.hasObject("SpacecraftPointing"));

  PvlGroup select = kerneldbPvl.findGroup("Selection", Pvl::Traverse);
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select.findKeyword("Time")[0], "2017 MAR 01 23:02:49.287637 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select.findKeyword("Time")[1], "2017 APR 01 00:01:04.267617 TDB");

}

TEST(Kerneldbgen, FunctionalTestKerneldbgenCoverageLevelInterval) {
  QTemporaryDir prefix;
  QVector<QString> args = {"to="+ prefix.path() + "/kernel_interval.db.pvl",
                           "type=CK",
                           "level=INTERVAL",
                           "recondir=data/kerneldbgen",
                           "reconfilter=em16_tgo_sc_??m_*.bc",
                           "sclk=data/kerneldbgen/em16_tgo_step_????????.tsc",
                           "lsk=$base/kernels/lsk/naif0012.tls"};

  UserInterface options(APP_XML, args);
  try {
    kerneldbgen(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to generate kernel db: " << e.what() << std::endl;
  }

  Pvl kerneldbPvl(options.GetFileName("TO").toStdString());

  EXPECT_TRUE(kerneldbPvl.hasObject("SpacecraftPointing"));

  PvlGroup select = kerneldbPvl.findGroup("Selection", Pvl::Traverse);
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[0][0], "2017 MAR 01 23:02:49.287637 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[0][1], "2017 MAR 01 23:02:53.287636 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[1][0], "2017 MAR 01 23:43:13.288601 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[1][1], "2017 MAR 01 23:43:49.288601 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[2][0], "2017 MAR 01 23:44:53.288599 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[2][1], "2017 MAR 01 23:44:53.288599 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[3][0], "2017 MAR 02 23:55:57.321517 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[3][1], "2017 MAR 03 00:01:05.321613 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[4][0], "2017 MAR 03 23:51:13.354049 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[4][1], "2017 MAR 04 00:01:05.454240 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[5][0], "2017 MAR 04 23:51:13.386676 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[5][1], "2017 MAR 05 00:01:05.486867 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[6][0], "2017 MAR 05 23:54:28.419399 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[6][1], "2017 MAR 06 00:01:08.419494 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[7][0], "2017 MAR 06 23:52:15.451927 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[7][1], "2017 MAR 07 00:01:07.452120 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[8][0], "2017 MAR 08 00:00:45.484745 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[8][1], "2017 MAR 08 00:01:05.484745 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[9][0], "2017 MAR 08 23:56:46.517274 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[9][1], "2017 MAR 09 00:01:06.517370 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[10][0], "2017 MAR 09 23:56:34.549899 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[10][1], "2017 MAR 10 00:01:06.549995 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[11][0], "2017 MAR 10 23:56:34.582523 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[11][1], "2017 MAR 11 00:01:06.582619 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[12][0], "2017 MAR 11 15:06:34.603116 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[12][1], "2017 MAR 11 15:12:38.603311 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[13][0], "2017 MAR 11 15:34:26.603792 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[13][1], "2017 MAR 11 15:34:26.603792 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[14][0], "2017 MAR 11 15:35:46.603791 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[14][1], "2017 MAR 11 15:35:50.603791 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[15][0], "2017 MAR 11 16:04:34.604465 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[15][1], "2017 MAR 11 16:04:34.604465 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[16][0], "2017 MAR 11 16:28:38.605044 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[16][1], "2017 MAR 11 16:31:26.605041 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[17][0], "2017 MAR 11 17:04:42.605812 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[17][1], "2017 MAR 11 17:04:42.605812 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[18][0], "2017 MAR 11 18:49:34.608219 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[18][1], "2017 MAR 11 18:49:54.608219 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[19][0], "2017 MAR 12 23:56:34.647770 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[19][1], "2017 MAR 13 00:01:06.647866 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[20][0], "2017 MAR 13 23:58:12.680491 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[20][1], "2017 MAR 14 00:01:08.680489 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[21][0], "2017 MAR 14 23:58:12.713114 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[21][1], "2017 MAR 15 00:01:08.713111 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[22][0], "2017 MAR 15 23:36:26.745155 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[22][1], "2017 MAR 15 23:46:14.745446 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[23][0], "2017 MAR 16 23:39:26.777874 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[23][1], "2017 MAR 16 23:41:14.777972 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[24][0], "2017 MAR 17 02:33:19.781819 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[24][1], "2017 MAR 17 02:42:43.782011 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[25][0], "2017 MAR 18 23:29:36.842924 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[25][1], "2017 MAR 18 23:31:00.842922 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[26][0], "2017 MAR 18 23:59:40.843597 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[26][1], "2017 MAR 19 00:01:08.843596 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[27][0], "2017 MAR 19 23:59:00.876218 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[27][1], "2017 MAR 20 00:01:08.876216 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[28][0], "2017 MAR 20 23:59:28.908837 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[28][1], "2017 MAR 21 00:01:08.908835 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[29][0], "2017 MAR 21 23:57:34.941357 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[29][1], "2017 MAR 22 00:01:06.941454 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[30][0], "2017 MAR 22 23:55:34.973978 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[30][1], "2017 MAR 23 00:01:06.974073 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[31][0], "2017 MAR 23 23:53:10.006498 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[31][1], "2017 MAR 24 00:01:06.006691 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[32][0], "2017 MAR 24 23:56:23.039212 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[32][1], "2017 MAR 25 00:01:07.039308 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[33][0], "2017 MAR 25 23:59:10.071927 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[33][1], "2017 MAR 26 00:01:06.071925 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[34][0], "2017 MAR 26 23:59:25.104543 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[34][1], "2017 MAR 27 00:01:05.104542 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[35][0], "2017 MAR 27 23:54:23.137064 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[35][1], "2017 MAR 28 00:01:07.137158 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[36][0], "2017 MAR 28 23:56:41.169677 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[36][1], "2017 MAR 29 00:01:05.169773 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[37][0], "2017 MAR 29 23:54:22.202295 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[37][1], "2017 MAR 30 00:01:06.202389 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[38][0], "2017 MAR 30 23:59:06.235005 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[38][1], "2017 MAR 31 00:01:06.235003 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[39][0], "2017 MAR 31 23:51:32.267426 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select[39][1], "2017 APR 01 00:01:04.267617 TDB");
}


TEST(Kerneldbgen, FunctionalTestKerneldbgenExtraDefault) {
  QTemporaryDir prefix;
  QVector<QString> args = {"to="+ prefix.path() + "/crism_kernels.db.pvl",
                           "type=CK",
                           "level=INTERVAL",
                           "recondir=$mro/kernels/ck",
                           "reconfilter=mro_crm_psp_??????_??????.bc",
                           "sclk=data/kerneldbgen/MRO_SCLKSCET.?????.65536.tsc",
                           "lsk=$base/kernels/lsk/naif????.tls",
                           "extra=($mro/kernels/fk/mro_v15.tf,$mro/kernels/fk/kernels.????.db)"};

  UserInterface options(APP_XML, args);
  try {
    kerneldbgen(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to generate kernel db: " << e.what() << std::endl;
  }

  Pvl kerneldbPvl(options.GetFileName("TO").toStdString());

  EXPECT_TRUE(kerneldbPvl.hasObject("SpacecraftPointing"));

  PvlGroup depend = kerneldbPvl.findGroup("Dependencies", Pvl::Traverse);
  EXPECT_PRED_FORMAT2(AssertStringsEqual, depend[2], "$mro/kernels/fk/mro_v15.tf");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, depend[3], "$mro/kernels/fk/mro_v16.tf");
}


TEST(Kerneldbgen, FunctionalTestKerneldbgenExtraBadKeyword) {
  QTemporaryDir prefix;
  QVector<QString> args = {"to="+ prefix.path() + "/crism_kernels.????.db.pvl",
                           "recondir=$mro/kernels/ck",
                           "reconfilter=mro_crm_psp_??????_??????.bc",
                           "sclk=data/kerneldbgen/MRO_SCLKSCET.?????.65536.tsc",
                           "lsk=$base/kernels/lsk/naif????.tls",
                           "extra=data/kerneldbgen/kernels_badkeywordvalue.db"};

  UserInterface options(APP_XML, args);
  try {
    kerneldbgen(options);
    FAIL() << "Should throw an exception" << std::endl;
  }
  catch (IException &e) {
    EXPECT_THAT(e.what(), HasSubstr("Expected the keyword File in [data/kerneldbgen/kernels_badkeywordvalue.db] to have two values"));
  }
}


TEST(Kerneldbgen, FunctionalTestKerneldbgenExtraBlank) {
  QTemporaryDir prefix;
  QVector<QString> args = {"to="+ prefix.path() + "/crism_kernels.????.db.pvl",
                           "recondir=$mro/kernels/ck",
                           "reconfilter=mro_crm_psp_??????_??????.bc",
                           "sclk=data/kerneldbgen/MRO_SCLKSCET.?????.65536.tsc",
                           "lsk=$base/kernels/lsk/naif????.tls",
                           "extra=data/kerneldbgen/kernels_blank.db"};

  UserInterface options(APP_XML, args);
  try {
    kerneldbgen(options);
    FAIL() << "Should throw an exception" << std::endl;
  }
  catch (IException &e) {
    EXPECT_THAT(e.what(), HasSubstr("Expected one Pvl Object in the DB file [data/kerneldbgen/kernels_blank.db] but found [0]"));
  }
}


TEST(Kerneldbgen, FunctionalTestKerneldbgenMisnamedGroup) {
  QTemporaryDir prefix;
  QVector<QString> args = {"to="+ prefix.path() + "/crism_kernels.????.db.pvl",
                           "recondir=$mro/kernels/ck",
                           "reconfilter=mro_crm_psp_??????_??????.bc",
                           "sclk=data/kerneldbgen/MRO_SCLKSCET.?????.65536.tsc",
                           "lsk=$base/kernels/lsk/naif????.tls",
                           "extra=data/kerneldbgen/kernels_misnamedgroup.db"};

  UserInterface options(APP_XML, args);
  try {
    kerneldbgen(options);
    FAIL() << "Should throw an exception" << std::endl;
  }
  catch (IException &e) {
    EXPECT_THAT(e.what(), HasSubstr("Expected Pvl Group in the first Pvl Object [Frame] in the DB file [data/kerneldbgen/kernels_misnamedgroup.db] to be named Selection but found [SomeSelection]."));
  }
}


TEST(Kerneldbgen, FunctionalTestKerneldbgenExtraMisnamedKeyword) {
  QTemporaryDir prefix;
  QVector<QString> args = {"to="+ prefix.path() + "/crism_kernels.????.db.pvl",
                           "recondir=$mro/kernels/ck",
                           "reconfilter=mro_crm_psp_??????_??????.bc",
                           "sclk=data/kerneldbgen/MRO_SCLKSCET.?????.65536.tsc",
                           "lsk=$base/kernels/lsk/naif????.tls",
                           "extra=data/kerneldbgen/kernels_misnamedkeyword.db"};

  UserInterface options(APP_XML, args);
  try {
    kerneldbgen(options);
    FAIL() << "Should throw an exception" << std::endl;
  }
  catch (IException &e) {
    EXPECT_THAT(e.what(), HasSubstr("Expected Pvl Group [Selection] in the first Pvl Object [Frame] in the DB file [data/kerneldbgen/kernels_misnamedkeyword.db] to have a single keyword named File, but the keyword was named [Files] instead."));
  }
}


TEST(Kerneldbgen, FunctionalTestKerneldbgenExtraNoGroup) {
  QTemporaryDir prefix;
  QVector<QString> args = {"to="+ prefix.path() + "/crism_kernels.????.db.pvl",
                           "recondir=$mro/kernels/ck",
                           "reconfilter=mro_crm_psp_??????_??????.bc",
                           "sclk=data/kerneldbgen/MRO_SCLKSCET.?????.65536.tsc",
                           "lsk=$base/kernels/lsk/naif????.tls",
                           "extra=data/kerneldbgen/kernels_nogroup.db"};

  UserInterface options(APP_XML, args);
  try {
    kerneldbgen(options);
    FAIL() << "Should throw an exception" << std::endl;
  }
  catch (IException &e) {
    EXPECT_THAT(e.what(), HasSubstr("Expected one Pvl Group in the first Pvl Object [Frame] in the DB file [data/kerneldbgen/kernels_nogroup.db] but found [0]."));
  }

}


TEST(Kerneldbgen, FunctionalTestKerneldbgenExtraNoKeyword) {
  QTemporaryDir prefix;
  QVector<QString> args = {"to="+ prefix.path() + "/crism_kernels.????.db.pvl",
                           "recondir=$mro/kernels/ck",
                           "reconfilter=mro_crm_psp_??????_??????.bc",
                           "sclk=data/kerneldbgen/MRO_SCLKSCET.?????.65536.tsc",
                           "lsk=$base/kernels/lsk/naif????.tls",
                           "extra=data/kerneldbgen/kernels_nokeyword.db"};

  UserInterface options(APP_XML, args);
  try {
    kerneldbgen(options);
    FAIL() << "Should throw an exception" << std::endl;
  }
  catch (IException &e) {
    EXPECT_THAT(e.what(), HasSubstr("Expected Pvl Group [Selection] in the first Pvl Object [Frame] in the DB file [data/kerneldbgen/kernels_nokeyword.db] to have a single keyword"));
  }
}


TEST(Kerneldbgen, FunctionalTestKerneldbgenSpk) {
  QTemporaryDir prefix;
  QVector<QString> args = {"to="+ prefix.path() + "/kernel.db.pvl",
                           "type=SPK",
                           "recondir=data/kerneldbgen",
                           "reconfilter=M3*.bsp",
                           "lsk=$base/kernels/lsk/naif0008.tls"};

  UserInterface options(APP_XML, args);
  try {
    kerneldbgen(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to generate kernel db: " << e.what() << std::endl;
  }

  Pvl kerneldbPvl(options.GetFileName("TO").toStdString());

  EXPECT_TRUE(kerneldbPvl.hasObject("SpacecraftPosition"));
  PvlObject &scPosition = kerneldbPvl.findObject("SpacecraftPosition");

  PvlGroup depend = kerneldbPvl.findGroup("Dependencies", Pvl::Traverse);
  EXPECT_PRED_FORMAT2(AssertStringsEqual, depend.findKeyword("LeapsecondKernel"), "$base/kernels/lsk/naif0008.tls");

  PvlGroup select = scPosition.group(1);
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select.findKeyword("Time")[0], "2009 FEB 04 23:51:59.498030 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select.findKeyword("Time")[1], "2009 FEB 04 23:52:03.370910 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select.findKeyword("File"), "data/kerneldbgen/M3G20090204T235053_V03_nadir-jig_2014-08-18.bsp");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select.findKeyword("Type"), "Reconstructed");

  PvlGroup select2 = scPosition.group(2);
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select2.findKeyword("Time")[0], "2009 JUL 30 20:51:02.374189 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select2.findKeyword("Time")[1], "2009 JUL 30 20:51:05.534750 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select2.findKeyword("File"), "data/kerneldbgen/M3G20090730T204955_V03_nadir-jig_2014-08-18.bsp");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select2.findKeyword("Type"), "Reconstructed");
}


TEST(Kerneldbgen, FunctionalTestKerneldbgenSmithedCkOffsets) {
  QTemporaryDir prefix;
  QVector<QString> args = {"to="+ prefix.path() + "/kernel.db.pvl",
                           "type=CK",
                           "smitheddir=data/kerneldbgen",
                           "smithedfilter=thmIR.bc",
                           "lsk=$base/kernels/lsk/naif0008.tls",
                           "sclk=data/kerneldbgen/ORB1_SCLKSCET.00274.tsc"};

  UserInterface options(APP_XML, args);
  try {
    kerneldbgen(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to generate kernel db: " << e.what() << std::endl;
  }

  Pvl kerneldbPvl(options.GetFileName("TO").toStdString());

  EXPECT_TRUE(kerneldbPvl.hasObject("SpacecraftPointing"));

  PvlGroup select = kerneldbPvl.findGroup("Selection", Pvl::Traverse);
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select.findKeyword("Time")[0], "2002 FEB 20 22:58:59.720231 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select.findKeyword("Time")[1], "2002 FEB 20 22:59:11.726211 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select.findKeyword("Instrument"), "THEMIS_IR");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select.findKeyword("EndOffset"), "169.442");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select.findKeyword("Type"), "Smithed");
}


TEST(Kerneldbgen, FunctionalTestKerneldbgenSmithedSpkOffsets) {
  QTemporaryDir prefix;
  QVector<QString> args = {"to="+ prefix.path() + "/kernel.db.pvl",
                           "type=SPK",
                           "smitheddir=data/kerneldbgen",
                           "smithedfilter=thmIR.bsp",
                           "lsk=$base/kernels/lsk/naif0008.tls"};

  UserInterface options(APP_XML, args);
  try {
    kerneldbgen(options);
  }
  catch (IException &e) {
    FAIL() << "Unable to generate kernel db: " << e.what() << std::endl;
  }

  Pvl kerneldbPvl(options.GetFileName("TO").toStdString());

  EXPECT_TRUE(kerneldbPvl.hasObject("SpacecraftPosition"));

  PvlGroup select = kerneldbPvl.findGroup("Selection", Pvl::Traverse);
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select.findKeyword("Time")[0], "2002 FEB 20 22:59:01.701369 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select.findKeyword("Time")[1], "2002 FEB 20 22:59:09.296828 TDB");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select.findKeyword("Instrument"), "THEMIS_IR");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select.findKeyword("StartOffset"), "0.263");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select.findKeyword("EndOffset"), "171.871");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, select.findKeyword("Type"), "Smithed");
}
