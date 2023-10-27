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
      op += PvlKeyword("DeltaLine", std::to_string(100));
      op += PvlKeyword("DeltaSamp", std::to_string(100));
      op += PvlKeyword("Samples", std::to_string(15));
      op += PvlKeyword("Lines", std::to_string(15));
      op += PvlKeyword("MinimumInterest", std::to_string(0.01));
      op += PvlKeyword("MinDN", std::to_string(1.0));
      op += PvlKeyword("MaxDN", std::to_string(-1.0));
      op += PvlKeyword("MinEmission", std::to_string(15.0));
      op += PvlKeyword("MaxEmission", std::to_string(25.0));
      op += PvlKeyword("MinIncidence", std::to_string(0.0));
      op += PvlKeyword("MaxIncidence", std::to_string(135.0));

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
      op += PvlKeyword("MinDN", std::to_string(-1.0));
      op += PvlKeyword("MaxDN", std::to_string(1.0));
      op += PvlKeyword("MinEmission", std::to_string(25.0));
      op += PvlKeyword("MaxEmission", std::to_string(15.0));
      op += PvlKeyword("MinIncidence", std::to_string(0.0));
      op += PvlKeyword("MaxIncidence", std::to_string(135.0));

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
      op += PvlKeyword("DeltaLine", std::to_string(100));
      op += PvlKeyword("DeltaSamp", std::to_string(100));
      op += PvlKeyword("Samples", std::to_string(15));
      op += PvlKeyword("Lines", std::to_string(15));
      op += PvlKeyword("MinimumInterest", std::to_string(0.01));
      op += PvlKeyword("MinDN", std::to_string(-1.0));
      op += PvlKeyword("MaxDN", std::to_string(1.0));
      op += PvlKeyword("MinEmission", std::to_string(0.0));
      op += PvlKeyword("MaxEmission", std::to_string(135.0));
      op += PvlKeyword("MinIncidence", std::to_string(0.0));
      op += PvlKeyword("MaxIncidence", std::to_string(150.0));

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
      op += PvlKeyword("MinDN", std::to_string(-1.0));
      op += PvlKeyword("MaxDN", std::to_string(1.0));
      op += PvlKeyword("MinEmission", std::to_string(0.0));
      op += PvlKeyword("MaxEmission", std::to_string(135.0));
      op += PvlKeyword("MinIncidence", std::to_string(0.0));
      op += PvlKeyword("MaxIncidence", std::to_string(135.0));
      op += PvlKeyword("MinResolution", std::to_string(100.0));
      op += PvlKeyword("MaxResolution", std::to_string(-1.0));

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
      op += PvlKeyword("MinDN", std::to_string(-1.0));
      op += PvlKeyword("MaxDN", std::to_string(1.0));
      op += PvlKeyword("MinEmission", std::to_string(0.0));
      op += PvlKeyword("MaxEmission", std::to_string(135.0));
      op += PvlKeyword("MinIncidence", std::to_string(0.0));
      op += PvlKeyword("MaxIncidence", std::to_string(135.0));
      op += PvlKeyword("MinResolution", std::to_string(100.0));
      op += PvlKeyword("MaxResolution", std::to_string(500.0));
      op += PvlKeyword("SampleResidual", std::to_string(5.0));
      op += PvlKeyword("LineResidual", std::to_string(5.0));
      op += PvlKeyword("ResidualMagnitude", std::to_string(10.0));
      op += PvlKeyword("SampleShift", std::to_string(5.0));
      op += PvlKeyword("LineShift", std::to_string(5.0));

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
      op += PvlKeyword("MinDN", std::to_string(-1.0));
      op += PvlKeyword("MaxDN", std::to_string(1.0));
      op += PvlKeyword("MinEmission", std::to_string(0.0));
      op += PvlKeyword("MaxEmission", std::to_string(135.0));
      op += PvlKeyword("MinIncidence", std::to_string(0.0));
      op += PvlKeyword("MaxIncidence", std::to_string(135.0));
      op += PvlKeyword("MinResolution", std::to_string(100.0));
      op += PvlKeyword("MaxResolution", std::to_string(500.0));
      op += PvlKeyword("SampleResidual", std::to_string(5.0));
      op += PvlKeyword("LineResidual", std::to_string(5.0));
      op += PvlKeyword("SampleShift", std::to_string(5.0));
      op += PvlKeyword("LineShift", std::to_string(5.0));
      op += PvlKeyword("PixelShift", std::to_string(10.0));

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
      op += PvlKeyword("MinDN", std::to_string(-1.0));
      op += PvlKeyword("MaxDN", std::to_string(1.0));
      op += PvlKeyword("MinEmission", std::to_string(0.0));
      op += PvlKeyword("MaxEmission", std::to_string(135.0));
      op += PvlKeyword("MinIncidence", std::to_string(0.0));
      op += PvlKeyword("MaxIncidence", std::to_string(135.0));
      op += PvlKeyword("MinResolution", std::to_string(100.0));
      op += PvlKeyword("MaxResolution", std::to_string(500.0));
      op += PvlKeyword("SampleResidual", std::to_string(5.0));
      op += PvlKeyword("LineResidual", std::to_string(5.0));
      op += PvlKeyword("SampleShift", std::to_string(5.0));
      op += PvlKeyword("LineShift", std::to_string(5.0));

      pvlLog += op;

      ControlNetValidMeasure cnetVM(pvlLog);
      cout << cnetVM.GetLogPvl() << endl;

      cout << "Test LocationString: " << cnetVM.LocationString(0.6, 1.6) << endl;
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
