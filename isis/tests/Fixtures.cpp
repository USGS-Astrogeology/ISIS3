#include <QTextStream>
#include <QUuid>

#include "CubeAttribute.h"
#include "FileName.h"

#include "Blob.h"
#include "csminit.h"
#include "Fixtures.h"
#include "Portal.h"
#include "LineManager.h"
#include "SpecialPixel.h"
#include "TestUtilities.h"
#include "ControlNet.h"

namespace Isis {

  void TempTestingFiles::SetUp() {
    ASSERT_TRUE(tempDir.isValid());
  }


  void SmallCube::SetUp() {
    TempTestingFiles::SetUp();

    testCube = new Cube();
    testCube->setDimensions(10, 10, 10);
    QString path = tempDir.path() + "/small.cub";
    testCube->create(path);

    LineManager line(*testCube);
    double pixelValue = 0.0;
    for(line.begin(); !line.end(); line++) {
      for(int i = 0; i < line.size(); i++) {
        line[i] = (double) pixelValue++;
      }
      testCube->write(line);
    }

    // Add a BandBin group to the cube label
    Pvl *label = testCube->label();
    PvlObject& cubeLabel = label->findObject("IsisCube");
    PvlGroup bandBin("BandBin");
    PvlKeyword originalBand("OriginalBand", "1");
    originalBand += "2";
    originalBand += "3";
    originalBand += "4";
    originalBand += "5";
    originalBand += "6";
    originalBand += "7";
    originalBand += "8";
    originalBand += "9";
    originalBand += "10";
    bandBin += originalBand;
    cubeLabel.addGroup(bandBin);
    testCube->close();
    testCube->open(path, "rw");
  }

  void SmallCube::TearDown() {
    if (testCube->isOpen()) {
      testCube->close();
    }

    if (testCube) {
      delete testCube;
    }
  }


  void LargeCube::SetUp() {
    TempTestingFiles::SetUp();

    testCube = new Cube();
    testCube->setDimensions(1000, 1000, 10);
    testCube->create(tempDir.path() + "/large.cub");

    LineManager line(*testCube);
    double pixelValue = 0.0;
    for(line.begin(); !line.end(); line++) {
      for(int i = 0; i < line.size(); i++) {
        line[i] = pixelValue;
      }

      pixelValue++;
      testCube->write(line);
    }
  }

  void LargeCube::TearDown() {
    if (testCube->isOpen()) {
      testCube->close();
    }

    if (testCube) {
      delete testCube;
    }
  }


  void SpecialSmallCube::SetUp() {
    TempTestingFiles::SetUp();

    testCube = new Cube();
    testCube->setDimensions(10, 10, 10);
    testCube->create(tempDir.path() + "/small.cub");

    // Use a line manager to update select lines with ISIS special pixel values
    LineManager line(*testCube);
    double pixelValue = 0.0;
    int lineNum = 0;
    for(line.begin(); !line.end(); line++) {
      for(int i = 0; i < line.size(); i++) {
        if (lineNum == 2) {
          line[i] = NULL8;
        }
        else if (lineNum == 3) {
          line[i] = LOW_REPR_SAT8;
        }
        else if (lineNum == 4) {
          line[i] = HIGH_REPR_SAT8;
        }
        else if (lineNum == 5) {
          line[i] = LOW_INSTR_SAT8;
        }
        else if (lineNum == 6) {
          line[i] = HIGH_INSTR_SAT8;
        }
        else {
          line[i] = (double) pixelValue++;
        }
      }
      lineNum++;
      testCube->write(line);
    }

  }

  void SpecialSmallCube::TearDown() {
    if (testCube->isOpen()) {
      testCube->close();
    }

    if (testCube) {
      delete testCube;
    }
  }


  void SmallGapCube::SetUp() {
    TempTestingFiles::SetUp();

    // Initialize horzCube
    horzCube = new Cube();
    horzCube->setDimensions(9, 9, 9);
    horzCube->create(tempDir.path() + "/horzgap.cub");

    // horizontal line of nulls through all bands
    LineManager h_line(*horzCube);
    double h_pixelValue = 0.0;
    int h_lineNum = 0;
    for(h_line.begin(); !h_line.end(); h_line++) {
      for(int i = 0; i < h_line.size(); i++) {
        if(h_lineNum == 4 || h_lineNum % 9 == 4) {
          h_line[i] = NULL8;
        }
        else {
          h_pixelValue = sin(h_lineNum * 180 / M_PI) + cos(i * 180 / M_PI);
          h_line[i] = (double) h_pixelValue;
        }
      }
      h_lineNum++;
      horzCube->write(h_line);
    }
    horzCube->reopen("rw");


    // Initialize vertCube
    vertCube = new Cube();
    vertCube->setDimensions(9, 9, 9);
    vertCube->create(tempDir.path() + "/vertgap.cub");

    // vertical line of nulls through all bands
    LineManager v_line(*vertCube);
    double v_pixelValue = 0.0;
    int v_lineNum = 0;
    for(v_line.begin(); !v_line.end(); v_line++) {
      for(int i = 0; i < v_line.size(); i++) {
        if(i == 4) {
          v_line[i] = NULL8;
        }
        else {
          v_pixelValue = sin(v_lineNum * 180 / M_PI) + cos(i * 180 / M_PI);
          v_line[i] = (double) v_pixelValue;
        }
      }
      v_lineNum++;
      vertCube->write(v_line);
    }
    vertCube->reopen("rw");


    // Initialize bandCube
    bandCube = new Cube();
    bandCube->setDimensions(9, 9, 9);
    bandCube->create(tempDir.path() + "/bandgap.cub");

    // vertical line of nulls on just one band
    LineManager b_line(*bandCube);
    double b_pixelValue = 0.0;
    int b_lineNum = 0;
    for(b_line.begin(); !b_line.end(); b_line++) {
      for(int i = 0; i < b_line.size(); i++) {
        if( b_lineNum == 22 ) {
          b_line[i] = NULL8;
        }
        else {
          b_pixelValue = sin(b_lineNum * 180 / M_PI) + cos(i * 180 / M_PI);
          b_line[i] = (double) b_pixelValue;
        }
      }
      b_lineNum++;
      bandCube->write(b_line);
    }
    bandCube->reopen("rw");

  }


  void SmallGapCube::TearDown() {
    if (horzCube->isOpen()) {
      horzCube->close();
    }
    if (vertCube->isOpen()) {
      vertCube->close();
    }
    if (bandCube->isOpen()) {
      bandCube->close();
    }

    if (horzCube) {
      delete horzCube;
    }
    if (vertCube) {
      delete vertCube;
    }
    if (bandCube) {
      delete bandCube;
    }
  }


