#include "Isis.h"
#include "Cube.h"
#include "Table.h"
#include "Camera.h"
#include "LineScanCameraRotation.h"
#include "PixelOffset.h"
#include "SpiceRotation.h"
#include "IString.h"
#include "FileList.h"
#include "IException.h"
#include "CameraDetectorMap.h"

using namespace std;
using namespace Isis;

void IsisMain() {
  UserInterface &ui = Application::GetUserInterface();
  /*Processing steps
  1.  Open and read the jitter table, convert the pixel offsets to angles,
      and create the polynomials (solve for the coefficients) to use to do
      the high pass filter putting the results into a rotation matrix in the jitter class.
  2.  Apply the jitter correction in the LineScanCameraRotation object of the master cube.
  3.  Loop through FROMLIST correcting the pointing and writing out the
      updated camera pointing from the master cube
      */

  int degree = ui.GetInteger("DEGREE");

  // Get the input file list to make sure it is not empty and the master cube is included
  FileList list;
  list.read(ui.GetFileName("FROMLIST"));

  if(list.size() < 1) {
    QString msg = "The input list file [" + ui.GetFileName("FROMLIST") + "is empty";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  int ifile = 0;
  // Make sure the master file is included in the input file list
  while(ifile < (int) list.size() && list[ifile].toString() != FileName(ui.GetCubeName("MASTER")).expanded()) {
    ifile++;
  }

  if(ifile >= list.size()) {
    QString msg = "The master file, [" + FileName(ui.GetCubeName("MASTER")).expanded() + " is not included in " +
                 "the input list file " + ui.GetCubeName("FROMLIST") + "]";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  bool step2 = false;
  PvlGroup gp("AppjitResults");

  //Step 1:  Create the jitter rotation

  try {
    // Open the master cube
    Cube cube;
    cube.open(ui.GetCubeName("MASTER"), "rw");

    //check for existing polygon, if exists delete it
    if(cube.label()->hasObject("Polygon")) {
      cube.label()->deleteObject("Polygon");
    }

    // Get the camera
    Camera *cam = cube.camera();
    if(cam->DetectorMap()->LineRate() == 0.0) {
      QString msg = "[" + ui.GetCubeName("MASTER") + "] is not a line scan camera image";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    // Create the master rotation to be corrected
    int frameCode = cam->instrumentRotation()->Frame();
    cam->SetImage(int(cube.sampleCount() / 2), int(cube.lineCount() / 2));
    double tol = cam->PixelResolution();

    if(tol < 0.) {
      // Alternative calculation of .01*ground resolution of a pixel
      tol = cam->PixelPitch() * cam->SpacecraftAltitude() * 1000. / cam->FocalLength() / 100.;
    }

    LineScanCameraRotation crot(frameCode, cube, cam->instrumentRotation()->GetFullCacheTime(), tol);

    crot.SetPolynomialDegree(ui.GetInteger("DEGREE"));
    crot.SetAxes(1, 2, 3);
    if(ui.WasEntered("PITCHRATE")) crot.ResetPitchRate(ui.GetDouble("PITCHRATE"));
    if(ui.WasEntered("YAW")) crot.ResetYaw(ui.GetDouble("YAW"));
    crot.SetPolynomial();
    double baseTime = crot.GetBaseTime();
    double timeScale = crot.GetTimeScale();
    double fl = cam->FocalLength();
    double pixpitch = cam->PixelPitch();
    std::vector<double> cacheTime = cam->instrumentRotation()->GetFullCacheTime();

    // Get the jitter in pixels, compute jitter angles, and fit a polynomial to each angle
    PixelOffset jitter(ui.GetFileName("JITTERFILE"), fl, pixpitch, baseTime, timeScale, degree);
    jitter.LoadAngles(cacheTime);
    jitter.SetPolynomial();

    // Set the jitter and apply to the instrument rotation
    crot.SetJitter(&jitter);
    crot.ReloadCache();

    // Pull out the pointing cache as a table and write it
    Table cmatrix = crot.Cache("InstrumentPointing");
    //    cmatrix.Label().addComment("Corrected using appjit and" + ui.GetFileName("JITTERFILE"));
    cmatrix.Label() += PvlKeyword("Description", "Corrected using appjit and" + ui.GetFileName("JITTERFILE").toStdString());
    cmatrix.Label() += PvlKeyword("Kernels");
    PvlKeyword ckKeyword = crot.InstrumentPointingValue();

    for (int i = 0; i < ckKeyword.size(); i++) {
      cmatrix.Label()["Kernels"].addValue(ckKeyword[i]);
    }

    cube.write(cmatrix);

    // Write out the instrument position table
    Isis::PvlGroup kernels = cube.label()->findGroup("Kernels", Isis::Pvl::Traverse);

    // Save original kernels in keyword before changing to "Table" in the kernels group
    PvlKeyword origCk = kernels["InstrumentPointing"];
    kernels["InstrumentPointing"] = "Table";

    for (int i = 0;  i < origCk.size();  i++) {
      kernels["InstrumentPointing"].addValue(origCk[i]);
    }

    cube.putGroup(kernels);
    cube.close();
    gp += PvlKeyword("StatusMaster", ui.GetCubeName("MASTER").toStdString() + ":  camera pointing updated");

    // Apply the dejittered pointing to the rest of the files
    step2 = true;
    for(int ifile = 0; ifile < list.size(); ifile++) {
      if(list[ifile].toString() != ui.GetCubeName("MASTER")) {
        // Open the cube
        cube.open(list[ifile].toString(), "rw");
        //check for existing polygon, if exists delete it
        if(cube.label()->hasObject("Polygon")) {
          cube.label()->deleteObject("Polygon");
        }
        // Get the camera and make sure it is a line scan camera
        Camera *cam = cube.camera();
        if(cam->DetectorMap()->LineRate() == 0.0) {
          QString msg = "[" + ui.GetCubeName("FROM") + "] is not a line scan camera";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        // Write out the pointing cache as a table
        cube.write(cmatrix);

        // Write out the new instrument pointing table
        Isis::PvlGroup kernels = cube.label()->findGroup("Kernels", Isis::Pvl::Traverse);

        // Save original kernels in keyword before changing to "Table" in the kernels group
        PvlKeyword origCk = kernels["InstrumentPointing"];
        kernels["InstrumentPointing"] = "Table";

        for (int i = 0;  i < origCk.size();  i++) {
           kernels["InstrumentPointing"].addValue(origCk[i]);
        }
        cube.putGroup(kernels);
        cube.close();
        gp += PvlKeyword("Status" + std::to_string(ifile), list[ifile].toString().toStdString() + ":  camera pointing updated");
      }
    }
    Application::Log(gp);
  }
  catch(IException &e) {
    QString msg;
    if(!step2) {
      msg = "Unable to fit pointing for [" + ui.GetCubeName("MASTER") + "]";
    }
    else {
      msg = "Unable to update pointing for nonMaster file(s)";
    }
    throw IException(e, IException::User, msg, _FILEINFO_);
  }
}
