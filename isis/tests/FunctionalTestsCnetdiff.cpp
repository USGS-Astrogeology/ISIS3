#include <QFileInfo> 
#include <QString>
#include <QTemporaryDir>
#include <QTextStream>

#include "Progress.h"
#include "Pvl.h"
#include "PvlGroup.h"

#include "cnetdiff.h"
#include "ControlMeasure.h"
#include "ControlPoint.h"
#include "ControlNet.h"
#include "SurfacePoint.h"
#include "TempFixtures.h"

#include "gtest/gtest.h"

using namespace Isis;

static QString APP_XML = FileName("$ISISROOT/bin/xml/cnetdiff.xml").expanded();

class CompareNetsReportFirstDiff : public TempTestingFiles {
  protected:
    QString cnetFile1;
    QString cnetFile2;
    QString diffsFile;

    void SetUp() override {
      TempTestingFiles::SetUp();

      cnetFile1 = tempDir.path() + "/cnet1.net";
      cnetFile2 = tempDir.path() + "/cnet2.net";

      diffsFile = tempDir.path() + "/diffs.txt";

      // setup net1
      ControlNet net1;

      net1.SetNetworkId("Test");
      net1.SetTarget("Mars");
      net1.SetUserName("TSucharski");
      net1.SetCreatedDate("2010-07-10T12:50:15");
      net1.SetModifiedDate("2010-07-10T12:50:55");
      net1.SetDescription("UnitTest of ControlNetwork");

      ControlPoint *cp1a = new ControlPoint("T0000");
      cp1a->SetType(ControlPoint::Fixed);
      cp1a->SetChooserName("autoseed");
      cp1a->SetAprioriSurfacePointSource(ControlPoint::SurfacePointSource::Basemap);
      cp1a->SetAprioriSurfacePointSourceFile("/work1/tsucharski/basemap.cub");
      cp1a->SetAprioriRadiusSource(ControlPoint::RadiusSource::DEM);
      cp1a->SetAprioriRadiusSourceFile("$base/dems/molaMarsPlanetaryRadius0003.cub");

      SurfacePoint surfacePt1(Displacement(-424.024048, Displacement::Meters),
                             Displacement(734.4311949, Displacement::Meters),
                             Displacement(529.919264, Displacement::Meters),
                             Distance(10, Distance::Meters),
                             Distance(50, Distance::Meters),
                             Distance(20, Distance::Meters));
      
      // set apriori and adjusted surface points to the same values here
      cp1a->SetAprioriSurfacePoint(surfacePt1);
      cp1a->SetAdjustedSurfacePoint(surfacePt1);

      ControlMeasure *measure1aid0 = new ControlMeasure();
      measure1aid0->SetCubeSerialNumber("id0");
      measure1aid0->SetCoordinate(0.0, 0.0);
      measure1aid0->SetType(ControlMeasure::Candidate);
      measure1aid0->SetChooserName("cnetref");
      measure1aid0->SetDateTime("2010-08-27T17:10:06");
      measure1aid0->SetEditLock(true);    

      ControlMeasure *measure1aid1 = new ControlMeasure();
      measure1aid1->SetCubeSerialNumber("id1");
      measure1aid1->SetCoordinate(1.0, 2.0);
      measure1aid1->SetDiameter(15.0);
      measure1aid1->SetType(ControlMeasure::Candidate);
      measure1aid1->SetIgnored(true);
      measure1aid1->SetAprioriSample(2.0);
      measure1aid1->SetAprioriLine(5.0);
      measure1aid1->SetSampleSigma(0.01);
      measure1aid1->SetLineSigma(0.21);
      measure1aid1->SetResidual(-3.0, 4.0);
      measure1aid1->SetChooserName("autoseed");
      measure1aid1->SetDateTime("2010-08-27T17:10:06");

      cp1a->Add(measure1aid0);
      cp1a->Add(measure1aid1);
      cp1a->SetDateTime("2010-08-27T17:10:06");
      cp1a->SetEditLock(true);
      net1.AddPoint(cp1a);

      ControlPoint *cp1b = new ControlPoint("T0001");
      cp1b->SetType(ControlPoint::Free);
      cp1b->SetChooserName("autoseed");
      cp1b->SetIgnored(true);

      ControlMeasure *measure1bid0 = new ControlMeasure();
      measure1bid0->SetCubeSerialNumber("id0");
      measure1bid0->SetCoordinate(0.0, 0.0);
      measure1bid0->SetType(ControlMeasure::Candidate);
      measure1bid0->SetChooserName("cnetref");
      measure1bid0->SetDateTime("2010-08-27T17:10:06");
      measure1bid0->SetEditLock(true);    

      ControlMeasure *measure1bid1 = new ControlMeasure();
      measure1bid1->SetCubeSerialNumber("id1");
      measure1bid1->SetCoordinate(1.0, 2.0);
      measure1bid1->SetDiameter(15.0);
      measure1bid1->SetType(ControlMeasure::Candidate);
      measure1bid1->SetIgnored(true);
      measure1bid1->SetAprioriSample(2.0);
      measure1bid1->SetAprioriLine(5.0);
      measure1bid1->SetSampleSigma(0.01);
      measure1bid1->SetLineSigma(0.21);
      measure1bid1->SetResidual(-3.0, 4.0);
      measure1bid1->SetChooserName("autoseed");
      measure1bid1->SetDateTime("2010-08-27T17:10:06");

      cp1b->Add(measure1bid0);
      cp1b->Add(measure1bid1);
      cp1b->SetDateTime("2010-08-27T17:10:06");
      net1.AddPoint(cp1b);

      ControlPoint *cp1c = new ControlPoint("T0003");
      cp1c->SetType(ControlPoint::Free);
      cp1c->SetChooserName("autoseed");

      ControlMeasure *measure1cid0 = new ControlMeasure();
      measure1cid0->SetCubeSerialNumber("id0");
      measure1cid0->SetCoordinate(0.0, 0.0);
      measure1cid0->SetType(ControlMeasure::Candidate);
      measure1cid0->SetChooserName("cnetref");
      measure1cid0->SetDateTime("2010-08-27T17:10:06");
      measure1cid0->SetEditLock(true);    

      ControlMeasure *measure1cid1 = new ControlMeasure();
      measure1cid1->SetCubeSerialNumber("id1");
      measure1cid1->SetCoordinate(1.0, 2.0);
      measure1cid1->SetDiameter(15.0);
      measure1cid1->SetType(ControlMeasure::Candidate);
      measure1cid1->SetIgnored(true);
      measure1cid1->SetAprioriSample(2.0);
      measure1cid1->SetAprioriLine(5.0);
      measure1cid1->SetSampleSigma(0.01);
      measure1cid1->SetLineSigma(0.21);
      measure1cid1->SetResidual(-3.0, 4.0);
      measure1cid1->SetChooserName("autoseed");
      measure1cid1->SetDateTime("2010-08-27T17:10:06");

      cp1c->Add(measure1cid0);
      cp1c->Add(measure1cid1);
      cp1c->SetDateTime("2010-08-27T17:10:06");
      net1.AddPoint(cp1c);

      ControlPoint *cp1d = new ControlPoint("T0004");
      cp1d->SetType(ControlPoint::Free);
      cp1d->SetChooserName("autoseed");

      ControlMeasure *measure1did0 = new ControlMeasure();
      measure1did0->SetCubeSerialNumber("id0");
      measure1did0->SetCoordinate(0.0, 0.0);
      measure1did0->SetType(ControlMeasure::Candidate);
      measure1did0->SetChooserName("cnetref");
      measure1did0->SetDateTime("2010-08-27T17:10:06");
      measure1did0->SetEditLock(true);    

      ControlMeasure *measure1did1 = new ControlMeasure();
      measure1did1->SetCubeSerialNumber("id1");
      measure1did1->SetCoordinate(1.0, 2.0);
      measure1did1->SetDiameter(15.0);
      measure1did1->SetType(ControlMeasure::Candidate);
      measure1did1->SetIgnored(true);
      measure1did1->SetAprioriSample(2.0);
      measure1did1->SetAprioriLine(5.0);
      measure1did1->SetSampleSigma(0.01);
      measure1did1->SetLineSigma(0.21);
      measure1did1->SetResidual(-3.0, 4.0);
      measure1did1->SetChooserName("autoseed");
      measure1did1->SetDateTime("2010-08-27T17:10:06");

      cp1d->Add(measure1did0);
      cp1d->Add(measure1did1);
      cp1d->SetDateTime("2010-08-27T17:10:06");
      net1.AddPoint(cp1d);

      net1.Write(cnetFile1);

      // construct net2 from net1 and change some items
      ControlNet net2(net1);

      ControlPoint *point2a = net2["T0000"];
      ControlMeasure* measure2aid1 = point2a->GetMeasure("id1");
      measure2aid1->SetResidual(-1.0, 4.0);
      measure2aid1->SetChooserName("autoseed");
      measure2aid1->SetDateTime("2010-08-27T17:10:06");

      ControlPoint *point2b = net2["T0001"];
      ControlMeasure* measure2bid1 = point2b->GetMeasure("id1");
      measure2bid1->SetResidual(-1.0, 4.0);
      measure2bid1->SetChooserName("autoseed");
      measure2bid1->SetDateTime("2010-08-27T17:10:06");

      ControlPoint *point2c = net2["T0003"];
      ControlMeasure* measure2cid1 = point2c->GetMeasure("id1");
      measure2cid1->SetResidual(-1.0, 4.0);
      measure2cid1->SetChooserName("autoseed");
      measure2cid1->SetDateTime("2010-08-27T17:10:06");

      ControlPoint *point2d = net2["T0004"];
      ControlMeasure* measure2did1 = point2d->GetMeasure("id1");
      measure2did1->SetResidual(-1.0, 4.0);
      measure2did1->SetChooserName("autoseed");
      measure2did1->SetDateTime("2010-08-27T17:10:06");
      
      net2.Write(cnetFile2);

      // set up pvl difference file
      Pvl diffs;
      PvlGroup toleranceGroup("Tolerances");
      toleranceGroup.addKeyword(PvlKeyword("SampleResidual", "1.5"));
      diffs.addGroup(toleranceGroup);
      diffs.write(diffsFile.toStdString());
    }
};

