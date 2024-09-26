/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "kerneldbgen.h"

#include "SpiceDbGen.h"
#include "FileList.h"
#include "iTime.h"

using namespace std;

namespace Isis {
  FileName safeHighestVersion(QString fileNameString);

  QList<FileName> evaluateDependencies(PvlGroup &dependencyGroup, QString kernelTypeName,
                                       QString parameterName, UserInterface &ui);

  void kerneldbgen(UserInterface &ui) {

    PvlGroup dependency("Dependencies");

    // Create the database writer based on the kernel type
    SpiceDbGen sdg(ui.GetString("TYPE"));

    // Load the SCLK. If it exists, add its location to the dependency group
    // If there is none, set a flag so that no file is searched for
    QList<FileName> sclkFiles = evaluateDependencies(dependency, "SpacecraftClockKernel", "SCLK", ui);
    QList<FileName> lskFiles = evaluateDependencies(dependency, "LeapsecondKernel", "LSK", ui);
    QList<FileName> extraFiles = evaluateDependencies(dependency, "ExtraKernel", "EXTRA", ui);

    sdg.FurnishDependencies(sclkFiles, lskFiles, extraFiles);

    // Determine the type of kernel that the user wants a database for. This will
    // eventually become the name of the object in the output PVL
    QString kernelType;
    if (ui.GetString("TYPE") == "CK") {
      kernelType = "SpacecraftPointing";
    }
    else if (ui.GetString("TYPE") == "SPK") {
      kernelType = "SpacecraftPosition";
    }
    PvlObject selections(kernelType.toStdString());

    // Specify whether to use SPICE segments (made up of SPICE intervals)
    // or SPICE intervals for the SPICE database. Naif referes to this as "coverage level"
    QString coverageLevel;
    if (ui.GetString("LEVEL") == "SEGMENT") {
      coverageLevel = "SEGMENT";
    }
    else if (ui.GetString("LEVEL") == "INTERVAL") {
      coverageLevel = "INTERVAL";
    }
    sdg.setCoverageLevel(coverageLevel);

    selections += PvlKeyword("RunTime", iTime::CurrentLocalTime().toStdString());
    selections.addGroup(dependency);

    /* Removed because Nadir is not done using this*/
    //if (ui.GetString("NADIRFILTER") != "none" &&
    //    ui.GetString("NADIRDIR") != "none"){
    //  QString location = "";
    //  location = ui.GetString("NADIRDIR");
    //  location.Trim("\\");
    //  std::vector<QString> filter;
    //  ui.GetString("NADIRFILTER", filter);
    //  PvlObject result = sdg.Direct("Nadir",location, filter);
    //  PvlObject::PvlGroupIterator grp = result.beginGroup();
    //  while(grp != result.endGroup()){ selections.addGroup(*grp);grp++;}
    //}
    double startOffset = ui.GetDouble("STARTOFFSET");
    double endOffset = ui.GetDouble("ENDOFFSET");

    if (ui.GetString("PREDICTFILTER") != "none" &&
        ui.GetString("PREDICTDIR") != "none") {
      QString location = "";
      location = ui.GetString("PREDICTDIR");
      location.remove("\\");
      std::vector<QString> filter;
      ui.GetString("PREDICTFILTER", filter);
      PvlObject result = sdg.Direct("Predicted", location, filter, startOffset, endOffset);
      PvlObject::PvlGroupIterator grp = result.beginGroup();
      while(grp != result.endGroup()) {
        selections.addGroup(*grp);
        grp++;
      }
    }
    else if (ui.WasEntered("PREDICTLIST")) {
      FileList kernList(ui.GetFileName("PREDICTLIST").toStdString());
      PvlObject result = sdg.Direct("Predicted", kernList, startOffset, endOffset);
      PvlObject::PvlGroupIterator grp = result.beginGroup();
      while(grp != result.endGroup()) {
        selections.addGroup(*grp);
        grp++;
      }
    }

    if (ui.GetString("RECONDIR") != "none" &&
        ui.GetString("RECONFILTER") != "none") {
      QString location = "";
      location = ui.GetString("RECONDIR");
      location.remove("\\");
      std::vector<QString> filter;
      ui.GetString("RECONFILTER", filter);
      PvlObject result = sdg.Direct("Reconstructed", location, filter, startOffset, endOffset);
      PvlObject::PvlGroupIterator grp = result.beginGroup();
      while(grp != result.endGroup()) {
        selections.addGroup(*grp);
        grp++;
      }
    }
    else if (ui.WasEntered("RECONLIST")) {
      FileList kernList(ui.GetFileName("RECONLIST").toStdString());
      PvlObject result = sdg.Direct("Reconstructed", kernList, startOffset, endOffset);
      PvlObject::PvlGroupIterator grp = result.beginGroup();
      while(grp != result.endGroup()) {
        selections.addGroup(*grp);
        grp++;
      }
    }

    if (ui.GetString("SMITHEDDIR") != "none" &&
        ui.GetString("SMITHEDFILTER") != "none") {
      QString location = "";
      location = ui.GetString("SMITHEDDIR");
      location.remove("\\");
      std::vector<QString> filter;
      ui.GetString("SMITHEDFILTER", filter);
      PvlObject result = sdg.Direct("Smithed", location, filter, startOffset, endOffset);
      PvlObject::PvlGroupIterator grp = result.beginGroup();
      while(grp != result.endGroup()) {
        selections.addGroup(*grp);
        grp++;
      }
    }
    else if (ui.WasEntered("SMITHEDLIST")) {
      FileList kernList(ui.GetFileName("SMITHEDLIST").toStdString());
      PvlObject result = sdg.Direct("Smithed", kernList, startOffset, endOffset);
      PvlObject::PvlGroupIterator grp = result.beginGroup();
      while(grp != result.endGroup()) {
        selections.addGroup(*grp);
        grp++;
      }
    }

    //if (filter == ""){
    if (!(ui.WasEntered("PREDICTFILTER") || ui.WasEntered("PREDICTLIST")) &&
        !(ui.WasEntered("RECONFILTER")   || ui.WasEntered("RECONLIST")) &&
        !(ui.WasEntered("SMITHEDFILTER") || ui.WasEntered("SMITHEDLIST"))) {
      std::string message =
        "No kernel selection arguments were entered. A directory and filter or a "
        "list must be entered for at least one quality of kernel.";
      throw IException(IException::User, message, _FILEINFO_);
    }

    //specify a name for the output file
    FileName to("./kernels.????.db");
    if (ui.WasEntered("TO")) {
      to = ui.GetFileName("TO").toStdString();
    }
    //create a new output version if the user specified any version sequence
    if (to.isVersioned()) {
      to = to.newVersion();
    }

    Pvl writer;
    writer.addObject(selections);
    writer.write(to.expanded());
  }


