/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <QString>
#include <QList>
#include "QRegularExpression"
#include "ImageHistogram.h"
#include "IException.h"
#include "Preference.h"
#include "LineManager.h"
#include "ControlNet.h"
#include "ControlMeasure.h"
#include "Progress.h"
#include "IException.h"
#include "FileName.h"
using namespace std;
#include <iostream>
#include <vector>
#include <algorithm>
#include <fstream>

void arrayDisplay(double *array, int size );
void histogramMembers(Isis::ImageHistogram *h);
void statCounters(Isis::ImageHistogram *h);
void histDisplay(Isis::ImageHistogram *h);

int main(int argc, char *argv[]) {
  try {

    Isis::FileName cubeFile("$ISISTESTDATA/isis/src/base/unitTestData/isisTruth.cub");
    Isis::Cube icube;
    icube.open(QString::fromStdString(cubeFile.expanded()));
    Isis::ImageHistogram *histcube;
    histcube = new Isis::ImageHistogram(icube, 1);

    cout << endl;

    cout << "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" <<endl;
    cout << "Constructor3:   " << endl;
    cout << "Histogram(icube,1) Real" << endl;
    cout << endl;
    cout << "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" <<endl;

    Isis::LineManager lm(icube);

    for (int i=1; i <= icube.lineCount(); i++) {
      lm.SetLine(i);
      icube.read(lm);
      histcube->AddData(lm.DoubleBuffer(),lm.size());
    }

    histogramMembers(histcube);
    statCounters(histcube);
    cout << "Resetting bin range to (70,110)" << endl;
    histcube ->SetValidRange(70,110);
    histogramMembers(histcube);
    statCounters(histcube);
    delete(histcube);
    cout << endl;

  }
  catch (Isis::IException &e) {

    e.print();

  }


  try {

    Isis::FileName cubeFile("$ISISTESTDATA/isis/src/base/unitTestData/isisTruth_Signed16Bit.cub");
    Isis::Cube icube;
    icube.open(QString::fromStdString(cubeFile.expanded()));
    Isis::ImageHistogram *histcube;
    histcube = new Isis::ImageHistogram(icube, 1);

    cout << endl;

    cout << "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" <<endl;
    cout << "Constructor3:   " << endl;
    cout << "Histogram(icube,1) SignedWord" << endl;
    cout << endl;
    cout << "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" <<endl;

    Isis::LineManager lm(icube);

    for (int i=1; i <= icube.lineCount(); i++) {
      lm.SetLine(i);
      icube.read(lm);
      histcube->AddData(lm.DoubleBuffer(),lm.size());
    }

    histogramMembers(histcube);
    statCounters(histcube);
    cout << "Resetting bin range to (70,110)" << endl;
    histcube ->SetValidRange(70,110);
    histogramMembers(histcube);
    statCounters(histcube);
    delete(histcube);
    cout << endl;

  }
  catch (Isis::IException &e) {

    e.print();

  }


  try {

    Isis::FileName cubeFile("$ISISTESTDATA/isis/src/base/unitTestData/isisTruth_Unsigned16Bit.cub");
    Isis::Cube icube;
    icube.open(QString::fromStdString(cubeFile.expanded()));
    Isis::ImageHistogram *histcube;
    histcube = new Isis::ImageHistogram(icube, 1);

    cout << endl;

    cout << "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" <<endl;
    cout << "Constructor3:   " << endl;
    cout << "Histogram(icube,1) UnsignedWord" << endl;
    cout << endl;
    cout << "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" <<endl;

    Isis::LineManager lm(icube);

    for (int i=1; i <= icube.lineCount(); i++) {
      lm.SetLine(i);
      icube.read(lm);
      histcube->AddData(lm.DoubleBuffer(),lm.size());
    }

    histogramMembers(histcube);
    statCounters(histcube);
    cout << "Resetting bin range to (70,110)" << endl;
    histcube ->SetValidRange(70,110);
    histogramMembers(histcube);
    statCounters(histcube);
    delete(histcube);
    cout << endl;

  }
  catch (Isis::IException &e) {

    e.print();

  }


  try {

    Isis::FileName cubeFile("$ISISTESTDATA/isis/src/base/unitTestData/isisTruth_8Bit.cub");
    Isis::Cube icube;
    icube.open(QString::fromStdString(cubeFile.expanded()));
    Isis::ImageHistogram *histcube;
    histcube = new Isis::ImageHistogram(icube, 1);

    cout << endl;

    cout << "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" <<endl;
    cout << "Constructor3:   " << endl;
    cout << "Histogram(icube,1) UnsignedByte" << endl;
    cout << endl;
    cout << "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" <<endl;

    Isis::LineManager lm(icube);

    for (int i=1; i <= icube.lineCount(); i++) {
      lm.SetLine(i);
      icube.read(lm);
      histcube->AddData(lm.DoubleBuffer(),lm.size());
    }

    histogramMembers(histcube);
    statCounters(histcube);
    cout << "Resetting bin range to (70,110)" << endl;
    histcube ->SetValidRange(70,110);
    histogramMembers(histcube);
    statCounters(histcube);
    delete(histcube);
    cout << endl;

  }
  catch (Isis::IException &e) {

    e.print();

  }
}

