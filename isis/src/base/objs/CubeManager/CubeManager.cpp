/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "CubeManager.h"

#include <iostream>
#include <sys/resource.h>
#include <cstdlib>  // atexit
#include <memory>   // smart pointers
#include <QCoreApplication>

#include "Cube.h"
#include "CubeAttribute.h"
#include "CubeManager.h"
#include "FileName.h"
#include "IString.h"

namespace Isis {
  CubeManager *CubeManager::p_instance = 0;

  /**
   * This initializes a CubeManager object
   *
   */
  CubeManager::CubeManager() {

    // Grab the Qcore application instace to check if it has been
    // instantiated yet. If not, instantiate it.
    if (QCoreApplication::instance() == 0) {
      static char **argv = 0;

      if(!argv) {
        argv = new char*[2];
        argv[0] = new char[1024];
        strcpy(argv[0], "CubeManager");
        argv[1] = 0;
      }

      static int argc  = 1;
      new QCoreApplication(argc, argv);
    }

    // Get the maximum allowable number of open files for a process
    struct rlimit fileLimit;

    if (getrlimit(RLIMIT_NOFILE, &fileLimit) != 0) {
      std::string msg = "Cannot read the maximum allowable open files from system resources.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    // Allow for library files, etc used by this process
    // So set or file limit to 60% of maximum allowed number of opened files
    p_maxOpenFiles = fileLimit.rlim_cur * .60;
    p_currentLimit = p_maxOpenFiles;

    // Add the CleanUp() call to QCoreApplication's clean up routine,
    // this ensures that the static instance of CubeManager is cleaned
    // up with the rest of ISIS before System application clean up begins
    // in order to ensure clean destruction. If System deletes CubeManager's
    // Cubes or any of the individual Cube's memebers before CubeManager gets
    // deleted, it will cause a segfault.
    qAddPostRoutine(CleanUp);
  }

  /**
   * This is the CubeManager destructor. This method calls CleanCˇubes().
   *
   */
  CubeManager::~CubeManager() {
    CleanCubes();
  }


  /**
   * This method opens a cube. If the cube is already opened, this method will
   * return the cube from memory. The CubeManager class retains ownership of this
   * cube pointer, so do not close the cube, destroy the pointer, or otherwise
   * modify the cube object or pointer such that another object using them would
   * fail. This method does not guarantee you are the only one with this pointer,
   * nor is it recommended to keep this pointer out of a local (method) scope.
   *
   * Note that this method will allow for 60% of the system's maximum file limit + 1
   * cubes to be opened, since the cube passed is opened and then cleanup follows. However,
   * since our maximum limit is 60% of the system limit, this will allow enough room for
   * this extra file to be opened before the cleanup occurs.
   *
   * @param cubeFileName The filename of the cube you wish to open
   *
   * @return Cube* A pointer to the cube object that CubeManager retains ownership
   *         to and may delete at any time
   */
  Cube *CubeManager::OpenCube(const QString &cubeFileName) {
    CubeAttributeInput attIn(cubeFileName.toStdString());
    IString attri = attIn.toString().toStdString();
    IString expName = FileName(cubeFileName.toStdString()).expanded();

    // If there are attributes, we need a plus sign on the name
    if (attri.size() > 0) {
      expName += "+";
    }

    IString fullName = expName + attri;
    QString fileName(QString::fromStdString(fullName));
    QMap<QString, Cube *>::iterator searchResult = p_cubes.find(fileName);

    if (searchResult == p_cubes.end()) {
      p_cubes.insert(fileName, new Cube());
      searchResult = p_cubes.find(fileName);

      // Need to clean up memory if there is a problem opening a cube
      // This allows the CubeManager class to clean up the dynamically alloc'd
      // Cube before rethrowing the exception from Cube's open method
      try {
        (*searchResult)->open(fileName);
      }
      catch (IException &e) {
        CleanCubes(fileName);
        throw;
      }
    }

    // Keep track of the newly opened cube in our queue
    p_opened.removeAll(fileName);
    p_opened.enqueue(fileName);

    // cleanup excess cubes
    while (p_opened.size() > (int)(p_currentLimit)) {
      QString needsCleaned = p_opened.dequeue();
      CleanCubes(needsCleaned);
    }

    return (*searchResult);
  }


  /**
   * This method removes a cube from memory, if it exists. If the cube is not
   * loaded into memory, nothing happens. This will cause any pointers to this
   * cube, obtained via OpenCube, to be NULL.
   *
   * @param cubeFileName The filename of the cube to remove from memory
   */
  void CubeManager::CleanCubes(const QString &cubeFileName) {

    QString fileName(QString::fromStdString(FileName(cubeFileName.toStdString()).expanded()));
    QMap<QString, Cube *>::iterator searchResult = p_cubes.find(fileName);

    if (searchResult == p_cubes.end()) {
      return;
    }

    delete searchResult.value();
    searchResult.value() = NULL;
    p_cubes.erase(searchResult);
  }


  /**
   * This method removes all cubes from memory. All pointers returned via OpenCube
   * will be NULL.
   */
  void CubeManager::CleanCubes() {
    QMap<QString, Cube *>::iterator pos = p_cubes.begin();

    while (pos != p_cubes.end()) {
      delete pos.value();
      pos.value() = NULL;
      pos ++;
    }

    p_cubes.clear();
  }
}
