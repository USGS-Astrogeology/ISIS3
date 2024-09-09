/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <iostream>
#include <fstream>
#include <thread>
#include <vector>

#include "FileName.h"
#include "IException.h"
#include "Preference.h"

using namespace std;
using namespace Isis;

void TestVersioning(std::string prefix, std::string name, bool containsDate = false);
void TestExpanded(std::string prefix, std::string name);
void TestExtensionChanges(std::string prefix, std::string name, bool showExpandedValues);
void TestGenericAccessors(std::string prefix, std::string name, bool showExpandedValues);

int main(int argc, char *argv[]) {
  Preference::Preferences(true);

  std::vector<std::string> filesToFullTest;
  filesToFullTest.push_back("/path/base.ext+attr");
  filesToFullTest.push_back("/path1/.path2/base.ext+attr");
  filesToFullTest.push_back("/path1/pat.h2/base+attr");
  filesToFullTest.push_back("/.path1/path2/base");
  filesToFullTest.push_back("/.path1/path2/base.+attr");
  filesToFullTest.push_back("/another/path/base.ex1.exten2.ext3");
  filesToFullTest.push_back("/$BADENV/base.ext+attr");
  filesToFullTest.push_back("/.path1/base+attr1+attr2");
  filesToFullTest.push_back("unitTest.cpp");
  filesToFullTest.push_back("./unitTest.cpp");
  filesToFullTest.push_back(".cub");
  filesToFullTest.push_back("/$TEMPORARY/unitTest.cpp");

  for (const std::string &fileToTest : filesToFullTest) {
    cout << "Running Full Test on [" << fileToTest << "]" << endl;
    TestGenericAccessors("\t", fileToTest, true);
    TestExtensionChanges("\t", fileToTest, true);
    TestExpanded("\t", fileToTest);
  }

  // Test temp files thoroughly
  cout << "Testing temporary file name placement" << endl;
  std::string tempFileNameTestStr = "$TEMPORARY/tttt.tmp";

  FileName n = FileName::createTempFile(tempFileNameTestStr);
  std::string prefix = n.name().substr(0, 4);
  std::string suffix = n.name().substr(n.name().size() - 4);
  std::string middle(n.name().size() - 8, '?');
  cout << "\tInput name and extension : " << tempFileNameTestStr << endl;
  cout << "\tExtension:               : " << n.extension() << endl;
  cout << "\tOriginal Path:           : " << n.originalPath() << endl;
  cout << "\tExists:                  : " << n.fileExists() << endl;
  cout << "\tName (cleaned):          : " << prefix + middle + suffix << endl;
  cout << endl;

  // if (!std::remove(n.toString())) {
  //   cout << "\t**Failed to delete temporary file [" << n.toString() << "]**" << endl;
  // }

  {
    cout << "Testing parallel temporary file name creation for atomicity" << endl;
    int numToTest = std::thread::hardware_concurrency() * 20;

    std::vector<std::string> templateFileNames;
    for (int i = 0; i < numToTest; i++)
      templateFileNames.push_back("tttt.tmp");

    std::vector<FileName> results;
    results.reserve(templateFileNames.size());
    std::transform(templateFileNames.begin(), templateFileNames.end(), std::back_inserter(results),
               [](const FileName& fileName) {
                   return FileName::createTempFile(fileName);
               });
    bool success = true;

    while (!results.empty()) {
      FileName result = results.front();
      results.erase(std::remove(results.begin(), results.end(), result), results.end());

      int count = std::count(results.begin(), results.end(), result);

      if (count != 0) {
        cout << "File name: " << result.toString() << " encountered "
             << count << " times" << endl;
        success = false;
      }

      if (!std::filesystem::exists(result.toString())) {
        cout << "File name: " << result.toString() << " encountered does not exists";
        success = false;
      }

      // std::remove(result.toString());
      std::filesystem::remove(result.toString());
    }

    if (success) {
      cout << "\tSuccess!" << endl;
    }
    else {
      cout << "\t**Failed to uniquely create temporary files in parallel**" << endl;
    }

    cout << endl;
  }


  // Test versioning thoroughly
  {
    std::vector<std::string> tempFiles;
    tempFiles.push_back("tttt000001");
    tempFiles.push_back("tttt000001.tmp");
    tempFiles.push_back("tttt000005.tmp");
    tempFiles.push_back("tttt000006.tmp");
    tempFiles.push_back("tttt000008.tmp");
    tempFiles.push_back("1tttt000008.tmp");
    tempFiles.push_back("2tttt000008.tmp");
    tempFiles.push_back("tttt_0.tmp");
    tempFiles.push_back("junk06.tmp");
    tempFiles.push_back("junk09.tmp");
    tempFiles.push_back("tttt05Sep2002.tmp");
    tempFiles.push_back("tttt20Jan2010.tmp");
    tempFiles.push_back("tttt14Apr2010.tmp");
    tempFiles.push_back("ttAprtt22yy99.tmp");
    tempFiles.push_back("ttMartt11yy00.tmp");
    tempFiles.push_back("ttFebtt04yy01.tmp");
    tempFiles.push_back("ttMartt072003.tmp");
    tempFiles.push_back("tt14ttNovember.tmp");
    tempFiles.push_back("tt2ttDecember.tmp");
    tempFiles.push_back("tttt.tmp");
    tempFiles.push_back("Apr-22-99_v001.tmp");
    tempFiles.push_back("Apr-22-99_v004.tmp");
    tempFiles.push_back("Apr-21-99_v009.tmp");

    for (const std::string &fileName : tempFiles) {
      std::ofstream file(fileName, std::ios::out | std::ios::trunc);
      if (!file.is_open()) {
        cout << "Failed to create temporary file for test: " << fileName << endl;
      }
    }


    cout << "Testing Versioning Missing Problems" << endl;
    TestVersioning("\t", "tttt");
    TestVersioning("\t", "tttt{}.tmp");
    TestVersioning("\t", "ttttt{}.tmp");

    cout << "Testing Numerical-Only Versioning" << endl;
    TestVersioning("\t", "tttt??????");
    TestVersioning("\t", "tttt??????.tmp");
    TestVersioning("\t", "tttt_?.tmp");
    TestVersioning("\t", "??tttt");
    TestVersioning("\t", "?tttt000008.tmp");
    TestVersioning("\t", "junk?");
    TestVersioning("\t", "??tttt??");


    cout << "Testing Date-Only Versioning" << endl;
    TestVersioning("\t", "tttt{ddMMMyyyy}.tmp", true);
    TestVersioning("\t", "tt{MMM}tt{dd}yy{yy}.tmp", true);
    TestVersioning("\t", "tt{d}tt{MMM}.tmp", true);
    TestVersioning("\t", "tt{d}tt{MMMM}.tmp", true);
    TestVersioning("\t", "tt{dd}.tmp", true);
    TestVersioning("\t", "tttt{dd}.tmp", true);
    TestVersioning("\t", "tttt{aaaa}.tmp");


    cout << "Testing Date and Numerical Versioning Combined" << endl;
    TestVersioning("\t", "$TEMPORARY/{MMM}-{dd}-{yy}_v???.tmp", true);

    // Test new version for date file.  Makes a file for today, which cannot be
    // printed as truth, so compare it with what we expect and print the result.
    FileName todayFileName("tttt{dd}tt{yyyy}tt{MMM}.tmp");
    cout << "Verifying NewVersion for file " << todayFileName.name() << " is today" << endl;
    bool success = false;
    try {
        todayFileName = todayFileName.newVersion();
        std::time_t now_c = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::tm *now_tm = std::localtime(&now_c);
        std::ostringstream oss;
        oss << "tttt" 
        << std::put_time(now_tm, "%d") << "tt" 
        << std::put_time(now_tm, "%Y") << "tt" 
        << std::put_time(now_tm, "%b") 
        << ".tmp";

        std::string expected = oss.str();
        success = todayFileName.name() == expected;
        cout << "\tMade today's filename successfully? " << success << endl;

      if (!success)
        cout << "\t\tMade: " << todayFileName.name() << "; expected: " << expected
             << endl;
    }
    catch(IException &error) {
      error.print();
    }
    cout << endl;

    for (const auto& fileName : tempFiles) {
        std::filesystem::path filePath(fileName);

        try {
            if (std::filesystem::exists(filePath) && std::filesystem::is_regular_file(filePath)) {
                if (!std::filesystem::remove(filePath)) {
                  std::cout << "Failed to delete temporary file for test: " << fileName << std::endl;
                }
            }
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Error deleting file: " << e.what() << std::endl;
        }
    }
  }

  return 0;
}


