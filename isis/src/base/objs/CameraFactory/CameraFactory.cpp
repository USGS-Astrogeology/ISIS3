/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <csm/Plugin.h>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QLibrary>

#include "CameraFactory.h"

#include "Camera.h"
#include "CSMCamera.h"
#include "FileName.h"
#include "IException.h"
#include "Plugin.h"
#include "Preference.h"

#include "csm/csm.h"
#include "csm/Model.h"
#include "csm/Plugin.h"

using namespace csm;
using namespace std;

namespace Isis {
  Plugin CameraFactory::m_cameraPlugin;
  bool CameraFactory::m_initialized = false;

  /**
   * Creates a Camera object using Pvl Specifications
   *
   * @param cube The original cube with the current version camera model
   *
   * @return Camera* The Camera object created
   *
   * @throws Isis::iException::System - Unsupported camera model, unable to find
   *                                    the plugin
   * @throws Isis::iException::Camera - Unable to initialize camera model
   */
  Camera *CameraFactory::Create(Cube &cube) {
    // Try to load a plugin file in the current working directory and then
    // load the system file

    initPlugin();

    try {
      // Is there a CSM blob on the cube?
      if (cube.hasBlob("CSMState", "String")) {
        // Create ISIS CSM Camera Model
        try {
          return new CSMCamera(cube);
        }
        catch (IException &e) {
          QString msg = "Unable to create CSM camera using CSMState Cube blob.";
          throw IException(e, IException::Unknown, msg, _FILEINFO_);
        }
      }
      else {
        // First get the spacecraft and instrument and combine them
        Pvl &lab = *cube.label();
        PvlGroup &inst = lab.findGroup("Instrument", Isis::Pvl::Traverse);
        QString spacecraft = QString::fromStdString(inst["SpacecraftName"]);
        QString name = QString::fromStdString(inst["InstrumentId"]);
        spacecraft = spacecraft.toUpper();
        name = name.toUpper();
        QString group = spacecraft + "/" + name;
        group = group.remove(" ");

        PvlGroup &kerns = lab.findGroup("Kernels", Isis::Pvl::Traverse);
        // Default version 1 for backwards compatibility (spiceinit'd cubes before camera model versioning)
        if (!kerns.hasKeyword("CameraVersion")) {
          kerns.addKeyword(PvlKeyword("CameraVersion", "1"));
        }

        int cameraOriginalVersion = (int)kerns["CameraVersion"];
        int cameraNewestVersion = CameraVersion(cube);

        if (cameraOriginalVersion != cameraNewestVersion) {
          string msg = "The camera model used to create a camera for this cube is out of date, " \
                       "please re-run spiceinit on the file or process with an old Isis version " \
                       "that has the correct camera model.";
          throw IException(IException::Unknown, msg, _FILEINFO_);
        }

        // See if we have a camera model plugin
        QFunctionPointer ptr;
        try {
          ptr = m_cameraPlugin.GetPlugin(group);
        }
        catch(IException &e) {
          QString msg = "Unsupported camera model, unable to find plugin for ";
          msg += "SpacecraftName [" + spacecraft + "] with InstrumentId [";
          msg += name + "]";
          throw IException(e, IException::Unknown, msg, _FILEINFO_);
        }

        // Now cast that pointer in the proper way
        Camera * (*plugin)(Isis::Cube &cube);
        plugin = (Camera * ( *)(Isis::Cube &cube)) ptr;

        // Create the camera as requested
        return (*plugin)(cube);
      }
    }
    catch(IException &e) {
      string message = "Unable to initialize camera model in Camera Factory.";
      throw IException(e, IException::Unknown, message, _FILEINFO_);
    }
  }


  /**
   * Reads the appropriate plugin file for the ISIS cameras, and scans the
   * directories specified in IsisPreferences for CSM cameras.
   */
  void CameraFactory::initPlugin() {
    if (!m_initialized) {
      // Handle the ISIS camera plugins
      if (m_cameraPlugin.fileName() == "") {
        FileName localFile("Camera.plugin");
        if (localFile.fileExists())
          m_cameraPlugin.read(localFile.expanded().toStdString());

        FileName systemFile("$ISISROOT/lib/Camera.plugin");
        if (systemFile.fileExists())
          m_cameraPlugin.read(systemFile.expanded().toStdString());
      }

      // Find the CSM plugins by searching the directories identified in the Preferences.
      // Load the found libraries. This causes the static instance(s) to be constructed,
      // and thus registering the model with the csm Plugin class.
      Preference &p = Preference::Preferences();
      PvlGroup &grp = p.findGroup("Plugins", Isis::Pvl::Traverse);
      for (int i = 0; i<grp["CSMDirectory"].size(); i++) {
        FileName csmDir = QString::fromStdString(grp["CSMDirectory"][i]);

        QDirIterator csmLib(csmDir.expanded(), {"*.so", "*.dylib"}, QDir::Files);
        while (csmLib.hasNext()) {
          QString csmLibName = csmLib.next();
          QLibrary csmDynamicLib(csmLibName);
          csmDynamicLib.load();
        }
      }
    }
    m_initialized = true;
  }


  /**
   * This looks up the current camera model version from the cube.
   *
   * @param cube Input cube
   *
   * @return int Latest Camera Version
   */
  int CameraFactory::CameraVersion(Cube &cube) {
    return CameraVersion(*cube.label());
  }


  /**
   * Looks up the current camera model version in the pvl labels.
   *
   * @param lab The pvl labels
   *
   * @returns The current camera model version
   */
  int CameraFactory::CameraVersion(Pvl &lab) {
    // Try to load a plugin file in the current working directory and then
    // load the system file
    initPlugin();

    try {
      // First get the spacecraft and instrument and combine them
      PvlGroup &inst = lab.findGroup("Instrument", Isis::Pvl::Traverse);
      QString spacecraft = QString::fromStdString(inst["SpacecraftName"]);
      QString name = QString::fromStdString(inst["InstrumentId"]);
      spacecraft = spacecraft.toUpper();
      name = name.toUpper();
      QString group = spacecraft + "/" + name;
      group = group.remove(" ");

      PvlGroup plugin;
      try {
        bool found = false;
        // Find the most recent (last) version of the camera model
        for (int i = m_cameraPlugin.groups() - 1; i >= 0; i--) {
          if (m_cameraPlugin.group(i) == group.toStdString()) {
            plugin = m_cameraPlugin.group(i);
            found = true;
            break;
          }
        }
        if (!found) {
          QString msg = "Unable to find PVL group [" + group + "].";
          throw IException(IException::Unknown, msg, _FILEINFO_);
        }
      }
      catch(IException &e) {
        QString msg = "Unsupported camera model, unable to find plugin for ";
        msg += "SpacecraftName [" + spacecraft + "] with InstrumentId [";
        msg += name + "]";
        throw IException(e, IException::Unknown, msg, _FILEINFO_);
      }

      if (!plugin.hasKeyword("Version")) {
        QString msg = "Camera model identified by [" + group + "] does not have a version number";
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }

      return (int)plugin["Version"];
    }
    catch(IException &e) {
      string msg = "Unable to locate latest camera model version number from group [Instrument]";
      throw IException(e, IException::Unknown, msg, _FILEINFO_);
    }
  }
} // end namespace isis