//Simple function for outputting a sorted vector

void arrayDisplay(double *array, int size ){

  vector<double> v(size);

  for (int i=0; i < size; i++ )
    v[i]= array[i];

  sort (v.begin(),v.end() );

  cout << "[ " ;
  for (std::vector<double>::iterator it = v.begin();  it != v.end(); it++)
      cout << *it << " ";

  cout << "]" << endl;

  }

void histDisplay(Isis::ImageHistogram *h){

  double low, high;

  for (int i = 0; i < h->Bins(); i++){

    h->BinRange(i,low,high);
    cout <<"Bin " << i << ": [" << low << "," << high << "], Count = " << h->BinCount(i)
        << endl;
  }

}


void histogramMembers(Isis::ImageHistogram *h){

  try{
  cout << endl;
  cout << "+++++++++++++++++++ Histogram Members +++++++++++++++++++" << endl;
  cout << endl;
  cout << "Stats Range:            (" << h->ValidMinimum() << "," << h->ValidMaximum() << ")"
       << endl;
  cout << "Bin Range:              (" << h->BinRangeStart() <<","<< h->BinRangeEnd()<<")"
       << endl;
  cout << "Average:               "; cout <<  h-> Average() << endl;
  cout << "Std Deviation:         "; cout <<  h->StandardDeviation() << endl;
  cout << "Variance:              "; cout <<  h->Variance() <<endl;
  cout << "Median:                "; cout <<  h->Median() << endl;
  cout << "Mode:                  "; cout <<  h->Mode() << endl;
  cout << "Skew:                  "; cout <<  h->Skew() << endl;
  cout << "Percent(0.5):          "; cout <<  h->Percent(0.5) << endl;
  cout << "Percent(99.5):         "; cout <<  h->Percent(99.5) << endl;
  cout << "Minimum:               "; cout <<  h-> Minimum() << endl;
  cout << "Maximum:               "; cout <<  h-> Maximum() << endl;
  cout << "# Bins:                "; cout <<  h->Bins() << endl;
  cout << "BinWidth:              "; cout <<  h->BinSize() << endl;
  cout << "MaxBinCount:           "; cout <<  h->MaxBinCount() << endl;
  cout << endl;
  cout << "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++" << endl;
  }
  catch(Isis::IException &e){

    e.print();
  }



}


void statCounters(Isis::ImageHistogram *h){

  cout << endl;
  cout << "++++++++++++++++++ Statistics Counters ++++++++++++++++++" << endl;
  cout << endl;
  cout << "Total pixels:          " << h->TotalPixels() << endl;
  cout << "Valid pixels:          " << h->ValidPixels() << endl;
  cout << "Over Range pixels:     " << h->OverRangePixels() << endl;
  cout << "Under Range pixels:    " << h->UnderRangePixels() << endl;
  cout << "Null pixels:           " << h->NullPixels() << endl;
  cout << "Lis pixels:            " << h->LisPixels() << endl;
  cout << "His pixels:            " << h->HisPixels() << endl;
  cout << "Lrs pixels:            " << h->LrsPixels() << endl;
  cout << "Hrs pixels:            " << h->HrsPixels() << endl;
  cout << "Out of range pixels:   " << h->OutOfRangePixels() << endl;
  cout << "Minimum:               " << h->Minimum() << endl;
  cout << "Maximum:               " << h->Maximum() << endl;
  cout << "Valid Minimum:         " << h->ValidMinimum() << endl;
  cout << "Valid Maximum:         " << h->ValidMaximum() << endl;
  cout << "Sum:                   " << h->Sum() << endl;
  cout << "Sum Square:            " << h->SumSquare() << endl;
  cout << endl;
  cout << "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++" << endl;



}
