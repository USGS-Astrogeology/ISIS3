#include "photomet.h"

#include <QTemporaryDir>

#include "Histogram.h"
#include "PvlGroup.h"
#include "TestUtilities.h"

#include "gmock/gmock.h"

using namespace Isis;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/photomet.xml").expanded());

TEST(Photomet, FunctionalTestPhotometClementine) {
  QTemporaryDir prefix;
  Pvl log;
  QString cubeFileName = prefix.path() + "/photometTEMP.cub";
  QVector<QString> args = {"from=data/photomet/lub2303j.191.lev1.cub", "to=" + cubeFileName, 
                          "frompvl=data/photomet/clem.pvl", "normname=moonalbedo",
	                        "phtname=lunarlambertmcewen"};
  UserInterface options(APP_XML, args);

  photomet(options, &log);

  Cube cube(cubeFileName.toStdString());

  std::unique_ptr<Histogram> hist (cube.histogram(0));
  EXPECT_NEAR(hist->Average(), 57.865063346038504, .000001);
  EXPECT_NEAR(hist->Sum(), 6399413.0855650902, .000001);
  EXPECT_EQ(hist->ValidPixels(), 110592);
  EXPECT_NEAR(hist->StandardDeviation(), 5.0229621098463886, .000001);
}

TEST(Photomet, FunctionalTestPhotometClemMaxemiMaxinc) {
  QTemporaryDir prefix;
  Pvl log;
  QString cubeFileName = prefix.path() + "/photometTEMP.cub";
  QVector<QString> args = {"from=data/photomet/lub2303j.191.lev1.cub", "to=" + cubeFileName, 
                          "frompvl=data/photomet/clem.pvl", "maxe=4.0", "maxi=5.0", "normname=moonalbedo", 
                          "phtname=lunarlambertmcewen", "xmul=1.0", "wl=1.0", "xb1=-.0817", "xb2=.0081"};
  UserInterface options(APP_XML, args);

  photomet(options, &log);

  Cube cube(cubeFileName.toStdString());

  std::unique_ptr<Histogram> hist (cube.histogram(0));
  EXPECT_NEAR(hist->Average(), 55.491706688909559, .000001);
  EXPECT_NEAR(hist->Sum(), 2439415.4260444641, .000001);
  EXPECT_EQ(hist->ValidPixels(), 43960);
  EXPECT_NEAR(hist->StandardDeviation(), 4.0919312912337835, .000001);
}

TEST(Photomet, FunctionalTestPhotometClemMaxemission) {
  QTemporaryDir prefix;
  Pvl log;
  QString cubeFileName = prefix.path() + "/photometTEMP.cub";
  QVector<QString> args = {"from=data/photomet/lub2303j.191.lev1.cub", "to=" + cubeFileName, 
                          "frompvl=data/photomet/clem.pvl", "maxemission=4.0", 
                          "normname=moonalbedo", "phtname=lunarlambertmcewen", "xmul=1.0", 
                          "wl=1.0", "xb1=-.0817", "xb2=.0081"};
  UserInterface options(APP_XML, args);

  photomet(options, &log);

  Cube cube(cubeFileName.toStdString());

  std::unique_ptr<Histogram> hist (cube.histogram(0));
  EXPECT_NEAR(hist->Average(), 58.134963167604397, .000001);
  EXPECT_NEAR(hist->Sum(), 6094695.1336021423, .000001);
  EXPECT_EQ(hist->ValidPixels(), 104837);
  EXPECT_NEAR(hist->StandardDeviation(), 4.7295549270234245, .000001);
}

