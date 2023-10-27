/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "KernelDb.h"

#include <iomanip>
#include <queue>

#include "CameraFactory.h"
#include "FileName.h"
#include "IException.h"
#include "IString.h"
#include "iTime.h"
#include "Kernel.h"
#include "Preference.h"
#include "Preference.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "PvlObject.h"

using namespace std;
namespace Isis {
  /**
   * Constructs a new KernelDb object with a given integer value representing
   * the Kernel::Type enumerations that are allowed.  The filename is set
   * to "None" if this constructor is used.
   *
   * The allowed kernel types is stored as the sum of the enumerations of the
   * allowed Kernel Types. The following enumeriations currently exist:
   * 0001 = 1 = Predicted
   * 0010 = 2 = Nadir
   * 0100 = 4 = Reconstructed
   * 1000 = 8 = Smithed
   *
   * So, for example, if allowedKernelTypes is 11, then we can represent it
   * as 1011.  In this case, Predicted, Nadir, and Smithed kernels are
   * allowed, but not Reconstructed.
   *
   * @param allowedKernelTypes Integer value representing Kernel::Type
   *                           enumerations that are allowed.
   *
   */
  KernelDb::KernelDb(const unsigned int allowedKernelTypes) {
    m_filename = "None";
    m_allowedKernelTypes = allowedKernelTypes;
    m_kernelDbFiles.clear();
  }


  /**
   * Constructs a new KernelDb object with the given file name and integer value
   * representing the Kernel::Type enumerations that are allowed.
   *
   * The allowed kernel types is stored as the sum of the enumerations of the
   * allowed Kernel Types. The following enumeriations currently exist:
   * 0001 = 1 = Predicted
   * 0010 = 2 = Nadir
   * 0100 = 4 = Reconstructed
   * 1000 = 8 = Smithed
   *
   * So, for example, if allowedKernelTypes is 11, then we can represent it
   * as 1011.  In this case, Predicted, Nadir, and Smithed kernels are
   * allowed, but not Reconstructed.
   *
   * @param dbName String containing the name of the kernel database
   * @param allowedKernelTypes Integer value representing Kernel::Type
   *                           enumerations that are allowed.
   *
   * @see KernelDb(int)
   */
  KernelDb::KernelDb(const QString &dbName, const unsigned int allowedKernelTypes) :
    m_kernelData(dbName.toStdString()) {
    m_filename = dbName;
    m_allowedKernelTypes = allowedKernelTypes;
    m_kernelDbFiles.clear();
  }


  /**
   * Constructs a new KernelDb object from the given stream and integer value
   * representing the Kernel::Type enumerations that are allowed. The filename
   * is set to "internal stream" if this constructor is used.
   *
   * The allowed kernel types is stored as the sum of the enumerations of the
   * allowed Kernel Types. The following enumeriations currently exist:
   * 0001 = 1 = Predicted
   * 0010 = 2 = Nadir
   * 0100 = 4 = Reconstructed
   * 1000 = 8 = Smithed
   *
   * So, for example, if allowedKernelTypes is 11, then we can represent it
   * as 1011.  In this case, Predicted, Nadir, and Smithed kernels are
   * allowed, but not Reconstructed.
   *
   * @param dbStream An input stream containing kernel database Pvl objects
   * @param allowedKernelTypes Integer value representing Kernel::Type
   *                           enumerations that are allowed.
   * @see KernelDb(int)
   */
  KernelDb::KernelDb(std::istream &dbStream, const unsigned int allowedKernelTypes) {
    dbStream >> m_kernelData;
    m_filename = "internal stream";
    m_allowedKernelTypes = allowedKernelTypes;
    m_kernelDbFiles.clear();
  }

  /**
   * Destructs KernelDb object.
   */
  KernelDb::~KernelDb() {
  }