  void DemCube::SetUp() {
    DefaultCube::SetUp();
    testCube->label()->object(4)["SolarLongitude"] = "294.73518831328";
    testCube->reopen("rw");

    std::ifstream cubeLabel("data/defaultImage/demCube.pvl");

    Pvl demLabel;
    cubeLabel >> demLabel;
    demLabel.findObject("IsisCube").findObject("Core").findGroup("Pixels")["Type"] = "Real";

    demCube = new Cube();
    demCube->fromLabel(tempDir.path() + "/demCube.cub", demLabel, "rw");

    TableField minRadius("MinimumRadius", TableField::Double);
    TableField maxRadius("MaximumRadius", TableField::Double);

    TableRecord record;
    record += minRadius;
    record += maxRadius;

    Table shapeModelStatistics("ShapeModelStatistics", record);

    record[0] = 3376.2;
    record[1] = 3396.19;
    shapeModelStatistics += record;

    demCube->write(shapeModelStatistics);

    int xCenter = int(demCube->lineCount()/2);
    int yCenter = int(demCube->sampleCount()/2);
    double radius = std::min(xCenter, yCenter);
    double depth = 30;
    double pointRadius;

    LineManager line(*demCube);
    double pixelValue;
    double base = demCube->label()->findObject("IsisCube").findObject("Core").findGroup("Pixels")["Base"];
    double xPos = 0.0;

    for(line.begin(); !line.end(); line++) {
      for(int yPos = 0; yPos < line.size(); yPos++) {
        pointRadius = pow(pow((xPos - xCenter), 2) + pow((yPos - yCenter), 2), 0.5);
        if (pointRadius < radius) {
          pixelValue = ((sin(((M_PI*pointRadius)/(2*radius))) * depth) + depth) + base;
        }
        else {
          pixelValue = base + (depth * 2);
        }
        line[yPos] = (double) pixelValue;
      }
      xPos++;
      demCube->write(line);
    }

    demCube->reopen("rw");
  }

  void DemCube::TearDown() {
    if (demCube->isOpen()) {
      demCube->close();
    }

    delete demCube;
  }


  void DefaultCube::SetUp() {
    TempTestingFiles::SetUp();

    std::ifstream isdFile("data/defaultImage/defaultCube.isd");
    std::ifstream cubeLabel("data/defaultImage/defaultCube.pvl");
    std::ifstream projCubeLabel("data/defaultImage/projDefaultCube.pvl");

    isdFile >> isd;
    cubeLabel >> label;
    projCubeLabel >> projLabel;

    testCube = new Cube();
    testCube->fromIsd(tempDir.path() + "/default.cub", label, isd, "rw");

    LineManager line(*testCube);
    int pixelValue = 1;
    for(line.begin(); !line.end(); line++) {
      for(int i = 0; i < line.size(); i++) {
        line[i] = (double) (pixelValue % 255);
        pixelValue++;
      }
      testCube->write(line);
    }

    projTestCube = new Cube();
    projTestCube->fromIsd(tempDir.path() + "/default.level2.cub", projLabel, isd, "rw");

    line = LineManager(*projTestCube);
    pixelValue = 1;
    for(line.begin(); !line.end(); line++) {
      for(int i = 0; i < line.size(); i++) {
        line[i] = (double) (pixelValue % 255);
        pixelValue++;
      }
      projTestCube->write(line);
    }
    projTestCube->reopen("rw");
  }

  void DefaultCube::resizeCube(int samples, int lines, int bands) {
    label = Pvl();
    PvlObject &isisCube = testCube->label()->findObject("IsisCube");
    label.addObject(isisCube);

    PvlGroup &dim = label.findObject("IsisCube").findObject("Core").findGroup("Dimensions");
    dim.findKeyword("Samples").setValue(QString::number(samples));
    dim.findKeyword("Lines").setValue(QString::number(lines));
    dim.findKeyword("Bands").setValue(QString::number(bands));

    delete testCube;
    testCube = new Cube();
    testCube->fromIsd(tempDir.path() + "/default.cub", label, isd, "rw");

    LineManager line(*testCube);
    int pixelValue = 1;
    for(int band = 1; band <= bands; band++) {
      for (int i = 1; i <= testCube->lineCount(); i++) {
        line.SetLine(i, band);
        for (int j = 0; j < line.size(); j++) {
          line[j] = (double) (pixelValue % 255);
          pixelValue++;
        }
        testCube->write(line);
      }
    }

    projLabel = Pvl();
    PvlObject &isisProjCube= projTestCube->label()->findObject("IsisCube");
    projLabel.addObject(isisProjCube);

    PvlGroup &projDim = projLabel.findObject("IsisCube").findObject("Core").findGroup("Dimensions");
    projDim.findKeyword("Samples").setValue(QString::number(samples));
    projDim.findKeyword("Lines").setValue(QString::number(lines));
    projDim.findKeyword("Bands").setValue(QString::number(bands));

    delete projTestCube;
    projTestCube = new Cube();
    projTestCube->fromIsd(tempDir.path() + "/default.level2.cub", projLabel, isd, "rw");

    line = LineManager(*projTestCube);
    pixelValue = 1;
    for(int band = 1; band <= bands; band++) {
      for (int i = 1; i <= projTestCube->lineCount(); i++) {
        line.SetLine(i, band);
        for (int j = 0; j < line.size(); j++) {
          line[j] = (double) (pixelValue % 255);
          pixelValue++;
        }
        projTestCube->write(line);
      }
    }
  }

  void DefaultCube::TearDown() {
    if (testCube->isOpen()) {
      testCube->close();
    }

    if (projTestCube->isOpen()) {
      projTestCube->close();
    }

    delete testCube;
    delete projTestCube;
  }

  void LineScannerCube::SetUp() {
    TempTestingFiles::SetUp();

    std::ifstream isdFile("data/LineScannerImage/defaultLineScanner.isd");
    std::ifstream cubeLabel("data/LineScannerImage/defaultLineScanner.pvl");
    std::ifstream projCubeLabel("data/LineScannerImage/projDefaultLineScanner.pvl");

    isdFile >> isd;
    cubeLabel >> label;
    projCubeLabel >> projLabel;

    testCube = new Cube();
    testCube->fromIsd(tempDir.path() + "/default.cub", label, isd, "rw");
    LineManager line(*testCube);
    int pixelValue = 1;
    for(line.begin(); !line.end(); line++) {
      for(int i = 0; i < line.size(); i++) {
        line[i] = (double) (pixelValue % 255);
        pixelValue++;
      }
      testCube->write(line);
    }

    projTestCube = new Cube();
    projTestCube->fromIsd(tempDir.path() + "/default.level2.cub", projLabel, isd, "rw");
    line = LineManager(*projTestCube);
    pixelValue = 1;
    for(line.begin(); !line.end(); line++) {
      for(int i = 0; i < line.size(); i++) {
        line[i] = (double) (pixelValue % 255);
        pixelValue++;
      }
      projTestCube->write(line);
    }
  }


  void LineScannerCube::TearDown() {
    if (testCube->isOpen()) {
      testCube->close();
    }

    if (projTestCube->isOpen()) {
      projTestCube->close();
    }

    delete testCube;
    delete projTestCube;
  }

  void OffBodyCube::SetUp() {
    TempTestingFiles::SetUp();
    testCube = new Cube("data/offBodyImage/EW0131773041G.cal.crop.cub");
  }


  void OffBodyCube::TearDown() {
    if (testCube->isOpen()) {
      testCube->close();
    }

    delete testCube;
  }


  void MiniRFCube::SetUp() {
    TempTestingFiles::SetUp();
    testCube = new Cube("data/miniRFImage/LSZ_04866_1CD_XKU_89N109_V1_lev1.crop.cub");
  }


