/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "InterestOperatorFactory.h"
#include "InterestOperator.h"
#include "Plugin.h"
#include "IException.h"
#include "FileName.h"

namespace Isis {
  /**
   * Create an InterestOperator object using a PVL specification.
   * An example of the PVL required for this is:
   *
   * @code
   * Object = InterestOperator
   *   Group = Operator
   *     Name      = StandardDeviation
   *     Samples   = 21
   *     Lines     = 21
   *     Delta     = 50
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
  InterestOperator *InterestOperatorFactory::Create(Pvl &pPvl) {
    // Get the algorithm name to create
    PvlGroup &op = pPvl.findGroup("Operator", Pvl::Traverse);
    QString operatorName = QString::fromStdString(op["Name"]);

    // Open the factory plugin file
    Plugin p;
    FileName f("InterestOperator.plugin");
    if(f.fileExists()) {
      p.read("InterestOperator.plugin");
    }
    else {
      p.read("$ISISROOT/lib/InterestOperator.plugin");
    }

    // Get the algorithm specific plugin and return it
    InterestOperator * (*plugin)(Pvl & pPvl);
    plugin = (InterestOperator * ( *)(Pvl & pPvl)) p.GetPlugin(operatorName);
    return (*plugin)(pPvl);
  }
} // end namespace isis