  /**
   * This method finds the top priority of all Leap Second kernels
   * (lsk) identified by the database and the allowed kernel types.
   *
   * If no database file or stream was provided to the constructor,
   * the loadSystemDb() method should be called prior to this
   * accessor.
   *
   * If no LSK is found, this method will return an empty Kernel
   * object.
   *
   * @param lab Pvl label containing an IsisCube with needed times
   *            and IDs.
   *
   * @return @b Kernel Highest version of all leap second kernels
   */
  Kernel KernelDb::leapSecond(Pvl &lab) {
    return findLast("LeapSecond", lab);
  }


  /**
   * This method finds the highest version of all Target Attitude Shape
   * kernels (pck) identified by the database and the allowed kernel types.
   *
   * If no database file or stream was provided to the constructor,
   * the loadSystemDb() method should be called prior to this
   * accessor.
   *
   * If no PCK is found, this method will return an empty Kernel
   * object.
   *
   * @param lab Pvl label containing an IsisCube with needed times
   *            and IDs.
   *
   * @return @b Kernel Highest version of all target attitude shape kernels
   */
  Kernel KernelDb::targetAttitudeShape(Pvl &lab) {
    return findLast("TargetAttitudeShape", lab);
  }


  /**
   * This method finds the highest version of all Target Position
   * kernels (tspk) identified by the database and the allowed kernel types.
   *
   * If no database file or stream was provided to the constructor,
   * the loadSystemDb() method should be called prior to this
   * accessor.
   *
   * If no TSPK is found, this method will return an empty Kernel
   * object.
   *
   * @param lab Pvl label containing an IsisCube with needed times
   *            and IDs.
   *
   * @return @b Kernel Highest version of all target position kernels
   */
  Kernel KernelDb::targetPosition(Pvl &lab) {
    return findLast("TargetPosition", lab);
  }


  /**
   * This method finds a list of the highest versions of all Spacecraft
   * Pointing kernels (ck) identified by the databases and the allowed Kernel
   * types.
   *
   * If no database file or stream was provided ot the constructor, the
   * loadSystemDb() method should be called prior to this accessor.
   *
   * If no CKs are found, and a list with one empty queue will be returned.
   *
   * @param lab Pvl label containing an IsisCube with needed times
   *            and IDs.
   *
   * @return @b QList\< @b priority_queue\<Kernel\> @b \>
   *         List of queues of Kernel objects for the given entry
   */
  QList< std::priority_queue<Kernel> > KernelDb::spacecraftPointing(Pvl &lab) {
    return findAll("SpacecraftPointing", lab);
  }


  /**
   * This method finds the highest version of all Spacecraft Clock kernels
   * (sclk) identified by the database and the allowed kernel types.
   *
   * If no database file or stream was provided to the constructor,
   * the loadSystemDb() method should be called prior to this
   * accessor.
   *
   * If no SCLK is found, this method will return an empty Kernel object.
   *
   * @param lab Pvl label containing an IsisCube with needed times
   *            and IDs.
   *
   * @return @b Kernel Highest version of all spacecraft clock kernels
   */
  Kernel KernelDb::spacecraftClock(Pvl &lab) {
    return findLast("SpacecraftClock", lab);
  }


  /**
   * This method finds the highest version of all Spacecraft Position kernels
   * (spk) identified by the database and the allowed kernel types.
   *
   * If no database file or stream was provided to the constructor,
   * the loadSystemDb() method should be called prior to this
   * accessor.
   *
   * If no SPK is found, this method will return an empty Kernel
   * object.
   *
   * @param lab Pvl label containing an IsisCube with needed times
   *            and IDs.
   *
   * @return @b Kernel Highest version of all spacecraft position kernels
   */
  Kernel KernelDb::spacecraftPosition(Pvl &lab) {
    return findLast("SpacecraftPosition", lab);
  }


  /**
   * This method finds the last Instrument kernel found that matches the
   * (ik) criteria in the database and the allowed kernel types.
   *
   * If no database file or stream was provided to the constructor,
   * the loadSystemDb() method should be called prior to this
   * accessor.
   *
   * If no IK is found, this method will return an empty Kernel
   * object.
   *
   * @param lab Pvl label containing an IsisCube with needed times
   *            and IDs.
   *
   * @return @b Kernel Highest version of all instrument kernels
   */
  Kernel KernelDb::instrument(Pvl &lab) {
    return findLast("Instrument", lab);
  }