TEST(Photomet, FunctionalTestPhotometMessCmdline) {
  QTemporaryDir prefix;
  Pvl log;
  QString cubeFileName = prefix.path() + "/photometTEMP.cub";
  QVector<QString> args = {"from=data/photomet/EN0213240738M.lev1.cub", "to=" + cubeFileName, 
                          "maxemission=85.0", "maxincidence=89.0", "phtname=hapkehen", 
                          "theta=6.021327012", "wh=0.283104821", "hg1=0.246493283", 
                          "hg2=0.559208947", "hh=0.075", "b0=2.3", "zerob0standard=false",
                          "normname=albedo", "incref=30.0", "incmat=0.0", "thresh=10e30", "albedo=1.0"};
  UserInterface options(APP_XML, args);

  photomet(options, &log);

  Cube cube(cubeFileName.toStdString());

  std::unique_ptr<Histogram> hist (cube.histogram(0));
  EXPECT_NEAR(hist->Average(),  0.05943594115596508, .000001);
  EXPECT_NEAR(hist->Sum(), 15489.481752773747, .000001);
  EXPECT_EQ(hist->ValidPixels(), 260608);
  EXPECT_NEAR(hist->StandardDeviation(), 0.026528112345021105, .000001);
}

TEST(Photomet, FunctionalTestPhotometMessenger) {
  QTemporaryDir prefix;
  Pvl log;
  QString cubeFileName = prefix.path() + "/photometTEMP.cub";
  QVector<QString> args = {"from=data/photomet/EN0213240738M.lev1.cub", "to=" + cubeFileName, 
                          "frompvl=data/photomet/pvlfile.txt", "normname=albedo", "phtname=hapkehen", "incref=0.0",
                          "incmat=0.0", "thresh=30.0", "albedo=1.0", "wh=0.5", "hh=0.0"};
  UserInterface options(APP_XML, args);

  photomet(options, &log);

  Cube cube(cubeFileName.toStdString());

  std::unique_ptr<Histogram> hist (cube.histogram(0));
  EXPECT_NEAR(hist->Average(),  0.12118244647528577, .000001);
  EXPECT_NEAR(hist->Sum(), 31581.115011031274, .000001);
  EXPECT_EQ(hist->ValidPixels(), 260608);
  EXPECT_NEAR(hist->StandardDeviation(), 0.054088426877270515, .000001);
}

TEST(Photomet, FunctionalTestPhotometMessMaxemiMaxinc) {
  QTemporaryDir prefix;
  Pvl log;
  QString cubeFileName = prefix.path() + "/photometTEMP.cub";
  QVector<QString> args = {"from=data/photomet/EN0213240738M.lev1.cub", "to=" + cubeFileName, 
                          "frompvl=data/photomet/pvlfile.txt", "maxe=75.0", "maxi=75.0", "normname=albedo", 
                          "phtname=hapkehen", "incref=0.0", "incmat=0.0", "thresh=30.0", 
                          "albedo=1.0", "wh=0.5", "hh=0.0"};
  UserInterface options(APP_XML, args);

  photomet(options, &log);

  Cube cube(cubeFileName.toStdString());

  std::unique_ptr<Histogram> hist (cube.histogram(0));
  EXPECT_NEAR(hist->Average(), -1.7976931348623149e+308, .000001);
  EXPECT_EQ(hist->Sum(), 0);
  EXPECT_EQ(hist->ValidPixels(), 0);
  EXPECT_NEAR(hist->StandardDeviation(), -1.7976931348623149e+308, .000001);
}

TEST(Photomet, FunctionalTestPhotometMessMaxemission) {
  QTemporaryDir prefix;
  Pvl log;
  QString cubeFileName = prefix.path() + "/photometTEMP.cub";
  QVector<QString> args = {"from=data/photomet/EN0213240738M.lev1.cub", "to=" + cubeFileName, 
                          "frompvl=data/photomet/pvlfile.txt", "maxe=75.0", "normname=albedo", 
                          "phtname=hapkehen", "incref=0.0", "incmat=0.0", "thresh=30.0", 
                          "albedo=1.0", "wh=0.5", "hh=0.0"};
  UserInterface options(APP_XML, args);

  photomet(options, &log);

  Cube cube(cubeFileName.toStdString());

  std::unique_ptr<Histogram> hist (cube.histogram(0));
  EXPECT_NEAR(hist->Average(), 0.12118244647528577, .000001);
  EXPECT_NEAR(hist->Sum(), 31581.115011031274, .000001);
  EXPECT_EQ(hist->ValidPixels(), 260608);
  EXPECT_NEAR(hist->StandardDeviation(), 0.054088426877270515, .000001);
}

