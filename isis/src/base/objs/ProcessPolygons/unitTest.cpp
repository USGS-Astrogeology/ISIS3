/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "Isis.h"

#include <QFile>

#include "ProcessPolygons.h"
#include "ProcessByLine.h"
#include "SpecialPixel.h"
#include "TextFile.h"

using namespace Isis;
using namespace std;

bool threeVectors(std::vector<double> &samples, std::vector<double> &lines,
                  std::vector<double> &value);
bool oneBandOneValue(std::vector<double> &samples, std::vector<double> &lines,
                     int &band, double &value);
void writeAscii(Isis::Buffer &in);

std::vector<double> diamondSamples, diamondLines, values1, values2;
std::vector<int> bands;

void IsisMain() {
  std::vector<double> samples, lines, values;

  cout << "Testing Isis::ProcessPolygons Class ... " << endl;
  ProcessPolygons p;
  ProcessByLine pbl;

  CubeAttributeOutput out_atts;
  out_atts.setPixelType(Real);

  QString output1 = "processPolygonsTest.cub";
  QString output2 = "processPolygonsTest_count.cub";
  p.SetStatCubes(output1 , output2 , out_atts, 4, 4, 2);

  samples.clear();
  lines.clear();

  double value = 0;
  for(int b = 1; b < 3; b++) {
    for(int l = 0; l < 4; l++) {
      for(int s = 0; s < 4; s++) {

        samples.push_back(s);
        samples.push_back(s + 5.0);
        samples.push_back(s);
        samples.push_back(s - 5.0);

        lines.push_back(l - 5.0);
        lines.push_back(l);
        lines.push_back(l + 5.0);
        lines.push_back(l);
        value = s + l + b;

        p.Rasterize(samples, lines, b, value);

        samples.clear();
        lines.clear();
      }
    }
  }

  p.EndProcess();


  /*   Write out the data    */

  CubeAttributeInput atts;

  cout << "FileName: " << output1.toStdString() << endl;
  pbl.SetInputCube(output1, atts, 0);
  pbl.StartProcess(writeAscii);
  pbl.EndProcess();

  cout << "FileName: " << output2.toStdString() << endl;
  pbl.SetInputCube(output2, atts, 0);
  pbl.StartProcess(writeAscii);
  pbl.EndProcess();

  QFile::remove(output1);
  QFile::remove(output2);
}

/**
 * This method reads in the cube file line by line and prints
 * out the DN value of each non-null pixel.
 *
 * @param in
 */
void writeAscii(Isis::Buffer &in) {
  bool notNull = false;
  int index = in.size() - 1;
  for(int i = 0; i < in.size(); i++) {
    if(in[index -i] > 0) {
      cout << "Band: " << in.Band() << " DN: " << in[index -i] << " ";
      notNull = true;
    }
  }
  if(notNull) {
    cout << std::endl;
  }
}