  /**
   * This method finds the highest version of all Frame kernels
   * (fk) identified by the database and the allowed kernel types.
   *
   * If no database file or stream was provided to the constructor,
   * the loadSystemDb() method should be called prior to this
   * accessor.
   *
   * If no FK is found, this method will return an empty Kernel
   * object.
   *
   * @param lab Pvl label containing an IsisCube with needed times
   *            and IDs.
   *
   * @return @b Kernel Highest version of all frame kernels
   */
  Kernel KernelDb::frame(Pvl &lab) {
    return findLast("Frame", lab);
  }

  /**
   * This method finds the highest version of all Instrument Addendum kernels
   * (iak) identified by the database and the allowed kernel types.
   *
   * If no database file or stream was provided to the constructor,
   * the loadSystemDb() method should be called prior to this
   * accessor.
   *
   * If no IAK is found, this method will return an empty Kernel
   * object.
   *
   * @param lab Pvl label containing an IsisCube with needed times
   *            and IDs.
   *
   * @return @b Kernel Highest version of all instrument addendum kernels
   */
  Kernel KernelDb::instrumentAddendum(Pvl &lab) {
    return findLast("InstrumentAddendum", lab);
  }


  /**
   * This method finds the highest version of all Digital Terrain Models
   * (DEMs) found that match the criteria in the database.
   *
   * If no database file or stream was provided to the constructor,
   * the loadSystemDb() method should be called prior to this
   * accessor.
   *
   * If no DEM is found, this method will return an empty Kernel
   * object.
   *
   * @param lab Pvl label containing an IsisCube with needed times
   *            and IDs.
   *
   * @return @b Kernel Highest version of all DEMs
   */
  Kernel KernelDb::dem(Pvl &lab) {
    return findLast("Dem", lab);
  }

  /**
   * Finds the highest priority Kernel object for the given entry based on the
   * allowed Kernel types. This method calls findAll() to get a list of
   * priority queues. The top priority of the first queue is returned, if it
   * exists. If not, an empty Kernel object is returned.
   *
   * @param entry The name of the kernel, dem, or other entry that will be
   *              searched for
   * @param lab The label containing the IsisCube object with times and
   *            instrument ID.
   *
   * @return @b Kernel Highest version of the given kernel or DEM entry
   */
  Kernel KernelDb::findLast(const QString &entry, Pvl &lab) {

    QList< priority_queue<Kernel> > queues = findAll(entry, lab);
    Kernel lastKernel;

    if (queues.size() > 0 && queues.at(0).size() > 0) {
      lastKernel = queues.at(0).top();
    }

    return lastKernel;
  }

