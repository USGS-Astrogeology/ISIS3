#include <iostream>

#include "cnetpvl2bin.h"
#include "NetworkFixtures.h"
#include "Progress.h"
#include "gmock/gmock.h"
#include "FileName.h"
#include "UserInterface.h"

using namespace Isis;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/cnetpvl2bin.xml").expanded());

TEST_F(ThreeImageNetwork, FunctionalTestCnetpvl2binDefault) {
  QString binOut = tempDir.path()+"/cnetbin2Network.net";

  QVector<QString> args = {"to="+binOut};
  UserInterface ui(APP_XML, args);

  Progress progress;
  cnetpvl2bin(*network, ui, &progress);

  ControlNet cnet;
  try {
    cnet.ReadControl(binOut);
  }
  catch (IException &e) {
    FAIL() << "Unable to read binary control network" << std::endl;
  }
}