  /**
   * This method converts the given string to a FileName. If the file is versioned, the result will
   *   be the highest version of the file.
   */
  FileName safeHighestVersion(QString fileNameString) {
    FileName result(fileNameString.toStdString());

    if (result.isVersioned()) {
      result = result.highestVersion();
    }

    return result;
  }


  /**
   * This method gets a list of kernels from the user and populates the dependency group with a list
   *   of the found kernel file names. If a kernel file has a "db" extension, this method will search
   *   the DB for kernel files. This only supports very bare-bones, basic kernel db files because
   *   no matching can be done/there isn't a cube label.
   *
   * @param dependencyGroup This is modified with new keywords (named kernelTypeName) with values that
   *                        match the kernel file names (without variables expanded).
   * @param kernelTypeName This is used to name new keywords in dependencyGroup
   * @param parameterName This is the user interface parameter to get the kernels from.
   *
   * @return A list of kernel files to be furnished
   */
  QList<FileName> evaluateDependencies(PvlGroup &dependencyGroup, QString kernelTypeName,
                                       QString parameterName , UserInterface &ui) {
    QList<FileName> results;

    if (ui.WasEntered(parameterName)) {
      vector<QString> kernelStrings;
      ui.GetAsString(parameterName, kernelStrings);

      foreach (QString kernelString, kernelStrings) {
        FileName kernelFileName = safeHighestVersion(kernelString);

        // Try to get kernel file names out of db, if a db was specified instead of an actual kernel
        if (kernelFileName.extension() == "db") {
          Pvl kernelDbPvl(kernelFileName.expanded());

          if (kernelDbPvl.objects() == 1) {
            PvlObject &primaryObject = kernelDbPvl.object(0);

            if (primaryObject.groups() == 1) {
              PvlGroup &primaryGroup = primaryObject.group(0);

              if (QString::fromStdString(primaryGroup.name()).toLower() == "selection") {

                if (primaryGroup.keywords() == 1) {
                  PvlKeyword &key = primaryGroup[0];

                  if (key.isNamed("File")) {
                    if (key.size() == 2) {
                      kernelFileName = safeHighestVersion(QString("$%1/%2").arg(QString::fromStdString(key[0])).arg(QString::fromStdString(key[1])));
                    }
                    else {
                      throw IException(IException::Unknown,
                                       "Expected the keyword File in ["+ kernelFileName.original() +"] to have two "
                                                   "values, a mission data directory and a path into "
                                                   "that directory. The keyword has ["+ toString(key.size()) +"] values.",
                                       _FILEINFO_);
                    }
                  }
                  else {
                    throw IException(IException::Unknown,
                                     "Expected Pvl Group [" + primaryGroup.name() + "] in the first Pvl Object ["
                                                 + primaryObject.name() + "] in the DB file [" + kernelFileName.original() + "] to have a single keyword "
                                                 "named File, but the keyword was named [" + key.name() + "] instead",
                                     _FILEINFO_);
                  }
                }
                else {
                  throw IException(IException::Unknown, "Expected Pvl Group [" + primaryGroup.name() + "] in the first Pvl Object [" + primaryObject.name() + "] "
                                               "in the DB file [" + kernelFileName.original() + "] to have a single keyword (named "
                                               "File), but found [" + toString(primaryGroup.keywords()) + "] keywords",
                                   _FILEINFO_);
                }
              }
              else {
                throw IException(IException::Unknown, "Expected Pvl Group in the first Pvl Object [" + primaryObject.name() + "] in "
                                             "the DB file [" + kernelFileName.original() + "] to be named Selection but found [" + primaryGroup.name() + "]",
                                 _FILEINFO_);
              }
            }
            else {
              throw IException(IException::Unknown,"Expected one Pvl Group in the first Pvl Object [" + primaryObject.name() + "] in "
                                           "the DB file [" + kernelFileName.original() + "] but found [" + toString(primaryObject.groups()) + "]", _FILEINFO_);
            }
          }
          else {
            throw IException(IException::Unknown,
                             "Expected one Pvl Object in the DB file ["+kernelFileName.original()+"] but "
                                         "found ["+ toString(kernelDbPvl.objects()) +"]",
                             _FILEINFO_);
          }
        }

        results.append(kernelFileName);
        dependencyGroup += PvlKeyword(kernelTypeName.toStdString(), (kernelFileName.original()));
      }
    }

    return results;
  }
}
