#include <iostream>

#include "Pvl.h"
#include "cnetbin2pvl.h"
#include "NetworkFixtures.h"
#include "Progress.h"
#include "gmock/gmock.h"
#include "FileName.h"
#include "UserInterface.h"

using namespace Isis;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/cnetbin2pvl.xml").expanded());

TEST_F(ThreeImageNetwork, FunctionalTestCnetbin2pvlDefault) {
  QString pvlOut = tempDir.path()+"/cnetbin2pvlNetwork.pvl";

  QVector<QString> args = {"to="+pvlOut};
  UserInterface ui(APP_XML, args);

  Progress progress;
  cnetbin2pvl(*network, ui, &progress);

  Pvl pvl;
  try {
    pvl.read(pvlOut.toStdString());
  }
  catch (IException &e) {
    FAIL() << "Unable to read PVL file" << std::endl;
  }
}
