#include "ProcessImportPds.h"

#include "Application.h"
#include "UserInterface.h"
#include "FileName.h"

using namespace std;
namespace Isis {
  void pds2isis(UserInterface &ui, Pvl *log) {
    ProcessImportPds p;
    Pvl label;
    QString labelFile = ui.GetFileName("FROM");
    QString imageFile("");
    if(ui.WasEntered("IMAGE")) {
      imageFile = ui.GetFileName("IMAGE");
    }

    p.SetPdsFile(labelFile, imageFile, label);

  
    CubeAttributeOutput &att = ui.GetOutputAttribute("TO");
    Cube *ocube = p.SetOutputCube(ui.GetCubeName("TO"), att);

    // Get user entered special pixel ranges
    if(ui.GetBoolean("SETNULLRANGE")) {
      p.SetNull(ui.GetDouble("NULLMIN"), ui.GetDouble("NULLMAX"));
    }
    if(ui.GetBoolean("SETHRSRANGE")) {
      p.SetHRS(ui.GetDouble("HRSMIN"), ui.GetDouble("HRSMAX"));
    }
    if(ui.GetBoolean("SETHISRANGE")) {
      p.SetHIS(ui.GetDouble("HISMIN"), ui.GetDouble("HISMAX"));
    }
    if(ui.GetBoolean("SETLRSRANGE")) {
      p.SetLRS(ui.GetDouble("LRSMIN"), ui.GetDouble("LRSMAX"));
    }
    if(ui.GetBoolean("SETLISRANGE")) {
      p.SetLIS(ui.GetDouble("LISMIN"), ui.GetDouble("LISMAX"));
    }

    // Export the cube
    p.StartProcess();

    // Get as many of the other labels as we can
    Pvl otherLabels;
    p.TranslatePdsProjection(otherLabels);
    if(p.IsIsis2()) {
      p.TranslateIsis2Labels(otherLabels);
    }
    else {
      p.TranslatePdsLabels(otherLabels);
    }

    if(otherLabels.hasGroup("Mapping") &&
        (otherLabels.findGroup("Mapping").keywords() > 0)) {
      ocube->putGroup(otherLabels.findGroup("Mapping"));
    }
    if(otherLabels.hasGroup("Instrument") &&
        (otherLabels.findGroup("Instrument").keywords() > 0)) {
      ocube->putGroup(otherLabels.findGroup("Instrument"));
    }
    if(otherLabels.hasGroup("BandBin") &&
        (otherLabels.findGroup("BandBin").keywords() > 0)) {
      ocube->putGroup(otherLabels.findGroup("BandBin"));
    }
    if(otherLabels.hasGroup("Archive") &&
        (otherLabels.findGroup("Archive").keywords() > 0)) {
      ocube->putGroup(otherLabels.findGroup("Archive"));
    }
    
    //  Check for and log any change from the default projection offsets and multipliers
    if (p.GetProjectionOffsetChange()) {
      PvlGroup results = p.GetProjectionOffsetGroup();
      results.setName("Results");
      results[0].addComment("Projection offsets and multipliers have been changed from");
      results[0].addComment("defaults. New values are below.");
      Application::Log(results);
    }

    p.EndProcess();

    return;
  }
}