class CompareNetsReportFull : public TempTestingFiles {
  protected:
    QString cnetFile1;
    QString cnetFile2;

    QString diffsFile;

    void SetUp() override {
      TempTestingFiles::SetUp();

      cnetFile1 = tempDir.path() + "/cnet1.net";
      cnetFile2 = tempDir.path() + "/cnet2.net";

      diffsFile = tempDir.path() + "/diffs.txt";

      // setup net1
      ControlNet net1;

      net1.SetNetworkId("Test");
      net1.SetTarget("Mars");
      net1.SetUserName("TSucharski");
      net1.SetCreatedDate("2010-07-10T12:50:15");
      net1.SetModifiedDate("2010-07-10T12:50:55");
      net1.SetDescription("UnitTest of ControlNetwork");

      ControlPoint *cp3a = new ControlPoint("T0001");
      cp3a->SetType(ControlPoint::Free);
      cp3a->SetChooserName("autoseed");
      cp3a->SetIgnored(true);

      ControlMeasure *measure3aid0 = new ControlMeasure();
      measure3aid0->SetCubeSerialNumber("id0");
      measure3aid0->SetCoordinate(0.0, 0.0);
      measure3aid0->SetType(ControlMeasure::Candidate);
      measure3aid0->SetChooserName("cnetref");
      measure3aid0->SetDateTime("2010-08-27T17:10:06");
      measure3aid0->SetEditLock(true);    

      ControlMeasure *measure3aid1 = new ControlMeasure();
      measure3aid1->SetCubeSerialNumber("id1");
      measure3aid1->SetCoordinate(1.0, 2.0);
      measure3aid1->SetDiameter(15.0);
      measure3aid1->SetType(ControlMeasure::Candidate);
      measure3aid1->SetIgnored(true);
      measure3aid1->SetAprioriSample(2.0);
      measure3aid1->SetAprioriLine(5.0);
      measure3aid1->SetSampleSigma(0.01);
      measure3aid1->SetLineSigma(0.21);
      measure3aid1->SetResidual(-3.0, 4.0);
      measure3aid1->SetChooserName("autoseed");
      measure3aid1->SetDateTime("2010-08-27T17:10:06");      

      cp3a->Add(measure3aid0);
      cp3a->Add(measure3aid1);
      cp3a->SetDateTime("2010-08-27T17:10:06");
      net1.AddPoint(cp3a);

      ControlPoint *cp3b = new ControlPoint("T0003");
      cp3b->SetType(ControlPoint::Free);
      cp3b->SetChooserName("autoseed");
      cp3b->SetAprioriSurfacePointSource(ControlPoint::SurfacePointSource::Basemap);
      cp3b->SetAprioriSurfacePointSourceFile("/work1/tsucharski/basemap.cub");
      cp3b->SetAprioriRadiusSource(ControlPoint::RadiusSource::DEM);
      cp3b->SetAprioriRadiusSourceFile("$base/dems/molaMarsPlanetaryRadius0003.cub");

      SurfacePoint surfacePt1(Displacement(-424.024048, Displacement::Meters),
                             Displacement(734.4311949, Displacement::Meters),
                             Displacement(529.919264, Displacement::Meters),
                             Distance(10, Distance::Meters),
                             Distance(50, Distance::Meters),
                             Distance(20, Distance::Meters));

      cp3b->SetAprioriSurfacePoint(surfacePt1);

      SurfacePoint surfacePt2(Displacement(-423.024048, Displacement::Meters),
                             Displacement(754.4311949, Displacement::Meters),
                             Displacement(523.919264, Displacement::Meters),
                             Distance(10, Distance::Meters),
                             Distance(50, Distance::Meters),
                             Distance(20, Distance::Meters));

      cp3b->SetAdjustedSurfacePoint(surfacePt2);

      ControlMeasure *measure3bid0 = new ControlMeasure();
      measure3bid0->SetCubeSerialNumber("id0");
      measure3bid0->SetCoordinate(0.0, 0.0);
      measure3bid0->SetType(ControlMeasure::Candidate);
      measure3bid0->SetChooserName("cnetref");
      measure3bid0->SetDateTime("2010-08-27T17:10:06");
      measure3bid0->SetEditLock(true);    

      ControlMeasure *measure3bid1 = new ControlMeasure();
      measure3bid1->SetCubeSerialNumber("id1");
      measure3bid1->SetCoordinate(1.0, 2.0);
      measure3bid1->SetDiameter(15.0);
      measure3bid1->SetType(ControlMeasure::Candidate);
      measure3bid1->SetIgnored(true);
      measure3bid1->SetAprioriSample(2.0);
      measure3bid1->SetAprioriLine(5.0);
      measure3bid1->SetSampleSigma(0.01);
      measure3bid1->SetLineSigma(0.21);
      measure3bid1->SetResidual(-3.0, 4.0);
      measure3bid1->SetChooserName("autoseed");
      measure3bid1->SetDateTime("2010-08-27T17:10:06");

      cp3b->Add(measure3bid0);
      cp3b->Add(measure3bid1);
      cp3b->SetDateTime("2010-08-27T17:10:06");
      net1.AddPoint(cp3b);

      ControlPoint *cp3c = new ControlPoint("T0004");
      cp3c->SetType(ControlPoint::Free);
      cp3c->SetChooserName("autoseed");
      cp3c->SetAprioriSurfacePoint(surfacePt1);
      cp3c->SetAdjustedSurfacePoint(surfacePt1);
      cp3c->SetAprioriRadiusSource(ControlPoint::RadiusSource::DEM);
      cp3c->SetAprioriRadiusSourceFile("$base/dems/molaMarsPlanetaryRadius0003.cub");

      ControlMeasure *measure3cid0 = new ControlMeasure();
      measure3cid0->SetCubeSerialNumber("id0");
      measure3cid0->SetCoordinate(4.0, 3.0);
      measure3cid0->SetType(ControlMeasure::Candidate);
      measure3cid0->SetResidual(1.0, -4.0);
      measure3cid0->SetAprioriSample(10.0);
      measure3cid0->SetAprioriLine(20.0);
      measure3cid0->SetChooserName("cnetref");
      measure3cid0->SetDateTime("2010-08-27T17:10:06");
      measure3cid0->SetEditLock(true);    

      ControlMeasure *measure3cid1 = new ControlMeasure();
      measure3cid1->SetCubeSerialNumber("id1");
      measure3cid1->SetCoordinate(1.0, 2.0);
      measure3cid1->SetDiameter(15.0);
      measure3cid1->SetType(ControlMeasure::Candidate);
      measure3cid1->SetIgnored(true);
      measure3cid1->SetAprioriSample(2.0);
      measure3cid1->SetAprioriLine(5.0);
      measure3cid1->SetSampleSigma(0.01);
      measure3cid1->SetLineSigma(0.21);
      measure3cid1->SetResidual(-3.0, 4.0);
      measure3cid1->SetChooserName("autoseed");
      measure3cid1->SetDateTime("2010-08-27T17:10:06");

      cp3c->Add(measure3cid0);
      cp3c->Add(measure3cid1);
      cp3c->SetDateTime("2010-08-27T17:10:06");
      cp3c->SetEditLock(true);
      net1.AddPoint(cp3c);

      ControlPoint *cp3d = new ControlPoint("T0000");
      cp3d->SetType(ControlPoint::Fixed);
      cp3d->SetChooserName("autoseed");

      ControlMeasure *measure3did0 = new ControlMeasure();
      measure3did0->SetCubeSerialNumber("id0");
      measure3did0->SetCoordinate(2.0, 3.0);
      measure3did0->SetType(ControlMeasure::Candidate);
      measure3did0->SetChooserName("cnetref");
      measure3did0->SetDateTime("2010-08-27T17:10:06");
      measure3did0->SetEditLock(true);    

      ControlMeasure *measure3did1 = new ControlMeasure();
      measure3did1->SetCubeSerialNumber("id1");
      measure3did1->SetCoordinate(1.0, 2.0);
      measure3did1->SetDiameter(15.0);
      measure3did1->SetType(ControlMeasure::Candidate);
      measure3did1->SetIgnored(true);
      measure3did1->SetAprioriSample(2.0);
      measure3did1->SetAprioriLine(5.0);
      measure3did1->SetSampleSigma(0.01);
      measure3did1->SetLineSigma(0.21);
      measure3did1->SetResidual(-3.0, 4.0);
      measure3did1->SetChooserName("autoseed");
      measure3did1->SetDateTime("2010-08-27T17:10:06");

      cp3d->Add(measure3did0);
      cp3d->Add(measure3did1);
      cp3d->SetDateTime("2010-08-27T17:10:06");
      cp3d->SetEditLock(true);
      net1.AddPoint(cp3d);

      net1.Write(cnetFile1);

      // setup net2
      ControlNet net2;

      net2.SetNetworkId("Test2");
      net2.SetTarget("Moon");
      net2.SetUserName("TSucharski");
      net2.SetCreatedDate("2010-07-10T12:50:15");
      net2.SetModifiedDate("2010-07-10T12:50:55");
      net2.SetDescription("UnitTest of ControlNetwork");

      ControlPoint *point4a = new ControlPoint("T0000");
      point4a->SetType(ControlPoint::Fixed);
      point4a->SetChooserName("autoseed");

      ControlMeasure *measure4aid0 = new ControlMeasure();
      measure4aid0->SetCubeSerialNumber("id0");
      measure4aid0->SetCoordinate(1.0, 2.0);
      measure4aid0->SetType(ControlMeasure::Candidate);
      measure4aid0->SetChooserName("cnetref");
      measure4aid0->SetDateTime("2010-08-27T17:10:06");
      measure4aid0->SetEditLock(true);    

      ControlMeasure *measure4aid1 = new ControlMeasure();
      measure4aid1->SetCubeSerialNumber("id1");
      measure4aid1->SetCoordinate(1.0, 2.0);
      measure4aid1->SetDiameter(15.0);
      measure4aid1->SetType(ControlMeasure::Candidate);
      measure4aid1->SetIgnored(true);
      measure4aid1->SetAprioriSample(2.0);
      measure4aid1->SetAprioriLine(5.0);
      measure4aid1->SetSampleSigma(0.01);
      measure4aid1->SetLineSigma(0.21);
      measure4aid1->SetResidual(-1.0, 4.0);
      measure4aid1->SetChooserName("autoseed");
      measure4aid1->SetDateTime("2010-08-27T17:10:06");

      point4a->Add(measure4aid0);
      point4a->Add(measure4aid1);
      point4a->SetDateTime("2010-08-27T17:10:06");
      net2.AddPoint(point4a);

      ControlPoint *point4b = new ControlPoint("T0002");
      point4b->SetType(ControlPoint::Free);
      point4b->SetChooserName("autoseed");
      point4b->SetIgnored(true);

      ControlMeasure *measure4bid0 = new ControlMeasure();
      measure4bid0->SetCubeSerialNumber("id0");
      measure4bid0->SetCoordinate(0.0, 0.0);
      measure4bid0->SetType(ControlMeasure::Candidate);
      measure4bid0->SetChooserName("cnetref");
      measure4bid0->SetDateTime("2010-08-27T17:10:06");
      measure4bid0->SetEditLock(true);    

      ControlMeasure *measure4bid1 = new ControlMeasure();
      measure4bid1->SetCubeSerialNumber("id1");
      measure4bid1->SetCoordinate(1.0, 2.0);
      measure4bid1->SetDiameter(15.0);
      measure4bid1->SetType(ControlMeasure::Candidate);
      measure4bid1->SetIgnored(true);
      measure4bid1->SetAprioriSample(2.0);
      measure4bid1->SetAprioriLine(5.0);
      measure4bid1->SetSampleSigma(0.01);
      measure4bid1->SetLineSigma(0.21);
      measure4bid1->SetResidual(-1.0, 4.0);
      measure4bid1->SetChooserName("autoseed");
      measure4bid1->SetDateTime("2010-08-27T17:10:06");

      point4b->Add(measure4bid0);
      point4b->Add(measure4bid1);
      point4b->SetDateTime("2010-08-27T17:10:06");
      net2.AddPoint(point4b);

      ControlPoint *point4c = new ControlPoint("T0004");
      point4c->SetType(ControlPoint::Free);
      point4c->SetAprioriSurfacePointSource(ControlPoint::SurfacePointSource::Basemap);
      point4c->SetAprioriSurfacePointSourceFile("/work1/tsucharski/basemap.cub");
      point4c->SetAprioriRadiusSource(ControlPoint::RadiusSource::DEM);
      point4c->SetAprioriRadiusSourceFile("$base/dems/molaMarsPlanetaryRadius0003.cub");
      point4c->SetAprioriSurfacePoint(surfacePt1);
      point4c->SetAdjustedSurfacePoint(surfacePt2);
      point4c->SetChooserName("autoseed");

      ControlMeasure *measure4cid0 = new ControlMeasure();
      measure4cid0->SetCubeSerialNumber("id0");
      measure4cid0->SetCoordinate(1.0, 2.0);
      measure4cid0->SetType(ControlMeasure::Candidate);
      measure4cid0->SetResidual(1.0, -4.0);
      measure4cid0->SetSampleSigma(0.01);
      measure4cid0->SetLineSigma(0.21);
      measure4cid0->SetResidual(-10.0, 4.0);
      measure4cid0->SetChooserName("cnetref");
      measure4cid0->SetDateTime("2010-08-27T17:10:06");
      measure4cid0->SetEditLock(true);    

      ControlMeasure *measure4cid2 = new ControlMeasure();
      measure4cid2->SetCubeSerialNumber("id2");
      measure4cid2->SetCoordinate(1.0, 2.0);
      measure4cid2->SetDiameter(15.0);
      measure4cid2->SetType(ControlMeasure::Candidate);
      measure4cid2->SetIgnored(true);
      measure4cid2->SetAprioriSample(2.0);
      measure4cid2->SetAprioriLine(5.0);
      measure4cid2->SetSampleSigma(0.01);
      measure4cid2->SetLineSigma(0.21);
      measure4cid2->SetResidual(-1.0, 4.0);
      measure4cid2->SetChooserName("autoseed");
      measure4cid2->SetDateTime("2010-08-27T17:10:06");

      ControlMeasure *measure4cid3 = new ControlMeasure();
      measure4cid3->SetCubeSerialNumber("id3");
      measure4cid3->SetCoordinate(1.0, 2.0);
      measure4cid3->SetDiameter(15.0);
      measure4cid3->SetType(ControlMeasure::Candidate);
      measure4cid3->SetIgnored(true);
      measure4cid3->SetAprioriSample(2.0);
      measure4cid3->SetAprioriLine(5.0);
      measure4cid3->SetSampleSigma(0.01);
      measure4cid3->SetLineSigma(0.21);
      measure4cid3->SetResidual(-1.0, 4.0);
      measure4cid3->SetChooserName("autoseed");
      measure4cid3->SetDateTime("2010-08-27T17:10:06");

      point4c->Add(measure4cid0);
      point4c->Add(measure4cid2);
      point4c->Add(measure4cid3);
      point4c->SetDateTime("2010-08-27T17:10:06");
      net2.AddPoint(point4c);

      net2.Write(cnetFile2);

      // set up pvl difference file
      Pvl diffs;
      PvlGroup ignoreKeysGroup("IgnoreKeys");
      ignoreKeysGroup.addKeyword(PvlKeyword("AprioriSample", "true"));
      ignoreKeysGroup.addKeyword(PvlKeyword("AprioriLine", "true"));
      ignoreKeysGroup.addKeyword(PvlKeyword("SampleSigma", "true"));
      ignoreKeysGroup.addKeyword(PvlKeyword("LineSigma", "true"));
      ignoreKeysGroup.addKeyword(PvlKeyword("NetworkId", "true"));
      ignoreKeysGroup.addKeyword(PvlKeyword("AprioriXYZSource", "true"));
      ignoreKeysGroup.addKeyword(PvlKeyword("EditLock", "true"));
      ignoreKeysGroup.addKeyword(PvlKeyword("AdjustedZ", "true"));
      diffs.addGroup(ignoreKeysGroup);


      PvlGroup toleranceGroup("Tolerances");
      toleranceGroup.addKeyword(PvlKeyword("Sample", "1"));
      toleranceGroup.addKeyword(PvlKeyword("Line", "1"));
      toleranceGroup.addKeyword(PvlKeyword("SampleResidual", "5"));
      toleranceGroup.addKeyword(PvlKeyword("AdjustedX", "3"));
      toleranceGroup.addKeyword(PvlKeyword("AdjustedY", "3"));
      diffs.addGroup(toleranceGroup);
      diffs.write(diffsFile.toStdString());
    }
};

