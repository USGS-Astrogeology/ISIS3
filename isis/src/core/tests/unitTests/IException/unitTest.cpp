/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <iostream>
#include "Preference.h"

#include "IException.h"

using namespace std;
using namespace Isis;

int main(void) {
  Isis::Preference::Preferences(true);

  IException lastError;

  cerr << "Test Error 1\n";
  {
    IException error;
    error.append(lastError);
    cerr << "print():\n";
    error.print();
    cerr << "what():\n" << error.what() << "\n";
    lastError = error;
  }
  cerr << "\n";

  cerr << "Test Error 2\n";
  {
    IException error(IException::User, "", "", -1);
    error.append(lastError);
    cerr << "print():\n";
    error.print();
    cerr << "what():\n" << error.what() << "\n";
    lastError = error;
  }
  cerr << "\n";

  cerr << "Test Error 3\n";
  {
    IException error(IException::User, "", _FILEINFO_);
    error.append(lastError);
    cerr << "print():\n";
    error.print();
    cerr << "what():\n" << error.what() << "\n";
    lastError = error;
  }
  cerr << "\n";

  cerr << "Test Error 4\n";
  {
    IException error(
        IException::Unknown, "Testing unknown error.", _FILEINFO_);
    error.append(lastError);
    cerr << "print():\n";
    error.print();
    cerr << "what():\n" << error.what() << "\n";
    lastError = error;
  }
  cerr << "\n";

  cerr << "Test Error 5\n";
  {
    IException error(
        lastError,
        IException::User, "Testing user error", _FILEINFO_);
    cerr << "toPvl():\n";
    Pvl pvlOutput(error.toPvl());
    cerr << pvlOutput << "\n";
    cerr << "what():\n" << error.what() << "\n";
    lastError = error;
  }
  cerr << "\n";

  cerr << "Test Error 6\n";
  {
    IException error(
        IException::Programmer, "Testing programmer error.", _FILEINFO_);
    error.append(lastError);
    cerr << "print():\n";
    error.print();
    cerr << "what():\n" << error.what() << "\n";
    lastError = error;
  }
  cerr << "\n";

  cerr << "Test Error 7\n";
  {
    IException error(
        lastError,
        IException::Io, "Testing I/O error.", _FILEINFO_);
    cerr << "print():\n";
    error.print(true);
    cerr << "what():\n" << error.what() << "\n";
    lastError = error;
  }
  cerr << "\n";

  cerr << "Test Throwing Error: No reaction\n";
  {
    try {
      throw lastError;
    }
    catch (IException &e) {
    }
  }
  cerr << "\n";

  cerr << "Test Throwing Error: Print\n";
  {
    try {
      throw lastError;
    }
    catch (IException &e) {
      cerr << "print():\n";
      e.print();
      cerr << "what():\n" << e.what() << "\n";
    }
  }
  cerr << "\n";

  cerr << "Test Throwing Error: Rethrow and print\n";
  {
    try {
      try {
        throw lastError;
      }
      catch (IException &e) {
        throw IException(e, IException::Unknown, "Another error", _FILEINFO_);
      }
    }
    catch (IException &e) {
      e.print();
    }
  }
  cerr << "\n";

  Pvl &prefs = Preference::Preferences();
  PvlGroup &errorPrefs = prefs.findGroup("ErrorFacility");

  cerr << "Test Preference 'FileLine'\n";
  {
    std::string &fileLineValue = errorPrefs["FileLine"][0];

    fileLineValue = "On";

    cerr << "Turned ON -- print():\n";
    lastError.print();
    cerr << "\n";

    fileLineValue = "Off";

    cerr << "Turned OFF -- print():\n";
    lastError.print();
    cerr << "\n";

    fileLineValue = "On";

    cerr << "Turned ON, override to OFF -- print(false):\n";
    lastError.print(false);
    cerr << "\n";

    fileLineValue = "Off"; // restore original value
  }

  cerr << "Test Preference 'Format'\n";
  {
    std::string &formatValue = errorPrefs["Format"][0];

    formatValue = "Standard";

    cerr << "Standard Format -- print():\n";
    lastError.print();
    cerr << "\n";

    cerr << "Standard Format -- toString():\n";
    cerr << lastError.toString() << "\n\n";

    formatValue = "Pvl";

    cerr << "Pvl Format -- print():\n";
    lastError.print();
    cerr << "\n";

    cerr << "Pvl Format -- toString():\n";
    cerr << lastError.toString() << "\n\n";

    cerr << "Pvl Format -- empty toString():\n";
    cerr << IException().toString() << "\n\n";

    formatValue = "Standard"; // restore original value
  }

  return 0;
}