  void MiniRFCube::TearDown() {
    if (testCube->isOpen()) {
      testCube->close();
    }

    delete testCube;
  }


  void ThreeImageNetwork::SetUp() {
    TempTestingFiles::SetUp();

    FileName labelPath1("data/threeImageNetwork/cube1.pvl");
    FileName labelPath2("data/threeImageNetwork/cube2.pvl");
    FileName labelPath3("data/threeImageNetwork/cube3.pvl");

    FileName mappedLabelPath1("data/threeImageNetwork/cube1map.pvl");
    FileName mappedLabelPath2("data/threeImageNetwork/cube2map.pvl");
    FileName mappedLabelPath3("data/threeImageNetwork/cube3map.pvl");

    isdPath1 = new FileName("data/threeImageNetwork/cube1.isd");
    isdPath2 = new FileName("data/threeImageNetwork/cube2.isd");
    isdPath3 = new FileName("data/threeImageNetwork/cube3.isd");

    threeImageOverlapFile = new FileName("data/threeImageNetwork/threeImageOverlaps.lis");
    twoImageOverlapFile = new FileName("data/threeImageNetwork/twoImageOverlaps.lis");

    cube1 = new Cube();
    cube1->fromIsd(tempDir.path() + "/cube1.cub", labelPath1, *isdPath1, "rw");

    ImagePolygon poly;
    coords = {{30, 0},
              {30, 10},
              {35, 10},
              {35, 0},
              {30, 0}};
    poly.Create(coords);
    cube1->write(poly);

    cube2 = new Cube();
    cube2->fromIsd(tempDir.path() + "/cube2.cub", labelPath2, *isdPath2, "rw");

    coords = {{31, 1},
              {31, 11},
              {36, 11},
              {36, 1},
              {31, 1}};
    poly.Create(coords);
    cube2->write(poly);

    cube3 = new Cube();
    cube3->fromIsd(tempDir.path() + "/cube3.cub", labelPath3, *isdPath3, "rw");

    LineManager line(*cube1);
    LineManager line2(*cube2);
    LineManager line3(*cube3);
    int pixelValue = 1;
    for(line.begin(); !line.end(); line++) {
      for(int i = 0; i < line.size(); i++) {
        line[i] = (double) (pixelValue %255);
        pixelValue++;
      }
      cube1->write(line);
    }

    for(line2.begin(); !line2.end(); line2++) {
      for(int i = 0; i < line.size(); i++) {
        line2[i] = (double) (pixelValue %255);
        pixelValue++;
      }
      cube2->write(line2);
    }

    for(line3.begin(); !line3.end(); line3++) {
      for(int i = 0; i < line3.size(); i++) {
        line3[i] = (double) (pixelValue %255);
        pixelValue++;
      }
      cube3->write(line3);
    }

    cube1->reopen("rw");
    cube2->reopen("rw");
    cube3->reopen("rw");

    cubeList = new FileList();
    cubeList->append(cube1->fileName());
    cubeList->append(cube2->fileName());

    twoCubeListFile = tempDir.path() + "/2cubes.lis";
    cubeList->write(twoCubeListFile);
    cubeList->append(cube3->fileName());

    cubeListFile = tempDir.path() + "/cubes.lis";
    cubeList->write(cubeListFile);

    networkFile = "data/threeImageNetwork/controlnetwork.net";

    network = new ControlNet();
    network->ReadControl(networkFile);

    cube1map = new Cube();
    cube2map = new Cube();
    cube3map = new Cube();
    cube1map->fromIsd(tempDir.path() + "/cube1map.cub", mappedLabelPath1, *isdPath1, "rw");
    cube2map->fromIsd(tempDir.path() + "/cube2map.cub", mappedLabelPath2, *isdPath2, "rw");
    cube3map->fromIsd(tempDir.path() + "/cube3map.cub", mappedLabelPath3, *isdPath3, "rw");
  }

  void ThreeImageNetwork::TearDown() {
    delete cubeList;
    delete network;

    delete cube1;
    delete cube2;
    delete cube3;

    delete cube1map;
    delete cube2map;
    delete cube3map;

    delete isdPath1;
    delete isdPath2;
    delete isdPath3;

    delete threeImageOverlapFile;
    delete twoImageOverlapFile;
  }


  void ApolloNetwork::SetUp() {
    TempTestingFiles::SetUp();

    cubes.fill(nullptr, 7);

    cubeList = new FileList();

    for(int i = 0; i < cubes.size(); i++) {
      int n = i+1; // filenames use 1 based indexing
      isdFiles.push_back(FileName("data/apolloNetwork/apolloImage"+QString::number(n)+".isd"));
      labelFiles.push_back(FileName("data/apolloNetwork/apolloImage"+QString::number(n)+".pvl"));
      cubes[i] = new Cube();
      cubes[i]->fromIsd(tempDir.path() + "/cube"+QString::number(n)+".cub", labelFiles[i], isdFiles[i], "rw");
      cubeList->append(cubes[i]->fileName());
    }

    cubeListFile = tempDir.path() + "/cubes.lis";
    cubeList->write(cubeListFile);

    network = new ControlNet("data/apolloNetwork/apolloNet.pvl");
    controlNetPath = tempDir.path() + "/apolloNet.net";
    network->Write(controlNetPath);
  }

  void ApolloNetwork::TearDown() {
    for(int i = 0; i < cubes.size(); i++) {
      if(cubes[i] && cubes[i]->isOpen()) {
        delete cubes[i];
      }
    }

    if (cubeList) {
      delete cubeList;
    }
  }

  void ObservationPair::SetUp() {
      FileName labelPathL = FileName("data/observationPair/observationImageL.pvl");
      FileName labelPathR = FileName("data/observationPair/observationImageR.pvl");

      isdPathL = new FileName("data/observationPair/observationImageL.isd");
      isdPathR = new FileName("data/observationPair/observationImageR.isd");

      cubeL = new Cube();
      cubeR = new Cube();

      cubeLPath = tempDir.path() + "/observationPairL.cub";
      cubeRPath = tempDir.path() + "/observationPairR.cub";

      cubeL->fromIsd(cubeLPath, labelPathL, *isdPathL, "rw");
      Pvl originalPdsLabL("data/observationPair/observationImageLOriginalLabel.pvl");
      OriginalLabel origLabel(originalPdsLabL);
      cubeL->write(origLabel);
      cubeL->reopen("rw");

      cubeR->fromIsd(cubeRPath, labelPathR, *isdPathR, "rw");

      cubeList = new FileList();
      cubeList->append(cubeL->fileName());
      cubeList->append(cubeR->fileName());

      cubeListFile = tempDir.path() + "/cubes.lis";
      cubeList->write(cubeListFile);

      cnetPath = "data/observationPair/observationPair.net";
      network = new ControlNet();
      network->ReadControl(cnetPath);
  }


  void ObservationPair::TearDown() {
    delete cubeList;
    delete network;

    if (cubeL) {
      delete cubeL;
    }

    if (cubeR) {
      delete cubeR;
    }

    delete isdPathL;
    delete isdPathR;
  }


