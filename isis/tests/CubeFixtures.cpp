#include "CubeFixtures.h"

#include "Brick.h"
#include "LineManager.h"
#include "Pvl.h"
#include "PvlObject.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "SpecialPixel.h"

namespace Isis {

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


  void ApolloCube::SetUp() {
    TempTestingFiles::SetUp();

    testCube = new Cube();
    testCube->setDimensions(22900, 22900, 1);
    testCube->create(tempDir.path() + "/large.cub");

    // Reseau centers as {sample, line} pairs
    reseaus = {{200, 200}, {400, 400}, {600, 600}};
    reseauSize = 103;
    int reseauValue = 100;

    Brick brick(reseauSize,reseauSize,1,testCube->pixelType());
    for (size_t res=0; res<reseaus.size(); res++) {
      int baseSamp = (int)(reseaus[res].first+0.5) - (reseauSize/2);
      int baseLine = (int)(reseaus[res].second+0.5) - (reseauSize/2);
      brick.SetBasePosition(baseSamp,baseLine,1);
      testCube->read(brick);
      // Fill the surrounding area with a base number
      for (int i = 0; i < reseauSize; i++) {
        for (int j = 0; j < reseauSize; j++) {
          brick[reseauSize*i + j] = res;
        }
      }

      // Create reseau
      for (int i = 0; i < reseauSize; i++) {
        for (int j = -2; j < 3; j++) {
          // Vertical line
          brick[reseauSize * i + reseauSize/2 + j] = reseauValue;

          // Horizontal line
          brick[reseauSize * (reseauSize/2 + j) + i] = reseauValue;
        }
      }
      testCube->write(brick);
    }

    PvlGroup reseausGroup("Reseaus");
    PvlKeyword samples = PvlKeyword("Sample", toString(reseaus[0].first));
    PvlKeyword lines = PvlKeyword("Line", toString(reseaus[0].second));
    PvlKeyword types = PvlKeyword("Type", "5");
    PvlKeyword valid = PvlKeyword("Valid", "1");
    for (size_t i = 1; i < reseaus.size(); i++) {
      samples += toString(reseaus[i].first);
      lines += toString(reseaus[i].second);
      types += "5";
      valid += "1";
    }

    reseausGroup += lines;
    reseausGroup += samples;
    reseausGroup += types;
    reseausGroup += valid;
    reseausGroup += PvlKeyword("Status", "Nominal");

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
    lab->findObject("IsisCube").addGroup(reseausGroup);
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
    cubeFileList.write(cubeListPath.toStdString());
  }

}