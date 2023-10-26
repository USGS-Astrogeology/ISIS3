/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "AutoRegFactory.h"
#include "AutoReg.h"
#include "Plugin.h"
#include "IException.h"
#include "FileName.h"

namespace Isis {
  /**
   * Create an AutoReg object using a PVL specification.
   * An example of the PVL required for this is:
   *
   * @code
   * Object = AutoRegistration
   *   Group = Algorithm
   *     Name      = MaximumCorrelation
   *     Tolerance = 0.7
   *   EndGroup
   *
   *   Group = PatternChip
   *     Samples = 21
   *     Lines   = 21
   *   EndGroup
   *
   *   Group = SearchChip
   *     Samples = 51
   *     Lines = 51
   *   EndGroup
   * EndObject
   * @endcode
   *
   * There are many other options that can be set via the pvl and are
   * described in other documentation (see below).
   *
   * @param pvl The pvl object containing the specification
   *
   * @see automaticRegistration.doc
   **/
  AutoReg *AutoRegFactory::Create(Pvl &pvl) {
    // Get the algorithm name to create
    PvlGroup &algo = pvl.findGroup("Algorithm", Pvl::Traverse);
    QString algorithm = QString::fromStdString(algo["Name"]);

    // Open the factory plugin file
    Plugin p;
    FileName f("AutoReg.plugin");
    if(f.fileExists()) {
      p.read("AutoReg.plugin");
    }
    else {
      p.read("$ISISROOT/lib/AutoReg.plugin");
    }

    // Get the algorithm specific plugin and return it
    AutoReg * (*plugin)(Pvl & pvl);
    plugin = (AutoReg * ( *)(Pvl & pvl)) p.GetPlugin(algorithm);
    return (*plugin)(pvl);
  }
} // end namespace isis
