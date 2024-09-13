/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <QString>
#include <QList>
#include "QRegularExpression"
#include "Histogram.h"
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
#include <regex>

void arrayDisplay(double *array, int size );
void histogramMembers(Isis::Histogram *h);
void statCounters(Isis::Histogram *h);
void histDisplay(Isis::Histogram *h);


int main(int argc, char *argv[]) {


    // Old unit test begins here

    Isis::Preference::Preferences(true);
    Isis::Histogram h(-10.0, 10.0, 20);
    double low, high;

    try {

      double a[9];
      a[0] = 1.0;
      a[1] = 2.0;
      a[2] = 3.0;
      a[3] = Isis::NULL8;
      a[4] = Isis::HIGH_REPR_SAT8;
      a[5] = Isis::LOW_REPR_SAT8;
      a[6] = Isis::HIGH_INSTR_SAT8;
      a[7] = Isis::LOW_INSTR_SAT8;
      a[8] = 2.0;

      h.AddData(a, 9);

      cout << "Median:              " << h.Median() << endl;
      cout << "Mode:                " << h.Mode() << endl;
      cout << "Skew:                " << h.Skew() << endl;
      cout << "Percent(0.5):        " << h.Percent(0.5) << endl;
      cout << "Percent(99.5):       " << h.Percent(99.5) << endl;
      cout << "Bins:                " << h.Bins() << endl;
      cout << "BinSize:             " << h.BinSize() << endl;
      cout << "BinMiddle:           " << h.BinMiddle(0) << endl;

      h.BinRange(0, low, high);
      cout << "BinRange(0,low):     " << low << endl;
      cout << "BinRange(0,high):    " << high << endl;
      cout << "BinCount(0):         " << h.BinCount(0) << endl;
      h.BinRange(19, low, high);
      cout << "BinRange(20,low):  " << low << endl;
      cout << "BinRange(20,high): " << high << endl;
      cout << "BinCount(20):      " << h.BinCount(19) << endl;
      cout << endl;

      h.RemoveData(a, 3);
      double b[4];
      b[0] = -11.0;
      b[1] = 11.0;
      b[2] = 5.0;
      b[3] = 5.0;
      h.AddData(b, 4);

      cout << "Average:             " << h.Average() << endl;
      cout << "Median:              " << h.Median() << endl;
      cout << "Mode:                " << h.Mode() << endl;
      cout << "Skew:                " << h.Skew() << endl;
      cout << "Percent(0.5):        " << h.Percent(0.5) << endl;
      cout << "Percent(99.5):       " << h.Percent(99.5) << endl;
      cout << "BinCount(0):         " << h.BinCount(0) << endl;
      cout << "BinCount(20):      " << h.BinCount(19) << endl;
      cout << endl;
    }
    catch(Isis::IException &e) {
      e.print();
    }

    try {
      Isis::Histogram g(1.0, 0.0);
    }
    catch(Isis::IException &e) {
      e.print();
    }

    try {
      h.Percent(-1.0);
    }
    catch(Isis::IException &e) {
      e.print();
    }

    try {
      h.Percent(101.0);
    }
    catch(Isis::IException &e) {
      e.print();
    }

    try {
      h.BinCount(-1);
    }
    catch(Isis::IException &e) {
      e.print();
    }

    try {
      h.BinCount(1024);
    }
    catch(Isis::IException &e) {
      e.print();
    }

    try {
      h.BinMiddle(-1);
    }
    catch(Isis::IException &e) {
      e.print();
    }

    try {
      h.BinMiddle(1024);
    }
    catch(Isis::IException &e) {
      e.print();
    }

    try {
      h.BinRange(-1, low, high);
    }
    catch(Isis::IException &e) {
      e.print();
    }

    try {
      h.BinRange(1024, low, high);
    }
    catch(Isis::IException &e) {
      e.print();
    }

    //End old unit test


    cout << endl;
    cout << "End old unit test." << endl;

  QList<QString> flist;
  Isis::Progress progress;
  Isis::FileName netName("$ISISTESTDATA/isis/src/base/unitTestData/enceladus_sp-Jig.net");
  flist.append(QString::fromStdString(netName.expanded()));
  std::regex pattern(R"(\/[\w\-\. ]*\/?Data)");
  cout << std::regex_replace(netName.toString(), pattern, "data") << endl;

  Isis::ControlNet net(flist[0], &progress);

  Isis::Histogram *hist1, *hist2;


  try {
    hist1 = new Isis::Histogram(net, &Isis::ControlMeasure::GetResidualMagnitude, 19);
    cout << endl;
    cout << "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" <<endl;
    cout << endl;
    cout << "Constructor1:   " << endl;
    cout << "Histogram(net, &Isis::ControlMeasure::GetResidualMagnitude,numbins)" << endl;
    cout << endl;
    cout << "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" <<endl;
    histogramMembers(hist1);
    statCounters(hist1);
    cout << "Resetting bin range to (0.1469787,0.3299682)" << endl;
    hist1 ->SetValidRange(0.1469787,0.3299682);
    histogramMembers(hist1);
    statCounters(hist1);

    delete(hist1);

    cout << endl;

  }
  catch (Isis::IException &e) {
    e.print();
  }


  try {
    hist2 = new Isis::Histogram(net, &Isis::ControlMeasure::GetResidualMagnitude, .01);
    cout << endl;
    cout << "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" <<endl;
    cout << "Constructor2:   " << endl;
    cout << "Histogram(net, &Isis::ControlMeasure::GetResidualMagnitude,double binwidth)" << endl;
    cout << endl;
    cout << "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" <<endl;
    histogramMembers(hist2);
    statCounters(hist2);
    cout << "Resetting bin range to (0.1469787,0.3299682)" << endl;
    hist2 ->SetValidRange(0.1469787,0.3299682);
    histogramMembers(hist2);
    statCounters(hist2);


    delete(hist2);

    cout << endl;

  }
  catch (Isis::IException &e) {
    e.print();
  }

  double low1  = -10;
  double high1 = 10;
  int nbins = 5;

  Isis::Histogram *ahist, *bhist;
  ahist = new Isis::Histogram (low1,high1,nbins);
  bhist = new Isis::Histogram (low1,high1,nbins);

  double a[9];
  a[0] = 1.0;
  a[1] = 2.0;
  a[2] = 3.0;
  a[3] = Isis::NULL8;
  a[4] = Isis::HIGH_REPR_SAT8;
  a[5] = Isis::LOW_REPR_SAT8;
  a[6] = Isis::HIGH_INSTR_SAT8;
  a[7] = Isis::LOW_INSTR_SAT8;
  a[8] = 2.0;

  double b[9];
  b[0] = -9.0;
  b[1] = -7.0;
  b[2] = -5.0;
  b[3] = 5.5;
  b[4] = 2.3;
  b[5] = -0.5;
  b[6] = 7.1;
  b[7] = 8.2;
  b[8] = 8.3;

  try{

    cout << "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" <<endl;
    cout << endl;
    cout << "Constructor4:   " << endl;
    cout << "Histogram(double mim, double max,int nbins)" << endl;
    cout << endl;
    cout << "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" <<endl;

    ahist->AddData(a,9);
    bhist->AddData(b,9);

    // Data output
    cout << "Data Vector a:  " << endl;
    arrayDisplay(a,9);
    cout << endl;

    statCounters(ahist);
    histogramMembers(ahist);

    cout << "**********************Data Vector A Histogram**********************" << endl;
    histDisplay(ahist);
    cout << "*******************************************************************" << endl;
    cout << endl;

    cout << "Data Vector b:  " << endl;
    arrayDisplay(b,9);
    cout << endl;

    statCounters(bhist);
    histogramMembers(bhist);

    cout << "**********************Data Vector B Histogram**********************" << endl;
    histDisplay(bhist);
    cout << "*******************************************************************" << endl;

    int remove = 3;
    bhist->RemoveData(b,remove);
    cout <<"Removing:  ";
    for(int i = 0; i < remove -1; i++){
      cout << b[i] << ",";
    }

    cout << b[remove-1] << endl;

    cout << "**********************Data Vector B Histogram**********************" << endl;
    histDisplay(bhist);
    cout << "*******************************************************************" << endl;
    histogramMembers(bhist);

    bhist->SetValidRange(0,10);
    cout << "Changing BinRange for b-vector's histogram..."  << endl;

    statCounters(bhist);
    histogramMembers(bhist);

    cout << endl;



    delete(ahist); delete(bhist);


  }//end try block

  catch(Isis::IException &e){

    e.print();
  }

}//end main


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

void histDisplay(Isis::Histogram *h){

  double low, high;

  for (int i = 0; i < h->Bins(); i++){

    h->BinRange(i,low,high);
    cout <<"Bin " << i << ": [" << low << "," << high << "], Count = " << h->BinCount(i)
        << endl;
  }

}


void histogramMembers(Isis::Histogram *h){

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


void statCounters(Isis::Histogram *h){

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