  void MroCtxCube::SetUp() {
    TempTestingFiles::SetUp();

    QString testPath = tempDir.path() + "/test.cub";
    QFile::copy("data/mroCtxImage/ctxTestImage.cub", testPath);
    testCube.reset(new Cube(testPath));
  }


  void MroCtxCube::TearDown() {
    testCube.reset();
  }


  void GalileoSsiCube::SetUp() {
    DefaultCube::SetUp();

    // Change default dims
    PvlGroup &dim = label.findObject("IsisCube").findObject("Core").findGroup("Dimensions");
    dim.findKeyword("Samples").setValue("800");
    dim.findKeyword("Lines").setValue("800");
    dim.findKeyword("Bands").setValue("1");

    delete testCube;
    testCube = new Cube();

    FileName newCube(tempDir.path() + "/testing.cub");

    testCube->fromIsd(newCube, label, isd, "rw");
    PvlGroup &kernels = testCube->label()->findObject("IsisCube").findGroup("Kernels");
    kernels.findKeyword("NaifFrameCode").setValue("-77001");
    PvlGroup &inst = testCube->label()->findObject("IsisCube").findGroup("Instrument");

    std::istringstream iss(R"(
      Group = Instrument
        SpacecraftName            = "Galileo Orbiter"
        InstrumentId              = "SOLID STATE IMAGING SYSTEM"
        TargetName                = IO
        SpacecraftClockStartCount = 05208734.39
        StartTime                 = 1999-10-11T18:05:15.815
        ExposureDuration          = 0.04583 <seconds>
        GainModeId                = 100000
        TelemetryFormat           = IM4
        LightFloodStateFlag       = ON
        InvertedClockStateFlag    = "NOT INVERTED"
        BlemishProtectionFlag     = OFF
        ExposureType              = NORMAL
        ReadoutMode               = Contiguous
        FrameDuration             = 8.667 <seconds>
        Summing                   = 1
        FrameModeId               = FULL
      End_Group
    )");

    PvlGroup newInstGroup;
    iss >> newInstGroup;
    inst = newInstGroup;

    PvlGroup &bandBin = testCube->label()->findObject("IsisCube").findGroup("BandBin");
    std::istringstream bss(R"(
      Group = BandBin
        FilterName   = RED
        FilterNumber = 2
        Center       = 0.671 <micrometers>
        Width        = .06 <micrometers>
      End_Group
    )");

    PvlGroup newBandBin;
    bss >> newBandBin;
    bandBin = newBandBin;

    PvlObject &naifKeywords = testCube->label()->findObject("NaifKeywords");

    std::istringstream nk(R"(
      Object = NaifKeywords
        BODY_CODE                  = 501
        BODY501_RADII              = (1829.4, 1819.3, 1815.7)
        BODY_FRAME_CODE            = 10023
        INS-77001_FOCAL_LENGTH     = 1500.46655964
        INS-77001_K1               = -2.4976983626e-05
        INS-77001_PIXEL_PITCH      = 0.01524
        INS-77001_TRANSX           = (0.0, 0.01524, 0.0)
        INS-77001_TRANSY           = (0.0, 0.0, 0.01524)
        INS-77001_ITRANSS          = (0.0, 65.6167979, 0.0)
        INS-77001_ITRANSL          = (0.0, 0.0, 65.6167979)
        INS-77001_BORESIGHT_SAMPLE = 400.0
        INS-77001_BORESIGHT_LINE   = 400.0
      End_Object
    )");

    PvlObject newNaifKeywords;
    nk >> newNaifKeywords;
    naifKeywords = newNaifKeywords;

    std::istringstream ar(R"(
    Group = Archive
      DataSetId     = GO-J/JSA-SSI-2-REDR-V1.0
      ProductId     = 24I0146
      ObservationId = 24ISGLOCOL01
      DataType      = RADIANCE
      CalTargetCode = 24
    End_Group
    )");

    PvlGroup &archive = testCube->label()->findObject("IsisCube").findGroup("Archive");
    PvlGroup newArchive;
    ar >> newArchive;
    archive = newArchive;

    LineManager line(*testCube);
    for(line.begin(); !line.end(); line++) {
        for(int i = 0; i < line.size(); i++) {
          line[i] = (double)(i+1);
        }
        testCube->write(line);
    }

    // need to remove old camera pointer
    delete testCube;
    testCube = new Cube(newCube, "rw");
  }


  void GalileoSsiCube::TearDown() {
    if (testCube) {
      delete testCube;
    }
  }