/**
   * CompareNetsReportFirst
   * 
   * Runs 4 comparisons between binary control nets (reporting only
   * the first difference)...
   *  a) cnet1 vs cnet1, should be identical
   *  b) cnet2 vs cnet2, should be identical
   *  c) cnet1 vs cnet2
   *  d) cnet1 vs cnet2 with sample residual tolerance of 1.5
   *     given in the input file diffs1.txt
   */
TEST_F(CompareNetsReportFirstDiff, FunctionalTestCnetdiffReportFirst) {
  QVector<QString> args = {"from=" + cnetFile1,
                           "from2=" + cnetFile1,
                           "to=" + tempDir.path() + "/compareSame1.txt"
                          };

  UserInterface ui1(APP_XML, args);

  Pvl log;

  try {
    log = cnetdiff(ui1);
  }
  catch(IException &e) {
    FAIL() << e.toString().toStdString().c_str() << std::endl;
  }

    // read back CompareSame1 output file
  Pvl comparePvlSame1;
  try {
    comparePvlSame1.read(tempDir.path().toStdString() + "/compareSame1.txt");
  }
  catch (IException &e) {
    FAIL() << "Unable to open error log pvl file: " << e.what() << std::endl;
  }

  PvlGroup &results = comparePvlSame1.findGroup("Results", Pvl::Traverse);
  EXPECT_EQ(results["Compare"][0], "Identical");

  // cnet2 vs cnet2
  args = {"from=" + cnetFile2,
          "from2=" + cnetFile2,
          "to=" + tempDir.path() + "/compareSame2.txt"
         };

  UserInterface ui2(APP_XML, args);

  try {
    log = cnetdiff(ui2);
  }
  catch(IException &e) {
    FAIL() << e.toString().toStdString().c_str() << std::endl;
  }

  // read back CompareSame2 output file
  Pvl comparePvlSame2;
  try {
    comparePvlSame2.read(tempDir.path().toStdString() + "/compareSame2.txt");
  }
  catch (IException &e) {
    FAIL() << "Unable to open error log pvl file: " << e.what() << std::endl;
  }

  results = comparePvlSame2.findGroup("Results", Pvl::Traverse);
  EXPECT_EQ(results["Compare"][0], "Identical");

    // cnet1 vs cnet2
  args = {"from=" + cnetFile1,
          "from2=" + cnetFile2,
          "to=" + tempDir.path() + "/compareDifferent.txt"
         };

  UserInterface ui3(APP_XML, args);

  try {
    log = cnetdiff(ui3);
  }
  catch(IException &e) {
    FAIL() << e.toString().toStdString().c_str() << std::endl;
  }

  // read back CompareDifferent output file
  Pvl comparePvlDifferent;
  try {
    comparePvlDifferent.read(tempDir.path().toStdString() + "/compareDifferent.txt");
  }
  catch (IException &e) {
    FAIL() << "Unable to open error log pvl file: " << e.what() << std::endl;
  }

  results = comparePvlDifferent.findGroup("Results", Pvl::Traverse);
  EXPECT_EQ(results["Compare"][0], "Different");
  EXPECT_EQ(results["Reason"][0],
                    "Control Point [T0000] Control Measure for Cube [id1] "
                    "Value [SampleResidual] difference is 2.0 (values are "
                    "[-3.0] and [-1.0], tolerance is [0.0])");

  // cnet1 vs cnet2 with sample residual tolerance of 1.5 input in diffs file
  args = {"from=" + cnetFile1,
          "from2=" + cnetFile2,
          "to=" + tempDir.path() + "/compareDifferent2.txt",
          "diff=" + diffsFile
         };

  UserInterface ui4(APP_XML, args);

  try {
    log = cnetdiff(ui4);
  }
  catch(IException &e) {
    FAIL() << e.toString().toStdString().c_str() << std::endl;
  }

  // read back CompareDifferent2 output file
  Pvl comparePvlDifferent2;
  try {
    comparePvlDifferent2.read(tempDir.path().toStdString() + "/compareDifferent2.txt");
  }
  catch (IException &e) {
    FAIL() << "Unable to open error log pvl file: " << e.what() << std::endl;
  }

  results = comparePvlDifferent2.findGroup("Results", Pvl::Traverse);
  EXPECT_EQ(results["Compare"][0], "Different");
  EXPECT_EQ(results["Reason"][0],
                    "Control Point [T0000] Control Measure for Cube [id1] "
                    "Value [SampleResidual] difference is 2.0 (values are "
                    "[-3.0] and [-1.0], tolerance is [1.5])");
}

