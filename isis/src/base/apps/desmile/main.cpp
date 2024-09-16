
#include "Isis.h"

#include <iostream>

#include "SpectralResampleFunctor.h"

#include "Camera.h"
#include "Cube.h"

#include "FileName.h"
#include "IException.h"
#include "IString.h"
#include "ProcessBySpectra.h"
#include "UserInterface.h"
#include "History.h"

#include "Spectel.h"
#include "SpectralDefinition.h"
#include "SpectralDefinitionFactory.h"
#include "SpectralDefinition1D.h"
#include "SpectralDefinition2D.h"

using namespace std;
using namespace Isis;


void IsisMain() {

  // TODO:
  // Handle input images that are not projected a little differently (simpler)

  UserInterface &ui = Application::GetUserInterface();

  ProcessBySpectra procSpectra;


  procSpectra.SetOutputRequirements(Isis::SpatialMatch);

  Cube *inCube = procSpectra.SetInputCube("FROM");

  // Get the spectral information for the input cube
  FileName smileDef = ui.GetCubeName("SMILEDEF").toStdString();
  // TODO: May want to add the cube to the constructor args so some error checks can be done
  SpectralDefinition* inputSpectralDef = SpectralDefinitionFactory::NewSpectralDefinition(smileDef);

  // Get the spectral information for the output cube
  FileName smileObjective = ui.GetFileName("OBJECTIVE").toStdString();
  SpectralDefinition* outputSpectralDef =
      SpectralDefinitionFactory::NewSpectralDefinition(smileObjective);

  // Set up the output cube. It may have a different number of bands than the input cube.
  Cube *outCube = procSpectra.SetOutputCube("TO", inCube->sampleCount(), inCube->lineCount(),
                                            outputSpectralDef->bandCount());

  // Correct the spectral smile
  SpectralResampleFunctor resampleFunctor(inputSpectralDef, outputSpectralDef, inCube->camera());
  procSpectra.Progress()->SetText("Adjusting spectra");

  // WARNING: DO NOT turn on threading for this process. The processing functor uses a camera
  procSpectra.ProcessCube(resampleFunctor, false);

  // Adjust the band bin group for the changes
  Isis::PvlGroup bandBin = outCube->label()->findGroup("BandBin", Isis::Pvl::Traverse);

  // Remove any keywords having the same number of values as input bands
  for (int key=0; key<bandBin.keywords(); ++key) {
    if (bandBin[key].size() == inCube->bandCount()) {
      bandBin.deleteKeyword(bandBin[key].name());
    }
  }

  //
  //// Add the new band bin keywords
  //PvlKeyword centers;
  //PvlKeyword widths;
  //for (int sample = 0; sample<inputSpectralDef) {
  //  for (int section = 0; section < inputSpectralDef->sectionCount(); ++section) {
  //    for (int i = 0; i < inputSpectralDef->bandCount(); ++i) {
  //      Spectel spec = getSpectel(1, 1, i);
  //      centers.addValue(inputSpectralDef->)
  //    }
  //  }
  //}
  //if () {
  //}
  //PvlGroup& bandBin = outCube->label().findGroup("BandBin");


  // TODO:
  // Put a new bandbin group in the output labels

  // TODO:
  // The camera model will no longer work on the output cube, so remove the ability to create a
  // camera on the output cube.
  // should be similar to what mosaic does to the instrument/kernels groups and SPICE blobs
  //  std::cout << *outCube->label() << std::endl;
    // Record apollofindrx history to the cube

  // create a History Blob with value found in the History PvlObject's Name keyword
  QString histName = QString::fromStdString(inCube->label()->findObject("History")["Name"]);
  // read cube's History PvlObject data into the History Object
  History histBlob = inCube->readHistory(histName);
  histBlob.AddEntry();
  outCube->write(histBlob, histName);

  procSpectra.Finalize();
  delete outputSpectralDef;
  outputSpectralDef = NULL;
  delete inputSpectralDef;
  inputSpectralDef = NULL;
}
