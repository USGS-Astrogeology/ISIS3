/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "Isis.h"
#include "ProcessByTile.h"
#include "Cube.h"
#include <string>

using namespace std;

void inPlaceFunction(Isis::Buffer &in);
void oneInAndOut(Isis::Buffer &in, Isis::Buffer &out);
void twoInAndOut(vector<Isis::Buffer *> &ib, vector<Isis::Buffer *> &ob);

class InPlaceFunctor{
public:
    void operator() (Isis::Buffer &in) const{
        static bool firstTime = true;
        if(firstTime) {
          firstTime = false;
          cout << "Input Functor:  " << endl;
          cout << "Testing cube processing in place. " << endl;
        }

        cout << "Sample:  " << in.Sample() <<
              "  Line:  " << in.Line() <<
              "  Band:  " << in.Band() << endl;
    }


};

class InputOutputFunctor{

public:

    void operator() (Isis::Buffer &in, Isis::Buffer &out) const{

        static bool firstTime = true;
        if(firstTime) {
          firstTime = false;
          cout << "InputOutput Functor:  " << endl;
          cout << "Testing one input and one output cube." << endl;

        }


        cout << "Sample:  " << in.Sample() << ":" << out.Sample() <<
              "  Line:  " << in.Line() << ":" << out.Line() <<
              "  Band:  " << in.Band() << ":" << out.Band() << endl;


    }


};

class InputOutputListFunctor{

public:

    void operator() (vector<Isis::Buffer *> &ib, vector<Isis::Buffer *> &ob) const{

        static bool firstTime = true;
        if(firstTime) {
          firstTime = false;
          cout << "InputOutputList Functor:  " << endl;
          cout << "Testing two input and two output cubes." << endl;

        }


        Isis::Buffer *in1 = ib[0];
        Isis::Buffer *in2 = ib[1];
        Isis::Buffer *out1 = ob[0];
        Isis::Buffer *out2 = ob[1];

        cout << "InputSample:  " << in1->Sample() << ":" << in2->Sample()
             << " InputLine:  " << in1->Line() << ":" << in2->Line()
             << " InputBand:  " << in1->Band() << ":" << in2->Band() << endl;

        cout << "OutputSample:  " << out1->Sample() << ":" << out2->Sample()
             << " OutputLine:  " << out1->Line() << ":" << out2->Line()
             << " OutputBand:  " << out1->Band() << ":" << out2->Band() << endl;



    }


};





void IsisMain() {

  InPlaceFunctor inPlace;
  InputOutputFunctor inputOutput;
  InputOutputListFunctor cubeList;
  Isis::Preference::Preferences(true);
  Isis::ProcessByTile p;


  //Testing one input cube for both normal functions and functor templates

  //tjw:  10/16/2015
  p.SetInputCube("FROM");

  try {
    p.StartProcess(inPlaceFunction);  //Call first without setting tiles to trip Programmer error
      }
  catch(Isis::IException &ex){

      std::string exMsg = ex.toString();
      cout << exMsg << endl;


  }
  p.SetTileSize(10, 10);
  p.StartProcess(inPlaceFunction); //No errors this time
  p.ProcessCubeInPlace(inPlace,false);

  p.EndProcess();





  //Testing one input and one output cube
  p.SetInputCube("FROM");
  p.SetOutputCube("TO");


  try{
  p.StartProcess(oneInAndOut);
  }
  catch(Isis::IException &ex){

      std::string exMsg = ex.toString();
      cout << exMsg << endl;

  }

  p.SetTileSize(10, 10);


  p.ProcessCube(inputOutput,false);
  p.StartProcess(oneInAndOut);

  p.EndProcess();

  //Testing two input and two output cubes


  Isis::Cube *icube = p.SetInputCube("FROM");
  p.SetOutputCube("TO", icube->sampleCount() + 10, icube->lineCount(), icube->bandCount());



  p.SetInputCube("FROM2");  
  p.SetOutputCube("TO2", icube->sampleCount() + 10, icube->lineCount(), icube->bandCount());


  try{
      p.StartProcess(twoInAndOut);
  }
  catch(Isis::IException &ex){

      std::string exMsg = ex.toString();
      cout << exMsg << endl;

  }
  p.SetTileSize(10,10);
  p.StartProcess(twoInAndOut);

  p.ProcessCubes(cubeList,false);

  p.EndProcess();

  p.Finalize();
  Isis::Cube cube;
  cube.open("$temporary/isisProcessByTile_01");
  cube.close(true);
  cube.open("$temporary/isisProcessByTile_02");
  cube.close(true);
}

void inPlaceFunction(Isis::Buffer &in){
            static bool firstTime = true;
            if(firstTime) {
              firstTime = false;

              cout << "Testing inplace cube processing function. " << endl;
            }

            cout << "Sample:  " << in.Sample() <<
                  "  Line:  " << in.Line() <<
                  "  Band:  " << in.Band() << endl;


}


void oneInAndOut(Isis::Buffer &in,Isis::Buffer &out){
    static bool firstTime = true;
    if(firstTime) {
      firstTime = false;
      cout << "Testing one input/one output cube function" << endl;

    }


    cout << "Sample:  " << in.Sample() << ":" << out.Sample() <<
          "  Line:  " << in.Line() << ":" << out.Line() <<
          "  Band:  " << in.Band() << ":" << out.Band() << endl;



}


void twoInAndOut(vector<Isis::Buffer *> &ib, vector<Isis::Buffer *> &ob) {
  static bool firstTime = true;
  if(firstTime) {
    firstTime = false;
    cout << "Testing two input and output cubes function." << endl;
    cout << "Number of input cubes:   " << ib.size() << endl;
    cout << "Number of output cubes:  " << ob.size() << endl;
    cout << endl;
  }

  Isis::Buffer *inone = ib[0];
  Isis::Buffer *intwo = ib[1];
  Isis::Buffer *outone = ob[0];
  Isis::Buffer *outtwo = ob[1];

  cout << "Sample:  " << inone->Sample() << ":" << intwo->Sample()
       << "  Line:  " << inone->Line() << ":" << intwo->Line()
       << "  Band:  " << inone->Band() << ":" << intwo->Band() << endl;

  if((inone->Sample() != intwo->Sample()) ||
      (inone->Line() != intwo->Line())) {
    cout << "Bogus error #1" << endl;
  }

  if((outone->Sample() != outtwo->Sample()) ||
      (outone->Line() != outtwo->Line()) ||
      (outone->Band() != outtwo->Band())) {
    cout << "Bogus error #2" << endl;
  }
}
