/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <QDebug>
#include <QFile>
#include <QStringList>

#include "FileName.h"
#include "IException.h"
#include "IString.h"
#include "Preference.h"
#include "Pvl.h"
#include "PvlFlatMap.h"
#include "PvlGroup.h"
#include "PvlFlatMap.h"
#include "PvlObject.h"
#include "TextFile.h"

using namespace Isis;
using namespace std;

static void print(const PvlFlatMap &map);

/**
 * Unit test for PvlFlatMap and PvlContraints classes.
 *
 *
 * @author 2016-02-28 Jeannie Backer
 *
 * @internal
 *   @history 2016-02-28 Jeannie Backer - Original version.
 *  
 *   @todo 2016-02-28 Jeannie Backer - The code coverage software was not
 *         working when this test was originally checked in.
 */
int main() {
  Preference::Preferences(true);

  // Create a PvlObject to map
  PvlObject pvlObject("Beasts");
  pvlObject += PvlKeyword("CAT", "Meow");
  pvlObject += PvlKeyword("Cat");
  pvlObject += PvlKeyword("cat", "Purr");
  pvlObject += PvlKeyword("Eagle", "Swoop");
  pvlObject += PvlKeyword("Rattler", "coil");

  PvlGroup includeGroup("Fish");
  includeGroup += PvlKeyword("Trout", "Brown");
  includeGroup += PvlKeyword("Bass", "Large mouth");
  pvlObject += includeGroup;

  PvlGroup includeObject("Birds");
  includeObject += PvlKeyword("Sparrow", "House");
  includeObject += PvlKeyword("Crow");
  includeObject += PvlKeyword("EAGLE", "Claws");
  pvlObject += includeObject;

  PvlGroup excludeGroup("ExcludeGroup");
  excludeGroup += PvlKeyword("CAT", "Woof");
  pvlObject += excludeGroup;

  PvlObject excludeObject("ExcludeObject");
  excludeObject += PvlKeyword("Dog", "Moo");
  pvlObject += excludeObject;

  qDebug() << "PvlFlatMap1 Source (PvlObject):";
  cout << pvlObject << endl;
  qDebug() << "";

  // Constructed from PvlObject no constraints
  PvlFlatMap map1(pvlObject);
  qDebug() << "Map1 - CAT and EAGLE values are overwritten. No Constraints:";
  print(map1);

  // Testing PvlConstraints from exclude list
  QStringList excludeList;
  excludeList << "ExcludeGroup";
  PvlConstraints constraints = PvlConstraints::withExcludes(excludeList);
  constraints.addExclude("ExcludeObject");
  qDebug() << "Map1B Constraints:";
  qDebug() << "    excludes: " << constraints.excludes();
  qDebug() << "    includes: " << constraints.includes();
  qDebug() << "    key list: " << constraints.keyList();
  qDebug() << "";
  PvlFlatMap map1b(pvlObject, constraints);
  qDebug() << "Map1B - ExcludeGroup and ExcludeObject are excluded:";
  print(map1b);
 
  // Testing adding includes to PvlConstraints
  constraints.addInclude("Beasts");
  qDebug() << "    Map1C Constraints:";
  qDebug() << "    size of excludes: " << constraints.excludeSize();
  qDebug() << "    size of includes: " << constraints.includeSize();
  qDebug() << "    excludes: " << constraints.excludes();
  qDebug() << "    includes: " << constraints.includes();
  qDebug() << "    key list: " << constraints.keyList();
  qDebug() << "    cat is excluded? " << constraints.isExcluded("cat");
  qDebug() << "    ExcludeGroup is excluded? " << constraints.isExcluded("ExcludeGroup");
  qDebug() << "    cat is included? " << constraints.isIncluded("cat");
  qDebug() << "    Beasts is included? " << constraints.isIncluded("Beasts");
  qDebug() << "";
  PvlFlatMap map1c(pvlObject, constraints);
  qDebug() << "Map1C - ExcludeGroup and ExcludeObject are excluded.\n"
              "        Beasts object is included:";
  print(map1c);
 
  // Testing PvlConstraints from include list
  QStringList includeList;
  includeList << "Fish" << "Birds";
  constraints = PvlConstraints::withIncludes(includeList);
  qDebug() << "Map1D Constraints:";
  qDebug() << "    excludes: " << constraints.excludes();
  qDebug() << "    includes: " << constraints.includes();
  qDebug() << "    key list: " << constraints.keyList();
  qDebug() << "";
  PvlFlatMap map1d(pvlObject, constraints);
  qDebug() << "Map1D - Fish group and Birds object are included.\n"
              "        However, parent object Beasts is not included,\n"
              "        so all subgroups/subobjects are skipped:";
  print(map1d);

  // Testing PvlConstraints from QStringList
  QStringList keyList;
  keyList << "cat" << "eagle" << "rattler" << "trout" << "bass" << "sparrow" << "crow";
  PvlConstraints keywordConstraints;
  keywordConstraints.addKeyToList(keyList);
  qDebug() << "Map1E Constraints:";
  qDebug() << "    size of key list: " << keywordConstraints.keyListSize();
  qDebug() << "    excludes: " << keywordConstraints.excludes();
  qDebug() << "    includes: " << keywordConstraints.includes();
  qDebug() << "    key list: " << keywordConstraints.keyList();
  qDebug() << "    cat is in list?  " << keywordConstraints.isKeyInList("cat");
  qDebug() << "    dog is in list?  " << keywordConstraints.isKeyInList("dog");
  qDebug() << "";
  PvlFlatMap map1e(pvlObject, keywordConstraints);
  qDebug() << "Map1E - key list provided:";
  print(map1e);

  // Testing PvlConstraints from file
  QString keyListFileName = FileName("$temporary/keyListFile.txt").expanded();
  keyList.replaceInStrings("cat", "dog");
  // write the keylist to a text file
  TextFile keyListFile(keyListFileName, "output");
  for (int i = 0; i < keyList.size(); i++) {
    keyListFile.PutLine(keyList[i]);
  }
  keyListFile.Close();
  PvlConstraints constraintsFromFile(keyListFileName);
  if (!QFile::remove(keyListFileName)) {
    throw IException(IException::Io, "Unable to remove file, [keyListFile.txt]", _FILEINFO_);
  }
  qDebug() << "Map1F Constraints:";
  qDebug() << "    size of key list: " << constraintsFromFile.keyListSize();
  qDebug() << "    excludes: " << constraintsFromFile.excludes();
  qDebug() << "    includes: " << constraintsFromFile.includes();
  qDebug() << "    key list: " << constraintsFromFile.keyList();
  qDebug() << "    cat is in list?  " << constraintsFromFile.isKeyInList("cat");
  qDebug() << "    dog is in list?  " << constraintsFromFile.isKeyInList("dog");
  qDebug() << "";
  PvlFlatMap map1f(pvlObject, constraintsFromFile);
  qDebug() << "Map1F - key list file provided:";
  print(map1f);

  // PvlFlatMap Copy Constructor
  PvlFlatMap otherMap(map1);
  qDebug() << "Map1 Copy:";
  print(otherMap);

  // Construct PvlFlatMap from PvlGroup
  PvlGroup pvlGroup("Snake");
  pvlGroup.addComment("Are slimey");
  pvlGroup += PvlKeyword("Rattler", "DiamondBack");

  qDebug() << "PvlFlatMap2 Source (PvlGroup):";
  cout << pvlGroup << endl;
  qDebug() << "";


  PvlFlatMap map2(pvlGroup, keywordConstraints);
  qDebug() << "Map2 - No constraints:";
  print(map2);

  // Construct map from maps 1,2
  PvlFlatMap map12(map1, map2);
  qDebug() << "Map1 and Map2 combined - RATTLER is overwritten:";
  print(map12);

  // Empty constructor
  PvlFlatMap map3;
  map3.add("Field", "Run");
  PvlKeyword climb("Climb", "Wall");
  climb.addValue("Rock");
  map3.add(climb);
  map3.append("CLIMB", "Tree");// append to existing
  map3.append("Fly", "Sugar");// append new
  qDebug() << "Map3 - CLIMB values are appended:";
  print(map3);

  // merge all three maps
  qDebug() << "Merging Map3 to Maps 1,2. Adding [" 
           << toString(map12.merge(map3)) << "] new keywords.";
  qDebug() << "Map1, Map2, and Map3 merged:";
  print(map12);

  // get map info
  qDebug() << "Get map info for existing keys:";
  qDebug() << "    Map has keyword=crow?" << map12.exists("crow");
  qDebug() << "    crow is Null? " << map12.isNull("crow");
  qDebug() << "    Map has keyword=climb?" << map12.exists("climb");
  qDebug() << "    All Values for climb:" << map12.allValues("Climb");
  int climbs = map12.count("climb");
  qDebug() << "    climb count: " << climbs;
  for (int i = 0; i < climbs; i++) {
    qDebug() << "    climb at index" << toString(i) << "is Null? " << map12.isNull("climb", i);
  }

  // test more map accessor methods
  qDebug() << "    Get first value of climb:  " << map12("Climb");
  qDebug() << "    Get second value of climb: " << map12.get("Climb", 1);
  qDebug() << "    Get third value of climb:  " << map12.get("Climb", 2);
  qDebug() << "    Get fourth value of climb or return KingKong: " 
           << map12.get("Climb", "KingKong", 3);
  cout << "    Get climb as PvlKeyword: " << map12.keyword("Climb") << endl;

  qDebug() << "";
  qDebug() << "Get map info for non-existent keys:";
  qDebug() << "    Map has keyword=pencil?" << map12.exists("pencil");
  qDebug() << "    All Values for pencil:" << map12.allValues("pencil");
  qDebug() << "    pencil count: " << map12.count("pencil");
  qDebug() << "    pencil is Null? " << map12.isNull("pencil");
  qDebug() << "    Get value of pencil or return Lucy: " << map12.get("pencil", "Lucy");
  qDebug() << "    Get values for PvlKeyword('pencil', 'Linus'): " 
           << map12.keywordValues(PvlKeyword("Sally", "Linus"));
  qDebug() << "";

  // erase...
  qDebug() << "Erasing climb keyword: " << map12.erase("Climb");
  qDebug() << "Reprint Map123 merged with CLIMB keyword erased:";
  print(map12);

  // test throw DNE
  qDebug() << "Try to get CLIMB keyword after erased...";
  try {
    map12.get("Climb");
  }
  catch (IException &e) {
    e.print();
  }

  qDebug() << "";
  qDebug() << "Try to get CAT keyword at non-existent index...";
  try {
    map12.get("CAT", 10);
  }
  catch (IException &e) {
    e.print();
  }

  qDebug() << "";
  qDebug() << "Try to get non-existent pencil keyword...";
  try {
    map12.keyword("pencil");
  }
  catch (IException &e) {
    e.print();
  }

}

/**
 * Unit test convenience function for printing the contents of PvlFlatMaps. If 
 * the map is empty, the string " < Empty Map >" will be printed. 
 *  
 * @param map The PvlFlatMap to print.
 * 
 * @author 2016-02-28 Jeannie Backer
 */
static void print(const PvlFlatMap &map) {
  if (map.isEmpty()) {
    cout << "\t <Empty Map>" << endl;
  }
  QMapIterator<QString, PvlKeyword> iterator(map);
  while(iterator.hasNext()) {
    iterator.next();
    cout << "\t" << iterator.value() << endl;
  }
  qDebug() << "";
}
