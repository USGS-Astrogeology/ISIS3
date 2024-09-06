/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "LineManager.h"

#include "IException.h"
#include "IString.h"

using namespace std;

namespace Isis {

  /**
   * Constructs a LineManager object
   *
   * @param cube The cube this buffer manager will be associated with.
   *
   * @param reverse Modifies the order of progression LineManager
   *             takes through the cube.  By default, progresses
   *             lines first then bands. If reverse = true, then
   *             the buffer progresses bands first, then lines.
   */

  LineManager::LineManager(const Isis::Cube &cube, const bool reverse) :
    Isis::BufferManager(cube.sampleCount(), cube.lineCount(),
                        cube.bandCount(), cube.sampleCount(), 1, 1,
                        cube.pixelType(), reverse) {
  }

  /**
   * Positions the buffer at the requested line and returns a status indicator
   * if the set was succesful or not
   *
   * @param line The line number within a band (1-based).
   *
   * @param band The band number within the cube (1-based). Defaults to 1
   *
   * @return bool Status indicator of the set being successful or not
   */

  bool LineManager::SetLine(const int line, const int band) {
    if(line < 1) {
      std::string message = "LineManager is unable to set the line to [" 
                       + toString(line) + "]. Minimum line value is 1.";
      throw IException(IException::Programmer, message, _FILEINFO_);
    }

    if(band < 1) {
      string message = "LineManager is unable to set the line for band [" 
                       + IString(band) + "]. Minimum band value is 1.";
      throw IException(IException::Programmer, message, _FILEINFO_);
    }

    int map = (band - 1) * MaxLines() + line - 1;
    return setpos(map);
  }

} // end namespace isis