  /**
   * Finds all of the Kernel objects for the given entry value based on the
   * allowed Kernel types. This method returns a list of priority queues.  Each
   * priority queue corresponds to a kernel db file object of the same name as
   * the entry in the kernelData pvl.
   *
   *
   * @param entry The name of the kernel, dem, or entry that will be searched
   *              for
   * @param lab The Pvl label containing an IsisCube object with times and
   *            instrument ID.
   *
   * @return @b QList\< @b priority_queue\<Kernel\> @b \>
   *         List of queues of Kernel objects for the given entry
   */
  QList< std::priority_queue<Kernel> > KernelDb::findAll(const QString &entry,
      Pvl &lab) {

    QList< priority_queue<Kernel> > queues;
    PvlObject &cube = lab.findObject("IsisCube");
    int cameraVersion = -1;

    try {
      cameraVersion = CameraFactory::CameraVersion(lab);
    }
    catch (IException &) {
    }

    // Make sure the entry has been loaded into memory
    if (!m_kernelData.hasObject(entry.toStdString())) {
      priority_queue<Kernel> emptyKernelQueue;
      emptyKernelQueue.push(Kernel());
      queues.push_back(emptyKernelQueue);
      return queues;
    }

    // Get the start and end time for the cube
    iTime start;
    iTime end;

    if (cube.hasGroup("Instrument")) {
      start = QString::fromStdString(cube.findGroup("Instrument")["StartTime"]);

      if (cube.findGroup("Instrument").hasKeyword("StopTime")) {
        end = QString::fromStdString(cube.findGroup("Instrument")["StopTime"]);
      }
      else {
        end = QString::fromStdString(cube.findGroup("Instrument")["StartTime"]);
      }
    }

    // Loop through the objects to look for all matches to the entry value
    for (int i = 0; i < m_kernelData.objects(); i++) {
      if (m_kernelData.object(i).isNamed(entry.toStdString())) {
        priority_queue<Kernel> filesFound;
        PvlObject &obj = m_kernelData.object(i);

        for (int groupIndex = obj.groups() - 1; groupIndex >= 0; groupIndex--) {
          // Get the group and start testing the cases in the keywords
          // to see if they all match this cube
          PvlGroup &grp = obj.group(groupIndex);

          // If the group name isn't selection, skip it.
          if (!grp.isNamed("Selection")) continue;

          QString type = "";

          // Make sure the type is allowed
          if (grp.hasKeyword("Type")) {
            type = QString::fromStdString(grp["Type"]);
            if (!(Kernel::typeEnum(type) & m_allowedKernelTypes)) {
              // will return 1 for each bit that has 1 in both type and allowed and
              // return 0 for all other bits
              //
              // so, if Type = 0010 and allowed = 1010, then this bitwise operator
              // (type & allowed) returns 0010 and is true.
              // That is, this type is allowed.
              continue;
            }
          }


          bool startMatches = matches(lab, grp, start, cameraVersion);
          bool endMatches = matches(lab, grp, end, cameraVersion);

          if (startMatches && endMatches) {
            // Simple case - the selection simply matches
            filesFound.push(Kernel(Kernel::typeEnum(type), files(grp)));
            QStringList kernelfiles = files(grp);
          }
          else if (startMatches) {
            // Well, the selection start matched but not the end.
            // Let's look for a second selection to handle overlap areas.
            for (int endTimeIndex = obj.groups() - 1;
                endTimeIndex >= 0;
                endTimeIndex--) {

              PvlGroup &endTimeGrp = obj.group(endTimeIndex);

              // The second selection must:
              //   Not be the current selection
              //   Be a selection
              //   Be of the same quality
              //   Match the end time
              //
              // *If start time is also matched, do not merge and simply take the
              // secondary match
              if (endTimeIndex == groupIndex) continue;
              if (!endTimeGrp.isNamed("Selection")) continue;
              if (grp.hasKeyword("Type") != endTimeGrp.hasKeyword("Type")) continue;
              if (grp.hasKeyword("Type") &&
                  grp["Type"] != endTimeGrp["Type"]) continue;
              if (!matches(lab, endTimeGrp, end, cameraVersion)) continue;

              // Better match is true if we find a full overlap
              bool betterMatch = false;

              // True if we have matching time ranges = we want to merge
              bool endTimesMatch = true;

              // Check for matching time ranges
              for (int keyIndex = 0;
                  !betterMatch && keyIndex < grp.keywords();
                  keyIndex++) {
                PvlKeyword &key = grp[keyIndex];

                if (!key.isNamed("Time")) continue;

                iTime timeRangeStart(QString::fromStdString(key[0]));
                iTime timeRangeEnd(QString::fromStdString(key[1]));

                bool thisEndMatches = matches(lab, endTimeGrp,
                                              timeRangeEnd, cameraVersion);
                endTimesMatch = endTimesMatch && thisEndMatches;

                if (matches(lab, endTimeGrp, start, cameraVersion)
                   && matches(lab, endTimeGrp, end, cameraVersion)) {
                  // If we run into a continuous kernel, we want to take that in all
                  //   cases.
                  betterMatch = true;
                }
              }

              // No exact match but time ranges overlap, merge the selections
              if (!betterMatch && endTimesMatch) {
                QStringList startMatchFiles = files(grp);
                QStringList endMatchFiles = files(endTimeGrp);

                while (endMatchFiles.size()) {
                  startMatchFiles.push_back(endMatchFiles[endMatchFiles.size() - 1]);
                  endMatchFiles.pop_back();
                }

                filesFound.push(
                  Kernel(Kernel::typeEnum(type), startMatchFiles));
              }
              // Found an exact match, use it
              else if (betterMatch) {
                filesFound.push(Kernel(Kernel::typeEnum(type), files(endTimeGrp)));
                QStringList kernelfiles = files(endTimeGrp);
              }
            }
          }
        }

        queues.push_back(filesFound);
      }
    }

    if (queues.size() == 0) {
      priority_queue<Kernel> emptyKernelQueue;
      emptyKernelQueue.push(Kernel());
      queues.push_back(emptyKernelQueue);
    }

    return queues;
  }

