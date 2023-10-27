#include "Isis.h"

#include "IException.h"
#include "IString.h"
#include "ProcessByBoxcar.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "SpecialPixel.h"

#include <vector>

using namespace std;
using namespace Isis;

void filter(Buffer &in, double &v);
vector <double> coefs;
double weight;

void IsisMain() {

  // Get information from the input kernel
  UserInterface &ui = Application::GetUserInterface();
  Pvl pvl(ui.GetFileName("KERNEL").toStdString());

  // Access the Kernel group section of the input file
  const PvlGroup &kern = pvl.findGroup("KERNEL");

  int lines =  kern["lines"];
  int samples = kern["samples"];

  // Error check kernel input for impossible boxcar sizes
  if(lines <= 0) {
    IString msg = "Your kernel must specify lines count greater than 0";
    throw IException(IException::User, msg, _FILEINFO_);
  }
  if(samples <= 0) {
    IString msg = "Your kernel must specify samples count greater than 0";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  // Error check kernel for proper amount of data - lines*samples
  if(lines *samples != kern["data"].size()) {
    IString msg = "Your kernel does not specify the correct amount of data, must";
    msg += " be equal to lines * samples [";
    msg += IString(lines * samples);
    msg += "] pieces of data";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  ProcessByBoxcar p;

  // Allocate cubes
  p.SetInputCube("FROM");
  p.SetOutputCube("TO");
  p.SetBoxcarSize(samples, lines);

  // Iterate through the input kernel's data values to fill the coefs array
  for(int i = 0 ; i < kern["data"].size() ; i ++) {
    coefs.push_back(std::stod(kern["data"][i]));
  }

  // Weight for multiplication of resultant immidately before completion
  weight = kern["weight"];

  p.StartProcess(filter);
  p.EndProcess();
}


//! The filter depends on the user input kernel
void filter(Buffer &in, double &result) {
  result = 0.0;
  for(int i = 0; i < in.size() && result != Isis::Null; i++) {
    if(!IsSpecial(in[i])) {
      result += in[i] * coefs[i] ;
    }
    else {
      // If a special pixel is encountered with the boxcar, resultant pixel is nulled
      result = Isis::Null;
    }
  }

  // If the result isn't null, finish the processing
  if(result != Isis::Null) {
    result *= weight;
  }
}
