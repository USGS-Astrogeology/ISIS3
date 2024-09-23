//************************************************************************
// See Full documentation in isis2raw.xml
//************************************************************************
#include "Isis.h"

#include <iostream>
#include <sstream>

#include "Histogram.h"
#include "ProcessExport.h"
#include "UserInterface.h"
#include "SpecialPixel.h"

using namespace std;
using namespace Isis;

enum Pixtype { NONE, NEG, BOTH };

void checkRange(UserInterface &ui, double &min, double &max);
void setRangeAndPixels(UserInterface &ui, ProcessExport &p,
                       double &min, double &max, Pixtype ptype);

Cube *p_cube;

// Main program
void IsisMain() {
  // Create an object for exporting Isis data
  ProcessExport p;

  // Open the input cube
  p_cube = p.SetInputCube("FROM");

  UserInterface &ui = Application::GetUserInterface();

  // Applies the input to output stretch options
  if(ui.GetString("STRETCH") == "LINEAR") {
   // if(ui.GetString("BITTYPE") != "32BIT")
    p.SetInputRange();
  }
  if(ui.GetString("STRETCH") == "MANUAL") {
    p.SetInputRange(ui.GetDouble("MINIMUM"), ui.GetDouble("MAXIMUM"));
  }

  //  Determine bit size, output range, and calculate number of bytes to write
  //  for each line.
  double min = -DBL_MAX;
  double max = DBL_MAX;
  Pixtype pixType = NONE;
  if(ui.GetString("BITTYPE") == "8BIT") {
    p.SetOutputType(Isis::UnsignedByte);
    min = 0.0;
    max = 255.0;
    pixType = BOTH;
  }
  else if(ui.GetString("BITTYPE") == "S16BIT") {
    p.SetOutputType(Isis::SignedWord);
    min = -32768.0;
    max = 32767.0;
    pixType = NEG;
  }
  else if(ui.GetString("BITTYPE") == "U16BIT") {
    p.SetOutputType(Isis::UnsignedWord);
    min = 0.0;
    max = 65535.0;
    pixType = BOTH;
  }
  else if(ui.GetString("BITTYPE") == "32BIT") {
    p.SetOutputType(Isis::Real);
    pixType = NONE;
  }

  if (ui.GetString("STRETCH") != "NONE" || ui.GetString("BITTYPE") != "32BIT") {
    checkRange(ui, min, max);
  }
  setRangeAndPixels(ui, p, min, max, pixType);

  // Set the output endianness
  if(ui.GetString("ENDIAN") == "MSB")
    p.SetOutputEndian(Isis::Msb);
  else if(ui.GetString("ENDIAN") == "LSB")
    p.SetOutputEndian(Isis::Lsb);
  // Open the cube for writing
  QString to = ui.GetFileName("TO", "raw");
  ofstream fout;
  fout.open(to.toLatin1().data(), ios::out | ios::binary);
  if(!fout.is_open()) {
    string msg = "Cannot open raw output file";
    throw IException(IException::Programmer, msg, _FILEINFO_);
  }

  // Set the Output Storage Format
  if(Isis::IString(ui.GetString("STORAGEORDER").toStdString()).DownCase()  == "bil") {
    p.setFormat(ProcessExport::BIL);
  }
  else if(Isis::IString(ui.GetString("STORAGEORDER").toStdString()).DownCase()  == "bip") {
    p.setFormat(ProcessExport::BIP);
  }

  // Write the raw cube data
  p.StartProcess(fout);

  fout.close();
  p.EndProcess();

  // Records what output values were used, then sends it to the print.prt file
  // as well as the terminal.
  PvlGroup results("DNs Used");
  results += PvlKeyword("Null", Isis::toString(p.OutputNull()));
  results += PvlKeyword("LRS", Isis::toString(p.OutputLrs()));
  results += PvlKeyword("LIS", Isis::toString(p.OutputLis()));
  results += PvlKeyword("HIS", Isis::toString(p.OutputHis()));
  results += PvlKeyword("HRS", Isis::toString(p.OutputHrs()));
  results += PvlKeyword("ValidMin", Isis::toString(min));
  results += PvlKeyword("ValidMax", Isis::toString(max));
  Application::Log(results);

  return;
}

// Validates provided range
void checkRange(UserInterface &ui, double &min, double &max) {
  Isis::Histogram *hist = p_cube->histogram(0);

  if(ui.WasEntered("OMIN")) {
    if(ui.GetDouble("OMIN") < min) {
      std::string message = "OMIN [" + toString(min) + "] is too small for the provided BITTYPE [";
      message += ui.GetString("BITTYPE").toStdString() + "]";
      throw IException(IException::User, message, _FILEINFO_);
    }
    else {
      min = ui.GetDouble("OMIN");
    }
  }
  else if(!ui.WasEntered("OMIN") && ui.GetString("BITTYPE") == "32BIT"){
    if(ui.GetString("STRETCH") == "LINEAR") {
      min = hist->Percent(Application::GetUserInterface().GetDouble("MINPERCENT"));
    }
    else if(ui.GetString("STRETCH") == "MANUAL") {
      min = ui.GetDouble("MINIMUM");
    }
  }

  if(ui.WasEntered("OMAX")) {
    if(ui.GetDouble("OMAX") > max) {
      std::string message = "OMAX [" + toString(max) + "] is too large for the provided BITTYPE [";
      message += ui.GetString("BITTYPE").toStdString() + "]";
      throw IException(IException::User, message, _FILEINFO_);
    }
    else {
      max = ui.GetDouble("OMAX");
    }
  }
  else if(!ui.WasEntered("OMIN") && ui.GetString("BITTYPE") == "32BIT"){
    if(ui.GetString("STRETCH") == "LINEAR") {
      max = hist->Percent(Application::GetUserInterface().GetDouble("MAXPERCENT"));
    }
    else if(ui.GetString("STRETCH") == "MANUAL") {
      max = ui.GetDouble("MAXIMUM");
    }
  }
  if(min >= max) {
    string message = "OMIN [" + IString(min) + "] cannot be greater than or equal to OMAX [";
    message += IString(max) + "]";
    throw IException(IException::User, message, _FILEINFO_);
  }
}

// Sets up special pixels and valid pixel ranges
void setRangeAndPixels(UserInterface &ui, ProcessExport &p, double &min, double &max, Pixtype ptype) {
  if(ptype == NEG) {
    if(ui.GetBoolean("NULL")) {
      p.SetOutputNull(min++);
    }
    if(ui.GetBoolean("LRS")) {
      p.SetOutputLrs(min++);
    }
    if(ui.GetBoolean("LIS")) {
      p.SetOutputLis(min++);
    }
    if(ui.GetBoolean("HIS")) {
      p.SetOutputHis(min++);
    }
    if(ui.GetBoolean("HRS")) {
      p.SetOutputHrs(min++);
    }
  }
  else if(ptype == BOTH) {
    if(ui.GetBoolean("NULL")) {
      p.SetOutputNull(min++);
    }
    if(ui.GetBoolean("LRS")) {
      p.SetOutputLrs(min++);
    }
    if(ui.GetBoolean("LIS")) {
      p.SetOutputLis(min++);
    }
    if(ui.GetBoolean("HRS")) {
      p.SetOutputHrs(max--);
    }
    if(ui.GetBoolean("HIS")) {
      p.SetOutputHis(max--);
    }
  }
  p.SetOutputRange(min, max);
}