  void MroHiriseCube::SetUp() {
    DefaultCube::SetUp();
    dejitteredCube.open("data/mroKernels/mroHiriseProj.cub");

    // force real DNs
    QString fname = testCube->fileName();

    PvlObject &core = label.findObject("IsisCube").findObject("Core");
    PvlGroup &pixels = core.findGroup("Pixels");
    pixels.findKeyword("Type").setValue("Real");

    delete testCube;
    testCube = new Cube();

    FileName newCube(tempDir.path() + "/testing.cub");

    testCube->fromIsd(newCube, label, isd, "rw");
    PvlGroup &kernels = testCube->label()->findObject("IsisCube").findGroup("Kernels");
    kernels.findKeyword("NaifFrameCode").setValue("-74999");
    PvlGroup &inst = testCube->label()->findObject("IsisCube").findGroup("Instrument");
    std::istringstream iss(R"(
      Group = Instrument
        SpacecraftName              = "MARS RECONNAISSANCE ORBITER"
        InstrumentId                = HIRISE
        TargetName                  = Mars
        StartTime                   = 2008-05-17T09:37:24.7300819
        StopTime                    = 2008-05-17T09:37:31.0666673
        ObservationStartCount       = 895484264:44383
        SpacecraftClockStartCount   = 895484264:57342
        SpacecraftClockStopCount    = 895484272:12777
        ReadoutStartCount           = 895484659:31935
        CalibrationStartTime        = 2006-11-08T04:49:13.952
        CalibrationStartCount       = 847428572:51413
        AnalogPowerStartTime        = 2006-11-08T04:48:34.478
        AnalogPowerStartCount       = 847428533:20297
        MissionPhaseName            = "PRIMARY SCIENCE PHASE"
        LineExposureDuration        = 95.0625 <MICROSECONDS>
        ScanExposureDuration        = 95.0625 <MICROSECONDS>
        DeltaLineTimerCount         = 337
        Summing                     = 1
        Tdi                         = 128
        FocusPositionCount          = 2020
        PoweredCpmmFlag             = (On, On, On, On, On, On, On, On, On, On, On,
                                      On, On, On)
        CpmmNumber                  = 8
        CcdId                       = RED5
        ChannelNumber               = 0
        LookupTableType             = Stored
        LookupTableNumber           = 19
        LookupTableMinimum          = -9998
        LookupTableMaximum          = -9998
        LookupTableMedian           = -9998
        LookupTableKValue           = -9998
        StimulationLampFlag         = (Off, Off, Off)
        HeaterControlFlag           = (On, On, On, On, On, On, On, On, On, On, On,
                                      On, On, On)
        OptBnchFlexureTemperature   = 19.5881 <C>
        OptBnchMirrorTemperature    = 19.6748 <C>
        OptBnchFoldFlatTemperature  = 19.9348 <C>
        OptBnchFpaTemperature       = 19.5015 <C>
        OptBnchFpeTemperature       = 19.2415 <C>
        OptBnchLivingRmTemperature  = 19.4148 <C>
        OptBnchBoxBeamTemperature   = 19.5881 <C>
        OptBnchCoverTemperature     = 19.6748 <C>
        FieldStopTemperature        = 17.9418 <C>
        FpaPositiveYTemperature     = 18.8082 <C>
        FpaNegativeYTemperature     = 18.6349 <C>
        FpeTemperature              = 18.0284 <C>
        PrimaryMirrorMntTemperature = 19.5015 <C>
        PrimaryMirrorTemperature    = 19.6748 <C>
        PrimaryMirrorBafTemperature = 2.39402 <C>
        MsTrussLeg0ATemperature     = 19.6748 <C>
        MsTrussLeg0BTemperature     = 19.8482 <C>
        MsTrussLeg120ATemperature   = 19.3281 <C>
        MsTrussLeg120BTemperature   = 20.1949 <C>
        MsTrussLeg240ATemperature   = 20.2816 <C>
        MsTrussLeg240BTemperature   = 20.7151 <C>
        BarrelBaffleTemperature     = -13.8299 <C>
        SunShadeTemperature         = -33.9377 <C>
        SpiderLeg30Temperature      = 17.5087 <C>
        SpiderLeg120Temperature     = -9999
        SpiderLeg240Temperature     = -9999
        SecMirrorMtrRngTemperature  = 20.6284 <C>
        SecMirrorTemperature        = 20.455 <C>
        SecMirrorBaffleTemperature  = -11.1761 <C>
        IeaTemperature              = 25.4878 <C>
        FocusMotorTemperature       = 21.4088 <C>
        IePwsBoardTemperature       = 16.3696 <C>
        CpmmPwsBoardTemperature     = 17.6224 <C>
        MechTlmBoardTemperature     = 34.7792 <C>
        InstContBoardTemperature    = 34.4121 <C>
        DllLockedFlag               = (YES, YES)
        DllResetCount               = 0
        DllLockedOnceFlag           = (YES, YES)
        DllFrequenceCorrectCount    = 4
        ADCTimingSetting            = -9999
        Unlutted                    = TRUE
      End_Group
    )");

    PvlGroup newInstGroup;
    iss >> newInstGroup;

    newInstGroup.findKeyword("InstrumentId").setValue("HIRISE");
    newInstGroup.findKeyword("SpacecraftName").setValue("MARS RECONNAISSANCE ORBITER");

    inst = newInstGroup;
    PvlObject &naifKeywords = testCube->label()->findObject("NaifKeywords");

    PvlKeyword startcc("SpacecraftClockStartCount", "33322515");
    PvlKeyword stopcc("SpaceCraftClockStopCount", "33322516");
    inst += startcc;
    inst += stopcc;

    json nk;
    nk["INS-74999_FOCAL_LENGTH"] = 11994.9988;
    nk["INS-74999_PIXEL_PITCH"] = 0.012;
    nk["INS-74605_TRANSX"] = {-89.496, -1.0e-06, 0.012};
    nk["INS-74605_TRANSY"] = {-12.001, -0.012, -1.0e-06};
    nk["INS-74605_ITRANSS"] = {-1000.86, -0.0087, -83.333};
    nk["INS-74605_ITRANSL"] = {7457.9, 83.3333, -0.0087};
    nk["INS-74999_OD_K"] = {-0.0048509, 2.41312e-07, -1.62369e-13};
    nk["BODY499_RADII"] = {3396.19, 3396.19, 3376.2};
    nk["CLOCK_ET_-74999_895484264:57342_COMPUTED"] = "8ed6ae8930f3bd41";

    nk["BODY_CODE"] = 499;
    nk["BODY_FRAME_CODE"] = 10014;
    PvlObject newNaifKeywords("NaifKeywords", nk);
    naifKeywords = newNaifKeywords;

    QString fileName = testCube->fileName();

    LineManager line(*testCube);
    for(line.begin(); !line.end(); line++) {
        for(int i = 0; i < line.size(); i++) {
          line[i] = (double)(i+1);
        }
        testCube->write(line);
    }
    testCube->reopen("rw");

    // need to remove old camera pointer
    delete testCube;
    // This is now a MRO cube

    testCube = new Cube(fileName, "rw");

    // create a jitter file
    QString jitter = R"(# Sample                 Line                   ET
-0.18     -0.07     264289109.96933
-0.11     -0.04     264289109.97
-0.05     -0.02     264289109.98
1.5     0.6     264289110.06
    )";

    jitterPath = tempDir.path() + "/jitter.txt";
    QFile jitterFile(jitterPath);

