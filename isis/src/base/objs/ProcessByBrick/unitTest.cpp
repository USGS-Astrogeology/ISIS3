/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "Isis.h"
#include "ProcessByBrick.h"
#include "Cube.h"
#include <iostream>
#include <string>

using namespace std;
using namespace Isis;
void oneInAndOut(Buffer &ob, Buffer &ib);
void twoInAndOut(vector<Buffer *> &ib, vector<Buffer *> &ob);

class Functor2 {
  public:
  void operator()(Buffer & in, Buffer & out) const {
    oneInAndOut(in, out);
  };
};

class Functor3 {
  public:
  void operator()(vector<Buffer *> &ib,
                  vector<Buffer *> &ob) const {
    twoInAndOut(ib, ob);
  };
};

class Functor4 {
  public:
  void operator()(Buffer &in, Buffer &out) const {
    for (int i = 0; i < out.size(); i++) {
      out[i] = in.Sample(i) + in.Line(i) * 10 + in.Band(i) * 100;
    }
  };
};

class Functor5 {
  public:
  void operator()(Buffer &inout) const {
    for (int i = 0; i < inout.size(); i++) {
      inout[i] = inout[i] * 2;
    }
  };
};

void IsisMain() {
  Preference::Preferences(true);
  Cube *icube;
  ProcessByBrick p;

  cout << "Testing Functors\n";
  {
    cout << "Functor2 - ProcessCube One Thread\n";
    //No cubes entered...will fail
    try{
    p.VerifyCubes(ProcessByBrick::InPlace);
    }
    catch(Isis::IException &ex){

        std::string exMsg = ex.toString();
        cout << "1:" + exMsg << endl;

    }

    //InputCubes.size() !=1, will fail
    try{
    p.VerifyCubes(ProcessByBrick::InputOutput);
    }
    catch(Isis::IException &ex){

        std::string exMsg = ex.toString();
        cout << "2:" + exMsg << endl;

    }




    icube = p.SetInputCube("FROM");
    p.SetBrickSize(10, 10, 2);


    //Input cube set, but output cube unset.  Will fail
    try{
    p.VerifyCubes(ProcessByBrick::InputOutput);
    }
    catch(Isis::IException &ex){

        std::string exMsg = ex.toString();
        cout << "3:" + exMsg << endl;

    }

    p.EndProcess();




    icube = p.SetInputCube("FROM");
    p.SetBrickSize(10, 10, 2);
    p.SetOutputCube("TO", icube->sampleCount()+10, icube->lineCount(),
                    icube->bandCount());


    //Samples don't match
    try{
    p.VerifyCubes(ProcessByBrick::InputOutput);
    }
    catch(Isis::IException &ex){

        std::string exMsg = ex.toString();
        cout << "4:" + exMsg << endl;

    }

    p.EndProcess();

    icube = p.SetInputCube("FROM");
    p.SetBrickSize(10, 10, 2);
    p.SetOutputCube("TO", icube->sampleCount(), icube->lineCount()+10,
                    icube->bandCount());


    //Lines don't match
    try{
    p.VerifyCubes(ProcessByBrick::InputOutput);
    }
    catch(Isis::IException &ex){

        std::string exMsg = ex.toString();
        cout << "5:" + exMsg << endl;

    }

    p.EndProcess();

    icube = p.SetInputCube("FROM");
    p.SetBrickSize(10, 10, 2);
    p.SetOutputCube("TO", icube->sampleCount(), icube->lineCount(),
                    icube->bandCount()+10);


    //Bands don't match
    try{
    p.VerifyCubes(ProcessByBrick::InputOutput);
    }
    catch(Isis::IException &ex){

        std::string exMsg = ex.toString();
        cout << "6:" + exMsg << endl;

    }

    p.EndProcess();

    icube = p.SetInputCube("FROM");
    p.SetBrickSize(10, 10, 2);
    p.SetOutputCube("TO", icube->sampleCount(), icube->lineCount(),
                    icube->bandCount());
    p.VerifyCubes(ProcessByBrick::InputOutput);  //Everything is correct





    icube = p.SetInputCube("FROM");
    p.SetBrickSize(10, 10, 2);
    p.SetOutputCube("TO", icube->sampleCount(), icube->lineCount(),
                    icube->bandCount());






    try{
    p.VerifyCubes(ProcessByBrick::InPlace);  //Will fail
    }
    catch(Isis::IException &ex){

        std::string exMsg = ex.toString();
        cout << "7:" + exMsg << endl;

    }


    p.EndProcess();
    Functor2 functor;
    icube = p.SetInputCube("FROM");
    p.SetBrickSize(10, 10, 2);
    p.SetOutputCube("TO", icube->sampleCount(), icube->lineCount(),
                    icube->bandCount());

    p.ProcessCube(functor, false);

    p.EndProcess();
    cout << "\n";
  }

  {
    cout << "Functor3 - ProcessCubes One Thread\n";


    // No input cubes specified, will fail
    try{
    p.VerifyCubes(ProcessByBrick::InputOutputList);
    }
    catch(Isis::IException &ex){

        std::string exMsg = ex.toString();
        cout << "8:" + exMsg << endl;

    }

    p.EndProcess();

    Cube *icube = p.SetInputCube("FROM");
    p.SetInputCube("FROM2");
    p.SetBrickSize(10, 10, 2);
    p.SetOutputCube("TO", icube->sampleCount(), icube->lineCount()+10, icube->bandCount());
    p.SetOutputCube("TO2", icube->sampleCount(), icube->lineCount(), icube->bandCount());
  }
  {

    //Output[0] cube does not have the same number of lines as input[0] cube
    try{
    p.VerifyCubes(ProcessByBrick::InputOutputList);
    }
    catch(Isis::IException &ex){

        std::string exMsg = ex.toString();
        cout << "9:" + exMsg << endl;

    }

    p.EndProcess();


  }
  {

    Cube *icube = p.SetInputCube("FROM");
    p.SetInputCube("FROM2");
    p.SetBrickSize(10, 10, 2);
    p.SetOutputCube("TO", icube->sampleCount(), icube->lineCount(), icube->bandCount()+10);
    p.SetOutputCube("TO2", icube->sampleCount(), icube->lineCount(), icube->bandCount());


    //Output[0] cube does not have the same number of bands as input[0] cube
    try{
    p.VerifyCubes(ProcessByBrick::InputOutputList);
    }
    catch(Isis::IException &ex){

        std::string exMsg = ex.toString();
        cout << "10:" + exMsg << endl;

    }


    p.EndProcess();
  }

  {

    Functor3 functor;
    Cube *icube = p.SetInputCube("FROM");
    p.SetInputCube("FROM2");
    p.SetBrickSize(10, 10, 2);
    p.SetOutputCube("TO", icube->sampleCount(), icube->lineCount(), icube->bandCount()+10);
    p.SetOutputCube("TO2", icube->sampleCount(), icube->lineCount(), icube->bandCount());


    p.ProcessCubes(functor, false);
    p.EndProcess();
    cout << "\n";
  }

  {
    cout << "Functor4 - ProcessCube Threaded\n";
    Cube *icube = p.SetInputCube("FROM");
    p.SetBrickSize(10, 10, 2);
    p.SetOutputCube("TO", icube->sampleCount(), icube->lineCount(),
                    icube->bandCount());
    Functor4 functor;
    p.ProcessCube(functor,false);
    p.EndProcess();
    Cube cube;
    cube.open(Application::GetUserInterface().GetCubeName("TO"));
    Statistics *statsBand1 = cube.statistics(1);
    Statistics *statsBand2 = cube.statistics(2);
    std::cerr << "Averages: " << statsBand1->Average() << ", " <<
                                 statsBand2->Average() << "\n";
    cout << "\n";
  }

  {
    cout << "Functor5 - ProcessCubeInPlace Threaded\n";
    Cube *cube = new Cube;
    cube->open(Application::GetUserInterface().GetCubeName("TO"), "rw");
    p.SetBrickSize(10, 10, 2);
    p.SetInputCube(cube);
    Functor5 functor;
    try{
    p.VerifyCubes(ProcessByBrick::InputOutputList);
    }
    catch(Isis::IException &ex){

        std::string msg = ex.toString();
        cout << msg << endl;

    }


    p.VerifyCubes(ProcessByBrick::InPlace);

    p.ProcessCubeInPlace(functor,false);
    p.EndProcess();
    cube->close();
    cube = new Cube;
    cube->open(Application::GetUserInterface().GetCubeName("TO"), "rw");
    Statistics *statsBand1 = cube->statistics(1);
    Statistics *statsBand2 = cube->statistics(2);
    delete cube;
    std::cerr << "Averages: " << statsBand1->Average() << ", " <<
                                 statsBand2->Average() << "\n";
    cout << "\n";
  }

  cout << "End Testing Functors\n\n";
  cout << "Testing StartProcess\n";

  {
    Cube *icube = p.SetInputCube("FROM");
    p.SetInputCube("FROM2");
    p.SetBrickSize(10, 10, 2);
    p.SetOutputCube("TO", icube->sampleCount(), icube->lineCount(), icube->bandCount());
    p.SetOutputCube("TO2", icube->sampleCount(), icube->lineCount(), icube->bandCount());
    p.StartProcess(twoInAndOut);
    p.EndProcess();
  }

  Cube cube;
  cube.open("$temporary/isisProcessByBrick_01");
  cube.close(true);
  cube.open("$temporary/isisProcessByBrick_02");
  cube.close(true);
}

