/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

#include <SpiceUsr.h>

#include "FileName.h"
#include "IException.h"
#include "PvlKeyword.h"
#include "Pvl.h"
#include "SpiceManager.h"
#include "NaifStatus.h"

using namespace std;

namespace Isis {

  /**
   * @brief Construct using an ISIS file name
   *
   * @param filename Name of ISIS cube file
   * @param furnish Do we load the kernels we find?
   */
  SpiceManager::SpiceManager(const QString &filename, bool furnish) {
    Pvl pvl(filename.toStdString());
    Load(pvl, furnish);
  }

  /**
   * @brief Construct using an ISIS Cube object
   *
   * @param cube    Cube object of ISIS file
   * @param furnish Do we load the kernels we find?
   */
  SpiceManager::SpiceManager(Cube &cube, bool furnish) {
    Load(*cube.label(), furnish);
  }

  /**
   * @brief Construct from an ISIS label
   *
   * @param pvl  ISIS label to get kernel information from
   * @param furnish  Do we load the kernels we find?
   */
  SpiceManager::SpiceManager(Pvl &pvl, bool furnish) {
    Load(pvl, furnish);
  }


  /**
   * @brief Perform the hunt for Spice kernels in an ISIS label
   *
   * This method must traverse the ISIS label hierarchy to find all the
   * pertinent kernel files.  Many of them are found in the Kernel Group, but
   * others exist in Table objects that are SPICE blobs.  The actual names are
   * found in the Kernels keyword in the named Table object.
   *
   * @param pvl  ISIS label
   * @param furnish Do we actually load the kernel files as we find them?
   */
  void SpiceManager::Load(Pvl &pvl, bool furnish) {
    Unload();
    _furnish = furnish;

    QString kernlist;
    // Get the kernel group and load main kernels
    PvlGroup kernels = pvl.findGroup("Kernels", Pvl::Traverse);
    //  Changed 2008-02-27 to load planetary ephemeris before spacecraft
    //  since MESSENGER team may update planet data in the s/c SPK.
    loadKernelFromTable(kernels["TargetPosition"], "SunPosition", pvl);

    //  Now do s/c ephemeris
    if(kernels.hasKeyword("SpacecraftPosition")) {
      loadKernel(kernels["SpacecraftPosition"]);
    }
    else {
      loadKernelFromTable(kernels["InstrumentPosition"], "InstrumentPosition",
                          pvl);
    }

    if(kernels.hasKeyword("SpacecraftPointing")) {
      loadKernel(kernels["SpacecraftPointing"]);
    }
    else {
      loadKernelFromTable(kernels["InstrumentPointing"], "InstrumentPointing",
                          pvl);
    }

    if(kernels.hasKeyword("Frame")) {
      loadKernel(kernels["Frame"]);
    }

    if(kernels.hasKeyword("Extra")) {
      loadKernel(kernels["Extra"]);
    }


    loadKernel(kernels["TargetAttitudeShape"]);
    loadKernel(kernels["Instrument"]);
    loadKernel(kernels["InstrumentAddendum"]);  // Always load after instrument
    loadKernel(kernels["LeapSecond"]);
    loadKernel(kernels["SpacecraftClock"]);
    return;
  }

  /**
   * @brief Add a specified kernel file to the pool
   *
   * This method adds a specified kernel file to the NAIF pool.  The caller can
   * provide a file pattern with "?" in the filename as this method will
   * determine the highest occuring version.
   *
   * @param kernel  Name of kernel file (or pattern) to add
   */
  void SpiceManager::add(const QString &kernfile) {

    QString kfile(kernfile);

    //  Check for versioned file naming
    FileName efile(kfile);
    if (efile.isVersioned()) {
      efile = efile.highestVersion();
      kfile = efile.expanded();
    }

    //  Add a specific kernel to the list
    PvlKeyword kernel("Kernels", kfile.toStdString());
    loadKernel(kernel);
    return;
  }