/**
   * FunctionalTestCnetdiffReportFull
   * 
   * 2) Runs 4 comparisons between binary control nets, generating full reports.
   *  a) cnet1 vs cnet1, should be identical
   *  b) cnet2 vs cnet2, should be identical
   *  c) cnet1 vs cnet2
   *  d) cnet1 vs cnet2 using diffs2.txt containing...
   *     Group = IgnoreKeys
   *       AprioriSample = true
   *       AprioriLine = true
   *       SampleSigma = true
   *       LineSigma = true
   *       NetworkId = true
   *       AprioriXYZSource = true
   *       EditLock = true
   *       AdjustedZ = true
   *     EndGroup
   *
   *     Group = Tolerances
   *       Sample = 1
   *       Line = 1
   *       SampleResidual = 5
   *       AdjustedX = 3
   *       AdjustedY = 3
   *     EndGroup 
   */
TEST_F(CompareNetsReportFull, FunctionalTestCnetdiffReportFull) {

  // compare cnet3 vs cnet3
  QVector<QString> args = {"from=" + cnetFile1,
                           "from2=" + cnetFile1,
                           "to=" + tempDir.path() + "/compareSame1.txt",
                           "report=full"
                          };

  UserInterface ui1(APP_XML, args);

  Pvl log;

  try {
    log = cnetdiff(ui1);
  }
  catch(IException &e) {
    FAIL() << e.toString().toStdString().c_str() << std::endl;
  }

  // read back compareSame1 output file
  Pvl compareSame1;
  try {
    compareSame1.read(tempDir.path().toStdString() + "/compareSame1.txt");
  }
  catch (IException &e) {
    FAIL() << "Unable to open error log pvl file: " << e.what() << std::endl;
  }

  // confirm there are no groups or objects in the difference
  // object confirming cnet3 is identical to itself 
  ASSERT_TRUE(compareSame1.hasObject("Differences"));
  PvlObject differences = compareSame1.findObject("Differences");
  EXPECT_EQ(differences.groups(), 0);
  EXPECT_EQ(differences.objects(), 0);

  // compare cnet4 vs cnet4
  args = {"from=" + cnetFile2,
          "from2=" + cnetFile2,
          "to=" + tempDir.path() + "/compareSame2.txt",
          "report=full"
         };

  UserInterface ui2(APP_XML, args);

  try {
    log = cnetdiff(ui2);
  }
  catch(IException &e) {
    FAIL() << e.toString().toStdString().c_str() << std::endl;
  }

  // read back compareSame2 output file
  Pvl compareSame2;
  try {
    compareSame2.read(tempDir.path().toStdString() + "/compareSame2.txt");
  }
  catch (IException &e) {
    FAIL() << "Unable to open error log pvl file: " << e.what() << std::endl;
  }

  // confirm there are no groups or objects in the difference
  // object confirming cnet4 is identical to itself 
  ASSERT_TRUE(compareSame2.hasObject("Differences"));
  differences = compareSame2.findObject("Differences");
  EXPECT_EQ(differences.groups(), 0);
  EXPECT_EQ(differences.objects(), 0);

  // compare cnet3 vs cnet4
  args = {"from=" + cnetFile1,
          "from2=" + cnetFile2,
          "to=" + tempDir.path() + "/compareDifferent1.txt",
          "report=full"
         };

  UserInterface ui3(APP_XML, args);

  try {
    log = cnetdiff(ui3);
  }
  catch(IException &e) {
    FAIL() << e.toString().toStdString().c_str() << std::endl;
  }

  // read back compareDifferent1 output file
  Pvl compareDifferent1;
  try {
    compareDifferent1.read(tempDir.path().toStdString() + "/compareDifferent1.txt");
  }
  catch (IException &e) {
    FAIL() << "Unable to open error log pvl file: " << e.what() << std::endl;
  }

  PvlObject &DifferencesObject1 = compareDifferent1.findObject("Differences");
  EXPECT_EQ(DifferencesObject1.keywords(), 4);
  EXPECT_EQ(DifferencesObject1.objects(), 5);
  EXPECT_EQ(DifferencesObject1["Filename"][0], "cnet1.net");
  EXPECT_EQ(DifferencesObject1["Filename"][1], "cnet2.net");  
  EXPECT_EQ(DifferencesObject1["Points"][0], "4");
  EXPECT_EQ(DifferencesObject1["Points"][1], "3");
  EXPECT_EQ(DifferencesObject1["NetworkId"][0], "Test");
  EXPECT_EQ(DifferencesObject1["NetworkId"][1], "Test2");
  EXPECT_EQ(DifferencesObject1["TargetName"][0], "Mars");
  EXPECT_EQ(DifferencesObject1["TargetName"][1], "Moon");

  PvlObject &comp1P1 = DifferencesObject1.findObject("Point");
  EXPECT_EQ(comp1P1.keywords(), 2);
  EXPECT_EQ(comp1P1.groups(), 2);
  EXPECT_EQ(comp1P1["PointId"][0], "T0000");
  EXPECT_EQ(comp1P1["EditLock"][0], "True");
  EXPECT_EQ(comp1P1["EditLock"][1], "N/A");
  
  PvlGroup &comp1P1M1 = comp1P1.findGroup("Measure");
  EXPECT_EQ(comp1P1M1.keywords(), 3);
  EXPECT_EQ(comp1P1M1["SerialNumber"][0], "id0");
  EXPECT_EQ(comp1P1M1["Line"][0], "3.0");
  EXPECT_EQ(comp1P1M1["Line"][1], "2.0");
  EXPECT_EQ(comp1P1M1["Sample"][0], "2.0");
  EXPECT_EQ(comp1P1M1["Sample"][1], "1.0");
  comp1P1.deleteGroup("Measure");

  PvlGroup &comp1P1M2 = comp1P1.findGroup("Measure");
  EXPECT_EQ(comp1P1M2.keywords(), 2);
  EXPECT_EQ(comp1P1M2["SerialNumber"][0], "id1");
  EXPECT_EQ(comp1P1M2["SampleResidual"][0], "-3.0");
  EXPECT_EQ(comp1P1M2["SampleResidual"][1], "-1.0");
  DifferencesObject1.deleteObject("Point");

  PvlObject &comp1P2 = DifferencesObject1.findObject("Point");
  EXPECT_EQ(comp1P2.keywords(), 1);
  EXPECT_EQ(comp1P2["PointId"][0], "T0001");
  EXPECT_EQ(comp1P2["PointId"][1], "N/A");
  DifferencesObject1.deleteObject("Point");

  PvlObject &comp1P3 = DifferencesObject1.findObject("Point");
  EXPECT_EQ(comp1P3.keywords(), 1);
  EXPECT_EQ(comp1P3["PointId"][0], "N/A");
  EXPECT_EQ(comp1P3["PointId"][1], "T0002");
  DifferencesObject1.deleteObject("Point");

  PvlObject &comp1P4 = DifferencesObject1.findObject("Point");
  EXPECT_EQ(comp1P4.keywords(), 1);
  EXPECT_EQ(comp1P4["PointId"][0], "T0003");
  EXPECT_EQ(comp1P4["PointId"][1], "N/A");
  DifferencesObject1.deleteObject("Point");

  PvlObject &comp1P5 = DifferencesObject1.findObject("Point");
  EXPECT_EQ(comp1P5.keywords(), 8);
  EXPECT_EQ(comp1P5.groups(), 4);
  EXPECT_EQ(comp1P5["PointId"][0], "T0004");
  EXPECT_EQ(comp1P5["Measures"][0], "2");
  EXPECT_EQ(comp1P5["Measures"][1], "3");
  EXPECT_EQ(comp1P5["AdjustedX"][0], "-424.024048");
  EXPECT_EQ(comp1P5["AdjustedX"][1], "-423.024048");
  EXPECT_EQ(comp1P5["AdjustedY"][0], "734.4311949");
  EXPECT_EQ(comp1P5["AdjustedY"][1], "754.4311949");
  EXPECT_EQ(comp1P5["AdjustedZ"][0], "529.919264");
  EXPECT_EQ(comp1P5["AdjustedZ"][1], "523.919264");
  EXPECT_EQ(comp1P5["AprioriXYZSource"][0], "N/A");
  EXPECT_EQ(comp1P5["AprioriXYZSource"][1], "Basemap");
  EXPECT_EQ(comp1P5["AprioriXYZSourceFile"][0], "N/A");
  EXPECT_EQ(comp1P5["AprioriXYZSourceFile"][1], "/work1/tsucharski/basemap.cub");
  EXPECT_EQ(comp1P5["EditLock"][0], "True");
  EXPECT_EQ(comp1P5["EditLock"][1], "N/A");

  PvlGroup &comp1P5M1 = comp1P5.findGroup("Measure");
  EXPECT_EQ(comp1P5M1.keywords(), 9);
  EXPECT_EQ(comp1P5M1["SerialNumber"][0], "id0");
  EXPECT_EQ(comp1P5M1["AprioriLine"][0], "20.0");
  EXPECT_EQ(comp1P5M1["AprioriLine"][1], "N/A");
  EXPECT_EQ(comp1P5M1["AprioriSample"][0], "10.0");
  EXPECT_EQ(comp1P5M1["AprioriSample"][1], "N/A");
  EXPECT_EQ(comp1P5M1["Line"][0], "3.0");
  EXPECT_EQ(comp1P5M1["Line"][1], "2.0");
  EXPECT_EQ(comp1P5M1["LineResidual"][0], "-4.0");
  EXPECT_EQ(comp1P5M1["LineResidual"][1], "4.0");
  EXPECT_EQ(comp1P5M1["LineSigma"][0], "N/A");
  EXPECT_EQ(comp1P5M1["LineSigma"][1], "0.21");       
  EXPECT_EQ(comp1P5M1["Sample"][0], "4.0");
  EXPECT_EQ(comp1P5M1["Sample"][1], "1.0");
  EXPECT_EQ(comp1P5M1["SampleResidual"][0], "1.0");
  EXPECT_EQ(comp1P5M1["SampleResidual"][1], "-10.0");
  EXPECT_EQ(comp1P5M1["SampleSigma"][0], "N/A");
  EXPECT_EQ(comp1P5M1["SampleSigma"][1], "0.01");
  comp1P5.deleteGroup("Measure");

  PvlGroup &comp1P5M2 = comp1P5.findGroup("Measure");
  EXPECT_EQ(comp1P5M2.keywords(), 1);
  EXPECT_EQ(comp1P5M2["SerialNumber"][0], "id1");
  EXPECT_EQ(comp1P5M2["SerialNumber"][1], "N/A");
  comp1P5.deleteGroup("Measure");

  PvlGroup &comp1P5M3 = comp1P5.findGroup("Measure");
  EXPECT_EQ(comp1P5M3.keywords(), 1);
  EXPECT_EQ(comp1P5M3["SerialNumber"][0], "N/A");
  EXPECT_EQ(comp1P5M3["SerialNumber"][1], "id2");
  comp1P5.deleteGroup("Measure");

  PvlGroup &comp1P5M4 = comp1P5.findGroup("Measure");
  EXPECT_EQ(comp1P5M4.keywords(), 1);
  EXPECT_EQ(comp1P5M4["SerialNumber"][0], "N/A");
  EXPECT_EQ(comp1P5M4["SerialNumber"][1], "id3");

  // cnet3 vs cnet4 with diffsFile
  args = {"from=" + cnetFile1,
          "from2=" + cnetFile2,
          "to=" + tempDir.path() + "/compareDifferent2.txt",
          "diff=" + diffsFile,
          "report=full"
         };
        
  UserInterface ui4(APP_XML, args);

  try {
    log = cnetdiff(ui4);
  }
  catch(IException &e) {
    FAIL() << e.toString().toStdString().c_str() << std::endl;
  }

  // read back compareDifferent2 output file
  Pvl compareDifferent2;
  try {
    compareDifferent2.read(tempDir.path().toStdString() + "/compareDifferent2.txt");
  }
  catch (IException &e) {
    FAIL() << "Unable to open error log pvl file: " << e.what() << std::endl;
  }

  PvlObject &DifferencesObject2 = compareDifferent2.findObject("Differences");
  EXPECT_EQ(DifferencesObject2.keywords(), 3);
  EXPECT_EQ(DifferencesObject2.objects(), 4);
  EXPECT_EQ(DifferencesObject2["Filename"][0], "cnet1.net");
  EXPECT_EQ(DifferencesObject2["Filename"][1], "cnet2.net");
  EXPECT_EQ(DifferencesObject2["Points"][0], "4");
  EXPECT_EQ(DifferencesObject2["Points"][1], "3");
  EXPECT_EQ(DifferencesObject2["TargetName"][0], "Mars");
  EXPECT_EQ(DifferencesObject2["TargetName"][1], "Moon");

  PvlObject &comp2P1 = DifferencesObject2.findObject("Point");
  EXPECT_EQ(comp2P1.keywords(), 1);
  EXPECT_EQ(comp2P1["PointId"][0], "T0001");
  EXPECT_EQ(comp2P1["PointId"][1], "N/A");
  DifferencesObject2.deleteObject("Point");

  PvlObject &comp2P2 = DifferencesObject2.findObject("Point");
  EXPECT_EQ(comp2P2.keywords(), 1);
  EXPECT_EQ(comp2P2["PointId"][0], "N/A");
  EXPECT_EQ(comp2P2["PointId"][1], "T0002");
  DifferencesObject2.deleteObject("Point");

  PvlObject &comp2P3 = DifferencesObject2.findObject("Point");
  EXPECT_EQ(comp2P3.keywords(), 1);
  EXPECT_EQ(comp2P3["PointId"][0], "T0003");
  EXPECT_EQ(comp2P3["PointId"][1], "N/A");
  DifferencesObject2.deleteObject("Point");

  PvlObject &comp2P4 = DifferencesObject2.findObject("Point");
  EXPECT_EQ(comp2P4.keywords(), 4);
  EXPECT_EQ(comp2P4.groups(), 4);
  EXPECT_EQ(comp2P4["PointId"][0], "T0004");
  EXPECT_EQ(comp2P4["Measures"][0], "2");
  EXPECT_EQ(comp2P4["Measures"][1], "3");
  EXPECT_EQ(comp2P4["AdjustedY"][0], "734.4311949");
  EXPECT_EQ(comp2P4["AdjustedY"][1], "754.4311949");
  EXPECT_EQ(comp2P4["AdjustedY"][2], "3.0");
  EXPECT_EQ(comp2P4["AprioriXYZSourceFile"][0], "N/A");
  EXPECT_EQ(comp2P4["AprioriXYZSourceFile"][1], "/work1/tsucharski/basemap.cub");

  PvlGroup &comp2P4M1 = comp2P4.findGroup("Measure");
  EXPECT_EQ(comp2P4M1.keywords(), 4);
  EXPECT_EQ(comp2P4M1["SerialNumber"][0], "id0");
  EXPECT_EQ(comp2P4M1["LineResidual"][0], "-4.0");
  EXPECT_EQ(comp2P4M1["LineResidual"][1], "4.0");
  EXPECT_EQ(comp2P4M1["Sample"][0], "4.0");
  EXPECT_EQ(comp2P4M1["Sample"][1], "1.0");
  EXPECT_EQ(comp2P4M1["Sample"][2], "1.0");
  EXPECT_EQ(comp2P4M1["SampleResidual"][0], "1.0");
  EXPECT_EQ(comp2P4M1["SampleResidual"][1], "-10.0");
  EXPECT_EQ(comp2P4M1["SampleResidual"][2], "5.0");
  comp2P4.deleteGroup("Measure");

  PvlGroup &comp2P4M2 = comp2P4.findGroup("Measure");
  EXPECT_EQ(comp2P4M2.keywords(), 1);
  EXPECT_EQ(comp2P4M2["SerialNumber"][0], "id1");
  EXPECT_EQ(comp2P4M2["SerialNumber"][1], "N/A");
  comp2P4.deleteGroup("Measure");

  PvlGroup &comp2P4M3 = comp2P4.findGroup("Measure");
  EXPECT_EQ(comp2P4M3.keywords(), 1);
  EXPECT_EQ(comp2P4M3["SerialNumber"][0], "N/A");
  EXPECT_EQ(comp2P4M3["SerialNumber"][1], "id2");
  comp2P4.deleteGroup("Measure");

  PvlGroup &comp2P4M4 = comp2P4.findGroup("Measure");
  EXPECT_EQ(comp2P4M4.keywords(), 1);
  EXPECT_EQ(comp2P4M4["SerialNumber"][0], "N/A");
  EXPECT_EQ(comp2P4M4["SerialNumber"][1], "id3");
}
