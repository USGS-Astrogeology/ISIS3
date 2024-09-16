#include "Isis.h"

#include <iostream>
#include <fstream>

#include "ProcessBySpectra.h"
#include "ProcessByLine.h"
#include "SpecialPixel.h"

using namespace std;
using namespace Isis;

// Global declarations
double TestSpecial(const double pixel);
void ascii2isis(Buffer &out);
ifstream fin;
QString order;
QString from;
//Initialize values to make special pixels invalid
double null_min = DBL_MAX;
double null_max = DBL_MIN;
double hrs_min = DBL_MAX;
double hrs_max = DBL_MIN;
double lrs_min = DBL_MAX;
double lrs_max = DBL_MIN;

void IsisMain() {
  //initialize fin
  if (fin.is_open()) {
    fin.close();
  }

  //  Open input text file
  UserInterface &ui = Application::GetUserInterface();
  from = ui.GetFileName("FROM");
  
  // Get storage order of data
  order = ui.GetString("ORDER");

  // Get the size of the cube
  int ns = ui.GetInteger("SAMPLES");
  int nl = ui.GetInteger("LINES");
  int nb = ui.GetInteger("BANDS");
  int skip = ui.GetInteger("SKIP");

  //  Setup output cube
  CubeAttributeOutput &att = ui.GetOutputAttribute("TO");
  
  // Set special pixel ranges
  if (ui.GetBoolean("SETNULLRANGE")) {
    null_min = ui.GetDouble("NULLMIN");
    null_max = ui.GetDouble("NULLMAX");
  }
  
  if (ui.GetBoolean("SETHRSRANGE")) {
    hrs_min = ui.GetDouble("HRSMIN");
    hrs_max = ui.GetDouble("HRSMAX");
  }
  if (ui.GetBoolean("SETLRSRANGE")) {
    lrs_min = ui.GetDouble("LRSMIN");
    lrs_max = ui.GetDouble("LRSMAX");
  }

  
  fin.open(from.toLatin1().data(), std::ios::in);
  if (!fin.is_open()) {
    std::string msg = "Cannot open input file [" + from.toStdString() + "]";
    throw IException(IException::Io, msg, _FILEINFO_);
  }

  
  //  Skip header information if it exists
  fin.seekg(skip, std::ios::beg);

  
  //  Set up process depending on order
  if (order == "BSQ") {
    ProcessByLine p;

    
    p.SetOutputCube(ui.GetCubeName("TO"), att, ns, nl, nb);
    p.StartProcess(ascii2isis);
    p.EndProcess();
  }
  if (order == "BIL") {
    ProcessBySpectra p(Isis::ProcessBySpectra::ByLine);

    
    // Set Special Pixel ranges
    p.SetOutputCube(ui.GetCubeName("TO"), att, ns, nl, nb);
    p.StartProcess(ascii2isis);
    p.EndProcess();
  }
  if (order == "BIP") {
    ProcessBySpectra p(Isis::ProcessBySpectra::PerPixel);

    
    p.SetOutputCube(ui.GetCubeName("TO"), att, ns, nl, nb);
    p.StartProcess(ascii2isis);
    p.EndProcess();
  }

  fin.close();
}

void ascii2isis(Buffer &out) {
  //Define all legal characters for the beginning of a number
  const string legal = ".0123456789+-";

    
  for (int i = 0; i < out.size(); i++) {
    
    //Discard all nonlegal characters
    while ((legal.find(fin.peek()) == string::npos) && !fin.eof()) {
      fin.ignore();
    }  
    
    fin >> out[i];

    // If we've reached the end of file and stream is bad, we didn't find enough numerical data
    if (!fin && fin.eof()) {
      std::string msg = "End of file reached. There is not enough data in [" + from.toStdString();
      msg += "] to fill the output cube.";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    // Check if there was an issue extracting a double
    if (!fin) {
      // Clean stream to get the position and invalid data that broke the stream
      fin.clear();
      std::string msg = "Could not extract non-numerical data [" + std::to_string(fin.peek()) + "] ";
      msg += "at byte position [" + std::to_string(fin.tellg()) + "]. ";
      msg += "Please make sure to skip any header data in [" + from.toStdString() + "].";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    out[i] = TestSpecial(out[i]);
  }
}

/**
  * Tests the pixel. If it is valid it will return the dn value,
  * otherwise it will return the Isis special pixel value that
  * corresponds to it
  *
  * @param pixel The double precision value that represents a
  *              pixel.
  * @return double  The double precision value representing the
  *         pixel will return as a valid dn or changed to an isis
  *         special pixel.
  */
double TestSpecial(const double pixel) {
  if (pixel <= null_max && pixel >= null_min) {
    return Isis::NULL8;
  }
  else if (pixel <= hrs_max && pixel >= hrs_min) {
    return Isis::HIGH_REPR_SAT8;
  }
  else if (pixel <= lrs_max && pixel >= lrs_min) {
    return Isis::LOW_REPR_SAT8;
  }
  else {
    return pixel;
  }
}
