/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "NormModelFactory.h"
#include "NormModel.h"
#include "Plugin.h"
#include "IException.h"
#include "FileName.h"

namespace Isis {
  /**
   * Create a NormModel object using a PVL specification.
   * An example of the PVL required for this is:
   *
   * @code
   * Object = NormalizationModel
   *   Group = Algorithm
   *     NormName/Name = ShadeAtm
   *     PhotoModel = Minnaert
   *     AtmosModel = Isotropic1
   *   EndGroup
   * EndObject
   * @endcode
   *
   * There are many other options that can be set via the pvl and are
   * described in other documentation (see below).
   *
   * @param pvl The pvl object containing the specification
   *
   * @see normalizationModels.doc
   **/
  NormModel *NormModelFactory::Create(Pvl &pvl, PhotoModel &pmodel) {
    // Get the algorithm name to create
    PvlGroup &algo = pvl.findObject("NormalizationModel")
                     .findGroup("Algorithm", Pvl::Traverse);
    QString algorithm = "";
    if (algo.hasKeyword("NormName")) {
      algorithm = QString::fromStdString(algo["NormName"]);
    }
    else if (algo.hasKeyword("Name")) {
      algorithm = QString::fromStdString(algo["Name"]);
    }
    else {
      std::string msg = "Keyword [Name] or keyword [NormName] must ";
      msg += "exist in [Group = Algorithm]";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    // Open the factory plugin file
    Plugin *p = new Plugin;
    FileName f("NormModel.plugin");
    if(f.fileExists()) {
      p->read("NormModel.plugin");
    }
    else {
      p->read("$ISISROOT/lib/NormModel.plugin");
    }

    // Get the algorithm specific plugin and return it
    NormModel * (*plugin)(Pvl & pvl, PhotoModel & pmodel);
    plugin = (NormModel * ( *)(Pvl & pvl, PhotoModel & pmodel))
             p->GetPlugin(algorithm);
    return (*plugin)(pvl, pmodel);
  }

  /**
   * Create a NormModel object using a PVL specification.
   * An example of the PVL required for this is:
   *
   * @code
   * Object = NormalizationModel
   *   Group = Algorithm
   *     NormName/Name = ShadeAtm
   *     PhotoModel = Minnaert
   *     AtmosModel = Isotropic1
   *   EndGroup
   * EndObject
   * @endcode
   *
   * There are many other options that can be set via the pvl and are
   * described in other documentation (see below).
   *
   * @param pvl The pvl object containing the specification
   *
   * @see normalizationModels.doc
   **/
  NormModel *NormModelFactory::Create(Pvl &pvl, PhotoModel &pmodel,
                                      AtmosModel &amodel) {
    // Get the algorithm name to create
    PvlGroup &algo = pvl.findObject("NormalizationModel")
                     .findGroup("Algorithm", Pvl::Traverse);
    QString algorithm = "";
    if (algo.hasKeyword("NormName")) {
      algorithm = QString::fromStdString(algo["NormName"]);
    }
    else if (algo.hasKeyword("Name")) {
      algorithm = QString::fromStdString(algo["Name"]);
    }
    else {
      IString msg = "Keyword [Name] or keyword [NormName] must ";
      msg += "exist in [Group = Algorithm]";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    // Open the factory plugin file
    Plugin *p = new Plugin;
    FileName f("NormModel.plugin");
    if(f.fileExists()) {
      p->read("NormModel.plugin");
    }
    else {
      p->read("$ISISROOT/lib/NormModel.plugin");
    }

    // Get the algorithm specific plugin and return it
    NormModel * (*plugin)(Pvl & pvl, PhotoModel & pmodel,
                          AtmosModel & amodel);
    plugin = (NormModel * ( *)(Pvl & pvl, PhotoModel & pmodel,
                               AtmosModel & amodel))
             p->GetPlugin(algorithm);
    return (*plugin)(pvl, pmodel, amodel);
  }
} // end namespace isis
