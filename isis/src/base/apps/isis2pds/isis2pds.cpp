#include <QString>

#include "Application.h"
#include "Cube.h"
#include "ExportDescription.h"
#include "FileName.h"
#include "isis2pds.h"
#include "Process.h"
#include "ProcessExportPds.h"
#include "ProcessExportPds4.h"
#include "Pvl.h"
#include "PvlKeyword.h"
#include "PvlToXmlTranslationManager.h"


using namespace std;
namespace Isis{

  enum Pixtype { NONE, NEG, BOTH };

  void setRangeAndPixels(UserInterface &ui, ProcessExport &p,
                         double &min, double &max, Pixtype ptype);

  void isis2pds(UserInterface &ui, Pvl *log){
    Cube icube;
    CubeAttributeInput inAtt = ui.GetInputAttribute("FROM");
    if (inAtt.bands().size() != 0) {
      icube.setVirtualBands(inAtt.bands());
    }
    icube.open(ui.GetCubeName("FROM"));
    isis2pds(&icube, ui, log);
  }

  void isis2pds(Cube *icube, UserInterface &ui, Pvl *log) {

    if (ui.GetString("PDSVERSION") == "PDS3") {
      // Set the processing object
      ProcessExportPds p;

      // Setup the input cube
      p.SetInputCube(icube);

      if (ui.GetString("STRETCH") == "LINEAR") {
        if (ui.GetString("BITTYPE") != "32BIT") {
          p.SetInputRange(ui);
        }
      }
      if (ui.GetString("STRETCH") == "MANUAL") {
        p.SetInputRange(ui.GetDouble("MINIMUM"), ui.GetDouble("MAXIMUM"));
      }

      double min = -DBL_MAX;
      double max = DBL_MAX;

      if (ui.GetString("BITTYPE") == "8BIT") {
        p.SetOutputType(Isis::UnsignedByte);
        min = 0.0;
        max = 255.0;
        setRangeAndPixels(ui, p, min, max, BOTH);
      }
      else if (ui.GetString("BITTYPE") == "S16BIT") {
        p.SetOutputType(Isis::SignedWord);
        min = -32768.0;
        max = 32767.0;
        setRangeAndPixels(ui, p, min, max, NEG);
      }
      else if (ui.GetString("BITTYPE") == "U16BIT") {
        p.SetOutputType(Isis::UnsignedWord);
        min = 0.0;
        max = 65535.0;
        setRangeAndPixels(ui, p, min, max, BOTH);
      }
      else {
        p.SetOutputType(Isis::Real);
        p.SetOutputNull(Isis::NULL4);
        p.SetOutputLrs(Isis::LOW_REPR_SAT4);
        p.SetOutputLis(Isis::LOW_INSTR_SAT4);
        p.SetOutputHrs(Isis::HIGH_REPR_SAT4);
        p.SetOutputHis(Isis::HIGH_INSTR_SAT4);
        setRangeAndPixels(ui, p, min, max, NONE);
      }

      if (ui.GetString("ENDIAN") == "MSB") {
        p.SetOutputEndian(Isis::Msb);
      }
      else if (ui.GetString("ENDIAN") == "LSB") {
        p.SetOutputEndian(Isis::Lsb);
      }

      if (ui.GetString("LABTYPE") == "FIXED") {
        p.SetExportType(ProcessExportPds::Fixed);
      }

      if (ui.GetBoolean("CHECKSUM")) {
        p.setCanGenerateChecksum(true);
      }

      // Set the resolution to  Kilometers
      p.SetPdsResolution(ProcessExportPds::Kilometer);

      p.StandardPdsLabel(ProcessExportPds::Image);

      FileName outFile(ui.GetFileName("TO", "img").toStdString());
      QString outFileName(QString::fromStdString(outFile.expanded()));
      ofstream oCube(outFileName.toLatin1().data());
      p.OutputLabel(oCube);
      p.StartProcess(oCube);
      if (ui.GetBoolean("CHECKSUM")) {
        p.updateChecksumInLabel(oCube);
      }
      oCube.close();
      p.EndProcess();

      //Records what it did to the print.prt file
      PvlGroup results("DNs Used");
      results += PvlKeyword("Null", std::to_string(p.OutputNull()));
      results += PvlKeyword("LRS", std::to_string(p.OutputLrs()));
      results += PvlKeyword("LIS", std::to_string(p.OutputLis()));
      results += PvlKeyword("HIS", std::to_string(p.OutputHis()));
      results += PvlKeyword("HRS", std::to_string(p.OutputHrs()));
      results += PvlKeyword("ValidMin", std::to_string(min));
      results += PvlKeyword("ValidMax", std::to_string(max));
      Application::Log(results);
    }
    else {
      // Setup the process and set the input cube
      ProcessExportPds4 process;

      process.SetInputCube(icube);

      PvlObject *label= icube->label();
      if (!label->hasObject("IsisCube")) {
        std::string msg = "Input file [" + ui.GetCubeName("FROM").toStdString() +
                      "] does not appear to be an ISIS cube.";
        throw  IException(IException::User, msg, _FILEINFO_);
      }

      FileName outFile(ui.GetFileName("TO", "img").toStdString());
      QString outFileName(QString::fromStdString(outFile.expanded()));

      if (ui.GetString("STRETCH") == "LINEAR") {
        if (ui.GetString("BITTYPE") != "32BIT") {
          process.SetInputRange(ui);
        }
      }
      if (ui.GetString("STRETCH") == "MANUAL") {
        process.SetInputRange(ui.GetDouble("MINIMUM"), ui.GetDouble("MAXIMUM"));
      }

      double min = -DBL_MAX;
      double max = DBL_MAX;

      if (ui.GetString("BITTYPE") == "8BIT") {
        process.SetOutputType(Isis::UnsignedByte);
        min = 0.0;
        max = 255.0;
        setRangeAndPixels(ui, process, min, max, BOTH);
      }
      else if (ui.GetString("BITTYPE") == "S16BIT") {
        process.SetOutputType(Isis::SignedWord);
        min = -32768.0;
        max = 32767.0;
        setRangeAndPixels(ui, process, min, max, NEG);
      }
      else if (ui.GetString("BITTYPE") == "U16BIT") {
        process.SetOutputType(Isis::UnsignedWord);
        min = 0.0;
        max = 65535.0;
        setRangeAndPixels(ui, process, min, max, BOTH);
      }
      else {
        process.SetOutputType(Isis::Real);
        process.SetOutputNull(Isis::NULL4);
        process.SetOutputLrs(Isis::LOW_REPR_SAT4);
        process.SetOutputLis(Isis::LOW_INSTR_SAT4);
        process.SetOutputHrs(Isis::HIGH_REPR_SAT4);
        process.SetOutputHis(Isis::HIGH_INSTR_SAT4);
        setRangeAndPixels(ui, process, min, max, NONE);
      }

      if (ui.GetString("ENDIAN") == "MSB") {
        process.SetOutputEndian(Isis::Msb);
      }
      else if (ui.GetString("ENDIAN") == "LSB") {
        process.SetOutputEndian(Isis::Lsb);
      }

      // Records what it did to the print.prt file
      PvlGroup results("DNs Used");
      results += PvlKeyword("Null", std::to_string(process.OutputNull()));
      results += PvlKeyword("LRS", std::to_string(process.OutputLrs()));
      results += PvlKeyword("LIS", std::to_string(process.OutputLis()));
      results += PvlKeyword("HIS", std::to_string(process.OutputHis()));
      results += PvlKeyword("HRS", std::to_string(process.OutputHrs()));
      results += PvlKeyword("ValidMin", std::to_string(min));
      results += PvlKeyword("ValidMax", std::to_string(max));
      Application::Log(results);

      process.StandardPds4Label();
      process.WritePds4(outFileName);
    }

    return;
  }

  //Sets up special pixels and valid pixel ranges
  void setRangeAndPixels(UserInterface &ui, ProcessExport &p, double &min, double &max, Pixtype ptype) {
    if (ptype == NEG) {
      if (ui.GetBoolean("NULL")) {
        p.SetOutputNull(min++);
      }
      if (ui.GetBoolean("LRS")) {
        p.SetOutputLrs(min++);
      }
      if (ui.GetBoolean("LIS")) {
        p.SetOutputLis(min++);
      }
      if (ui.GetBoolean("HIS")) {
        p.SetOutputHis(min++);
      }
      if (ui.GetBoolean("HRS")) {
        p.SetOutputHrs(min++);
      }
    }
    else if (ptype == BOTH) {
      if (ui.GetBoolean("NULL")) {
        p.SetOutputNull(min++);
      }
      if (ui.GetBoolean("LRS")) {
        p.SetOutputLrs(min++);
      }
      if (ui.GetBoolean("LIS")) {
        p.SetOutputLis(min++);
      }
      if (ui.GetBoolean("HRS")) {
        p.SetOutputHrs(max--);
      }
      if (ui.GetBoolean("HIS")) {
        p.SetOutputHis(max--);
      }
    }
    p.SetOutputRange(min, max);
  }
}
