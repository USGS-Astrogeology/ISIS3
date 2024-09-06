/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "PhotoModelFactory.h"
#include "PhotoModel.h"
#include "Plugin.h"
#include "IException.h"
#include "FileName.h"

namespace Isis {
  /**
   * Create a PhotoModel object using a PVL specification.
   * An example of the PVL required for this is:
   *
   * @code
   * Object = PhotometricModel
   *   Group = Algorithm
   *     PhtName/Name = Minnaert
   *     K = 0.7
   *   EndGroup
   * EndObject
   * @endcode
   *
   * There are many other options that can be set via the pvl and are
   * described in other documentation (see below).
   *
   * @param pvl The pvl object containing the specification
   *
   * @see photometricModels.doc
   **/
  PhotoModel *PhotoModelFactory::Create(Pvl &pvl) {
    // Get the algorithm name to create
    PvlGroup &algo = pvl.findObject("PhotometricModel")
                     .findGroup("Algorithm", Pvl::Traverse);

    QString algorithm = "";
    if (algo.hasKeyword("PhtName")) {
      algorithm = QString::fromStdString(algo["PhtName"][0]);
    }
    else if (algo.hasKeyword("Name")) {
      algorithm = QString::fromStdString(algo["Name"][0]);
    }
    else {
      std::string msg = "Keyword [Name] or keyword [PhtName] must ";
      msg += "exist in [Group = Algorithm]";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    // Open the factory plugin file
    Plugin *p = new Plugin;
    FileName f("PhotoModel.plugin");
    if(f.fileExists()) {
      p->read("PhotoModel.plugin");
    }
    else {
      p->read("$ISISROOT/lib/PhotoModel.plugin");
    }

    // Get the algorithm specific plugin and return it
    PhotoModel * (*plugin)(Pvl & pvl);
    plugin = (PhotoModel * ( *)(Pvl & pvl)) p->GetPlugin(algorithm);
    return (*plugin)(pvl);
  }
} // end namespace isis