TEST(Photomet, FunctionalTestPhotometMessMaxincidence) {
  QTemporaryDir prefix;
  Pvl log;
  QString cubeFileName = prefix.path() + "/photometTEMP.cub";
  QVector<QString> args = {"from=data/photomet/EN0213240738M.lev1.cub", "to=" + cubeFileName, 
                          "frompvl=data/photomet/pvlfile.txt", "maxincidence=75", "normname=albedo", 
                          "phtname=hapkehen", "incref=0.0", "incmat=0.0", "thresh=30.0", 
                          "albedo=1.0", "wh=0.5", "hh=0.0"};
  UserInterface options(APP_XML, args);

  photomet(options, &log);

  Cube cube(cubeFileName.toStdString());

  std::unique_ptr<Histogram> hist (cube.histogram(0));
  EXPECT_NEAR(hist->Average(), -1.7976931348623149e+308, .000001);
  EXPECT_EQ(hist->Sum(), 0);
  EXPECT_EQ(hist->ValidPixels(),  0);
  EXPECT_NEAR(hist->StandardDeviation(), -1.7976931348623149e+308, .000001);
}

TEST(Photomet, FunctionalTestPhotometMessBackplane) {
  QTemporaryDir prefix;
  Pvl log;
  QString cubeFileName = prefix.path() + "/photometTEMP.cub";
  QVector<QString> args = {"from=data/photomet/EN0213240738M.lev1.cub", "to=" + cubeFileName, 
                           "phase_angle_file=data/photomet/EN0213240738M.lev1.cub",
                           "incidence_angle_file=data/photomet/EN0213240738M.lev1.cub",
                           "emission_angle_file=data/photomet/EN0213240738M.lev1.cub",
                           "anglesource=backplane", "normname=albedo", "phtname=hapkehen", "incref=0.0",
                          "incmat=0.0", "thresh=30.0", "albedo=1.0", "wh=0.5", "hh=0.0", "theta=90",
                          "B0=0", "hg1=0.235185185", "hg2=0.221193416", "hh=0.378600823"};
  UserInterface options(APP_XML, args);

  photomet(options, &log);

  Cube cube(cubeFileName.toStdString());

  std::unique_ptr<Histogram> hist (cube.histogram(0));
  EXPECT_NEAR(hist->Average(),  -1.7976931348623149e+308, .000001);
  EXPECT_EQ(hist->Sum(), 0);
  EXPECT_EQ(hist->ValidPixels(), 0);
  EXPECT_NEAR(hist->StandardDeviation(), -1.7976931348623149e+308, .000001);
}

TEST(Photomet, FunctionalTestPhotometTestDemWarningMixed) {
  QTemporaryDir prefix;
  Pvl log;
  QString cubeFileName = prefix.path() + "/photometTEMP.cub";
  QVector<QString> args = {"from=data/photomet/cropped.cub", "to=" + cubeFileName, 
                          "maxemission=87.0", "anglesource=dem", 
                          "phtname=minnaert", "k=0.5", "normname=mixed", "incref=0", 
                          "incmat=80", "thresh=30", "albedo=1.0"};
  UserInterface options(APP_XML, args);

  photomet(options, &log);

  PvlGroup warning = log.findGroup("Warnings");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, warning["Warning"], "The MIXED Normalized model is not recommended for use with the DEM Angle Source option");
}

TEST(Photomet, FunctionalTestPhotometTestDemWarningTopo) {
  QTemporaryDir prefix;
  Pvl log;
  QString cubeFileName = prefix.path() + "/photometTEMP.cub";
  QVector<QString> args = {"from=data/photomet/cropped.cub", "to=" + cubeFileName, 
                          "maxemission=87.0", "anglesource=dem", 
                          "phtname=minnaert", "k=0.5", "normname=topo", "incref=0", 
                          "thresh=30", "albedo=1.0"};
  UserInterface options(APP_XML, args);

  photomet(options, &log);

  PvlGroup warning = log.findGroup("Warnings");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, warning["Warning"], "The TOPO Normalized model is not recommended for use with the DEM Angle Source option");
}

