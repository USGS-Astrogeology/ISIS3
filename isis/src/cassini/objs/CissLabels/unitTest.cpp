/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <iostream>
#include "CissLabels.h"
#include "IException.h"
#include "Pvl.h"

using namespace std;
using namespace Isis;

int main(int argc, char *argv[]) {
  try {
    cout << endl << "Unit test for CissLabels" << endl << endl;

    Pvl p1("$ISISTESTDATA/isis/src/cassini/unitTestData/W1294561261_1.c2i.nospice.cub");
    Pvl p2("$ISISTESTDATA/isis/src/cassini/unitTestData/N1355543510_1.c2i.nospice.cub");
    Pvl p3("$ISISTESTDATA/isis/src/cassini/unitTestData/N1536363784_1.c2i.spice.cub");
    Pvl p4("$ISISTESTDATA/isis/src/cassini/unitTestData/N1313633704_1.c2i.nospice.cub");
    CissLabels lab1(p1);
    CissLabels lab2(p2);
    CissLabels lab3(p3);
    CissLabels lab4(p4);

    cout << endl << "ISSWA,NotCompressed,12Bit,CAS-ISS,WacOnly,Full test..."  << endl;
    cout << "IsNarrowAngle?                 " << lab1.NarrowAngle()           << endl;
    cout << "Bias Strip Mean              = " << lab1.BiasStripMean()         << endl;
    cout << "Compression Ratio            = " << lab1.CompressionRatio().toStdString()      << endl;
    cout << "Compression Type             = " << lab1.CompressionType().toStdString()       << endl;
    cout << "Data Conversion Type         = " << lab1.DataConversionType().toStdString()    << endl;
    cout << "Delayed Readout Flag         = " << lab1.DelayedReadoutFlag().toStdString()    << endl;
    cout << "Exposure Duration            = " << lab1.ExposureDuration()      << endl;
    cout << "Filter 1 Name                = " << lab1.FilterName()[0].toStdString()         << endl;
    cout << "Filter 2 Name                = " << lab1.FilterName()[1].toStdString()         << endl;
    cout << "Filter 1 Index               = " << lab1.FilterIndex()[0]        << endl;
    cout << "Filter 2 Index               = " << lab1.FilterIndex()[1]        << endl;
    cout << "Flight Software Version(FSW) = " << lab1.FlightSoftwareVersion().toStdString() << endl;
    cout << "Front Optics Temp            = " << lab1.FrontOpticsTemp()       << endl;
    cout << "Gain Mode ID                 = " << lab1.GainModeId()            << endl;
    cout << "Gain State                   = " << lab1.GainState()             << endl;
    cout << "Image Number                 = " << lab1.ImageNumber()           << endl;
    cout << "Instrument Data Rate         = " << lab1.InstrumentDataRate()    << endl;
    cout << "Instrument ID                = " << lab1.InstrumentId().toStdString()          << endl;
    cout << "Readout Cycle Index          = " << lab1.ReadoutCycleIndex().toStdString()     << endl;
    cout << "Shutter Mode ID              = " << lab1.ShutterModeId().toStdString()         << endl;
    cout << "Shutter State ID             = " << lab1.ShutterStateId().toStdString()        << endl;
    cout << "Summing Mode                 = " << lab1.SummingMode()           << endl;


    cout << endl << "ISSNA,Lossy,Table,CAS-ISS2,NacOnly,Sum2 test..."         << endl;
    cout << "IsNarrowAngle?                 " << lab2.NarrowAngle()           << endl;
    cout << "Bias Strip Mean              = " << lab2.BiasStripMean()         << endl;
    cout << "Compression Ratio            = " << lab2.CompressionRatio().toStdString()      << endl;
    cout << "Compression Type             = " << lab2.CompressionType().toStdString()       << endl;
    cout << "Data Conversion Type         = " << lab2.DataConversionType().toStdString()    << endl;
    cout << "Delayed Readout Flag         = " << lab2.DelayedReadoutFlag().toStdString()    << endl;
    cout << "Exposure Duration            = " << lab2.ExposureDuration()      << endl;
    cout << "Filter 1 Name                = " << lab2.FilterName()[0].toStdString()         << endl;
    cout << "Filter 2 Name                = " << lab2.FilterName()[1].toStdString()         << endl;
    cout << "Filter 1 Index               = " << lab2.FilterIndex()[0]        << endl;
    cout << "Filter 2 Index               = " << lab2.FilterIndex()[1]        << endl;
    cout << "Flight Software Version(FSW) = " << lab2.FlightSoftwareVersion().toStdString() << endl;
    cout << "Front Optics Temp            = " << lab2.FrontOpticsTemp()       << endl;
    cout << "Gain Mode ID                 = " << lab2.GainModeId()            << endl;
    cout << "Gain State                   = " << lab2.GainState()             << endl;
    cout << "Image Number                 = " << lab2.ImageNumber()           << endl;
    cout << "Instrument Data Rate         = " << lab2.InstrumentDataRate()    << endl;
    cout << "Instrument ID                = " << lab2.InstrumentId().toStdString()          << endl;
    cout << "Readout Cycle Index          = " << lab2.ReadoutCycleIndex().toStdString()     << endl;
    cout << "Shutter Mode ID              = " << lab2.ShutterModeId().toStdString()         << endl;
    cout << "Shutter State ID             = " << lab1.ShutterStateId().toStdString()        << endl;
    cout << "Summing Mode                 = " << lab2.SummingMode()           << endl;


    cout << endl << "ISSNA,Lossless,Table,CAS-ISS4,BothSim,Sum4 test..."      << endl;
    cout << "IsNarrowAngle?                 " << lab3.NarrowAngle()           << endl;
    cout << "Bias Strip Mean              = " << lab3.BiasStripMean()         << endl;
    cout << "Compression Ratio            = " << lab3.CompressionRatio().toStdString()      << endl;
    cout << "Compression Type             = " << lab3.CompressionType().toStdString()       << endl;
    cout << "Data Conversion Type         = " << lab3.DataConversionType().toStdString()    << endl;
    cout << "Delayed Readout Flag         = " << lab3.DelayedReadoutFlag().toStdString()    << endl;
    cout << "Exposure Duration            = " << lab3.ExposureDuration()      << endl;
    cout << "Filter 1 Name                = " << lab3.FilterName()[0].toStdString()         << endl;
    cout << "Filter 2 Name                = " << lab3.FilterName()[1].toStdString()         << endl;
    cout << "Filter 1 Index               = " << lab3.FilterIndex()[0]        << endl;
    cout << "Filter 2 Index               = " << lab3.FilterIndex()[1]        << endl;
    cout << "Flight Software Version(FSW) = " << lab3.FlightSoftwareVersion().toStdString() << endl;
    cout << "Front Optics Temp            = " << lab3.FrontOpticsTemp()       << endl;
    cout << "Gain Mode ID                 = " << lab3.GainModeId()            << endl;
    cout << "Gain State                   = " << lab3.GainState()             << endl;
    cout << "Image Number                 = " << lab3.ImageNumber()           << endl;
    cout << "Instrument Data Rate         = " << lab3.InstrumentDataRate()    << endl;
    cout << "Instrument ID                = " << lab3.InstrumentId().toStdString()          << endl;
    cout << "Readout Cycle Index          = " << lab3.ReadoutCycleIndex().toStdString()     << endl;
    cout << "Shutter Mode ID              = " << lab3.ShutterModeId().toStdString()         << endl;
    cout << "Shutter State ID             = " << lab1.ShutterStateId().toStdString()        << endl;
    cout << "Summing Mode                 = " << lab3.SummingMode()           << endl;

    // 8LSB
    cout << endl << "ISSNA,Lossless,8LSB,CAS-ISS,NacOnly,Full test..."        << endl;
    cout << "IsNarrowAngle?                 " << lab4.NarrowAngle()           << endl;
    cout << "Bias Strip Mean              = " << lab4.BiasStripMean()         << endl;
    cout << "Compression Ratio            = " << lab4.CompressionRatio().toStdString()      << endl;
    cout << "Compression Type             = " << lab4.CompressionType().toStdString()       << endl;
    cout << "Data Conversion Type         = " << lab4.DataConversionType().toStdString()    << endl;
    cout << "Delayed Readout Flag         = " << lab4.DelayedReadoutFlag().toStdString()    << endl;
    cout << "Exposure Duration            = " << lab4.ExposureDuration()      << endl;
    cout << "Filter 1 Name                = " << lab4.FilterName()[0].toStdString()         << endl;
    cout << "Filter 2 Name                = " << lab4.FilterName()[1].toStdString()         << endl;
    cout << "Filter 1 Index               = " << lab4.FilterIndex()[0]        << endl;
    cout << "Filter 2 Index               = " << lab4.FilterIndex()[1]        << endl;
    cout << "Flight Software Version(FSW) = " << lab4.FlightSoftwareVersion().toStdString() << endl;
    cout << "Front Optics Temp            = " << lab4.FrontOpticsTemp()       << endl;
    cout << "Gain Mode ID                 = " << lab4.GainModeId()            << endl;
    cout << "Gain State                   = " << lab4.GainState()             << endl;
    cout << "Image Number                 = " << lab4.ImageNumber()           << endl;
    cout << "Instrument Data Rate         = " << lab4.InstrumentDataRate()    << endl;
    cout << "Instrument ID                = " << lab4.InstrumentId().toStdString()          << endl;
    cout << "Readout Cycle Index          = " << lab4.ReadoutCycleIndex().toStdString()     << endl;
    cout << "Shutter Mode ID              = " << lab4.ShutterModeId().toStdString()         << endl;
    cout << "Shutter State ID             = " << lab1.ShutterStateId().toStdString()        << endl;
    cout << "Summing Mode                 = " << lab4.SummingMode()           << endl;
    cout  << endl;
    return 0;
  }
  catch(IException &e) {
    e.print();
  }
}
