/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#ifndef skypt_h
#define skypt_h

#include "Pvl.h"
#include "UserInterface.h"

namespace Isis{
  extern void skypt(UserInterface &ui, Pvl *log);
  extern void skypt(Cube *cube, UserInterface &ui, Pvl *log);
}

#endif