TEST(Photomet, FunctionalTestPhotometTestAngleSourceDemUseDemFalse) {
  QTemporaryDir prefix;
  Pvl log;
  QString cubeFileName = prefix.path() + "/photometTEMP.cub";
  QVector<QString> args = {"from=data/photomet/cropped.cub", "to=" + cubeFileName, 
                          "maxemission=87.0", "anglesource=dem", 
                          "phtname=minnaert", "k=0.5", "normname=albedo", "incref=0", 
                          "thresh=30","incmat=80", "albedo=1.0"};
  UserInterface options(APP_XML, args);

  photomet(options, &log);

  Cube cube(cubeFileName.toStdString());

  std::unique_ptr<Histogram> hist (cube.histogram(0));
  EXPECT_NEAR(hist->Average(), 14792.765466992187, .000001);
  EXPECT_NEAR(hist->Sum(), 147927654.66992188, .000001);
  EXPECT_EQ(hist->ValidPixels(),  10000);
  EXPECT_NEAR(hist->StandardDeviation(), 658.03792272969724, .000001);
}

TEST(Photomet, FunctionalTestPhotometTestAngleSourceEllipsoidUseDemFalse) {
  QTemporaryDir prefix;
  Pvl log;
  QString cubeFileName = prefix.path() + "/photometTEMP.cub";
  QVector<QString> args = {"from=data/photomet/cropped.cub", "to=" + cubeFileName, 
                          "maxemission=87.0", "phtname=minnaert", "k=0.5", "normname=albedo", "incref=0", 
                          "thresh=30","incmat=80", "albedo=1.0"};
  UserInterface options(APP_XML, args);

  photomet(options, &log);

  Cube cube(cubeFileName.toStdString());

  std::unique_ptr<Histogram> hist (cube.histogram(0));
  EXPECT_NEAR(hist->Average(), 14749.07615234375, .000001);
  EXPECT_NEAR(hist->Sum(), 147490761.5234375, .000001);
  EXPECT_EQ(hist->ValidPixels(),  10000);
  EXPECT_NEAR(hist->StandardDeviation(), 563.58172889222919, .000001);
}

TEST(Photomet, FunctionalTestPhotometTestAngleSourceEllipsoidUseDemTrue) {
  QTemporaryDir prefix;
  Pvl log;
  QString cubeFileName = prefix.path() + "/photometTEMP.cub";
  QVector<QString> args = {"from=data/photomet/cropped.cub", "to=" + cubeFileName, 
                          "maxemission=87.0", "phtname=minnaert", "k=0.5", "normname=albedo", "incref=0", 
                          "thresh=30","incmat=80", "albedo=1.0", "usedem=yes"};
  UserInterface options(APP_XML, args);

  photomet(options, &log);

  Cube cube(cubeFileName.toStdString());

  std::unique_ptr<Histogram> hist (cube.histogram(0));
  EXPECT_NEAR(hist->Average(), 14749.07615234375, .000001);
  EXPECT_NEAR(hist->Sum(), 147490761.5234375, .000001);
  EXPECT_EQ(hist->ValidPixels(), 10000);
  EXPECT_NEAR(hist->StandardDeviation(), 563.58172889222919, .000001);
}

TEST(Photomet, FunctionalTestPhotometTestAngleSourceDemUseDemTrue) {
  QTemporaryDir prefix;
  Pvl log;
  QString cubeFileName = prefix.path() + "/photometTEMP.cub";
  QVector<QString> args = {"from=data/photomet/cropped.cub", "to=" + cubeFileName, 
                          "maxemission=87.0", "phtname=minnaert", "anglesource=dem", "k=0.5", "normname=albedo", "incref=0", 
                          "thresh=30","incmat=80", "albedo=1.0", "usedem=yes"};
  UserInterface options(APP_XML, args);

  photomet(options, &log);

  Cube cube(cubeFileName.toStdString());

  std::unique_ptr<Histogram> hist (cube.histogram(0));
  EXPECT_NEAR(hist->Average(), 14792.765466992187, .000001);
  EXPECT_NEAR(hist->Sum(), 147927654.66992188, .000001);
  EXPECT_EQ(hist->ValidPixels(),  10000);
  EXPECT_NEAR(hist->StandardDeviation(), 658.03792272969724, .000001);
}