void TestVersioning(std::string prefix, std::string name, bool containsDate) {
  cout << prefix << "Testing Versioning Methods [" << name << "]" << endl;

  try {
    FileName test(name);

    try {
      cout << prefix << "\tHighest Version Name:          ";
      cout << test.highestVersion().name() << endl;
      cout << prefix << "\tHighest Version Orig:          ";
      cout << test.highestVersion().original() << endl;
      cout << prefix << "\tHighest Version Orig Path:     ";
      cout << test.highestVersion().originalPath() << endl;
      cout << prefix << "\tHigh version changed FileName: ";
      cout << (test != test.highestVersion()) << endl;
    }
    catch (IException &e) {
      cout << prefix << "\tHighest Version Failed:     " << e.toString() << endl;
    }

    try {
      if (!containsDate) {
        cout << prefix << "\tNew Version Name:              ";
        cout << test.newVersion().name() << endl;
        cout << prefix << "\tNew Version Orig:              ";
        cout << test.newVersion().original() << endl;
        cout << prefix << "\tNew Version Orig Path:         ";
        cout << test.newVersion().originalPath() << endl;
        cout << prefix << "\tNew version changed FileName: ";
        cout << (test != test.newVersion()) << endl;
      }
    }
    catch (IException &e) {
      cout << prefix << "\tNew Version Failed:         " << e.toString() << endl;
    }
  }
  catch(IException &e) {
    cout << prefix << "\tError creating file name: " << e.toString() << endl;
  }

  cout << endl;
}


