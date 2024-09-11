/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#ifndef isisminer_h
#define isisminer_h

#include "Pvl.h"
#include "UserInterface.h"

#include <QString>

namespace Isis{
  extern void isisminer(UserInterface &ui);
  extern void isisminer(QString &configFileName, UserInterface &ui,  
                        Pvl *pvl_globals=nullptr);
}

#endif
