/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "PolygonSeederFactory.h"
#include "Plugin.h"
#include "IException.h"
#include "FileName.h"

namespace Isis {
  /**
   * Create a PolygonSeeder object using a PVL specification.
   * An example of the PVL required for this is:
   *
   * @code
   * Object = AutoSeed
   *   Group = PolygonSeederAlgorithm
   *     Name      = Grid
   *     Tolerance = 0.7
   *   EndGroup
   *
   *   Group = InterestOperatorAlgorithm
   *     Name      = StandardDeviation
   *     Tolerance = 10
   *   EndGroup
   *
   * EndObject
   * @endcode
   *
   * @param pvl The pvl object containing the PolygonSeeder specification
   *
   **/
  PolygonSeeder *PolygonSeederFactory::Create(Pvl &pvl) {
    // Get the algorithm name to create
    PvlGroup &algo = pvl.findGroup("PolygonSeederAlgorithm", Pvl::Traverse);
    QString algorithm = QString::fromStdString(algo["Name"]);

    // Open the factory plugin file
    Plugin p;
    FileName f("PolygonSeeder.plugin");
    if(f.fileExists()) {
      p.read("PolygonSeeder.plugin");
    }
    else {
      p.read("$ISISROOT/lib/PolygonSeeder.plugin");
    }

    // Get the algorithm specific plugin and return it
    PolygonSeeder* (*plugin)(Pvl & pvl);
    plugin = (PolygonSeeder * ( *)(Pvl & pvl)) p.GetPlugin(algorithm);
    return (*plugin)(pvl);
  }
} // end namespace isis
