/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

#include "cam2cam.h"

#include "CameraFactory.h"
#include "Camera.h"
#include "Distance.h"
#include "ProcessRubberSheet.h"
#include "IException.h"


using namespace std;
using namespace Isis;

void IsisMain() {
  UserInterface &ui = Application::GetUserInterface();
  cam2cam(ui);
}

