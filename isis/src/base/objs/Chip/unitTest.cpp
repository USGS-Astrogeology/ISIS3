/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */#include <cstdlib>

#include <iomanip>
#include <iostream>

#include "Affine.h"
#include "Chip.h"
#include "Cube.h"
#include "FileName.h"
#include "IString.h"
#include "LineManager.h"
#include "Preference.h"
#include "SpecialPixel.h"
#include "geos/geom/Coordinate.h"
#include "geos/geom/CoordinateSequence.h"
#include "geos/geom/GeometryFactory.h"
#include "geos/geom/Polygon.h"
#include "geos/geom/MultiPolygon.h"

using namespace std;
using namespace Isis;

void ReportError(QString err);

int main() {
  Preference::Preferences(true);
  Chip chip(51, 50);
  cout << "Test basics" << endl;
  cout << chip.Samples() << endl;
  cout << chip.Lines() << endl;

  chip.TackCube(453.5, 568.5);
  cout << chip.TackSample() << endl;
  cout << chip.TackLine() << endl;

  cout << "Test chip-to-cube and cube-to-chip mapping" << endl;
  chip.SetChipPosition(chip.TackSample(), chip.TackLine());
  cout << chip.CubeSample() << endl;
  cout << chip.CubeLine() << endl;

  chip.SetChipPosition(1.0, 1.0);
  cout << chip.CubeSample() << endl;
  cout << chip.CubeLine() << endl;

  chip.SetCubePosition(chip.CubeSample(), chip.CubeLine());
  cout << chip.ChipSample() << endl;
  cout << chip.ChipLine() << endl;


  cout << "Test assignment of chip data to constant" << endl;
  chip.SetAllValues(10.0);
  for(int i = 1; i <= chip.Lines(); i++) {
    for(int j = 1; j <= chip.Samples(); j++) {
      double value = chip.GetValue(j, i);
      if(value != 10.0) {
        cout << "bad constant (!= 10) at " << j << ", " << i << endl;
      }
    }
  }

  cout << "Test loading chip data" << endl;
  for(int i = 1; i <= chip.Lines(); i++) {
    for(int j = 1; j <= chip.Samples(); j++) {
      chip.SetValue(j, i, (double)(i * 100 + j));
    }
  }

  for(int i = 1; i <= chip.Lines(); i++) {
    for(int j = 1; j <= chip.Samples(); j++) {
      double value = chip.GetValue(j, i);
      if(value != (double)(i * 100 + j)) {
        cout << "bad at " << j << ", " << i << endl;
      }
    }
  }




  chip.SetValidRange(0.0, 5050.0);
  cout << "Valid tests" << endl;
  // is chip valid at 51, 50?
  cout << chip.IsValid(chip.Samples(), chip.Lines()) << endl;
  // is chip valid at 50, 50?
  cout << chip.IsValid(chip.Samples() - 1, chip.Lines()) << endl;
  // is at least 95% of chip values valid?
  cout << chip.IsValid(95.0) << endl;
  // is at least 99.99% of chip values valid?
  cout << chip.IsValid(99.99) << endl;

  cout << "Extract test" << endl;
  // Extract 4 by 3 subchip at 26, 25
  Chip sub = chip.Extract(4, 3, chip.TackSample(), chip.TackLine());
  for(int i = 1; i <= sub.Lines(); i++) {
    for(int j = 1; j <= sub.Samples(); j++) {
      cout << sub.GetValue(j, i) << " ";
    }
    cout << endl;
  }

  cout << "Test writing chip" << endl;
  chip.Write("junk.cub");

  Cube junk;
  junk.open("junk.cub");
  LineManager line(junk);

  for(int i = 1; i <= chip.Lines(); i++) {
    line.SetLine(i);
    junk.read(line);
    for(int j = 1; j <= chip.Samples(); j++) {
      double value = chip.GetValue(j, i);
      if(value != line[j-1]) {
        cout << "bad at " << j << ", " << i << endl;
      }
    }
  }

  cout << "Test load chip from cube with rotation" << endl;
  chip.TackCube(26.0, 25.0);
  chip.Load(junk, 45.0);
  for(int i = 1; i <= chip.Lines(); i++) {
    for(int j = 1; j <= chip.Samples(); j++) {
      cout << std::setw(14) << chip.GetValue(j, i) << " ";
    }
    cout << endl;
  }

  cout << "Test load chip from cube with rotation and clipping polygon " << endl;
  chip.TackCube(26.0, 25.0);

  geos::geom::CoordinateSequence pts;
  pts.add(geos::geom::Coordinate(23.0, 22.0));
  pts.add(geos::geom::Coordinate(28.0, 22.0));
  pts.add(geos::geom::Coordinate(28.0, 27.0));
  pts.add(geos::geom::Coordinate(25.0, 28.0));
  pts.add(geos::geom::Coordinate(23.0, 22.0));
  vector <const geos::geom::Geometry *> polys;
  geos::geom::GeometryFactory::Ptr gf = geos::geom::GeometryFactory::create();
  polys.push_back(gf->createPolygon(gf->createLinearRing(pts)).release());
  geos::geom::MultiPolygon *mPolygon = gf->createMultiPolygon(polys).release();

  chip.SetClipPolygon(*mPolygon);
  chip.Load(junk, 45.0);
  for(int i = 1; i <= chip.Lines(); i++) {
    for(int j = 1; j <= chip.Samples(); j++) {
      cout << std::setw(14) << chip.GetValue(j, i) << " ";
    }
    cout << endl;
  }

  // Test affine transformation
  cout << "\nTesting Affine transformation extraction (-1, -1)...\n";
  Affine affine;
  affine.Translate(-1.0, -1.0);

  Chip mychip(51, 50);          //  Needed because chip has poly clipping
  mychip.TackCube(26.0, 25.0);
  mychip.Load(junk);
  mychip.SetChipPosition(mychip.TackSample(), mychip.TackLine());
  cout << "Cube Sample, Line = " << mychip.CubeSample() << ", "
       << mychip.CubeLine() << endl;
  Chip shift(25, 25);
  mychip.Extract(shift, affine);
  // shift.SetChipPosition(shift.TackSample(), shift.TackLine());
  cout << "Shift Cube Sample, Line = " << shift.CubeSample() << ", "
       << shift.CubeLine() << endl;

  Chip io = shift;
  io.TackCube(25.0, 24.0);
  io.Load(junk);
  io.SetChipPosition(io.TackSample(), io.TackLine());
  cout << "New Cube Sample, Line = " << io.CubeSample() << ", "
       << io.CubeLine() << endl;

  int ioNull(0), shiftNull(0);
  double sumDiff(0.0);
  for(int il = 1 ; il <= io.Lines() ; il++) {
    for(int is = 1 ; is <= io.Samples() ; is++) {
      if(IsSpecial(io.GetValue(is, il))) {
        ioNull++;
      }
      else if(IsSpecial(shift.GetValue(is, il))) {
        shiftNull++;
      }
      else {
        sumDiff += io.GetValue(is, il) - shift.GetValue(is, il);
      }
    }
  }

  cout << "I/O Nulls:   " << ioNull << endl;
  cout << "Shift Nulls: " << shiftNull << endl;
  cout << "Sum Diff:    " << sumDiff << endl;

  cout << "\nTesting direct Affine Application...\n";
  Chip affchip(25, 25);
  affchip.TackCube(25.0, 24.0);
  affchip.SetTransform(io.GetTransform());
  affchip.SetChipPosition(affchip.TackSample(), affchip.TackLine());
  cout << "Affine Cube Sample, Line = " << affchip.CubeSample() << ", "
       << affchip.CubeLine() << endl;

  cout << "\nTest reading with new Affine transform...\n";
  affchip.Load(junk, io.GetTransform());
  ioNull = shiftNull = 0;
  sumDiff = 0.0;
  for(int il = 1 ; il <= io.Lines() ; il++) {
    for(int is = 1 ; is <= io.Samples() ; is++) {
      if(IsSpecial(io.GetValue(is, il))) {
        ioNull++;
      }
      else if(IsSpecial(affchip.GetValue(is, il))) {
        shiftNull++;
      }
      else {
        sumDiff += io.GetValue(is, il) - affchip.GetValue(is, il);
      }
    }
  }

  cout << "I/O Nulls:   " << ioNull << endl;
  cout << "Shift Nulls: " << shiftNull << endl;
  cout << "Sum Diff:    " << sumDiff << endl;

  affchip.SetChipPosition(affchip.TackSample(), affchip.TackLine());
  cout << "Affine Cube loaded at Sample, Line = " << affchip.CubeSample() << ", "
       << affchip.CubeLine() << endl;


  // Test Load using match chip method
  cout << "\nTest reading with match chip and cube...\n";
  Cube junkCube;
  junkCube.open("$ISISTESTDATA/isis/src/base/unitTestData/ab102401_ideal.cub");
  // 4 by 4 chip at samle 1000 line 500
  Chip matchChip(4, 4);
  matchChip.TackCube(1000, 500);
  matchChip.Load(junkCube);
  cout << "\nMatch chip values..." << endl;
  for(int i = 1; i <= matchChip.Lines(); i++) {
    for(int j = 1; j <= matchChip.Samples(); j++) {
      cout << std::setw(14) << matchChip.GetValue(j, i) << " ";
    }
    cout << endl;
  }
  // make sure that if we create a new chip from the same cube that is matched
  // to the match chip, the chips should be almost identical
  Chip newChip(4, 4);
  newChip.TackCube(1000, 500);
  newChip.Load(junkCube, matchChip, junkCube);
  cout << "\nLoading new chip values from match chip..." << endl;
  cout << "Passes if difference is less than EPSILON = " << 2E-6 << endl;
  for(int i = 1; i <= newChip.Lines(); i++) {
    for(int j = 1; j <= newChip.Samples(); j++) {
      double difference = newChip.GetValue(j, i) - matchChip.GetValue(j, i);
      if(fabs(difference) > 2E-6) {
        cout << "bad at " << j << ", " << i << endl;
        cout << "difference at " << j << ", " << i << " is " << difference << endl;
      }
      else{
        cout << "\tPASS\t\t";
        // the following comment prints actual difference.
        // cout << std::setw(14) << difference << "\t";
      }
    }
    cout  << endl;
  }


  cout << endl;
  cout << endl;
  cout << "Test interpolator set/get methods" << endl;
  cout << "default: " << chip.GetReadInterpolator() << endl;
  chip.SetReadInterpolator(Isis::Interpolator::NearestNeighborType);
  cout << "nearest neighbor: " << chip.GetReadInterpolator() << endl;
  chip.SetReadInterpolator(Isis::Interpolator::BiLinearType);
  cout << "bilinear: " << chip.GetReadInterpolator() << endl;
  chip.SetReadInterpolator(Isis::Interpolator::CubicConvolutionType);
  cout << "cubic convolution: " << chip.GetReadInterpolator() << endl;

  cout << endl;
  cout << endl;
  cout << "Generate Errors:" << endl;
  Cube junkCube2;
  junkCube2.open("$ISISTESTDATA/isis/src/base/unitTestData/f319b18_ideal.cub");
  // 4 by 4 chip at samle 1000 line 500
  matchChip.TackCube(1, 1);
  matchChip.Load(junkCube2);

  cout << "Try to set interpolator to type 0 (Interpolator::None):" << endl;
  try {
    chip.SetReadInterpolator(Isis::Interpolator::None);
  }
  catch(IException &e) {
    ReportError(QString::fromStdString(e.toString()));
  }
  cout << "Try to set interpolator to type 3 (enum value not assigned):" << endl;
  try {
    chip.SetReadInterpolator((Isis::Interpolator::interpType) 3);
  }
  catch(IException &e) {
    ReportError(QString::fromStdString(e.toString()));
  }
  cout << "Try to set chip size with input parameter equal to 0:" << endl;
  try {
    newChip.SetSize(0, 1);
  }
  catch(IException &e) {
    ReportError(QString::fromStdString(e.toString()));
  }
  cout << "Try to load a cube that is not camera or map projection:" << endl;
  try {
    newChip.Load(junk, matchChip, junkCube);
  }
  catch(IException &e) {
    ReportError(QString::fromStdString(e.toString()));
  }
  cout << "Try to load a cube with a match cube that is not camera or map projection:" << endl;
  try {
    newChip.Load(junkCube, matchChip, junk);
  }
  catch(IException &e) {
    ReportError(QString::fromStdString(e.toString()));
  }
  cout << "Try to load a cube with match chip and cube that can not find at least 3 points for Affine Transformation:" << endl;
  try {
    newChip.Load(junkCube, matchChip, junkCube2);
  }
  catch(IException &e) {
    ReportError(QString::fromStdString(e.toString()));
  }
  cout << "Try to set valid range with larger number passed in as first parameter:" << endl;
  try {
    newChip.SetValidRange(4, 3);
  }
  catch(IException &e) {
    ReportError(QString::fromStdString(e.toString()));
  }
  cout << "Try to extract a sub-chip with samples or lines greater than original chip:" << endl;
  try {
    newChip.Extract(2, 5, 1, 1);
  }
  catch(IException &e) {
    ReportError(QString::fromStdString(e.toString()));
  }


  junk.close(true);// the "true" flag removes junk.cub from the /tmp/ directory
  junkCube.close(); // these cubes are kept in test data area, do not delete
  junkCube2.close();


#if 0
  try {
    junk.Open("/work2/janderso/moc/ab102401.lev1.cub");
    chip.TackCube(453.0, 567.0);
    chip.Load(junk);

    Cube junk2;
    junk2.Open("/work2/janderso/moc/ab102402.lev0.cub");
    Chip chip2(75, 70);
    chip2.TackCube(166.0, 567.0);
    chip2.Load(junk2, chip);

    chip.Write("junk3.cub");
    chip2.Write("junk4.cub");
  }
  catch(IException &e) {
    e.print();
  }
#endif

  return 0;
}

/**
 * Reports error messages from Isis:iException without full paths of filenames
 * @param err Error string of iException
 * @author Jeannie Walldren
 * @internal
 *   @history 2010-06-15 Jeannie Walldren - Original version.
 */
void ReportError(QString err) {
  cout << err.replace(QRegExp("\\[[^\\]]*\\.cub\\]"), "[]").toStdString() << endl << endl;
}

