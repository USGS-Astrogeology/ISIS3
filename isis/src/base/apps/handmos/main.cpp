#include "Isis.h"
#include "ProcessMosaic.h"
#include "ProcessByLine.h"
#include "SpecialPixel.h"

using namespace std;
using namespace Isis;

void CreateMosaic(Buffer &buf);

/**
 * Handmos Application- Allows to hand place an image on the
 * mosaic with input, mosaic and band priorities. Band priority allows the
 * user the option to place a pixel on the mosaic based on the pixel in the
 * chosen band. The band can be specified by band number or Keyword as it
 * appears in BandBin group of the PVL label. Also has the  ability to track the
 * origin by adding a band to mosaic at the time of creation. As input images
 * are placed on the mosaic, the are stored as records in the table "InputImages"
 * in the mosaic. Ability to copy HS, LS, NULL values from input onto the mosaic.
 *
 */
void IsisMain() {
  // See if we need to create the output file
  UserInterface &ui = Application::GetUserInterface();

  int ns, nl, nb;

  ProcessMosaic p;

  bool bTrack = ui.GetBoolean("TRACK");
  p.SetTrackFlag(bTrack);

  QString inputFile = ui.GetAsString("FROM");
  QString mosaicFile = ui.GetCubeName("MOSAIC");

  // Set up the mosaic priority, either the input cube will be
  // placed ontop of the mosaic or beneath it
  ProcessMosaic::ImageOverlay overlay = ProcessMosaic::StringToOverlay(
      ui.GetString("PRIORITY"));
  QString sType;

  if (overlay == ProcessMosaic::UseBandPlacementCriteria) {
    if (ui.GetString("TYPE") == "BANDNUMBER") {
      p.SetBandNumber(ui.GetInteger("NUMBER"));
    }
    else {
      // Key name & value
      p.SetBandKeyword(ui.GetString("KEYNAME"), ui.GetString("KEYVALUE"));
    }
    // Band Criteria
    p.SetBandUseMaxValue( (ui.GetString("CRITERIA") == "GREATER") );
  }

  // Priority
  p.SetImageOverlay(overlay);

  if (ui.GetString("CREATE") == "YES") {
    ns = ui.GetInteger("NSAMPLES");
    nl = ui.GetInteger("NLINES");
    nb = ui.GetInteger("NBANDS");

    if (overlay == ProcessMosaic::AverageImageWithMosaic) {
      nb *= 2;
    }
    p.SetCreateFlag(true);

    ProcessByLine bl;

    bl.Progress()->SetText("Initializing base mosaic");
    CubeAttributeInput iAtt(inputFile.toStdString());
    bl.SetInputCube(inputFile, iAtt);

    if (!ui.GetBoolean("Propagate")) {
      bl.PropagateHistory(false);
      bl.PropagateLabels(false);
      bl.PropagateTables(false);
      bl.PropagatePolygons(false);
      bl.PropagateOriginalLabel(false);
    }

    CubeAttributeOutput oAtt = ui.GetOutputAttribute("MOSAIC");
    bl.SetOutputCube(mosaicFile, oAtt, ns, nl, nb);
    bl.ClearInputCubes();

    // Initialize the mosaic to defaults
    bl.StartProcess(CreateMosaic);
    bl.EndProcess();
  }

  // Set the input cube for the process object
  p.SetBandBinMatch(ui.GetBoolean("MATCHBANDBIN"));
  p.Progress()->SetText("Mosaicking");

  // Get the MatchDEM Flag
  p.SetMatchDEM(ui.GetBoolean("MATCHDEM"));

  // Get the value for HS, LS, NULL flags whether to transfer the special pixels
  // onto the mosaic. Holds good for "ontop" and "band" priorities only
  if (overlay != ProcessMosaic::PlaceImagesBeneath) {
    p.SetHighSaturationFlag(ui.GetBoolean("HIGHSATURATION"));
    p.SetLowSaturationFlag(ui.GetBoolean("LOWSATURATION"));
    p.SetNullFlag(ui.GetBoolean("NULL"));
  }

  // Get the starting line/sample/band to place the input cube
  int outSample = ui.GetInteger("OUTSAMPLE") - ui.GetInteger("INSAMPLE") + 1;
  int outLine   = ui.GetInteger("OUTLINE")   - ui.GetInteger("INLINE")   + 1;
  int outBand   = ui.GetInteger("OUTBAND")   - ui.GetInteger("INBAND")   + 1;

  // Set the input image and attributes
  CubeAttributeInput inAtt(inputFile.toStdString());
  p.SetInputCube(inputFile, inAtt);

  // Set the output mosaic
  Cube *to = p.SetOutputCube("MOSAIC");

  p.WriteHistory(*to);

  // Place the input in the mosaic
  //p.StartProcess(outSample, outLine, outBand, priority);
  p.StartProcess(outSample, outLine, outBand);

  if (bTrack != p.GetTrackFlag()) {
    ui.Clear("TRACK");
    ui.PutBoolean("TRACK", p.GetTrackFlag());
  }
  p.EndProcess();

  // Log the changes to NBANDS and to TRACK if any
  PvlObject hist = Isis::iApp->History();
  Isis::iApp->Log(hist.findGroup("UserParameters"));

  // Logs the input file location in the mosaic
  for (int i = 0; i < p.imagePositions().groups(); i++) {
    Application::Log(p.imagePositions().group(i));
  }
}

/**
 * Initialize the mosaic to defaults
 *
 * @author sprasad (10/14/2009)
 *
 * @param buf
 */
void CreateMosaic(Buffer &buf) {
  for(int i = 0; i < buf.size(); i++) {
    buf[i] = NULL8;
  }
}
