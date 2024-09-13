/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <iostream>
#include <QList>

#include "ExportDescription.h"
#include "PixelType.h"
#include "Preference.h"
#include "IString.h"

using namespace Isis;
using std::cout;
using std::endl;


const IString className = "ExportDescription";


void printFailure(IString methodName, IString message="") {
  cout <<
    "FAIL : " << className << "::" << methodName <<
    " : " << message << endl;
}

void compareEqual(IString methodName, double expected, double found,
    IString expectedString="", IString foundString="") {

  bool equal = expected == found;
  cout <<
    (equal ? "PASS : " : "FAIL : ") <<
    className << "::" << methodName << " : " <<
    (expectedString != "" ? expectedString : IString(expected)) <<
    (equal ? " == " : " != ") <<
    (foundString != "" ? foundString : IString(found)) << endl;
}

void compareEqual(IString methodName, int expected, int found,
    IString expectedString="", IString foundString="") {

  bool equal = expected == found;
  cout <<
    (equal ? "PASS : " : "FAIL : ") <<
    className << "::" << methodName << " : " <<
    (expectedString != "" ? expectedString : IString(expected)) <<
    (equal ? " == " : " != ") <<
    (foundString != "" ? foundString : IString(found)) << endl;
}

void compareEqual(IString methodName, IString expected, IString found,
    IString expectedString="", IString foundString="") {

  bool equal = expected == found;
  cout <<
    (equal ? "PASS : " : "FAIL : ") <<
    className << "::" << methodName << " : " <<
    (expectedString != "" ? expectedString : IString(expected)) <<
    (equal ? " == " : " != ") <<
    (foundString != "" ? foundString : IString(found)) << endl;
}


int main() {
  Preference::Preferences(true);

  cout << "********* Start testing of " << className << " *********" << endl;
  ExportDescription desc;

  compareEqual("pixelType()", None, desc.pixelType(), "None");

  desc.setPixelType(SignedWord);
  compareEqual("setPixelType()", SignedWord, desc.pixelType(), "SignedWord");

  compareEqual("outputPixelValidMin()", -32752.0, desc.outputPixelValidMin());
  compareEqual("outputPixelValidMax()", 32767.0, desc.outputPixelValidMax());
  compareEqual("outputPixelNull()", -32768.0, desc.outputPixelNull());

  QList<FileName> filenames;
  filenames.append(FileName("red.cub"));
  filenames.append(FileName("green.cub"));
  filenames.append(FileName("blue.cub"));
  try {
    CubeAttributeInput att;
    att.setAttributes("+1");

    int index = desc.addChannel(filenames[0], att);
    compareEqual("addChannel()", 0, index);

    index = desc.addChannel(filenames[1], att, 100.0, 500.0);
    compareEqual("addChannel()", 1, index);

    index = desc.addChannel(filenames[2], att, 500.0, 1000.0);
    compareEqual("addChannel()", 2, index);
  }
  catch (IException &e) {
    printFailure("addChannel()", e.toString());
  }

  IString innerName = "ChannelDescription";
  compareEqual("channelCount()", 3, desc.channelCount());
  for (int i = 0; i < desc.channelCount(); i++) {
    const ExportDescription::ChannelDescription &channel = desc.channel(i);
    compareEqual(innerName + "::filename()",
        filenames[i].name(), channel.filename().name());
    compareEqual(innerName + "::attributes()", "1", channel.attributes().toString().toStdString().substr(1));

    if (i == 0) {
      compareEqual(innerName + "::hasCustomRange()",
          false, channel.hasCustomRange(), "false");
    }
    else {
      compareEqual(innerName + "::hasCustomRange()",
          true, channel.hasCustomRange(), "true");

      if (i == 2) {
        compareEqual(innerName + "::inputMinimum()",
            500.0, channel.inputMinimum());
        compareEqual(innerName + "::inputMaximum()",
            1000.0, channel.inputMaximum());
      }
    }
  }

  cout << "********* Finished testing of " << className << " *********" << endl;
  return 0;
}

