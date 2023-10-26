/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "AtmosModelFactory.h"
#include "AtmosModel.h"
#include "Plugin.h"
#include "IException.h"
#include "FileName.h"

namespace Isis {
  /**
   * Create an AtmosModel object using a PVL specification.
   * An example of the PVL required for this is:
   *
   * @code
   * Object = AtmosphericModel
   *   Group = Algorithm
   *     # Use 'AtmName' instead of 'Name' if using the Gui combo box
   *     # for unique Pvl keyword in DefFile
   *     AtmName/Name = Isotropic1
   *     Tau = 0.7
   *     Tauref = 0.0
   *     Wha = 0.5
   *     Hnorm = 0.003
   *     Nulneg = NO
   *   EndGroup
   * EndObject
   * @endcode
   *
   * There are many other options that can be set via the pvl and are
   * described in other documentation (see below).
   *
   * @param pvl The pvl object containing the specification
   * @param pmodel The PhotoModel objects contining the data
   *
   * @return A pointer to the new AtmosModel
   *
   * @see atmosphericModels.doc
   **/
  AtmosModel *AtmosModelFactory::Create(Pvl &pvl, PhotoModel &pmodel) {

    // Get the algorithm name to create
    PvlGroup &algo = pvl.findObject("AtmosphericModel")
                     .findGroup("Algorithm", Pvl::Traverse);

    QString algorithm = "";
    if(algo.hasKeyword("AtmName")) {
      algorithm = QString::fromStdString(algo["AtmName"]);
    }
    else if(algo.hasKeyword("Name")) {
      algorithm = QString::fromStdString(algo["Name"]);
    }
    else {
      std::string msg = "Keyword [Name] or keyword [AtmName] must ";
      msg += "exist in [Group = Algorithm]";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    // Open the factory plugin file
    Plugin *p = new Plugin;
    FileName f("AtmosModel.plugin");
    if(f.fileExists()) {
      p->read("AtmosModel.plugin");
    }
    else {
      p->read("$ISISROOT/lib/AtmosModel.plugin");
    }

    // Get the algorithm specific plugin and return it
    AtmosModel * (*plugin)(Pvl & pvl, PhotoModel & pmodel);
    plugin = (AtmosModel * ( *)(Pvl & pvl, PhotoModel & pmodel))
             p->GetPlugin(algorithm);
    return (*plugin)(pvl, pmodel);
  }
} // end namespace isis