    if (jitterFile.open(QIODevice::WriteOnly)) {
      QTextStream out(&jitterFile);
      out << jitter;
      jitterFile.close();
    }
    else {
      FAIL() << "Failed to create Jitter file" << std::endl;
    }
  }


  void NewHorizonsCube::setInstrument(QString ikid, QString instrumentId, QString spacecraftName) {
    PvlObject &isisCube = testCube->label()->findObject("IsisCube");

    label = Pvl();
    label.addObject(isisCube);

    PvlGroup &kernels = label.findObject("IsisCube").findGroup("Kernels");
    kernels.findKeyword("NaifFrameCode").setValue(ikid);
    kernels["ShapeModel"] = "Null";

    PvlGroup &dim = label.findObject("IsisCube").findObject("Core").findGroup("Dimensions");
    dim.findKeyword("Samples").setValue("10");
    dim.findKeyword("Lines").setValue("10");
    dim.findKeyword("Bands").setValue("2");

    PvlGroup &pixels = label.findObject("IsisCube").findObject("Core").findGroup("Pixels");
    pixels.findKeyword("Type").setValue("Real");

    PvlGroup &inst = label.findObject("IsisCube").findGroup("Instrument");
    std::istringstream iss(R"(
      Group = Instrument
        SpacecraftName            = "NEW HORIZONS"
        InstrumentId              = LEISA
        TargetName                = Jupiter
        SpacecraftClockStartCount = 1/0034933739:00000
        ExposureDuration          = 0.349
        StartTime                 = 2007-02-28T01:57:01.3882862
        StopTime                  = 2007-02-28T02:04:53.3882861
        FrameRate                 = 2.86533 <Hz>
      End_Group
    )");

    PvlGroup newInstGroup;
    iss >> newInstGroup;

    newInstGroup.findKeyword("InstrumentId").setValue(instrumentId);
    newInstGroup.findKeyword("SpacecraftName").setValue(spacecraftName);

    inst = newInstGroup;

    PvlGroup &bandBin = label.findObject("IsisCube").findGroup("BandBin");
    std::istringstream bss(R"(
      Group = BandBin
        Center       = (2.4892, 1.2204)
        Width        = (0.011228, 0.005505)
        OriginalBand = (1, 200)
      End_Group
    )");

    PvlGroup newBandBin;
    bss >> newBandBin;
    bandBin = newBandBin;

    std::istringstream alphaSS(R"(
      Group = AlphaCube
        AlphaSamples        = 256
        AlphaLines          = 1354
        AlphaStartingSample = 0.5
        AlphaStartingLine   = 229.5
        AlphaEndingSample   = 100.5
        AlphaEndingLine     = 329.5
        BetaSamples         = 100
        BetaLines           = 100
      End_Group
    )");

    PvlGroup alphaGroup;
    alphaSS >> alphaGroup;
    label.findObject("IsisCube").addGroup(alphaGroup);

    std::ifstream isdFile("data/leisa/nh_leisa.isd");
    isdFile >> isd;

    QString fileName = tempDir.path() + "/leisa.cub";
    delete testCube;
    testCube = new Cube();
    testCube->fromIsd(fileName, label, isd, "rw");

    LineManager line(*testCube);
    double pixelValue = 0.0;
    for(line.begin(); !line.end(); line++) {
      for(int i = 0; i < line.size(); i++) {
        line[i] = (double) pixelValue++;
      }
      testCube->write(line);
    }
  }


  void ApolloCube::SetUp() {
    TempTestingFiles::SetUp();

    testCube = new Cube();
    testCube->setDimensions(22900, 22900, 1);
    testCube->create(tempDir.path() + "/large.cub");

    LineManager line(*testCube);
    double pixelValue = 0.0;
    for(line.begin(); !line.end(); line++) {
      for(int i = 0; i < line.size(); i++) {
        line[i] = pixelValue;
      }

      pixelValue++;
      testCube->write(line);
    }

    PvlGroup reseaus("Reseaus");
    PvlKeyword samples = PvlKeyword("Sample", "200");
    samples += "400";
    samples += "600";

    PvlKeyword lines = PvlKeyword("Line", "200");
    lines += "400";
    lines += "600";

    PvlKeyword types = PvlKeyword("Type", "5");
    types += "5";
    types += "5";

    PvlKeyword valid = PvlKeyword("Valid", "1");
    valid += "1";
    valid += "1";

    reseaus += lines;
    reseaus += samples;
    reseaus += types;
    reseaus += valid;
    reseaus += PvlKeyword("Status", "Nominal");

    std::istringstream instStr (R"(
      Group = Instrument
          SpacecraftName = "APOLLO 15"
          InstrumentId   = METRIC
          TargetName     = MOON
          StartTime      = 1971-08-01T14:58:03.78
      End_Group
    )");

    PvlGroup instGroup;
    instStr >> instGroup;

    Pvl *lab = testCube->label();
    lab->findObject("IsisCube").addGroup(reseaus);
    lab->findObject("IsisCube").addGroup(instGroup);

    testCube->reopen("r");
  }

  void RingsCube::SetUp() {
    TempTestingFiles::SetUp();

    ring1 = new Cube("data/rings/rings1proj.cub", "r");
    ring2 = new Cube("data/rings/rings2proj.cub", "r");

    cubeListPath = tempDir.path() + "/filelist.txt";
    cubeFileList.append("data/rings/rings1proj.cub");
    cubeFileList.append("data/rings/rings2proj.cub");
    cubeFileList.write(cubeListPath);
  }


 void OsirisRexCube::setInstrument(QString ikid, QString instrumentId) {
    delete testCube;
    testCube = new Cube();

    FileName newCube(tempDir.path() + "/testing.cub");

    testCube->fromIsd(newCube, label, isd, "rw");

    PvlGroup &kernels = testCube->label()->findObject("IsisCube").findGroup("Kernels");
    kernels.findKeyword("NaifFrameCode").setValue(ikid);
    kernels["ShapeModel"] = "Null";

    PvlGroup &inst = testCube->label()->findObject("IsisCube").findGroup("Instrument");
    std::istringstream iss(R"(
      Group = Instrument
        MissionName               = OSIRIS-REx
        SpacecraftName            = OSIRIS-REX
        InstrumentId              = PolyCam
        TargetName                = Bennu
        StartTime                 = 2019-01-13T23:36:05.000
        ExposureDuration          = 100 <ms>
        SpacecraftClockStartCount = 1/0600694569.00000
        FocusPosition             = 21510
      End_Group
    )");

    PvlGroup newInstGroup;
    iss >> newInstGroup;

    newInstGroup.findKeyword("InstrumentId").setValue(instrumentId);

    inst = newInstGroup;

    PvlGroup &bandBin = label.findObject("IsisCube").findGroup("BandBin");
    std::istringstream bss(R"(
      Group = BandBin
        FilterName = Unknown
      End_Group
    )");

    PvlGroup newBandBin;
    bss >> newBandBin;
    bandBin = newBandBin;

    json nk;
    nk["BODY2101955_RADII"] =  {2825, 2675, 254};
    nk["INS"+ikid.toStdString()+"_FOCAL_LENGTH"] = 630.0;
    nk["INS"+ikid.toStdString()+"_PIXEL_SIZE"] = 8.5;
    nk["CLOCK_ET_-64_1/0600694569.00000_COMPUTED"] = "8ed6ae8930f3bd41";
    nk["INS"+ikid.toStdString()+"_TRANSX"] = {0.0, 0.0085, 0.0};
    nk["INS"+ikid.toStdString()+"_TRANSY"] = {0.0, 0.0, -0.0085};
    nk["INS"+ikid.toStdString()+"_ITRANSS"] = {0.0, 117.64705882353, 0.0};
    nk["INS"+ikid.toStdString()+"_ITRANSL"] = {0.0, 0.0, -117.64705882353};
    nk["INS"+ikid.toStdString()+"_CCD_CENTER"] = {511.5, 511.5};
    nk["BODY_FRAME_CODE"] = 2101955;

    PvlObject &naifKeywords = testCube->label()->findObject("NaifKeywords");
    PvlObject newNaifKeywords("NaifKeywords", nk);
    naifKeywords = newNaifKeywords;

    QString fileName = testCube->fileName();
    delete testCube;
    testCube = new Cube(fileName, "rw");
 }

  void CSMCubeFixture::SetUp() {
    SmallCube::SetUp();

    // Instrument group
    // Just need a target name
    PvlGroup instGroup("Instrument");
    instGroup += PvlKeyword("TargetName", "TestTarget");
    instGroup += PvlKeyword("InstrumentId", "TestId");
    testCube->putGroup(instGroup);

    // Kernels group
    // Just need a shapemodel specified
    PvlGroup kernGroup("Kernels");
    kernGroup += PvlKeyword("ShapeModel", "Null");
    testCube->putGroup(kernGroup);

    // CSMInfo group
    // This just has to exist, but fill it out for completeness and incase it
    // ever does matter
    PvlGroup infoGroup("CsmInfo");
    infoGroup += PvlKeyword("CSMPlatformID", "TestPlatform");
    infoGroup += PvlKeyword("CSMInstrumentId", "TestInstrument");
    infoGroup += PvlKeyword("ReferenceTime", "2000-01-01T11:58:55.816"); // J2000 epoch

    PvlKeyword paramNames("ModelParameterNames");
    paramNames += "TestNoneParam";
    paramNames += "TestFictitiousParam";
    paramNames += "TestRealParam";
    paramNames += "TestFixedParam";
    PvlKeyword paramUnits("ModelParameterUnits");
    paramUnits += "unitless";
    paramUnits += "m";
    paramUnits += "rad";
    paramUnits += "lines/sec";
    PvlKeyword paramTypes("ModelParameterTypes");
    paramTypes += "NONE";
    paramTypes += "FICTITIOUS";
    paramTypes += "REAL";
    paramTypes += "FIXED";

    infoGroup += paramNames;
    infoGroup += paramUnits;
    infoGroup += paramTypes;

    testCube->putGroup(infoGroup);

    // Register the mock with our plugin
    std::string mockModelName = QUuid().toString().toStdString();
    MockCsmPlugin loadablePlugin;
    loadablePlugin.registerModel(mockModelName, &mockModel);

    // CSMState BLOB
    Blob csmStateBlob("CSMState", "String");
    csmStateBlob.setData(mockModelName.c_str(), mockModelName.size());
    csmStateBlob.Label() += PvlKeyword("ModelName", QString::fromStdString(mockModelName));
    csmStateBlob.Label() += PvlKeyword("PluginName", QString::fromStdString(loadablePlugin.getPluginName()));
    testCube->write(csmStateBlob);
    filename = testCube->fileName();
    testCube->close();
    testCube->open(filename, "rw");
  }

  void CSMCameraFixture::SetUp() {
    CSMCubeFixture::SetUp();

    // Account for calls that happen while making a CSMCamera
    EXPECT_CALL(mockModel, getSensorIdentifier())
        .Times(2)
        .WillRepeatedly(::testing::Return("MockSensorID"));
    EXPECT_CALL(mockModel, getPlatformIdentifier())
        .Times(2)
        .WillRepeatedly(::testing::Return("MockPlatformID"));
    EXPECT_CALL(mockModel, getReferenceDateAndTime())
        .Times(1)
        .WillRepeatedly(::testing::Return("2000-01-01T11:58:55.816"));

    testCam = testCube->camera();
  }

  void CSMCameraSetFixture::SetUp() {
    CSMCameraFixture::SetUp();

    imagePt = csm::ImageCoord(4.5, 4.5);
    groundPt = csm::EcefCoord(wgs84.getSemiMajorRadius(), 0, 0);
    imageLocus = csm::EcefLocus(wgs84.getSemiMajorRadius() + 50000, 0, 0, -1, 0, 0);

    // Setup the mock for setImage and ensure it succeeds
    EXPECT_CALL(mockModel, imageToRemoteImagingLocus(MatchImageCoord(imagePt), ::testing::_, ::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Return(imageLocus));
    EXPECT_CALL(mockModel, getImageTime)
        .Times(1)
        .WillOnce(::testing::Return(10.0));

    ASSERT_TRUE(testCam->SetImage(5, 5)); // Assert here so that the test code doesn't run if the camera isn't set
  }

  void CSMCameraDemFixture::SetUp() {
    CSMCubeFixture::SetUp();

    // Record the demRadius at 0 lat, 0 lon
    demRadius = 3394200.43980104;

    // Update the shapemodel on the cube
    PvlGroup &kernGroup = testCube->group("Kernels");
    kernGroup.addKeyword(PvlKeyword("ShapeModel", "data/CSMCamera/mola_compressed_prep.cub"), Pvl::Replace);

    // Close and re-open the cube, then save off the new camera
    testCube->close();
    testCube->open(filename, "rw");

    // Account for calls that happen while making a CSMCamera
    EXPECT_CALL(mockModel, getSensorIdentifier())
        .Times(2)
        .WillRepeatedly(::testing::Return("MockSensorID"));
    EXPECT_CALL(mockModel, getPlatformIdentifier())
        .Times(2)
        .WillRepeatedly(::testing::Return("MockPlatformID"));
    EXPECT_CALL(mockModel, getReferenceDateAndTime())
        .Times(1)
        .WillRepeatedly(::testing::Return("2000-01-01T11:58:55.816"));

    testCam = testCube->camera();
  }

  void HistoryBlob::SetUp() {
    TempTestingFiles::SetUp();

    std::istringstream hss(R"(
      Object = mroctx2isis
        IsisVersion       = "4.1.0  | 2020-07-01"
        ProgramVersion    = 2016-06-10
        ProgramPath       = /Users/acpaquette/repos/ISIS3/build/bin
        ExecutionDateTime = 2020-07-01T16:48:40
        HostName          = Unknown
        UserName          = acpaquette
        Description       = "Import an MRO CTX image as an Isis cube"

        Group = UserParameters
          FROM    = /Users/acpaquette/Desktop/J03_045994_1986_XN_18N282W.IMG
          TO      = /Users/acpaquette/Desktop/J03_045994_1986_XN_18N282W_isis.cub
          SUFFIX  = 18
          FILLGAP = true
        End_Group
      End_Object)");

    hss >> historyPvl;

    std::ostringstream ostr;
    ostr << historyPvl;
    std::string histStr = ostr.str();

    historyBlob = Blob("IsisCube", "History");
    historyBlob.setData(histStr.c_str(), histStr.size());
  }


  void MgsMocCube::SetUp() {
    TempTestingFiles::SetUp();

    QString testPath = tempDir.path() + "/test.cub";
    QFile::copy("data/mgsImages/mocImage.cub", testPath);
    testCube.reset(new Cube(testPath));
  }


  void MgsMocCube::TearDown() {
    testCube.reset();
  }


  void NullPixelCube::SetUp() {
    TempTestingFiles::SetUp();

    testCube = new Cube();
    testCube->setDimensions(10, 10, 10);
    QString path = tempDir.path() + "/null.cub";
    testCube->create(path);

    LineManager line(*testCube);
    for(line.begin(); !line.end(); line++) {
      for(int i = 0; i < line.size(); i++) {
        line[i] =  NULL8;
      }
      testCube->write(line);
    }
  }


  void NullPixelCube::TearDown() {
    if (testCube->isOpen()) {
      testCube->close();
    }

    if (testCube) {
      delete testCube;
    }
  }


  void MiniRFNetwork::SetUp() {
    TempTestingFiles::SetUp();

    testCube1 = new Cube("data/miniRFImage/LSZ_00455_1CD_XKU_87S324_V1_S1_Null.crop.cub");
    testCube2 = new Cube("data/miniRFImage/LSZ_00457_1CD_XKU_87S321_V1_S1_Null.crop.cub");
    testCube3 = new Cube("data/miniRFImage/LSZ_00459_1CD_XKU_88S327_V1_S1_Null.crop.cub");

    cubeList = new FileList();

    cubeList->append(testCube1->fileName());
    cubeList->append(testCube2->fileName());
    cubeList->append(testCube3->fileName());


    cubeListFile = tempDir.path() + "/cubes.lis";
    cubeList->write(cubeListFile);

    network = new ControlNet("data/miniRFImage/Cabeus_Orbit400_withSS_AprioriPts.net");
    controlNetPath = tempDir.path() + "/miniRFNet.net";
    network->Write(controlNetPath);
  }

  void MiniRFNetwork::TearDown() {
    if (testCube1->isOpen()) {
      testCube1->close();
    }
    delete testCube1;
    if (testCube2->isOpen()) {
      testCube2->close();
    }
    delete testCube2;
    if (testCube3->isOpen()) {
      testCube3->close();
    }
    delete testCube3;

    if (cubeList) {
      delete cubeList;
    }
  }

  void VikThmNetwork::SetUp() {
    TempTestingFiles::SetUp();

    testCube1 = new Cube("data/vikingThemisNetwork/F704b51.lev1_slo_crop.cub");
    testCube2 = new Cube("data/vikingThemisNetwork/F857a32.lev1_slo_crop.cub");
    testCube3 = new Cube("data/vikingThemisNetwork/I28234014RDR_crop.cub");
    testCube4 = new Cube("data/vikingThemisNetwork/I52634011RDR_crop.cub");

    cubeList = new FileList();

    cubeList->append(testCube1->fileName());
    cubeList->append(testCube2->fileName());
    cubeList->append(testCube3->fileName());
    cubeList->append(testCube4->fileName());


    cubeListFile = tempDir.path() + "/cubes.lis";
    cubeList->write(cubeListFile);

    network = new ControlNet("data/vikingThemisNetwork/themis_dayir_VO_arcadia_extract_hand.net");
    controlNetPath = tempDir.path() + "/vikThmNet.net";
    network->Write(controlNetPath);
  }

  void VikThmNetwork::TearDown() {
    if (testCube1->isOpen()) {
      testCube1->close();
    }
    delete testCube1;
    if (testCube2->isOpen()) {
      testCube2->close();
    }
    delete testCube2;
    if (testCube3->isOpen()) {
      testCube3->close();
    }
    delete testCube3;
    if (testCube4->isOpen()) {
      testCube4->close();
    }
    delete testCube4;

    if (cubeList) {
      delete cubeList;
    }
  }

  void CSMNetwork::SetUp(){
    QString APP_XML = FileName("$ISISROOT/bin/xml/csminit.xml").expanded();
    QVector<QString> fNames = {"/Test_A", "/Test_B",
                               "/Test_C", "/Test_D",
                               "/Test_E", "/Test_F",
                               "/Test_G", "/Test_H",
                               "/Test_I", "/Test_J"
                              };

    cubes.fill(nullptr, 10);

    cubeList = new FileList();
    cubeListFile = tempDir.path() + "/cubes.lis";
    // Create CSMInit-ed cubes
    for (int i = 0; i < cubes.size() ; i++){
      cubes[i] = new Cube();
      cubes[i]->setDimensions(1024,1024,1);
      FileName cubName = FileName(tempDir.path()+fNames[i]+".cub");
      cubes[i]->create(cubName.expanded());
      cubeList->append(cubes[i]->fileName());
      QVector<QString> args = {"from="+cubName.expanded(),
                               "state=data/CSMNetwork/"+fNames[i]+".json",
                               "modelname=TestCsmModel",
                               "pluginname=TestCsmPlugin"
                              };
      UserInterface ui(APP_XML, args);
      csminit(ui);
    }
    cubeList->write(cubeListFile);
  }

  void CSMNetwork::TearDown() {
    for(int i = 0; i < cubes.size(); i++) {
      if(cubes[i] && cubes[i]->isOpen()) {
        delete cubes[i];
      }
    }

    if (cubeList) {
      delete cubeList;
    }
  }

  void ClipperWacFcCube::SetUp() {
    TempTestingFiles::SetUp();

    QString testPath = tempDir.path() + "/test.cub";
    QFile::copy("data/clipper/ClipperWacFc.cub", testPath);
    wacFcCube = new Cube(testPath);

    PvlGroup &wacKernels = wacFcCube->label()->findObject("IsisCube").findGroup("Kernels");
    wacKernels.findKeyword("NaifFrameCode").setValue("-159102");

    double offset = 10;
    AlphaCube aCube(wacFcCube->sampleCount(), wacFcCube->lineCount(),
                    wacFcCube->sampleCount()-offset, wacFcCube->lineCount() - offset,
                    0, offset, wacFcCube->sampleCount(), wacFcCube->lineCount());

    aCube.UpdateGroup(*wacFcCube);

    wacFcCube->reopen("rw");
  }

  void ClipperWacFcCube::TearDown() {
    if (wacFcCube) {
      delete wacFcCube;
    }
  }

  void ClipperNacRsCube::SetUp() {
    DefaultCube::SetUp();

    delete testCube;
    testCube = new Cube();

    FileName newCube(tempDir.path() + "/testing.cub");

    testCube->fromIsd(newCube, label, isd, "rw");

    PvlGroup &kernels = testCube->label()->findObject("IsisCube").findGroup("Kernels");
    kernels.findKeyword("NaifFrameCode").setValue("-159101");

    PvlGroup &inst = testCube->label()->findObject("IsisCube").findGroup("Instrument");
    std::istringstream iss(R"(
      Group = Instrument
        SpacecraftName            = Clipper
        InstrumentId              = EIS-NAC-RS
        TargetName                = Europa
        StartTime                 = 2025-01-01T00:00:00.000
        JitterSampleCoefficients = (0.0, 0.0, 0.0)
        JitterLineCoefficients   = (0.0, 0.0, 0.0)
      End_Group
    )");

    PvlGroup newInstGroup;
    iss >> newInstGroup;
    inst = newInstGroup;

    PvlObject &naifKeywords = testCube->label()->findObject("NaifKeywords");
    std::istringstream nk(R"(
      Object = NaifKeywords
        BODY_CODE               = 502
        BODY502_RADII           = (1562.6, 1560.3, 1559.5)
        BODY_FRAME_CODE         = 10024
        INS-159101_FOCAL_LENGTH = 150.40199
        INS-159101_PIXEL_PITCH  = 0.014
        INS-159101_TRANSX       = (0.0, 0.014004651, 0.0)
        INS-159101_TRANSY       = (0.0, 0.0, 0.01399535)
        INS-159101_ITRANSS      = (0.0, 71.404849, 0.0)
        INS-159101_ITRANSL      = (0.0, 0.0, 71.4523)
        INS-159101_OD_K         = (0.0, 0.0, 0.0)
      End_Object
    )");

    PvlObject newNaifKeywords;
    nk >> newNaifKeywords;
    naifKeywords = newNaifKeywords;

    QString fileName = testCube->fileName();
    delete testCube;
    testCube = new Cube(fileName, "rw");

    double offset = 10;
    AlphaCube aCube(testCube->sampleCount(), testCube->lineCount(),
                    testCube->sampleCount()-offset, testCube->lineCount() - offset,
                    0, offset, testCube->sampleCount(), testCube->lineCount());

    aCube.UpdateGroup(*testCube);
    testCube->reopen("rw");
  }

  void ClipperNacRsCube::TearDown() {
    if (testCube) {
      delete testCube;
    }
  }

  void ClipperPbCube::setInstrument(QString instrumentId) {
    TempTestingFiles::SetUp();

    if (instrumentId == "EIS-NAC-PB") {
      QString testPath = tempDir.path() + "/nacTest.cub";
      QFile::copy("data/clipper/ClipperNacPb.cub", testPath);
      testCube = new Cube(testPath, "rw");
    }
    else if (instrumentId == "EIS-WAC-PB") {
      QString testPath = tempDir.path() + "/wacTest.cub";
      QFile::copy("data/clipper/ClipperWacPb.cub", testPath);
      testCube = new Cube(testPath, "rw");
    }
  }
}
