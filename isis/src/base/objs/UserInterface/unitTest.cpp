/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <iostream>

#include "UserInterface.h"
#include "IException.h"
#include "IString.h"
#include "Preference.h"
#include "FileName.h"

using namespace Isis;
using namespace std;

int main(int argc, char *argv[]) {
  Isis::Preference::Preferences(true);

  cout << "Unit test for Isis::UserInterface ..." << endl;

  std::string unitTestXml = Isis::FileName((std::string)ISISROOT + "/src/base/objs/UserInterface/unitTest.xml").expanded();
  std::string highpass = Isis::FileName((std::string)ISISBUILDDIR + "/bin/xml/highpass.xml").expanded();

  char *myArgv[15];// = {"unitTest", "from=input.cub", "to=output.cub"};

  for (int i = 0; i < 15; i++)
    myArgv[i] = new char[128];

  try {
    cout << "Basic FROM/TO Test" << endl;
    {
      int myArgc = 0;
      strcpy(myArgv[myArgc++], "unitTest");
      strcpy(myArgv[myArgc++], "from=input.cub");
      strcpy(myArgv[myArgc++], "to=output.cub");

      Isis::UserInterface ui(QString::fromStdString(unitTestXml), myArgc, myArgv);
      cout << "FROM:    " << ui.GetAsString("FROM").toStdString() << endl;
      cout << "TO:      " << ui.GetAsString("TO").toStdString() << endl;
      cout << "GUI:     " << ui.IsInteractive() << endl;
      cout << endl;
    }


    cout << "Testing param= value Format" << endl;
    {
      int myArgc = 0;
      strcpy(myArgv[myArgc++], "highpass");
      strcpy(myArgv[myArgc++], "from=dog");
      strcpy(myArgv[myArgc++], "to=biscuit");
      strcpy(myArgv[myArgc++], "line=");
      strcpy(myArgv[myArgc++], "3");
      strcpy(myArgv[myArgc++], "samp=");
      strcpy(myArgv[myArgc++], "3");

      Isis::UserInterface ui(QString::fromStdString(highpass), myArgc, myArgv);
      cout << "FROM:    " << ui.GetAsString("FROM").toStdString() << endl;
      cout << "TO:      " << ui.GetAsString("TO").toStdString() << endl;
      cout << "GUI:     " << ui.IsInteractive() << endl;
      cout << endl;
    }


    cout << "Testing No Arguments (Defaults)" << endl;
    {
      int myArgc = 0;
      strcpy(myArgv[myArgc++], "unitTest");

      int myArgcQ = myArgc;
      Isis::UserInterface ui(QString::fromStdString(unitTestXml), myArgcQ, myArgv);
      cout << "FROM:    " << ui.GetAsString("FROM").toStdString() << endl;
      cout << "TO:      " << ui.GetAsString("TO").toStdString() << endl;
      cout << "GUI:     " << ui.IsInteractive() << endl;
      cout << endl;
    }


    cout << "Testing Basic Array Argument" << endl;
    {
      int myArgc = 0;
      strcpy(myArgv[myArgc++], "highpass");
      strcpy(myArgv[myArgc++], "from=dog");
      strcpy(myArgv[myArgc++], "to=(biscuit,bread)");
      strcpy(myArgv[myArgc++], "line=");
      strcpy(myArgv[myArgc++], "3");
      strcpy(myArgv[myArgc++], "samp=");
      strcpy(myArgv[myArgc++], "3");

      Isis::UserInterface ui(QString::fromStdString(highpass), myArgc, myArgv);
      vector<QString> vals;
      cout << "FROM:    " << ui.GetAsString("FROM").toStdString() << endl;
      cout << "TO:      ";
      ui.GetAsString("TO", vals);

      for (unsigned int i = 0; i < vals.size(); i++) {
        if (i != 0) cout << ",";

        cout << vals[i].toStdString();
      }
      cout << endl;
      cout << "GUI:     " << ui.IsInteractive() << endl;
      cout << endl;
    }


    cout << "Testing Common Array Argument" << endl;
    {
      int myArgc = 0;
      strcpy(myArgv[myArgc++], "highpass");
      strcpy(myArgv[myArgc++], "from=dog");
      strcpy(myArgv[myArgc++], "to=( \"dog\" , \"cat\", \" cow \", 'frog')");
      strcpy(myArgv[myArgc++], "line=");
      strcpy(myArgv[myArgc++], "3");
      strcpy(myArgv[myArgc++], "samp=");
      strcpy(myArgv[myArgc++], "3");

      Isis::UserInterface ui(QString::fromStdString(highpass), myArgc, myArgv);
      vector<QString> vals;
      cout << "FROM:    " << ui.GetAsString("FROM").toStdString() << endl;
      cout << "TO:      " << endl;
      ui.GetAsString("TO", vals);

      for(unsigned int i = 0; i < vals.size(); i++) {
        cout << " >> '" << vals[i].toStdString() << "'" << endl;
      }
      cout << endl;
      cout << "GUI:     " << ui.IsInteractive() << endl;
      cout << endl;
    }


    cout << "Testing Complicated Array Argument" << endl;
    {
      int myArgc = 0;
      strcpy(myArgv[myArgc++], "highpass");
      strcpy(myArgv[myArgc++], "from=dog");
      strcpy(myArgv[myArgc++], "to=(biscuit\\\\,,'bread,',\",b,\\\\,iscuit2,\"\\,,)");
      strcpy(myArgv[myArgc++], "line=");
      strcpy(myArgv[myArgc++], "3");
      strcpy(myArgv[myArgc++], "samp=");
      strcpy(myArgv[myArgc++], "3");

      Isis::UserInterface ui(QString::fromStdString(highpass), myArgc, myArgv);
      vector<QString> vals;
      cout << "FROM:    " << ui.GetAsString("FROM").toStdString() << endl;
      cout << "TO:      " << endl;
      ui.GetAsString("TO", vals);

      for(unsigned int i = 0; i < vals.size(); i++) {
        cout << " >> " << vals[i].toStdString() << endl;
      }
      cout << endl;
      cout << "GUI:     " << ui.IsInteractive() << endl;
      cout << endl;
    }


    cout << "Testing Escaped Array \\(" << endl;
    {
      int myArgc = 0;
      strcpy(myArgv[myArgc++], "./unitTest");
      strcpy(myArgv[myArgc++], "to=\\(escaped, argument)");

      Isis::UserInterface ui(QString::fromStdString(unitTestXml), myArgc, myArgv);
      vector<QString> vals;
      cout << "FROM:    " << ui.GetAsString("FROM").toStdString() << endl;
      cout << "TO:      " << endl;
      ui.GetAsString("TO", vals);
      for(unsigned int i = 0; i < vals.size(); i++) {
        cout << " >> " << vals[i].toStdString() << endl;
      }
      cout << endl;
    }


    cout << "Testing Escaped Array \\\\(" << endl;
    {
      int myArgc = 0;
      strcpy(myArgv[myArgc++], "./unitTest");
      strcpy(myArgv[myArgc++], "to=\\\\(escaped, argument)");

      Isis::UserInterface ui(QString::fromStdString(unitTestXml), myArgc, myArgv);
      vector<QString> vals;
      cout << "FROM:    " << ui.GetAsString("FROM").toStdString() << endl;
      cout << "TO:      " << endl;
      ui.GetAsString("TO", vals);
      for(unsigned int i = 0; i < vals.size(); i++) {
        cout << " >> " << vals[i].toStdString() << endl;
      }
      cout << endl;
    }


    cout << "Testing param = value Format" << endl;
    {
      int myArgc = 0;
      strcpy(myArgv[myArgc++], "highpass");
      strcpy(myArgv[myArgc++], "from");
      strcpy(myArgv[myArgc++], "=");
      strcpy(myArgv[myArgc++], "dog");
      strcpy(myArgv[myArgc++], "to");
      strcpy(myArgv[myArgc++], "=");
      strcpy(myArgv[myArgc++], "bread");
      strcpy(myArgv[myArgc++], "line");
      strcpy(myArgv[myArgc++], "=");
      strcpy(myArgv[myArgc++], "3");
      strcpy(myArgv[myArgc++], "samp");
      strcpy(myArgv[myArgc++], "=");
      strcpy(myArgv[myArgc++], "3");

      Isis::UserInterface ui(QString::fromStdString(highpass), myArgc, myArgv);
      cout << "FROM:    " << ui.GetAsString("FROM").toStdString() << endl;
      cout << "TO:      " << ui.GetAsString("TO").toStdString() << endl;
      cout << "GUI:     " << ui.IsInteractive() << endl;
      cout << endl;
    }


    cout << "Testing Space in Parameter Value" << endl;
    {
      int myArgc = 0;
      strcpy(myArgv[myArgc++], "unitTest");
      strcpy(myArgv[myArgc++], "from=input file.cub");
      strcpy(myArgv[myArgc++], "to=output.cub");

      Isis::UserInterface ui(QString::fromStdString(unitTestXml), myArgc, myArgv);
      cout << "FROM:    " << ui.GetAsString("FROM").toStdString() << endl;
      cout << "TO:      " << ui.GetAsString("TO").toStdString() << endl;
      cout << "GUI:     " << ui.IsInteractive() << endl;
      cout << endl;
    }


    cout << "Testing =value" << endl;
    try {
      int myArgc = 0;
      strcpy(myArgv[myArgc++], "unitTest");
      strcpy(myArgv[myArgc++], "=input.cub");
      strcpy(myArgv[myArgc++], "to");
      strcpy(myArgv[myArgc++], "=output.cub");

      Isis::UserInterface ui(QString::fromStdString(unitTestXml), myArgc, myArgv);
      cout << endl;
    }
    catch (Isis::IException &e) {
      e.print();
      cout << endl;
    }


    cout << "Testing param =value" << endl;
    try {
      int myArgc = 0;
      strcpy(myArgv[myArgc++], "highpass");
      strcpy(myArgv[myArgc++], "from=dog");
      strcpy(myArgv[myArgc++], "to");
      strcpy(myArgv[myArgc++], "=bread");
      strcpy(myArgv[myArgc++], "line");
      strcpy(myArgv[myArgc++], "=");
      strcpy(myArgv[myArgc++], "3");
      strcpy(myArgv[myArgc++], "samp");
      strcpy(myArgv[myArgc++], "=");
      strcpy(myArgv[myArgc++], "3");

      Isis::UserInterface ui(QString::fromStdString(highpass), myArgc, myArgv);
      cout << endl;
   }
    catch (Isis::IException &e) {
      e.print();
      cout << endl;
    }


    cout << "Testing mismatched quotes for array-value" << endl;
    try {
      int myArgc = 0;
      strcpy(myArgv[myArgc++], "./unitTest");
      strcpy(myArgv[myArgc++], "from=(\"hello)");

      Isis::UserInterface ui(QString::fromStdString(unitTestXml), myArgc, myArgv);
      cout << endl;
    }
    catch (Isis::IException &e) {
      e.print();
      cout << endl;
    }


    cout << "Testing array-value ending in backslash" << endl;
    try {
      int myArgc = 0;
      strcpy(myArgv[myArgc++], "./unitTest");
      strcpy(myArgv[myArgc++], "from=(hello)\\");

      Isis::UserInterface ui(QString::fromStdString(unitTestXml), myArgc, myArgv);
      cout << endl;
    }
    catch (Isis::IException &e) {
      e.print();
      cout << endl;
    }


    cout << "Testing Invalid Parameter" << endl;
    try {
      int myArgc = 0;
      strcpy(myArgv[myArgc++], "$ISISROOT/bin/highpass/highpass");
      strcpy(myArgv[myArgc++], "bogus=parameter");

      Isis::UserInterface ui(QString::fromStdString(unitTestXml), myArgc, myArgv);
      cout << endl;
    }
    catch (Isis::IException &e) {
      e.print();
      cout << endl;
    }


    cout << "Testing Invalid Reserved Parameter" << endl;
    try {
      int myArgc = 0;
      strcpy(myArgv[myArgc++], "./unitTest");
      strcpy(myArgv[myArgc++], "-lastt");

      Isis::UserInterface ui(QString::fromStdString(unitTestXml), myArgc, myArgv);
      cout << endl;
    }
    catch (Isis::IException &e) {
      e.print();
      cout << endl;
    }


    cout << "Testing Reserved Parameter=Invalid Value" << endl;
    try {
      int myArgc = 0;
      strcpy(myArgv[myArgc++], "./unitTest");
      strcpy(myArgv[myArgc++], "-verbose=(\"invalid\", \"value\")");

      Isis::UserInterface ui(QString::fromStdString(unitTestXml), myArgc, myArgv);
      cout << endl;

    }
    catch (Isis::IException &e) {
      e.print();
      cout << endl;
    }


    cout << "Testing Unambiguous Reserved Parameter Resolution (-la)" << endl;
    {
      int myArgc = 0;
      strcpy(myArgv[myArgc++], "./unitTest");
      strcpy(myArgv[myArgc++], "-la");

      Isis::UserInterface ui(QString::fromStdString(unitTestXml), myArgc, myArgv);
      cout << "FROM:    " << ui.GetAsString("FROM").toStdString() << endl;
      cout << "TO:      " << ui.GetAsString("TO").toStdString() << endl;
      cout << "GUI:     " << ui.IsInteractive() << endl;
      cout << endl;
    }


    cout << "Testing Ambiguous Reserved Parameter Resolution" << endl;
    try {
      int myArgc = 0;
      strcpy(myArgv[myArgc++], "./unitTest");
      strcpy(myArgv[myArgc++], "-l");

      Isis::UserInterface ui(QString::fromStdString(unitTestXml), myArgc, myArgv);
      cout << endl;
    }
    catch (Isis::IException &e) {
      e.print();
      cout << endl;
    }


    cout << "Testing unitTest v. ./unitTest for GUI" << endl;
    {
      int myArgc = 0;
      strcpy(myArgv[myArgc++], "unitTest");

      Isis::UserInterface ui(QString::fromStdString(unitTestXml), myArgc, myArgv);
      cout << "GUI:     " << ui.IsInteractive() << endl;
      cout << endl;
    }

    {
      int myArgc = 0;
      strcpy(myArgv[myArgc++], "./unitTest");

      Isis::UserInterface ui(QString::fromStdString(unitTestXml), myArgc, myArgv);
      cout << "GUI:     " << ui.IsInteractive() << endl;
      cout << endl;
    }


    cout << "Testing -PID and -GUI" << endl;
    try {
      int myArgc = 0;
      strcpy(myArgv[myArgc++], "./unitTest");
      strcpy(myArgv[myArgc++], "-pid=1");
      strcpy(myArgv[myArgc++], "-gui");

      Isis::UserInterface ui(QString::fromStdString(unitTestXml), myArgc, myArgv);
      cout << endl;
    }
    catch (Isis::IException &e) {
      e.print();
      cout << endl;
    }


    cout << "Testing ParentId() and TheGui()" << endl;
    {
      int myArgc = 0;
      strcpy(myArgv[myArgc++], "./unitTest");

      Isis::UserInterface ui(QString::fromStdString(unitTestXml), myArgc, myArgv);
      ui.ParentId();
      ui.TheGui();
    }


    cout << "Testing -NOGUI" << endl;
    {
      int myArgc = 0;
      strcpy(myArgv[myArgc++], "./unitTest");
      strcpy(myArgv[myArgc++], "-nogui");

      Isis::UserInterface ui(QString::fromStdString(highpass), myArgc, myArgv);
      cout << "GUI:     " << ui.IsInteractive() << endl;
      cout << endl;
    }


    cout << "Starting Batchlist Test" << endl;
    {
      int myArgc = 0;
      strcpy(myArgv[myArgc++], "unitTest");
      strcpy(myArgv[myArgc++], "from=$1");
      strcpy(myArgv[myArgc++], "to=$2");
      strcpy(myArgv[myArgc++], "-batchlist=unitTest.lis");

      Isis::UserInterface ui(QString::fromStdString(unitTestXml), myArgc, myArgv);
      for(int i = 0; i < ui.BatchListSize(); i++) {
        ui.SetBatchList(i);
        cout << "FROM:    " << ui.GetAsString("FROM").toStdString() << endl;
        cout << "TO:      " << ui.GetAsString("TO").toStdString() << endl;
        cout << "GUI:     " << ui.IsInteractive() << endl;
        cout << endl;
      }
      cout << "Finished Batchlist Test" << endl;
      cout << endl;
    }


    // The following four tests should all catch thrown exceptions -
    // -BATCHLIST cannot be used with -GUI, -SAVE, -RESTORE, or -LAST
    cout << "Testing -BATCHLIST with -GUI" << endl;
    try {
      int myArgc = 0;
      strcpy(myArgv[myArgc++], "./unitTest");
      strcpy(myArgv[myArgc++], "-gui");
      strcpy(myArgv[myArgc++], "from=$1");
      strcpy(myArgv[myArgc++], "to=$2");
      strcpy(myArgv[myArgc++], "-batchlist=unitTest.lis");

      Isis::UserInterface ui(QString::fromStdString(unitTestXml), myArgc, myArgv);
      cout << endl;
    }
    catch (Isis::IException &e) {
      e.print();
      cout << endl;
    }

    cout << "Testing -BATCHLIST with -SAVE" << endl;
    try {
      int myArgc = 0;
      strcpy(myArgv[myArgc++], "./unitTest");
      strcpy(myArgv[myArgc++], "-save");
      strcpy(myArgv[myArgc++], "from=$1");
      strcpy(myArgv[myArgc++], "to=$2");
      strcpy(myArgv[myArgc++], "-batchlist=unitTest.lis");

      Isis::UserInterface ui(QString::fromStdString(unitTestXml), myArgc, myArgv);
      cout << endl;
    }
    catch (Isis::IException &e) {
      e.print();
      cout << endl;
    }

    cout << "Testing -BATCHLIST with -RESTORE" << endl;
    try {
      int myArgc = 0;
      strcpy(myArgv[myArgc++], "./unitTest");
      strcpy(myArgv[myArgc++], "-restore=unitTest.par");
      strcpy(myArgv[myArgc++], "from=$1");
      strcpy(myArgv[myArgc++], "to=$2");
      strcpy(myArgv[myArgc++], "-batchlist=unitTest.lis");

      Isis::UserInterface ui(QString::fromStdString(unitTestXml), myArgc, myArgv);
      cout << endl;
    }
    catch (Isis::IException &e) {
      e.print();
      cout << endl;
    }

    cout << "Testing -BATCHLIST with -LAST" << endl;
    try {
      int myArgc = 0;
      strcpy(myArgv[myArgc++], "./unitTest");
      strcpy(myArgv[myArgc++], "-last");
      strcpy(myArgv[myArgc++], "from=$1");
      strcpy(myArgv[myArgc++], "to=$2");
      strcpy(myArgv[myArgc++], "-batchlist=unitTest.lis");

      Isis::UserInterface ui(QString::fromStdString(unitTestXml), myArgc, myArgv);
      cout << endl;
    }
    catch (Isis::IException &e) {
      e.print();
      cout << endl;
    }

    cout << "Testing -BATCHLIST with nonexistent .lis file" << endl;
    try {
      int myArgc = 0;
      strcpy(myArgv[myArgc++], "./unitTest");
      strcpy(myArgv[myArgc++], "-batchlist=doesntExist.lis");

      Isis::UserInterface ui(QString::fromStdString(unitTestXml), myArgc, myArgv);
      cout << endl;
    }
    catch (Isis::IException &e) {
      e.print();
      cout << endl;
    }


    cout << "Testing -BATCHLIST with empty .lis file" << endl;
    try {
      int myArgc = 0;
      strcpy(myArgv[myArgc++], "./unitTest");
      strcpy(myArgv[myArgc++], "-batchlist=unitTestEmpty.lis");

      Isis::UserInterface ui(QString::fromStdString(unitTestXml), myArgc, myArgv);
      cout << endl;
    }
    catch (Isis::IException &e) {
      e.print();
      cout << endl;
    }


    cout << "Testing -BATCHLIST with mismatched columns in .lis file" << endl;
    try {
      int myArgc = 0;
      strcpy(myArgv[myArgc++], "./unitTest");
      strcpy(myArgv[myArgc++], "-batchlist=unitTestBadColumns.lis");

      Isis::UserInterface ui(QString::fromStdString(unitTestXml), myArgc, myArgv);
      cout << endl;
    }
    catch (Isis::IException &e) {
      e.print();
      cout << endl;
    }


    cout << "Testing -ONERROR=CONTINUE" << endl;
    {
      int myArgc = 0;
      strcpy(myArgv[myArgc++], "./unitTest");
      strcpy(myArgv[myArgc++], "-batchlist=unitTest.lis");
      strcpy(myArgv[myArgc++], "-onerror=continue");

      Isis::UserInterface ui(QString::fromStdString(unitTestXml), myArgc, myArgv);
      cout << "AbortOnError() returns: " << ui.AbortOnError() << endl;
      cout << endl;
    }


    cout << "Testing -ONERROR=ABORT" << endl;
    {
      int myArgc = 0;
      strcpy(myArgv[myArgc++], "./unitTest");
      strcpy(myArgv[myArgc++], "-batchlist=unitTest.lis");
      strcpy(myArgv[myArgc++], "-onerror=abort");

      Isis::UserInterface ui(QString::fromStdString(unitTestXml), myArgc, myArgv);
      cout << "AbortOnError() returns: " << ui.AbortOnError() << endl;
      cout << endl;
    }


    cout << "Testing -ONERROR=badValue" << endl;
    try {
      int myArgc = 0;
      strcpy(myArgv[myArgc++], "./unitTest");
      strcpy(myArgv[myArgc++], "-batchlist=unitTest.lis");
      strcpy(myArgv[myArgc++], "-onerror=badValue");

      Isis::UserInterface ui(QString::fromStdString(unitTestXml), myArgc, myArgv);
      cout << endl;
    }
    catch (Isis::IException &e) {
      e.print();
      cout << endl;
    }


    cout << "Testing -ONERROR=CONTINUE without -BATCHLIST" << endl;
    try {
      int myArgc = 0;
      strcpy(myArgv[myArgc++], "./unitTest");
      strcpy(myArgv[myArgc++], "-onerror=continue");

      Isis::UserInterface ui(QString::fromStdString(unitTestXml), myArgc, myArgv);
      cout << endl;
      }
    catch (Isis::IException &e) {
      e.print();
      cout << endl;
    }


    cout << "Testing -ERRLIST=value without -BATCHLIST" << endl;
    try {
      int myArgc = 0;
      strcpy(myArgv[myArgc++], "./unitTest");
      strcpy(myArgv[myArgc++], "-errlist=unitTest.txt");

      Isis::UserInterface ui(QString::fromStdString(unitTestXml), myArgc, myArgv);
      cout << endl;
    }
    catch (Isis::IException &e) {
      e.print();
      cout << endl;
    }


    cout << "Testing -ERRLIST with no value" << endl;
    try {
      int myArgc = 0;
      strcpy(myArgv[myArgc++], "./unitTest");
      strcpy(myArgv[myArgc++], "-errlist");
      strcpy(myArgv[myArgc++], "-batchlist=unitTest.lis");

      Isis::UserInterface ui(QString::fromStdString(unitTestXml), myArgc, myArgv);
      cout << endl;
    }
    catch (Isis::IException &e) {
      e.print();
      cout << endl;
    }


    cout << "Testing -ERRLIST=value" << endl;
    {
      int myArgc = 0;
      strcpy(myArgv[myArgc++], "./unitTest");
      strcpy(myArgv[myArgc++], "-errlist=unitTestErr.txt");
      strcpy(myArgv[myArgc++], "-batchlist=unitTest.lis");

      Isis::UserInterface ui(QString::fromStdString(unitTestXml), myArgc, myArgv);
      for(int i = 0; i < ui.BatchListSize(); i++) {
        ui.SetBatchList(i);
        ui.SetErrorList(i);
      }
      cout << endl;
    }


    // evaluating -HELP during a unitTest should throw an exception (instead of exiting)
    cout << "Testing -HELP Priority (invalid parameters present)" << endl;
    try {
      int myArgc = 0;
      strcpy(myArgv[myArgc++], "./unitTest");
      strcpy(myArgv[myArgc++], "from=");
      strcpy(myArgv[myArgc++], "test.cub");
      strcpy(myArgv[myArgc++], "-invalid");
      strcpy(myArgv[myArgc++], "-webhelp");
      strcpy(myArgv[myArgc++], "invalid=parameter");
      strcpy(myArgv[myArgc++], "-help");

      Isis::UserInterface ui(QString::fromStdString(unitTestXml), myArgc, myArgv);
      cout << "Evaluating -HELP should have thrown an exception during unit testing" << endl;
      cout << endl;
    }
    catch (Isis::IException &e) {
      e.print();
      cout << endl;
    }


    cout << "Testing -HELP=value ..." << endl;
    cout << endl;
    cout << "Testing pixelType" << endl;
    try {
      int myArgc = 0;
      strcpy(myArgv[myArgc++], "./unitTest");
      strcpy(myArgv[myArgc++], "-help=to");

      Isis::UserInterface ui(QString::fromStdString(unitTestXml), myArgc, myArgv);
      cout << "Evaluating -HELP should have thrown an exception during unit testing" << endl;
      cout << endl;
    }
    catch (Isis::IException &e) {
      e.print();
      cout << endl;
    }

    cout << "Testing inclusive min and max, lessThan, lessThanOrEqual, internalDefault" << endl;
    try {
      int myArgc = 0;
      strcpy(myArgv[myArgc++], "./unitTest");
      strcpy(myArgv[myArgc++], "-help=testone");

      Isis::UserInterface ui(QString::fromStdString(unitTestXml), myArgc, myArgv);
      cout << "Evaluating -HELP should have thrown an exception during unit testing" << endl;
      cout << endl;
    }
    catch (Isis::IException &e) {
      e.print();
      cout << endl;
    }

    cout << "Testing odd, noninclusive min and max, greaterThan, greaterThanOrEqual" << endl;
    try {
      int myArgc = 0;
      strcpy(myArgv[myArgc++], "./unitTest");
      strcpy(myArgv[myArgc++], "-help=testtwo");

      Isis::UserInterface ui(QString::fromStdString(unitTestXml), myArgc, myArgv);
      cout << "Evaluating -HELP should have thrown an exception during unit testing" << endl;
      cout << endl;
    }
    catch (Isis::IException &e) {
      e.print();
      cout << endl;
    }

    cout << "Testing inclusions, exclusions" << endl;
    try {
      int myArgc = 0;
      strcpy(myArgv[myArgc++], "./unitTest");
      strcpy(myArgv[myArgc++], "-help=testthree");

      Isis::UserInterface ui(QString::fromStdString(unitTestXml), myArgc, myArgv);
      cout << "Evaluating -HELP should have thrown an exception during unit testing" << endl;
      cout << endl;
    }
    catch (Isis::IException &e) {
      e.print();
      cout << endl;
    }

   cout << "Testing list inclusions, exclusions, defaults" << endl;
    try {
      int myArgc = 0;
      strcpy(myArgv[myArgc++], "./unitTest");
      strcpy(myArgv[myArgc++], "-help=listtest");

      Isis::UserInterface ui(QString::fromStdString(unitTestXml), myArgc, myArgv);
      cout << "Evaluating -HELP should have thrown an exception during unit testing" << endl;
      cout << endl;
    }
    catch (Isis::IException &e) {
      e.print();
      cout << endl;
    }
    cout << "...End testing -HELP=value" << endl;
    cout << endl;


    cout << "Testing -INFO" << endl;
    {
      int myArgc = 0;
      strcpy(myArgv[myArgc++], "./unitTest");
      strcpy(myArgv[myArgc++], "-info");

      Isis::UserInterface ui(QString::fromStdString(unitTestXml), myArgc, myArgv);
      cout << "GetInfoFlag() returns: " << ui.GetInfoFlag() << endl;
      cout << "GetInfoFileName() returns: " << ui.GetInfoFileName().toStdString() << endl;
      cout << endl;
    }


    cout << "Testing -INFO=value" << endl;
    {
      int myArgc = 0;
      strcpy(myArgv[myArgc++], "./unitTest");
      strcpy(myArgv[myArgc++], "-info=debug.log");

      Isis::UserInterface ui(QString::fromStdString(unitTestXml), myArgc, myArgv);
      cout << "GetInfoFlag() returns: " << ui.GetInfoFlag() << endl;
      cout << "GetInfoFileName() returns: " << ui.GetInfoFileName().toStdString() << endl;
      cout << endl;
    }


    cout << "Testing -LAST" << endl;
    {
      int myArgc = 0;
      strcpy(myArgv[myArgc++], "./unitTest");
      strcpy(myArgv[myArgc++], "-last");

      Isis::UserInterface ui(QString::fromStdString(unitTestXml), myArgc, myArgv);
      cout << "FROM:    " << ui.GetAsString("FROM").toStdString() << endl;
      cout << "TO:      " << ui.GetAsString("TO").toStdString() << endl;
      cout << "GUI:     " << ui.IsInteractive() << endl;
      cout << endl;
    }


    cout << "Testing -LAST with other app parameters" << endl;
    {
      int myArgc = 0;
      strcpy(myArgv[myArgc++], "./unitTest");
      strcpy(myArgv[myArgc++], "from=otherParam");
      strcpy(myArgv[myArgc++], "-last");

      Isis::UserInterface ui(QString::fromStdString(unitTestXml), myArgc, myArgv);
      cout << "FROM:    " << ui.GetAsString("FROM").toStdString() << endl;
      cout << "TO:      " << ui.GetAsString("TO").toStdString() << endl;
      cout << "GUI:     " << ui.IsInteractive() << endl;
      cout << endl;
    }


    cout << "Testing -LOG" << endl;
    {
      Preference &tempTestPrefs = Isis::Preference::Preferences(true);
      int myArgc = 0;
      strcpy(myArgv[myArgc++], "./unitTest");
      strcpy(myArgv[myArgc++], "-log");

      Isis::UserInterface ui(QString::fromStdString(unitTestXml), myArgc, myArgv);
      cout << tempTestPrefs.findGroup("SessionLog")["FileOutput"] << endl;
      cout << tempTestPrefs.findGroup("SessionLog")["FileName"] << endl;
      cout << endl;
    }


    cout << "Testing -LOG=value" << endl;
    {
      Preference &tempTestPrefs = Isis::Preference::Preferences(true);
      int myArgc = 0;
      strcpy(myArgv[myArgc++], "./unitTest");
      strcpy(myArgv[myArgc++], "-log=unitTest.prt");

      Isis::UserInterface ui(QString::fromStdString(unitTestXml), myArgc, myArgv);
      cout << tempTestPrefs.findGroup("SessionLog")["FileOutput"] << endl;
      cout << tempTestPrefs.findGroup("SessionLog")["FileName"] << endl;
      cout << endl;
    }


    cout << "Testing -RESTORE with valid (existing) .par file" << endl;
    {
      int myArgc = 0;
      strcpy(myArgv[myArgc++], "./unitTest");
      strcpy(myArgv[myArgc++], "-restore=unitTest.par");

      Isis::UserInterface ui(QString::fromStdString(unitTestXml), myArgc, myArgv);
      cout << "FROM:    " << ui.GetAsString("FROM").toStdString() << endl;
      cout << "TO:      " << ui.GetAsString("TO").toStdString() << endl;
      cout << "GUI:     " << ui.IsInteractive() << endl;
      cout << endl;
    }


    cout << "Testing -RESTORE with corrupt .par file" << endl;
    try {
      int myArgc = 0;
      strcpy(myArgv[myArgc++], "./unitTest");
      strcpy(myArgv[myArgc++], "-restore=unitTestCorrupt.par");

      Isis::UserInterface ui(QString::fromStdString(unitTestXml), myArgc, myArgv);
      cout << endl;
    }
    catch (Isis::IException &e) {
      e.print();
      cout << endl;
    }


    cout << "Testing -RESTORE with invalid (non-existing) .par file" << endl;
    try {
      int myArgc = 0;
      strcpy(myArgv[myArgc++], "$ISISROOT/bin/highpass");
      strcpy(myArgv[myArgc++], "-restore=junk.par");

      Isis::UserInterface ui(QString::fromStdString(unitTestXml), myArgc, myArgv);
      cout << endl;
    }
    catch (Isis::IException &e) {
      e.print();
      cout << endl;
    }


    // testing loadHistory()
    cout << "Testing -RESTORE with an empty .par file" << endl;
    {
      int myArgc = 0;
      strcpy(myArgv[myArgc++], "./unitTest");
      strcpy(myArgv[myArgc++], "-restore=unitTestEmpty.par");

      Isis::UserInterface ui(QString::fromStdString(unitTestXml), myArgc, myArgv);
      cout << endl;
    }


    // unitTestLoadHistory.par has more object groups to test loadHistory()
    cout << "Testing -RESTORE with a more populated .par file" << endl;
    {
      int myArgc = 0;
      strcpy(myArgv[myArgc++], "./unitTest");
      strcpy(myArgv[myArgc++], "-restore=unitTestLoadHistory.par");

      Isis::UserInterface ui(QString::fromStdString(unitTestXml), myArgc, myArgv);
      cout << endl;
    }


    // TestPreferences for unit tests have HistoryRecording set to Off
    cout << "Testing -SAVE with HistoryRecording Off" << endl;
    {
      int myArgc = 0;
      strcpy(myArgv[myArgc++], "./unitTest");
      strcpy(myArgv[myArgc++], "-save=unitTestSaveArgs.par");

      Isis::UserInterface ui(QString::fromStdString(unitTestXml), myArgc, myArgv);
      ui.SaveHistory();
      cout << endl;
    }


    cout << "Starting -SAVE, -PREFERECE, and -RESTORE Test" << endl;
    {
      cout << "Testing -SAVE=value with HistoryRecording On" << endl;
      int myArgc = 0;
      strcpy(myArgv[myArgc++], "./unitTest");
      strcpy(myArgv[myArgc++], "from=saveParam");
      strcpy(myArgv[myArgc++], "to=works");
      strcpy(myArgv[myArgc++], "-save=unitTestSave.par");
      strcpy(myArgv[myArgc++], "-preference=unitTestPrefs");

      Isis::UserInterface ui(QString::fromStdString(unitTestXml), myArgc, myArgv);
      cout << "FROM:    " << ui.GetAsString("FROM").toStdString() << endl;
      cout << "TO:      " << ui.GetAsString("TO").toStdString() << endl;
      cout << "GUI:     " << ui.IsInteractive() << endl;
      cout << endl;
      ui.SaveHistory();

      cout << "Restoring Saved Parameters:" << endl;
      myArgc = 0;
      strcpy(myArgv[myArgc++], "./unitTest");
      strcpy(myArgv[myArgc++], "-restore=unitTestSave.par");

      Isis::UserInterface ui2(QString::fromStdString(unitTestXml), myArgc, myArgv);
      cout << "FROM:    " << ui2.GetAsString("FROM").toStdString() << endl;
      cout << "TO:      " << ui2.GetAsString("TO").toStdString() << endl;
      cout << "GUI:     " << ui2.IsInteractive() << endl;
      cout << endl;

      cout << "Finished -SAVE, PREFERENCE, and -RESTORE Test" << endl;
      cout << endl;
    }


    cout << "Testing SetBatchList()..." << endl;
    {
      cout << "Testing with param=array-value" << endl;
      int myArgc = 0;
      strcpy(myArgv[myArgc++], "./unitTest");
      strcpy(myArgv[myArgc++], "from=$$1");
      strcpy(myArgv[myArgc++], "to=($2,$2copy)");
      strcpy(myArgv[myArgc++], "-batchlist=unitTest.lis");

      Isis::UserInterface ui(QString::fromStdString(unitTestXml), myArgc, myArgv);
      ui.SetBatchList(0);
      cout << endl;

      cout << "Testing with param= " << endl;
      myArgc = 0;
      strcpy(myArgv[myArgc++], "./unitTest");
      strcpy(myArgv[myArgc++], "from=$1");
      strcpy(myArgv[myArgc++], "to= ");
      strcpy(myArgv[myArgc++], "-batchlist=unitTest.lis");

      Isis::UserInterface ui2(QString::fromStdString(unitTestXml), myArgc, myArgv);
      ui2.SetBatchList(0);
      cout << endl;
    }
    cout << "...End SetBatchList() Test" << endl;
    cout << endl;


    cout << "Testing SetErrorList() with p_errList == \"\"" << endl;
    {
      int myArgc = 0;
      strcpy(myArgv[myArgc++], "./unitTest");

      Isis::UserInterface ui(QString::fromStdString(unitTestXml), myArgc, myArgv);
      ui.SetErrorList(0);
      cout << endl;
    }


    cout << "Testing -VERBOSE" << endl;
    {
      Preference &tempTestPrefs = Isis::Preference::Preferences(true);
      int myArgc = 0;
      strcpy(myArgv[myArgc++], "./unitTest");
      strcpy(myArgv[myArgc++], "-verbose");

      Isis::UserInterface ui(QString::fromStdString(unitTestXml), myArgc, myArgv);
      cout << tempTestPrefs.findGroup("SessionLog")["TerminalOutput"] << endl;
      cout << endl;
    }


    // evaluating -webhelp should throw an error during unit test (instead of exiting)
    cout << "Testing -WEBHELP" << endl;
    try {
      int myArgc = 0;
      strcpy(myArgv[myArgc++], "./unitTest");
      strcpy(myArgv[myArgc++], "bogus=parameter");
      strcpy(myArgv[myArgc++], "-webhelp");

      Isis::UserInterface ui(QString::fromStdString(unitTestXml), myArgc, myArgv);
      cout << "Evaluating -WEBHELP should have thrown an exception during unit testing" << endl;
    }
    catch (Isis::IException &e) {
      e.print();
      cout << endl;
    }

  }
  catch (Isis::IException &e) {
    e.print();
  }
}
