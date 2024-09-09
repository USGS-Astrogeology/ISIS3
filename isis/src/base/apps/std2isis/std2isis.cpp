#include <QString>

#include "ImageImporter.h"
#include "UserInterface.h"

#include "std2isis.h"

namespace Isis {
  void std2isis(UserInterface &ui) {
    FileName inputName = ui.GetFileName("FROM").toStdString();
    ImageImporter *importer = ImageImporter::fromFileName(inputName);

    // Explicitly set band dimension if a specific color mode is desired
    IString mode = ui.GetString("MODE").toStdString();
    if (mode != "AUTO") {
        int bands = mode == "GRAYSCALE" ? 1 : mode == "ARGB" ? 4 : 3;
        importer->setBands(bands);
    }

    // Set special pixel ranges
    if (ui.GetBoolean("SETNULLRANGE"))
        importer->setNullRange(ui.GetDouble("NULLMIN"), ui.GetDouble("NULLMAX"));
    if (ui.GetBoolean("SETHRSRANGE"))
        importer->setHrsRange(ui.GetDouble("HRSMIN"), ui.GetDouble("HRSMAX"));
    if (ui.GetBoolean("SETLRSRANGE"))
        importer->setLrsRange(ui.GetDouble("LRSMIN"), ui.GetDouble("LRSMAX"));

    // Import the image
    FileName outputName = ui.GetCubeName("TO").toStdString();
    CubeAttributeOutput &att = ui.GetOutputAttribute("TO");
    importer->import(outputName, att);

    delete importer;
  }

}