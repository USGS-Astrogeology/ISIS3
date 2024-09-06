#include "fits2isis.h"
#include "Cube.h"
#include "IException.h"
#include "IString.h"
#include "OriginalLabel.h"
#include "ProcessImportFits.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "UserInterface.h"

#include <fstream>
#include <iostream>
#include <QString>

using namespace std;


namespace Isis{

  void fits2isis(UserInterface &ui) {
    ProcessImportFits pfits;
    pfits.setFitsFile(FileName(ui.GetFileName("FROM").toStdString()));

    if(ui.GetString("ORGANIZATION") == "BIL") {
      pfits.SetOrganization(ProcessImport::BIL);
    }
    else if(ui.GetString("ORGANIZATION") == "BSQ") {
      pfits.SetOrganization(ProcessImport::BSQ);
    }
    else if(ui.GetString("ORGANIZATION") == "BIP") {
      pfits.SetOrganization(ProcessImport::BIP);
    }
    else {
      std::string msg = "Unknow value for ORGANIZATION [" + ui.GetString("ORGANIZATION") + "]";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    pfits.setProcessFileStructure(ui.GetInteger("IMAGENUMBER"));

    CubeAttributeOutput &att = ui.GetOutputAttribute("TO");
    Cube *output = pfits.SetOutputCube(ui.GetCubeName("TO"), att);

    // Add instrument group if any keywords were put into it.
    PvlGroup instGrp = pfits.standardInstrumentGroup(pfits.fitsImageLabel(0));
    if (instGrp.keywords() > 0) {

      output->label()->findObject("IsisCube") += instGrp;
    }

    // Save the input FITS label in the Cube original labels
    Pvl pvl;
    pvl += pfits.fitsImageLabel(0);
    OriginalLabel originals(pvl);
    output->write(originals);

    // Convert the image data
    pfits.StartProcess();
    pfits.EndProcess();
  }
}
