/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "TextFile.h"
#include "FileName.h"
#include "IException.h"
#include "IString.h"
#include "Preference.h"
#include <iostream>
#include <stdio.h>
#include <vector>
#include <regex>

using namespace std;
using namespace Isis;

int main(int argc, char *argv[]) {
  Isis::Preference::Preferences(true);
  void ReportError(QString err);

  cout << "Unit test for TextFile" << endl << endl;


// ----------------------------------------------------------------------------------

  QString testFile = "$temporary/TextFile.tmp";
  // setup test data
  QString testLines[21];

  // setup line test data
  testLines[0]  = "#   0  zero     line";
  testLines[1]  = "#   1  first    line";
  testLines[2]  = "#   2  second   line";
  testLines[3]  = "   #3  third    line";
  testLines[4]  = "/#  4  fourth   line";
  testLines[5]  = "";
  testLines[6]  = "#";
  testLines[7]  = "//  7  seventh  line";
  testLines[8]  = "//  8  eighth   line";
  testLines[9]  = "/*  9  ninth    line";
  testLines[10] = "/* 10  tenth    line";
  testLines[11] = "/* 11  eleventh line";
  testLines[12] = "/* 12  twelfth  line";
  testLines[13] = "    1    replacement";
  testLines[14] = "    2    replacement";
  testLines[15] = "    3    replacement";
  testLines[16] = "   even line replace";
  testLines[17] = "";
  testLines[18] = "";
  testLines[19] = "";
  testLines[20] = "\0";


  // calc cumulative byte counts for each line - to check TextFile.Size()

  streamsize testLineBytes[21];
  streamsize numBytes = 0;
  streamsize numBytesFiltered = 0;

  vector<QString> testLinesVector;

  for(int i = 0; i <= 19; i++) {
    numBytes += testLines[i].length() + strlen("\n");
    testLineBytes[i] = numBytes;
    testLinesVector.push_back(testLines[i]);
    int locComment = testLines[i].indexOf("#", 0);
    if((locComment != -1) && (locComment != 1)) {
      numBytesFiltered += testLines[i].length() + strlen("\n");
    }
  }

  numBytesFiltered = numBytes - numBytesFiltered;


// ----------------------------------------------------------------------------------

  cout << "1) Create / Overwrite file " << testFile.toStdString() << " with prefilled vector" << endl;

  try {
    Isis::TextFile p(testFile, "overwrite", testLinesVector);     // write file

    if(p.Size() != numBytes) {                                  // test file size
      cout << " *** Failed Size Test WRITE *** " << endl;
      cout << "Calc bytes = " << numBytes << " methodSize = "
           << p.Size() << endl;
    }
  }
  catch(Isis::IException &e) {
    e.print();
  }
  cout << endl;

// ----------------------------------------------------------------------------------

  cout << "2) Read file " << testFile.toStdString() << " into vector" << endl;

  try {
    vector<QString> linesIn;

    // read entire file, filter comments
    Isis::TextFile g(testFile, "input", linesIn);

    int chkVectorSize = 0;                               // chk num bytes read
    for(int i = 0; i < (int) linesIn.size(); i++) {
      chkVectorSize += QString(linesIn[i]).length() + strlen("\n");
    }
    if(chkVectorSize != numBytesFiltered) {
      cout << " *** Failed Size Test Filtered *** " << endl;
      cout << "Calc bytes = " << numBytesFiltered << " Vector Size = "
           << chkVectorSize << endl;
    }
    g.Close();

    linesIn.erase(linesIn.begin(), linesIn.end());

    // read entire file, do not filter comments
    Isis::TextFile g2(testFile, "input", linesIn, 0, false);

    if(g2.Size() != numBytes) {                          // chk num bytes read
      cout << " *** Failed Size Test READ *** " << endl;
      cout << "Calc bytes = " << numBytes << " methodSize = "
           << g2.Size() << endl;
    }

    for(int i = 0; i <= 19; i++) {                       // compare data read to orig data
      if(linesIn[i] != testLines[i]) {
        cout << " *** Failed IString Comparison Test *** " << endl;
        cout << i
             << " Original IString =>" << testLines[i].toStdString()
             << "<= Vector read =>"   << linesIn[i].toStdString()
             << "<=" << endl;
      }
    }
  }
  catch(Isis::IException &e) {
    e.print();
  }
  cout << endl;


// ----------------------------------------------------------------------------------

  cout << "3) Create / Overwrite file " << testFile.toStdString() << " with prefilled QString array" << endl;

  try {
    // write first four lines
    Isis::TextFile p(testFile, "overwrite", testLines, 4);

    if(p.Size() != testLineBytes[4]) {                   // chk num bytes
      cout << " *** Failed Size Test WRITE sense NULL in QString array*** " << endl;
      cout << "Calc bytes = " << testLineBytes[4] << " methodSize = "
           << p.Size() << endl;
    }
  }
  catch(Isis::IException &e) {
    e.print();
  }
  cout << endl;


// ----------------------------------------------------------------------------------

  cout << "4) Read file " << testFile.toStdString() << " into QString array" << endl;

  try {
    // prefill QString array to nonnull, set last element to null
    // the read file call below will not specify howmany lines to read
    // so TextFile will stop filling QString array when it encounters a null in output array

    QString linesIn[4] = {" ", " ", " ", ""};

    Isis::TextFile g(testFile, "input", linesIn, 0, false); // read entire file unfiltered

    for(int i = 0; i <= 2; i++) {                         // chk orig data against data read
      if(linesIn[i] != testLines[i]) {
        cout << " *** Failed Compare Test READ sense NULL in QString array*** " << endl;
        break;
      }
    }
  }
  catch(Isis::IException &e) {
    e.print();
  }
  cout << endl;


// ----------------------------------------------------------------------------------

  cout << "5) Overwrite file " << testFile.toStdString() << endl;

  try {                                                  // open file, will truncate
    Isis::TextFile f(testFile, "OverWrite");
  }
  catch(Isis::IException &e) {
    e.print();
  }
  cout << endl;


// ----------------------------------------------------------------------------------

  cout << "6) Overwrite file and write 6 lines $temporary/TextFile.tmp" << endl;

  try {
    Isis::TextFile f(testFile, "OverWrite");               // open overwrite

    f.SetNewLine("");                                    // turn off append new line
    f.SetComment("");                                    // turn off comment character

    f.PutLine("#   0  zero     line\n");                 // char output

    f.SetNewLine();                                      // reset default append new line
    f.SetComment();                                      // reset default comment QString

    f.PutLineComment("   1  first    line");             // char output
    f.PutLineComment("   2  second   line");             // char output

    f.PutLine(testLines[3]);
    // line count
    if(f.LineCount() != 4) {
      cout << " *** Failed Line Count = 4 *** " << endl;
      cout << " methodLineCount = " << f.LineCount() << endl;
    }
    // byte count
    if(f.Size() != testLineBytes[3]) {
      cout << " *** Failed Size Test After Line 4 *** " << endl;
      cout << "Calc bytes = " << testLineBytes[3] << " methodSize = "
           << f.Size() << endl;
    }

    f.PutLine(testLines[4]);

    f.PutLine();                                         // output blank line

    if(f.LineCount() != 6) {                             // line count
      cout << " *** Failed Line Count = 6 *** " << endl;
      cout << " methodLineCount = " << f.LineCount() << endl;
    }

    f.PutLineComment();                                  // output blank comment line

    if(f.Size() != testLineBytes[6]) {                   // byte count
      cout << " *** Failed Size Test After Line 7 *** " << endl;
      cout << "Calc bytes = " << testLineBytes[6] << " methodSize = "
           << f.Size() << endl;
    }
  }
  catch(Isis::IException &e) {
    e.print();
  }
  cout << endl;


// ----------------------------------------------------------------------------------

  cout << "7) Append 6 lines to file $temporary/TextFile.tmp" << endl;

  try {
    Isis::TextFile f(testFile, "Append");
    // append lines 7 & 8
    f.PutLine(testLines[7]);
    f.PutLine(testLines[8]);

    if(f.LineCount() != 9) {                             // line count
      cout << " *** Failed Line Count = 9 *** " << endl;
      cout << " methodLineCount = " << f.LineCount() << endl;
    }

    if(f.Size() != testLineBytes[8]) {                   // byte count
      cout << " *** Failed Size Test After Line 9 *** " << endl;
      cout << "Calc bytes = " << testLineBytes[8] << " methodSize = "
           << f.Size() << endl;
    }

    f.PutLine(testLines[9]);                             // append line 9

    if(f.Size() != testLineBytes[9]) {                   // byte count
      cout << " *** Failed Size Test After Line 9 *** " << endl;
      cout << "Calc bytes = " << testLineBytes[9] << " methodSize = "
           << f.Size() << endl;
    }

    for(int i = 10; i <= 12; i++) {                      // append lines 10 -> 12
      f.PutLine(testLines[i]);
    }
  }
  catch(Isis::IException &e) {
    e.print();
  }
  cout << endl;


// ----------------------------------------------------------------------------------

  cout << "8) Input (read) file $temporary/TextFile.tmp" << endl;

  try {
    Isis::TextFile f(testFile, "Input");

    QString line;

    for(int i = 0; i <= 12; i++) {      // chk each file line against internal array
      f.GetLineNoFilter(line);
      if(line != testLines[i]) {
        cout << " *** Failed Compare Input Array Line: " << i << " *** " << endl;
      }
    }

    f.Rewind();                                           // set input ptr back to beginning
    f.SetComment();                                       // set comment chk, default is '#'

    f.GetLine(line);
    // first 3 lines are commented
    // so should return fourth line
    if(line != testLines[4]) {
      cout << " *** Failed Ignore comment lines *** " << endl;
      cout << "should be:   =>" << testLines[4].toStdString() << "<=" << endl;
      cout << "returned is: =>" << line.toStdString()         << "<=" << endl;
    }
    if(f.LineCount() != 13) {                             // total line count should be 12
      cout << " *** Failed Line Count = 13 *** " << endl;
      cout << " methodLineCount = " << f.LineCount() << endl;
    }

    if(f.Size() != testLineBytes[12]) {                   // byte count
      cout << " *** Failed Size Test with Lines = 12 *** " << endl;
      cout << "Calc bytes = " << testLineBytes[12] << " methodSize = "
           << f.Size() << endl;
    }

    f.Rewind();                                           // set input ptr back to beginning
    f.SetComment("/*");                                   // set comment to '/*'

    QString lastLine;

    while(f.GetLine(line)) {                              // read file's 12 lines
      lastLine = line;
    }
    if(lastLine != testLines[8]) {                        // line 8 was last non '/*' line read
      cout << " *** Failed To see last 4 lines as comments *** " << endl;
    }

    f.Rewind();                                           // set input ptr back to beginning
    f.SetComment();                                       // set comment to default #
    f.GetLineNoFilter(line);                              // should return line 1
    if(line != testLines[0]) {
      cout << " *** Failed Read Do Not Skip Comment lines *** " << endl;
    }

    f.Rewind();                                           // reset input ptr to beginning
    while(f.GetLine(line)) {                              // read file's 12 lines
      lastLine = line;
    }
    if(lastLine != testLines[12]) {                       // line 12 does not begin with '#'
      cout << " *** Failed Read to end of file *** " << endl;
    }
  }
  catch(Isis::IException &e) {
    e.print();
  }
  cout << endl;


#if 0
// Jeff Anderson removed this test.  It seems strange that you would want
// to open a file for append and then rewind it.  This test used to pass
// but didn't when we went to a newer operating system.  Again saying
// you want to open append and then rewind is lame.
// ----------------------------------------------------------------------------------

  cout << "9) Replace Lines and Verify Replacement in $temporary/TextFile.tmp" << endl;


  try {
    Isis::TextFile f(testFile, "Append");

    // although opened append,
    f.Rewind();                                          //  can set ptr to beginning

    for(int i = 1; i <= 3; i++) {
      f.PutLine(testLines[i+12]);                        // replace lines 1 to 3
    }

    f.Rewind();                                          // set ptr back to begining

    QString(line);
    // re-read file to line 3
    for(int i = 1; i <= 3; i++) {                        //  to chk replacement
      f.GetLineNoFilter(line);
      if(line != testLines[i+12]) {
        cout << " *** Failed Read Replaced line: " << i << " *** " << endl;
      }
    }

    f.Rewind();                                          // set ptr back to begining

    // replace even lines
    for(int i = 1; i <= 6; i++) {
      f.GetLineNoFilter();                               // moves ptr to next line
      f.PutLine(testLines[16]);                          // replaces line
    }

    f.Rewind();                                          // set ptr back to begining

    // re-read file to line 2
    for(int i = 1; i <= 2; i++) {
      f.GetLineNoFilter(line);
    }                                                    // chk line 2 was replaced
    if(line != testLines[16]) {
      cout << " *** Failed Read Replaced lines *** " << endl;
    }
  }
  catch(Isis::IException &e) {
    e.print();
  }
  cout << endl;


// ----------------------------------------------------------------------------------
#endif

  cout << "10) Trigger Error messages" << endl;

  cout << "  a) Try to open non-existent file" << endl;

  testFile = "$temporary/NoSuchDir/TextFile.tmp";

  try {
    Isis::TextFile f(testFile, "Input");
  }
  catch(Isis::IException &e) {
    ReportError(QString::fromStdString(e.toString()));
  }
  cout << endl;


  cout << "  b) Try open as output to pre-existing file" << endl;

  testFile = "$temporary/TextFile.tmp";

  try {
    Isis::TextFile f(testFile, "Output");
  }
  catch(Isis::IException &e) {
    ReportError(QString::fromStdString(e.toString()));
  }
  cout << endl;


  cout << "  c) Open file with bad open mode" << endl;

  try {
    Isis::TextFile fxText(testFile, "xxxInputxxx");
  }
  catch(Isis::IException &e) {
    ReportError(QString::fromStdString(e.toString()));
  }
  cout << endl;


  cout << "  d) Try to write to Input - Read Only file" << endl;

  try {
    Isis::TextFile f(testFile, "Input");
    f.PutLine("Line 1");
  }
  catch(Isis::IException &e) {
    ReportError(QString::fromStdString(e.toString()));
  }
  cout << endl;


  cout << "  e) Try to Write to a closed file" << endl;

  try {
    Isis::TextFile f(testFile, "append");
    f.Close();
    f.PutLine("Line 1");
  }
  catch(Isis::IException &e) {
    ReportError(QString::fromStdString(e.toString()));
  }
  cout << endl;


  cout << "  f) Try to Read from a closed file" << endl;

  try {
    Isis::TextFile f(testFile, "input");
    f.Close();
    f.GetLine();
  }
  catch(Isis::IException &e) {
    ReportError(QString::fromStdString(e.toString()));
  }
  cout << endl;

  // create file that doesn't end in a newline and then test GetLine
  FILE *fp;
  FileName testFileName(testFile.toStdString());
  fp = fopen(testFileName.expanded().c_str(), "w");
  fprintf(fp, "this file has no newline chars in it!");   //???SEG FAULT HERE!!!!!!!
  fclose(fp);
  TextFile tf;
  tf.Open(testFile);

  QString fileContents = "";
  QString line = "";
  while(tf.GetLine(line, true)) {
    fileContents += line;
    line = "";
  }

  // this is the real test - line shouldn't be wiped even though tf.GetLine's
  // call to getline returned false.
  fileContents += line;

  QString passed = "Failed";
  if(fileContents != "")
    passed = "Passed";

  cout << "testing GetLine for files that do not end in a newline: " << passed.toStdString() << "\n\n";
  cout << fileContents.toStdString() << "\n\n";


// ----------------------------------------------------------------------------------

  cout << "11) Remove temp file -> " << testFile.toStdString() << " <-\n" << endl;

  if(std::remove(testFileName.expanded().c_str())) {                    // cleanup tmp file
    cout << "*** Failed to remove tmp file: " << testFile.toStdString() << endl;
  }
}

/**
 * Reports error messages from Isis:iException without full paths of filenames
 * @param err Error QString of iException
 * @author Jeannie Walldren
 * @internal
 *   @history 2011-08-05 Jeannie Backer - Copied from Cube class.
 */
void ReportError(QString err) {
  std::regex pattern("\\[[^\\]*\\]");
  cout << std::regex_replace(err.toStdString(), pattern, std::string("[]")) << endl;
}