  /**
   * @brief Provide a list of all the kernels found
   *
   * This method will return all the kernel file references as found in the ISIS
   * label.  It will optionally remove the file paths and return only the kernel
   * file name if requested.  If removePath is false, it returns the complete
   * path to the SPICE kernels.
   *
   * @param removePath Do we remove the file paths and return only the file
   *                   names?
   *
   * @return std::vector<QString> A vector of filenames of SPICE kernels
   */
  std::vector<QString> SpiceManager::getList(bool removePath) const {
    std::vector<QString> flist;
    for(unsigned int i = 0 ; i < _kernlist.size() ; i++) {
      if(removePath) {
        FileName kfile(_kernlist[i]);
        flist.push_back(kfile.name());
      }
      else {
        flist.push_back(_kernlist[i]);
      }
    }
    return (flist);
  }

  /**
   * @brief Unloads all kernels if they were loaded when found
   */
  void SpiceManager::Unload() {
    NaifStatus::CheckErrors();
    if(_furnish) {
      for(unsigned int i = 0 ; i < _kernlist.size() ; i++) {
        /**
        *  Changed to work with hipeaks crappy compiler.
        *  string kernName(FileName(_kernlist[i]).expanded());
        *  unload_c(kernName.c_str());
        */
        FileName f(_kernlist[i]);
        QString kernName(f.expanded());
        unload_c(kernName.toLatin1().data());
      }
    }
    _kernlist.clear();
    NaifStatus::CheckErrors();
    return;
  }

  /**
   * @brief Loops through PvlKeyword containing Kernel file names
   *
   * This method interogates a PvlKeyword that is determined to
   * contain SPICE kernel names.  It will optionally load the
   * kernels if initially requested and then adds the name to the
   * internally managed list.
   *
   * Some keywords may contain special keywords.  These are ignored in this
   * method and can be handled explicitly in other methods.
   *
   * @param key PvlKeyword containing SPICE kernels
   * @see  loadKernelFromTable()
   */
  void SpiceManager::loadKernel(PvlKeyword &key) {
    NaifStatus::CheckErrors();
    for(int i = 0; i < key.size(); i++) {
      if(key[i] == "") continue;
      if(IString(key[i]).UpCase() == "NULL") continue;
      if(IString(key[i]).UpCase() == "NADIR") continue;
      if(IString(key[i]).UpCase() == "TABLE") continue;
      Isis::FileName file(QString::fromStdString(key[i]));
      if(!file.fileExists()) {
        std::string msg = "Spice file does not exist [" + file.expanded() + "]";
        throw IException(IException::Io, msg, _FILEINFO_);
      }
      QString fileName(file.expanded());
      if(_furnish) furnsh_c(fileName.toLatin1().data());
      addKernelName(QString::fromStdString(key[i]));
    }
    NaifStatus::CheckErrors();
  }

  /**
   * @brief Interogate a PvlKeyword for location of kernel file names
   *
   * This method is intended to find keywords that refer to SPICE Table Blobs
   * and look in those Table objects for the actual names of SPICE kernel files.
   * They are then loaded via the loadKernel() method.
   *
   * @param key PvlKeyword containing SPICE kernel names
   * @param tblname Name of Table where the SPICE blob is located in the label
   * @param pvl Pvl label to search for the SPICE Table Object Blob
   */
  void SpiceManager::loadKernelFromTable(PvlKeyword &key,
                                         const QString &tblname, Pvl &pvl) {
    if(QString::fromStdString(key[0]).toUpper() != "TABLE") {
      loadKernel(key);
    }
    else {
      PvlObject::PvlObjectIterator objIter;
      for(objIter = pvl.beginObject() ; objIter != pvl.endObject() ; ++objIter) {
        if(QString::fromStdString(objIter->name()).toUpper() == "TABLE") {
          if(objIter->hasKeyword("Name")) {
            if(QString::fromStdString(objIter->findKeyword("Name")[0]).toUpper() == tblname.toUpper()) {
              loadKernel(objIter->findKeyword("Kernels"));
              return;
            }
          }
        }
      }
    }
    return;
  }
  /**
   * @brief Adds the named kernel file to the internal list
   *
   * This method will add the name of the kernel file to the list.  It will
   * first determine if it already exists.  If it does, it will not be added a
   * second time.
   *
   * @param kname Name of SPICE kernel file to add
   */
  void SpiceManager::addKernelName(const QString &kname) {
    for(unsigned int i = 0 ; i < _kernlist.size() ; i++) {
      if(_kernlist[i] == kname)  return;
    }
    _kernlist.push_back(kname);
    return;
  }

}  // namespace Isis
