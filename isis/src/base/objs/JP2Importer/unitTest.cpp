/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <iomanip>

#include <QFile>

#include "FileName.h"
#include "IException.h"
#include "JP2Importer.h"
#include "Preference.h"

using namespace std;
using namespace Isis;


int main(int argc, char *argv[]) {
  Preference::Preferences(true);
  cout << setprecision(9);

  try {
    cout << "Testing JP2Importer..." << endl << endl;
    FileName inputName("test.jp2");

    cout << "Creating Instance" << endl;
    JP2Importer *importer = new JP2Importer(inputName);

    cout << "Importing" << endl;
    FileName outputName("test.cub");
    importer->import(outputName);

    cout << "Clean-up" << endl;
    delete importer;
    QFile::remove(QString::fromStdString(outputName.expanded()));

    cout << endl << "Done" << endl;
  }
  catch (IException &e) {
    e.print();
  }
}

