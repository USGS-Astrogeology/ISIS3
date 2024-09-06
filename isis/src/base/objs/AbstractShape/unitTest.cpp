/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <QDebug>

#include "AbstractShape.h"
#include "IException.h"
#include "Preference.h"

using namespace Isis;
/**
 *  
 * @internal 
 *   @history 2015-02-25 Jeannie Backer - Original version.
 *                           Code coverage: Not Applicable. No implementation.
 *
 */
int main(int argc, char *argv[]) {
  try {
    qDebug() << "Unit test for Abstract Shape.";
    Preference::Preferences(true);
  }
  catch (IException &e) {
    qDebug() << "";
    qDebug() << "";
    std::string msg = "**************** UNIT TEST FAILED! **************** ";
    IException(e, IException::Unknown, msg, _FILEINFO_).print();
  }
}