  /**
   * This static method determines whether the given cube label matches
   * the given criteria. The method can check for three criteria types:
   * (1) Time, (2) CameraVersion, and/or (3) the Match keyword values in
   * the given PvlGroup.  All three are optional.  If any of the given
   * criteria are not met, the method will return false.
   *
   * (1) If the given PvlGroup does not have a "Time" keyword, then the
   * time will not be compared.
   *
   * (2) If the given PvlGroup does not have a "CameraTime" keyword, then
   * the time will not be compared.
   *
   * (3) If the given PvlGroup does not have a "Match" keyword, then
   * individual keywords will not be compared. If the "Match" keyword exists in
   * the given PvlGroup, it will have the form:
   *
   * <code>
   * Object = ObjectName
   *   Group = grp
   *     Match = (MatchGroup, MatchKeyword, MatchKeywordValue)
   *   EndGroup
   * EndObject
   * </code>
   *
   * The first entry of the vector passed into the Match keyword of this
   * Pvl represents the name of a group in the labels.  This group will be
   * searched for the keyword name passed in as the second entry of the
   * Match keyword.  This criteria is met if the keyword value in the
   * labels matches the third entry of the Match keyword.
   *
   * @param lab The labels of an IsisCube to be searched, usually for times and
   *            InstrumentId.
   * @param grp A PvlGroup from the kernel database containing values to
   *            compare to the labels
   * @param timeToMatch The time value (if time is not being compared,
   *                    "iTime()" may be passed in for this value)
   * @param cameraVersion The camera version to be matched with the cube
   *                      labels (if camera version is not being compared, "1"
   *                      may be passed in for this parameter)
   *
   * @return @b bool Indicates whether all of the given criteria was matched.
   */
  bool KernelDb::matches(const Pvl &lab, PvlGroup &grp,
                         iTime timeToMatch, int cameraVersion) {
    // These are the conditions that make this test pass:
    //   1) No time OR At least one matching time
    //   2) All keyword matches are true OR No keyword matches present
    //
    // In order to accomplish this, matchTime is initialized to true and remains
    // as such if and only if there are no time conditionals. If Time keywords
    // exist, one of them has to set matchTime to true. The matchKeywords is
    // true until a mismatch is found.
    const PvlObject &cube = lab.findObject("IsisCube");
    bool matchTime = !grp.hasKeyword("Time");
    bool matchKeywords = true;
    double startOffset = 0;
    double endOffset = 0;
    QString instrument = "";

    if (grp.hasKeyword("StartOffset")){
      startOffset = (double) grp["StartOffset"] + 0.001;
    }
    if (grp.hasKeyword("EndOffset")){
      endOffset = (double) grp["EndOffset"] + 0.001;
    }
    if (grp.hasKeyword("Instrument")){
      instrument = QString::fromStdString(grp["Instrument"]);
    }

    // First, the time search. Loop through the keywords, if the name isn't
    //  Time then skip it. If it is, then get the start/end times and keep
    //  looking until one is found.
    for (int keyword = 0; keyword < grp.keywords(); keyword++) {
      PvlKeyword key = grp[keyword];

      if (key.isNamed("Time")) {

        // Pull the selections start and end time out
        iTime kernelStart = QString::fromStdString(key[0]);
        iTime kernelEnd   = QString::fromStdString(key[1]);

        // If the kernel times inside of the requested times we
        // set the matchTime to be true.
        if ((kernelStart - startOffset <= timeToMatch) && (kernelEnd + endOffset >= timeToMatch)) {
          matchTime = true;
        }
        // If the kernel segment has an instrument specification that doesn't match
        // the instrument id in the label then the timing is always invalid
        if (!instrument.isEmpty()) {
          const PvlGroup &inst = lab.findGroup("Instrument", Pvl::Traverse);
          QString instId = QString::fromStdString(inst.findKeyword("InstrumentId"));
          if (instId.compare(instrument) != 0) {
            matchTime = false;
          }
        }
      }

      else if (key.isNamed("Match")) {
        try {
          QString matchGroup = QString::fromStdString(key[0]);
          QString matchKey   = QString::fromStdString(key[1]);
          QString matchValue = QString::fromStdString(key[2]);

          QString cubeValue = QString::fromStdString(cube.findGroup(matchGroup.toStdString())[matchKey.toStdString()]);
          cubeValue = cubeValue.simplified().trimmed().toUpper();
          matchValue = matchValue.simplified().trimmed().toUpper();

          // If QStrings are not the same, match automatically fails
          if (cubeValue.compare(matchValue) != 0) {
            matchKeywords = false;
          }
        }
        catch (IException &e) {
          // This error is thrown if the group or keyword do not exist in 'lab'
          matchKeywords = false;
        }
      }
      else if (key.isNamed("CameraVersion")) {
        try {
          for (int camVersionKeyIndex = 0;
              camVersionKeyIndex < key.size();
              camVersionKeyIndex++) {

            bool versionMatch = false;
            IString val = key[camVersionKeyIndex];
            IString commaTok;

            while ((commaTok = val.Token(",")).ToQt().length() > 0) {
              if (commaTok.find('-') != string::npos) {
                QString dashTok;
                int start = commaTok.Token("-").ToInteger();
                int end = commaTok.Token("-").ToInteger();
                int direction;
                direction = (start <= end) ? 1 : -1;
                // Save the entire range of bands
                for (int version = start;
                    version != end + direction;
                    version += direction) {
                  if (version == cameraVersion) {
                    versionMatch = true;
                  }
                }
              }
              // This token is a single band specification
              else {
                if (commaTok.ToInteger() == cameraVersion) {
                  versionMatch = true;
                }
              }
            }

            if (!versionMatch) {
              matchKeywords = false;
            }
          }
        }
        catch (IException &) {
          matchKeywords = false;
        }
      }
    }

    return matchKeywords && matchTime;
  }

