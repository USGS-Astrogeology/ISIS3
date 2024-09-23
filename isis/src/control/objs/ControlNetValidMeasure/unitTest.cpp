/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "ControlNetValidMeasure.h"
#include "Preference.h"
#include "IException.h"

using namespace std;
using namespace Isis;

int main(void) {
  Preference::Preferences(true);
  try {
    try {
      Pvl pvlLog;
      PvlGroup op("ValidMeasure");
      op += PvlKeyword("Name", "StandardDeviation");
      op += PvlKeyword("DeltaLine", toString(100));
      op += PvlKeyword("DeltaSamp", toString(100));
      op += PvlKeyword("Samples", toString(15));
      op += PvlKeyword("Lines", toString(15));
      op += PvlKeyword("MinimumInterest", toString(0.01));
      op += PvlKeyword("MinDN", toString(1.0));
      op += PvlKeyword("MaxDN", toString(-1.0));
      op += PvlKeyword("MinEmission", toString(15.0));
      op += PvlKeyword("MaxEmission", toString(25.0));
      op += PvlKeyword("MinIncidence", toString(0.0));
      op += PvlKeyword("MaxIncidence", toString(135.0));

      pvlLog += op;

      ControlNetValidMeasure cnetVM(pvlLog);
      cout << cnetVM.GetLogPvl();
    }
    catch(IException &e) {
      e.print();
    }

    try {
      Pvl pvlLog;
      PvlGroup op("ValidMeasure");
      op += PvlKeyword("Name", "None");
      op += PvlKeyword("MinDN", toString(-1.0));
      op += PvlKeyword("MaxDN", toString(1.0));
      op += PvlKeyword("MinEmission", toString(25.0));
      op += PvlKeyword("MaxEmission", toString(15.0));
      op += PvlKeyword("MinIncidence", toString(0.0));
      op += PvlKeyword("MaxIncidence", toString(135.0));

      pvlLog += op;

      ControlNetValidMeasure cnetVM(pvlLog);
      cout << cnetVM.GetLogPvl();
    }
    catch(IException &e) {
      e.print();
    }

    try {
      Pvl pvlLog;
      PvlGroup op("ValidMeasure");
      op += PvlKeyword("Name", "StandardDeviation");
      op += PvlKeyword("DeltaLine", toString(100));
      op += PvlKeyword("DeltaSamp", toString(100));
      op += PvlKeyword("Samples", toString(15));
      op += PvlKeyword("Lines", toString(15));
      op += PvlKeyword("MinimumInterest", toString(0.01));
      op += PvlKeyword("MinDN", toString(-1.0));
      op += PvlKeyword("MaxDN", toString(1.0));
      op += PvlKeyword("MinEmission", toString(0.0));
      op += PvlKeyword("MaxEmission", toString(135.0));
      op += PvlKeyword("MinIncidence", toString(0.0));
      op += PvlKeyword("MaxIncidence", toString(150.0));

      pvlLog += op;

      ControlNetValidMeasure cnetVM(pvlLog);
      cout << cnetVM.GetLogPvl();
    }
    catch(IException &e) {
      e.print();
    }

    try {
      Pvl pvlLog;
      PvlGroup op("ValidMeasure");
      op += PvlKeyword("Name", "None");
      op += PvlKeyword("MinDN", toString(-1.0));
      op += PvlKeyword("MaxDN", toString(1.0));
      op += PvlKeyword("MinEmission", toString(0.0));
      op += PvlKeyword("MaxEmission", toString(135.0));
      op += PvlKeyword("MinIncidence", toString(0.0));
      op += PvlKeyword("MaxIncidence", toString(135.0));
      op += PvlKeyword("MinResolution", toString(100.0));
      op += PvlKeyword("MaxResolution", toString(-1.0));

      pvlLog += op;

      ControlNetValidMeasure cnetVM(pvlLog);
      cout << cnetVM.GetLogPvl();
    }
    catch(IException &e) {
      e.print();
    }

    try {
      Pvl pvlLog;
      PvlGroup op("ValidMeasure");
      op += PvlKeyword("Name", "None");
      op += PvlKeyword("MinDN", toString(-1.0));
      op += PvlKeyword("MaxDN", toString(1.0));
      op += PvlKeyword("MinEmission", toString(0.0));
      op += PvlKeyword("MaxEmission", toString(135.0));
      op += PvlKeyword("MinIncidence", toString(0.0));
      op += PvlKeyword("MaxIncidence", toString(135.0));
      op += PvlKeyword("MinResolution", toString(100.0));
      op += PvlKeyword("MaxResolution", toString(500.0));
      op += PvlKeyword("SampleResidual", toString(5.0));
      op += PvlKeyword("LineResidual", toString(5.0));
      op += PvlKeyword("ResidualMagnitude", toString(10.0));
      op += PvlKeyword("SampleShift", toString(5.0));
      op += PvlKeyword("LineShift", toString(5.0));

      pvlLog += op;

      ControlNetValidMeasure cnetVM(pvlLog);
      cout << cnetVM.GetLogPvl();
    }
    catch(IException &e) {
      e.print();
    }

    try {
      Pvl pvlLog;
      PvlGroup op("ValidMeasure");
      op += PvlKeyword("Name", "None");
      op += PvlKeyword("MinDN", toString(-1.0));
      op += PvlKeyword("MaxDN", toString(1.0));
      op += PvlKeyword("MinEmission", toString(0.0));
      op += PvlKeyword("MaxEmission", toString(135.0));
      op += PvlKeyword("MinIncidence", toString(0.0));
      op += PvlKeyword("MaxIncidence", toString(135.0));
      op += PvlKeyword("MinResolution", toString(100.0));
      op += PvlKeyword("MaxResolution", toString(500.0));
      op += PvlKeyword("SampleResidual", toString(5.0));
      op += PvlKeyword("LineResidual", toString(5.0));
      op += PvlKeyword("SampleShift", toString(5.0));
      op += PvlKeyword("LineShift", toString(5.0));
      op += PvlKeyword("PixelShift", toString(10.0));

      pvlLog += op;

      ControlNetValidMeasure cnetVM(pvlLog);
      cout << cnetVM.GetLogPvl();
    }
    catch(IException &e) {
      e.print();
    }

    try {
      Pvl pvlLog;
      PvlGroup op("ValidMeasure");
      op += PvlKeyword("Name", "None");
      op += PvlKeyword("MinDN", toString(-1.0));
      op += PvlKeyword("MaxDN", toString(1.0));
      op += PvlKeyword("MinEmission", toString(0.0));
      op += PvlKeyword("MaxEmission", toString(135.0));
      op += PvlKeyword("MinIncidence", toString(0.0));
      op += PvlKeyword("MaxIncidence", toString(135.0));
      op += PvlKeyword("MinResolution", toString(100.0));
      op += PvlKeyword("MaxResolution", toString(500.0));
      op += PvlKeyword("SampleResidual", toString(5.0));
      op += PvlKeyword("LineResidual", toString(5.0));
      op += PvlKeyword("SampleShift", toString(5.0));
      op += PvlKeyword("LineShift", toString(5.0));

      pvlLog += op;

      ControlNetValidMeasure cnetVM(pvlLog);
      cout << cnetVM.GetLogPvl() << endl;

      cout << "Test LocationString: " << cnetVM.LocationString(0.6, 1.6).toStdString() << endl;
    }
    catch(IException &e) {
      e.print();
    }
  }
  catch(IException &e) {
    throw IException(e,
        IException::Unknown, "ControlNetValidMeasure Unit test Exception",
        _FILEINFO_);
  }

  return 0;
}