void TestGenericAccessors(std::string prefix, std::string name, bool showExpandedValues) {
  FileName a(name);

  // Test our assignment & copy construct every time
  FileName b;
  b = a;
  FileName c(b);
  FileName test;
  test = c;

  cout << prefix << "Testing Basics [" << name << "]" << endl;
  cout << prefix << "\tName:              " << test.name() << endl;
  cout << prefix << "\tBase Name:         " << test.baseName() << endl;

  if (showExpandedValues) {
    cout << prefix << "\tPath:              " << test.path() << endl;
  }

  cout << prefix << "\tOriginal path      " << test.originalPath() << endl;

  cout << prefix << "\tExtension:         " << test.extension() << endl;

  cout << prefix << "\tComparison (==):   " << (a == c) << endl;

  cout << prefix << "\tComparison (!=):   " << (a != c) << endl;

  if (showExpandedValues) {
    cout << prefix << "\tExpanded           " << test.expanded() << endl;
  }

  cout << prefix << "\tExists             " << test.fileExists() << endl;
  cout << endl;
}


void TestExtensionChanges(std::string prefix, std::string name, bool showExpandedValues) {
  FileName a(name);

  // Test our assignment & copy construct every time
  FileName b;
  b = a;
  FileName c(b);
  FileName test;
  test = c;

  std::string (FileName::*toStringMethod)() const = &FileName::toString;

  FileName beforeLastChange = test;

  if (!showExpandedValues) {
    toStringMethod = &FileName::original;
  }

  cout << prefix << "Testing Extension change [" << name << "]" << endl;
  cout << prefix << "\tBefore modification:      " << (test.*toStringMethod)() << endl;
  cout << prefix << "\t\tChanged:                " << (beforeLastChange != test) << endl;
  cout << prefix << "\t\tUnchanged:              " << (beforeLastChange == test) << endl;
  beforeLastChange = test;
  test = test.removeExtension();
  cout << prefix << "\tRemoved Extension:        " << (test.*toStringMethod)() << endl;
  cout << prefix << "\t\tChanged:                " << (beforeLastChange != test) << endl;
  cout << prefix << "\t\tUnchanged:              " << (beforeLastChange == test) << endl;
  beforeLastChange = test;
  test = test.addExtension("tmp");
  cout << prefix << "\tAdded Extension [tmp]:    " << (test.*toStringMethod)() << endl;
  cout << prefix << "\t\tChanged:                " << (beforeLastChange != test) << endl;
  cout << prefix << "\t\tUnchanged:              " << (beforeLastChange == test) << endl;
  beforeLastChange = test;
  test = test.addExtension("jpg");
  cout << prefix << "\tAdded Extension [jpg]:    " << (test.*toStringMethod)() << endl;
  cout << prefix << "\t\tChanged:                " << (beforeLastChange != test) << endl;
  cout << prefix << "\t\tUnchanged:              " << (beforeLastChange == test) << endl;
  beforeLastChange = test;
  test = test.addExtension("jpg");
  cout << prefix << "\tAdded Extension [jpg]:    " << (test.*toStringMethod)() << endl;
  cout << prefix << "\t\tChanged:                " << (beforeLastChange != test) << endl;
  cout << prefix << "\t\tUnchanged:              " << (beforeLastChange == test) << endl;
  beforeLastChange = test;
  test = test.setExtension("gif");
  cout << prefix << "\tSet Extension   [gif]:    " << (test.*toStringMethod)() << endl;
  cout << prefix << "\t\tChanged:                " << (beforeLastChange != test) << endl;
  cout << prefix << "\t\tUnchanged:              " << (beforeLastChange == test) << endl;
  beforeLastChange = test;
  test = test.addExtension("jpg");
  cout << prefix << "\tAdded Extension [jpg]:    " << (test.*toStringMethod)() << endl;
  cout << prefix << "\t\tChanged:                " << (beforeLastChange != test) << endl;
  cout << prefix << "\t\tUnchanged:              " << (beforeLastChange == test) << endl;
  beforeLastChange = test;
  test = test.removeExtension();
  cout << prefix << "\tRemoved Extension:        " << (test.*toStringMethod)() << endl;
  cout << prefix << "\t\tChanged:                " << (beforeLastChange != test) << endl;
  cout << prefix << "\t\tUnchanged:              " << (beforeLastChange == test) << endl;
  cout << endl;
}


void TestExpanded(std::string prefix, std::string name) {
  FileName a(name);

  // Test our assignment & copy construct every time
  FileName b;
  b = a;
  FileName c(b);
  FileName test;
  test = c;

  cout << prefix << "Testing Expanded [" << name << "]" << endl;

  cout << prefix << "\tName:              " << test.name() << endl;
  cout << prefix << "\tBase Name:         " << test.baseName() << endl;
  cout << prefix << "\tExtension:         " << test.extension() << endl;
  cout << prefix << "\tOriginal path      " << test.originalPath() << endl;
  cout << prefix << "\tExists             " << test.fileExists() << endl;

  cout << endl;
}