  /**
   * Loads the appropriate kernel database files with the defined BASE and
   * MISSION info for each type of kernel.
   *
   * This method always gets the following from the mission directory:
   * <ul>
   *   <li> ck </li>
   *   <li> fk </li>
   *   <li> ik </li>
   *   <li> sclk </li>
   *   <li> spk </li>
   *   <li> iak </li>
   * </ul>
   *  For the following, this method looks for appropriate kernels in the
   *  mission directory first, then if not found, uses the kernels in the base
   *  directory:
   * <ul>
   *   <li> pck </li>
   *   <li> tspk </li>
   * </ul>
   *  The following are always found in the base directory
   * <ul>
   *   <li> lsk </li>
   *   <li> dems </li>
   * </ul>
   *
   * To check which kernel database files have been loaded, file
   * names may be accessed by calling kernelDbFiles().
   *
   * @param mission A QString containing the name of the mission whose kernel
   *                database files will be loaded.
   * @param lab Reference to the labels of a cube. This is used match the
   *            appropriate InstrumentId value, if needed.
   *
   * @see readKernelDbFiles()
   * @see kernelDbFiles()
   */
  void KernelDb::loadSystemDb(const QString &mission, const Pvl &lab) {

    // Get the base DataDirectory
    PvlGroup &dataDir = Preference::Preferences().findGroup("DataDirectory");
    QString baseDir = QString::fromStdString(dataDir["Base"]);

    // Get the mission DataDirectory
    QString missionDir = QString::fromStdString(dataDir[mission.toStdString()]);

    // Load the leapsecond DB
    loadKernelDbFiles(dataDir, baseDir + "/kernels/lsk", lab);
    // Load the target attitude shape DB
    // Try to get mission specific pck data, if that
    // fails, fallback to the base pck data
    try {
      loadKernelDbFiles(dataDir, missionDir + "/kernels/pck", lab);
    }
    catch (IException &e) {
      loadKernelDbFiles(dataDir, baseDir + "/kernels/pck", lab);
    }
    // Load the target position DB
    FileName tpDbPath(missionDir + "/kernels/tspk");
    if (tpDbPath.fileExists()) {
      loadKernelDbFiles(dataDir, missionDir + "/kernels/tspk", lab);
    }
    else {
      loadKernelDbFiles(dataDir, baseDir + "/kernels/spk", lab);
    }
    // Load the DEM DB
    loadKernelDbFiles(dataDir, baseDir + "/dems", lab);
    // Load the mission specific spacecraft pointing DB
    loadKernelDbFiles(dataDir, missionDir + "/kernels/ck", lab);
    // Load the mission specific frame DB
    loadKernelDbFiles(dataDir, missionDir + "/kernels/fk", lab);
    // Load the mission specific instrument DB
    loadKernelDbFiles(dataDir, missionDir + "/kernels/ik", lab);
    // Load the mission specific spacecraft clock DB
    loadKernelDbFiles(dataDir, missionDir + "/kernels/sclk", lab);
    // Load the mission specific spacecraft position DB
    loadKernelDbFiles(dataDir, missionDir + "/kernels/spk", lab);
    // Load the mission specific instrument addendum DB
    loadKernelDbFiles(dataDir, missionDir + "/kernels/iak", lab);
    readKernelDbFiles();
  }

