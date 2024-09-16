//************************************************************************
// See Full documentation in raw2isis.xml
//************************************************************************
#include "Isis.h"
#include "ProcessImport.h"

#include "UserInterface.h"
#include "FileName.h"

using namespace std;
using namespace Isis;

// Line-by-Line from raw file
void raw2isis(Buffer &out);

void IsisMain() {
  ProcessImport p;

  UserInterface &ui = Application::GetUserInterface();
  p.SetDimensions(ui.GetInteger("SAMPLES"), ui.GetInteger("LINES"),
                  ui.GetInteger("BANDS"));
  p.SetFileHeaderBytes(ui.GetInteger("SKIP"));
  p.SetPixelType(PixelTypeEnumeration(ui.GetString("BITTYPE")));
  p.SetByteOrder(ByteOrderEnumeration(ui.GetString("BYTEORDER").toStdString()));
  p.SetInputFile(ui.GetFileName("FROM"));
  p.SetOutputCube("TO");

  if(ui.GetBoolean("SETNULLRANGE")) {
    p.SetNull(ui.GetDouble("NULLMIN"), ui.GetDouble("NULLMAX"));
  }
  if(ui.GetBoolean("SETHRSRANGE")) {
    p.SetHRS(ui.GetDouble("HRSMIN"), ui.GetDouble("HRSMAX"));
  }
  if(ui.GetBoolean("SETLRSRANGE")) {
    p.SetLRS(ui.GetDouble("LRSMIN"), ui.GetDouble("LRSMAX"));
  }

  p.StartProcess();
  p.EndProcess();

  return;
}

