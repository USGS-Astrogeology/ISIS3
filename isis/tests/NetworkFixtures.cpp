#include "NetworkFixtures.h"

#include "ImagePolygon.h"
#include "LineManager.h"
#include "OriginalLabel.h"

namespace Isis {

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
    cube1->fromIsd(tempDir.path().toStdString() + "/cube1.cub", labelPath1, *isdPath1, "rw");

    ImagePolygon poly;
    coords = {{30, 0},
              {30, 10},
              {35, 10},
              {35, 0},
              {30, 0}};
    poly.Create(coords);
    cube1->write(poly);

    cube2 = new Cube();
    cube2->fromIsd(tempDir.path().toStdString() + "/cube2.cub", labelPath2, *isdPath2, "rw");

    coords = {{31, 1},
              {31, 11},
              {36, 11},
              {36, 1},
              {31, 1}};
    poly.Create(coords);
    cube2->write(poly);

    cube3 = new Cube();
    cube3->fromIsd(tempDir.path().toStdString() + "/cube3.cub", labelPath3, *isdPath3, "rw");

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
    cubeList->append(cube1->fileName().toStdString());
    cubeList->append(cube2->fileName().toStdString() );

    twoCubeListFile = tempDir.path() + "/2cubes.lis";
    cubeList->write(twoCubeListFile.toStdString());
    cubeList->append(cube3->fileName().toStdString());

    cubeListFile = tempDir.path() + "/cubes.lis";
    cubeList->write(cubeListFile.toStdString());

    networkFile = "data/threeImageNetwork/controlnetwork.net";

    network = new ControlNet();
    network->ReadControl(networkFile);

    cube1map = new Cube();
    cube2map = new Cube();
    cube3map = new Cube();
    cube1map->fromIsd(tempDir.path().toStdString() + "/cube1map.cub", mappedLabelPath1, *isdPath1, "rw");
    cube2map->fromIsd(tempDir.path().toStdString() + "/cube2map.cub", mappedLabelPath2, *isdPath2, "rw");
    cube3map->fromIsd(tempDir.path().toStdString() + "/cube3map.cub", mappedLabelPath3, *isdPath3, "rw");

    grid_fastgeom_config = "data/threeImageNetwork/grid_fastgeom_algorithm.pvl";
    radial_fastgeom_config = "data/threeImageNetwork/radial_fastgeom_algorithm.pvl";
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
      isdFiles.push_back(FileName("data/apolloNetwork/apolloImage"+toString(n)+".isd"));
      labelFiles.push_back(FileName("data/apolloNetwork/apolloImage"+toString(n)+".pvl"));
      cubes[i] = new Cube();
      cubes[i]->fromIsd(tempDir.path().toStdString() + "/cube"+toString(n)+".cub", labelFiles[i], isdFiles[i], "rw");
      cubeList->append(cubes[i]->fileName().toStdString());
    }

    cubeListFile = tempDir.path() + "/cubes.lis";
    cubeList->write(cubeListFile.toStdString());

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

      cubeL->fromIsd(cubeLPath.toStdString(), labelPathL, *isdPathL, "rw");
      Pvl originalPdsLabL("data/observationPair/observationImageLOriginalLabel.pvl");
      OriginalLabel origLabel(originalPdsLabL);
      cubeL->write(origLabel);
      cubeL->reopen("rw");

      cubeR->fromIsd(cubeRPath.toStdString(), labelPathR, *isdPathR, "rw");

      cubeList = new FileList();
      cubeList->append(cubeL->fileName().toStdString());
      cubeList->append(cubeR->fileName().toStdString());

      cubeListFile = tempDir.path() + "/cubes.lis";
      cubeList->write(cubeListFile.toStdString());

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


  void MiniRFNetwork::SetUp() {
    TempTestingFiles::SetUp();

    testCube1 = new Cube("data/miniRFImage/LSZ_00455_1CD_XKU_87S324_V1_S1_Null.crop.cub");
    testCube2 = new Cube("data/miniRFImage/LSZ_00457_1CD_XKU_87S321_V1_S1_Null.crop.cub");
    testCube3 = new Cube("data/miniRFImage/LSZ_00459_1CD_XKU_88S327_V1_S1_Null.crop.cub");

    cubeList = new FileList();

    cubeList->append(testCube1->fileName().toStdString());
    cubeList->append(testCube2->fileName().toStdString());
    cubeList->append(testCube3->fileName().toStdString());


    cubeListFile = tempDir.path() + "/cubes.lis";
    cubeList->write(cubeListFile.toStdString());

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

    cubeList->append(testCube1->fileName().toStdString());
    cubeList->append(testCube2->fileName().toStdString());
    cubeList->append(testCube3->fileName().toStdString());
    cubeList->append(testCube4->fileName().toStdString());


    cubeListFile = tempDir.path() + "/cubes.lis";
    cubeList->write(cubeListFile.toStdString());

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


  void LidarObservationPair::SetUp() {
      TempTestingFiles::SetUp();

      FileName labelPath1 = FileName("data/lidarObservationPair/lidarObservationImage1.pvl");
      FileName labelPath2 = FileName("data/lidarObservationPair/lidarObservationImage2.pvl");

      isdPath1 = new FileName("data/lidarObservationPair/lidarObservationImage1.isd");
      isdPath2 = new FileName("data/lidarObservationPair/lidarObservationImage2.isd");

      cube1 = new Cube();
      cube2 = new Cube();

      cube1Path = tempDir.path() + "/lidarObservationPair1.cub";
      cube2Path = tempDir.path() + "/lidarObservationPair2.cub";

      cube1->fromIsd(cube1Path.toStdString(), labelPath1, *isdPath1, "rw");
      Pvl originalPdsLab1("data/lidarObservationPair/lidarObservationImage1OriginalLabel.pvl");
      OriginalLabel origLabel(originalPdsLab1);
      cube1->write(origLabel);
      cube1->reopen("rw");

      cube2->fromIsd(cube2Path.toStdString(), labelPath2, *isdPath2, "rw");

      cubeList = new FileList();
      cubeList->append(cube1->fileName().toStdString());
      cubeList->append(cube2->fileName().toStdString());

      cubeListFile = tempDir.path() + "/cubes.lis";
      cubeList->write(cubeListFile.toStdString());

      csvPath = "data/lidarObservationPair/lidarPoints.csv";
  }

  void LidarObservationPair::TearDown() {
    delete cubeList;

    if (cube1) {
      delete cube1;
    }

    if (cube2) {
      delete cube2;
    }

    delete isdPath1;
    delete isdPath2;
  }

  void LidarNetwork::SetUp() {
      LidarObservationPair::SetUp();

      // Lidar Data file was generated from the following
      // lrolola2isis from=../isis/tests/data/lidarObservationPair/lidarPoints.csv cubes=cubes.lis point_range_sigma=10 point_lat=10 point_rad=10 point_lon=10 pointid="Lidar????" outputtype=JSON to=lidarData.json
      // Additionally the point Lidar0068 may need to be removed from a newly generated data file
      lidarDataPath = "data/LidarNetwork/lidarData.json";
      rangeData.read(lidarDataPath.toStdString());
      controlNetPath = "data/LidarNetwork/network.pvl";

      network = new ControlNet(controlNetPath);

      // Create the filelist

  }

  void LidarNetwork::TearDown() {
    if (network) {
      delete network;
    }
  }

}