  /**
   * This method is called by loadSystemDb() to create a list of all appropriate
   * kernel database files to be read.
   *
   * The method first checks whether the directory contains a config file of the
   * form kernels.????.conf
   *
   * If not, the highest version of the database file of the form
   * kernels.????.db file is read in.
   *
   * A config file will exist if this mission requires multiple kernel database
   * files or instrument dependent database files. The kernel data base files
   * listed in the highest version of the config file group will be read in. The
   * "Match" keyword maybe used in this config file for a particular Instrument,
   * if required.
   *
   * To check which kernel database files have been loaded, file names may be
   * accessed by calling kernelDbFiles().
   *
   * @param dataDir The PvlGroup found in the Preferences DataDirectory group
   * @param directory The full directory path of the kernel (or dem)
   *                  whose kernel database file(s) will be read.
   * @param lab Reference to the labels of a cube. This is used to the match
   *            the appropriate InstrumentId value, if needed.
   *
   * @see loadSystemDb()
   * @see kernelDbFiles()
   */
  void KernelDb::loadKernelDbFiles(PvlGroup &dataDir,
                                   QString directory, const Pvl &lab) {
    // get most recent version of config file
    FileName configFile = directory + "/kernels.????.conf";
    bool noConfigFile = false;
    // if there is no config file, default to the most recent kernel db file
    try {
      configFile = configFile.highestVersion();
    }
    catch (IException &e) {
      noConfigFile = true;
    }
    if (noConfigFile) {
      FileName kernelDb(directory + "/kernels.????.db");
      m_kernelDbFiles.append(kernelDb.highestVersion());
    }
    else { // else, read in the appropriate database files from the config file
      PvlObject inst = Pvl(configFile.expanded().toStdString()).findObject("Instrument");
      bool foundMatch = false;
      // loop through each group until we find a match
      for (int groupIndex = 0; groupIndex < inst.groups(); groupIndex++) {
        if (!foundMatch) {
          PvlGroup &grp = inst.group(groupIndex);
          // Only add files in Selection groups with matching intrument id
          if (grp.isNamed("Selection")
              && KernelDb::matches(lab, grp, iTime(), 1)) {
            foundMatch = true;
            // add each File keywords in the matching group to the list
            for (int keyIndex = 0; keyIndex < grp.keywords(); keyIndex++) {
              PvlKeyword keyword = grp[keyIndex];
              if (keyword.isNamed("File")) {
                QString dir = QString::fromStdString(dataDir[keyword[0]]);
                FileName kernelDb( dir + "/" + QString::fromStdString(keyword[1]));
                m_kernelDbFiles.append(kernelDb.highestVersion());
              }
            }
          }
        }
      }
    }
      return;
  }


