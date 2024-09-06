/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "Brick.h"
#include "IException.h"

namespace Isis {
  /**
   * Resizes the memory buffer to the specified number of samples, lines, and
   * bands.
   *
   * @param nsamps Number of samples
   * @param nlines Number of lines
   * @param nbands Number of bands
   */
  void Brick::Resize(const int nsamps, const int nlines, const int nbands) {
    delete [] p_buf;
    delete [](char *) p_rawbuf;
    p_nsamps = nsamps;
    p_nlines = nlines;
    p_nbands = nbands;
    p_npixels = p_nsamps * p_nlines * p_nbands;
    Allocate();
  }

  /**
  * Sets the current brick as requested
  *
  * @param brick  The brick number within a cube. This number starts with the
  *              upper left corner of the cube and proceedes across the samples
  *              then down the lines and lastly through the bands. The first
  *              brick starts at (1,1,1). The last brick contains the point
  *              (cubeSamples,cubeLines,cubeBands).
  *
  * @return bool
  *
  * @throws Isis::iException::Programmer - invalid argument value
  */
  bool Brick::SetBrick(const int brick) {
    if(brick < 1) {
      std::string message = "Invalid value for argument [brick]";
      throw IException(IException::Programmer, message, _FILEINFO_);
    }

    return setpos(brick - 1);
  }
} // end namespace isis