void oneInAndOut(Buffer &ib, Buffer &ob) {
  if((ib.Line() == 1) && (ib.Band() == 1)) {
    cout << endl;
    cout << "Testing one input and output cube ... " << endl;
    cout << "Buffer Samples:  " << ib.size() << endl;
    cout << "Buffer Lines:    " << ib.LineDimension() << endl;
    cout << "Buffer Bands:    " << ib.BandDimension() << endl;
    cout << endl;
  }
  cout << "Sample:  " << ib.Sample()
       << "  Line:  " << ib.Line()
       << "  Band:  " << ib.Band() << endl;

  if((ib.Sample() != ob.Sample()) ||
      (ib.Line() != ob.Line()) ||
      (ib.Band() != ob.Band())) {
    cout << "Bogus error #1" << endl;
  }
}

void twoInAndOut(vector<Buffer *> &ib, vector<Buffer *> &ob) {
  static bool firstTime = true;
  if(firstTime) {
    firstTime = false;
    cout << "Testing two input and output cubes ... " << endl;
    cout << "Number of input cubes:   " << ib.size() << endl;
    cout << "Number of output cubes:  " << ob.size() << endl;
    cout << endl;
  }

  Buffer &inone = *ib[0];
  Buffer &intwo = *ib[1];
  Buffer &outone = *ob[0];
  Buffer &outtwo = *ob[1];

  cout << "Sample:  " << inone.Sample() << ":" << intwo.Sample()
       << "  Line:  " << inone.Line() << ":" << intwo.Line()
       << "  Band:  " << inone.Band() << ":" << intwo.Band() << endl;

  if((inone.Sample() != intwo.Sample()) ||
      (inone.Line() != intwo.Line())) {
    cout << "Bogus error #1" << endl;
  }

  if((inone.Sample() != outone.Sample()) ||
      (inone.Line() != outone.Line()) ||
      (inone.Band() != outone.Band())) {
    cout << "Bogus error #2" << endl;
  }

  if((outone.Sample() != outtwo.Sample()) ||
      (outone.Line() != outtwo.Line()) ||
      (outone.Band() != outtwo.Band())) {
    cout << "Bogus error #3" << endl;
  }
}