  /**
   * This method is called by loadSystemDb() to read kernel database file list
   * compiled by loadKernelDbFiles() and add the contents of these database
   * files to the kernelData pvl.
   *
   * To check which kernel database files will be read in by this method, file
   * names may be accessed by calling kernelDbFiles().
   *
   * @param dataDir The PvlGroup found in the Preferences DataDirectory group
   * @param directory The full directory path of the kernel (or dem)
   *                  whose kernel database file(s) will be read.
   * @param lab Reference to the labels of a cube. This is used to the match
   *            the appropriate InstrumentId value, if needed.
   *
   * @see loadSystemDb()
   * @see kernelDbFiles()
   */
  void KernelDb::readKernelDbFiles() {
    // read each of the database files appended to the list into m_kernelData
    foreach (FileName kernelDbFile, m_kernelDbFiles) {
      try {
        m_kernelData.read(kernelDbFile.expanded().toStdString());
      }
      catch (IException &e) {
        QString msg = "Unable to read kernel database file ["
                      + kernelDbFile.expanded() + "].";
        throw IException(e, IException::Unknown, msg, _FILEINFO_);
      }
    }
  }

  /**
   * Accessor method to retrieve the list of kernel database files that were
   * read in when loadSystemDb() is called.
   *
   * @return @b QList\<FileName\> A list containing the kernel database file
   *         names
   */
  QList<FileName> KernelDb::kernelDbFiles() {
    return m_kernelDbFiles;
  }


  /**
   * This method retrieves the values of all of the "File" keywords in the given
   * PvlGroup.
   *
   * @param grp The PvlGroup that containing file names to be retrieved
   *
   * @return @b QStringList A list containing the file names found in the
   *         given group.
   */
  QStringList KernelDb::files(PvlGroup &grp) {
    QStringList files;

    for (int i = 0; i < grp.keywords(); i++) {
      PvlKeyword kfile = grp[i];
      if (kfile.name() != "File") continue;

      // Two values in the "File" keyword from the DB,
      // indicates an ISIS preference in the DataDirectory section
      // and a filename
      if (kfile.size() == 2) {
        QString pref = QString::fromStdString(kfile[0]);
        QString version = QString::fromStdString(kfile[1]);
        FileName filename("$" + pref + "/" + version);
        if (filename.isVersioned())
          filename = filename.highestVersion();
        files.push_back(filename.originalPath() + "/" + filename.name());
      }
      // One value in "File" indicates a full file spec
      else if (kfile.size() == 1) {
        FileName filename(QString::fromStdString(kfile[0]));
        if (filename.isVersioned())
          filename = filename.highestVersion();
        files.push_back(filename.originalPath() + "/" + filename.name());
      }
      else {
        std::string msg = "Invalid File keyword value in [Group = ";
        msg += grp.name() + "] in database file [";
        msg += m_filename.toStdString() + "]";
        throw IException(IException::Unknown, msg, _FILEINFO_);
      }
    }

    return files;
  }
} //end namespace isis
