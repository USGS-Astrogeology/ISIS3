/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */ 
#include "PvlFlatMap.h"

#include <QDebug>

// other ISIS
#include "IException.h"
#include "FileName.h"
#include "Pvl.h"
#include "PvlContainer.h"
#include "PvlFlatMap.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "PvlObject.h"
#include "TextFile.h"

using namespace std;

namespace Isis {

  /** 
   * Default constructor for an empty PvlConstraints object.
   */  
  PvlConstraints::PvlConstraints() {
  }


  /** 
   * Constructs a PvlConstraints object from a file containing PvlKeyword names.
   * 
   * @param keyListFile The name of the file containing PvlKeyword names.
   */  
  PvlConstraints::PvlConstraints(const QString &keyListFile) {
    addKeyToList(FileName(keyListFile.toStdString()));
    return;
  }
  

  /**
   * Static method to construct a PvlConstraints object from a list of
   * names for the PvlObjects and PvlGroups to exclude. 
   * 
   * @param excludes QStringList of names of PvlObjects or PvlGroups 
   *                 to be excluded.
   * 
   * @return @b PvlConstraints with excludes set from given list.
   * 
   */
  PvlConstraints PvlConstraints::withExcludes(const QStringList &excludes) {
    PvlConstraints constraints;
    constraints.addExclude(excludes);
    return (constraints);
  }


  /**
   * Static method to construct a PvlConstraints object from a list of 
   * names for the PvlObjects and PvlGroups to include. 
   * 
   * @param includes QStringList of names of PvlObjects or PvlGroups 
   *                 to be included.
   * 
   * @return @b PvlConstraints with includes set from given list.
   */
  PvlConstraints PvlConstraints::withIncludes(const QStringList &includes) {
   PvlConstraints constraints;
   constraints.addInclude(includes);
   return (constraints);
  }
  

  /** 
   * Destructor
   */  
  PvlConstraints::~PvlConstraints() {
  }
  
  
  /** 
   * Returns the number of PvlObjects and PvlGroups to exclude.
   * 
   * This method returns the number of values associated with the Excludes keyword.
   * 
   * @return int The number of PvlObjects and PvlGroups to exclude
   */  
  int PvlConstraints::excludeSize() const {  
    return (m_excludes.size());  
  }
  
  
  /** 
   * Returns the number of PvlObjects and PvlGroups to include.
   * 
   * This method returns the number of values associated with the Includes keyword.
   * 
   * @return int The number of PvlObjects and PvlGroups to include
   */  
  int PvlConstraints::includeSize() const {  
    return (m_includes.size());  
  }
  
  
  /** 
   * Returns the number of PvlKeywords to include.
   * 
   * This method returns the number of values associated with the KeyList keyword. If none of 
   * these keyword constraints exist, then all PvlKeywords in a Pvl structure will be included 
   * when creating a PvlFlatMap from a Pvl structure. 
   * 
   * @return int The number of PvlKeywords in the KeyListFile
   */  
  int PvlConstraints::keyListSize() const {
      return (m_keylist.size());  
  }
  
  
  /** 
   * Adds a PvlObject/PvlGroup exclusion constraint.
   * 
   * By providing the name of a PvlObject or PvlGroup object, the object will be 
   * excluded when creating a PvlFlatMap from a Pvl structure.
   * 
   * Note that if this structure is contained within a PvlObject, then excluding 
   * the top level object also excludes all subgroups/subobjects contained 
   * within. 
   *   
   * @param name The name of the PvlObject or PvlGroup to exclude
   */  
  void PvlConstraints::addExclude(const QString &name) { 
     m_excludes.append(name); 
  }
  
  
  /** 
   * Adds a PvlObject/PvlGroup inclusion constraint.
   * 
   * By providing the name of a PvlObject or PvlGroup object, the object will be
   * included when creating a PvlFlapMap from a Pvl structure. 
   *  
   * Note that if this structure is contained within a PvlObject, then both must 
   * be added to the includes. 
   *   
   * @param name The name of the PvlObject or PvlGroup to include
   */  
  void PvlConstraints::addInclude(const QString &name) { 
     m_includes.append(name); 
  }
  
  
  /** 
   * Adds a PvlKeyword inclusion constraint.
   * 
   * By providing the name of a PvlKeyword, the PvlKeyword will be exclusively included along with
   * any other keyword constraints when creating a PvlFlatMap from a Pvl structure.
   * 
   * @param name The QString name of the PvlKeyword to add
   */  
  void PvlConstraints::addKeyToList(const QString &name) { 
     m_keylist.append(name); 
  }
  
  
  /** 
   * Adds multiple PvlObject/PvlGroup exclusion constraints.
   * 
   * @param other A list of exclusion contraints
   */  
  void PvlConstraints::addExclude(const QStringList &other) { 
    m_excludes += other; 
  }
  
  
  /** 
   * Adds multiple PvlObject/PvlGroup inclusion constraints.
   * 
   * @param other A list of inclusion constraints
   */  
  void PvlConstraints::addInclude(const QStringList &other) { 
    m_includes += other; 
  }
  
  
  /** 
   * Adds multiple PvlKeyword inclusion constraints.
   * 
   * @param other A list of PvlKeyword names
   */  
  void PvlConstraints::addKeyToList(const QStringList &other) { 
    m_keylist += other;
  }
  
  
  /** 
   * Adds PvlKeywords contained in a file to the keyword constraints.
   * 
   * @param name The name of the file that contains the PvlKeywords
   */  
  void PvlConstraints::addKeyToList(const FileName &fileName) { 
    readKeyListFile(fileName); 
    return;
  }
  
  
  /** 
   * Determines if a PvlObject or PvlGroup is excluded.
   * 
   * Checks if a PvlObject or PvlGroup's name is associated with the Excludes
   * keyword.
   * 
   * @param name The name of the PvlObject or PvlGroup
   * 
   * @return bool True if the PvlObject or PvlGroup is to be excluded 
   */  
  bool PvlConstraints::isExcluded(const QString &name) const {
    return (m_excludes.contains(name, Qt::CaseInsensitive));
  }
  
  
  /** 
   * Determines if a PvlObject or PvlGroup is included.
   * 
   * Checks if a PvlObject or PvlGroup's name is associated with the Includes
   * keyword.
   * 
   * @param name The name of the PvlObject or PvlGroup
   * 
   * @return bool True if the PvlObject or PvlGroup is to be included
   */  
  bool PvlConstraints::isIncluded(const QString &name) const {
    return (m_includes.contains(name, Qt::CaseInsensitive));
  }
  
  
  /** 
   * Determines if a PvlKeyword is included.
   * 
   * Checks if a PvlKeyword's name is associated with the KeyListFile keyword.
   * 
   * @param name The name of The PvlKeyword
   * 
   * @return bool True if the PvlKeyword is in the KeyListFile 
   */  
  bool PvlConstraints::isKeyInList(const QString &name) const {
    return (m_keylist.contains(name, Qt::CaseInsensitive));
  }
  
  
  /** 
   * Returns the PvlObject/PvlGroup exclusion constraints.
   * 
   * Returns the values of the Excludes keyword. Excluded PvlObjects and PvlGroups will not be used
   * during the creation of a PvlFlatMap from a Pvl structure. Refer to the PvlFlatMap class 
   * documentation for more information.
   * 
   * @return QStringList A list of excluded PvlObjects or PvlGroups  
   */  
  const QStringList &PvlConstraints::excludes() const { 
    return (m_excludes);
  }
  
  
  /** 
   * Returns the PvlObject/PvlGroup inclusion constraints.
   * 
   * Returns the values of the Includes keyword. Included PvlObjects and PvlGroups will exclusively
   * be used during the creation of a PvlFlatMap from a Pvl structure. This means anything not 
   * included is automatically excluded. Refer to the PvlFlatMap class documentation for more 
   * information.
   * 
   * @return QStringList A list of included PvlObjects or PvlGroups
   */  
  const QStringList &PvlConstraints::includes() const { 
    return (m_includes);
  }
  
  
  /** 
   * Returns the PvlKeyword inclusion constraints.
   * 
   * Returns the values of the KeyListFile keyword. Only PvlKeywords that are specified in the 
   * KeyListFile will be included during the creation of a PvlFlatMap. Refer to the PvlFlatMap 
   * class documentation for more information.
   * 
   * @return QStringList A list of PvlKeywords 
   */  
  const QStringList &PvlConstraints::keyList()  const { 
    return (m_keylist);
  }


  /** 
   * Reads PvlKeyword inclusion constraints from a file.
   * 
   * Reads through a file containting PvlKeywords and adds each of the PvlKeyword names
   * as keyword constraints.
   * 
   * @param keyListFile
   */  
  void PvlConstraints::readKeyListFile(const FileName &keyListFile) {
    // Pvl pvl(keyListFile.expanded());
    // 
    // if (pvl.hasObject("KeyList")) {
    //   PvlContainer keylist = pvl.findObject("KeyList");
    //   PvlContainer::ConstPvlKeywordIterator k = keylist.begin();
    //   while (k != keylist.end()) {
    //     addKeyToList(k->name());
    //     ++k;
    //   }
    // }
    // else {
    //   PvlContainer::ConstPvlKeywordIterator k = pvl.begin();
    //   while (k != pvl.end()) {
    //     addKeyToList(k->name());
    //     ++k;
    //   }
    // }
    // return;

//     Pvl pvl(keyListFile.expanded());
//     qDebug() << "INPUT FILE = ";
//     qDebug() << keyListFile.expanded();
//     for (int k = 0; k < pvl.keywords(); k++) {
//       cout << "HI!" << endl;
//       addKeyToList(pvl[k].name());
//     }
    TextFile keyList(QString::fromStdString(keyListFile.expanded()));
    QString keywordName = "";
    while (keyList.GetLine(keywordName)) {
      addKeyToList(keywordName);
    }

  }
    
      
  /** 
   * Default constructor
   */  
  PvlFlatMap::PvlFlatMap() : QMap<QString, PvlKeyword> () { 
  }
  
  
  /** 
   * Constructs a PvlFlatMap from another PvlFlatMap object.
   * 
   * @param other The PvlFlatMap to copy 
   */  
  PvlFlatMap::PvlFlatMap(const PvlFlatMap &other) : 
                         QMap<QString, PvlKeyword> ()  { 
    merge(other);
  }
  
  
  /** 
   * Constructs a PvlFlatMap from two PvlFlatMap objects.
   * 
   * This constructor creates a PvlFlatMap from the contents of two PvlFlatMap objects.
   * The constructed PvlFlatMap contains the first PvlFlatMap's contents followed by the
   * second PvlFlatMap's contents. 
   *  
   * If there are keywords of the same in both maps, then the last loaded 
   * keyword in pmap2 will overwrite any previous values. 
   *  
   * @see merge() 
   *  
   * @param pmap1 The first PvlFlatMap
   * @param pmap2 The second PvlFlatMap
   */  
  PvlFlatMap::PvlFlatMap(const PvlFlatMap &pmap1, 
                         const PvlFlatMap &pmap2) :
                         QMap<QString, PvlKeyword> () {
    merge(pmap1);
    merge(pmap2);
  }
  
  
  /** 
   * Constructs a PvlFlatMap (with constraints) from a PvlObject.
   * 
   * This Constructor creates a PvlFlatMap from a PvlObject according to any constraints
   * passed to the constructor. If no PvlConstraints object is provided, the PvlFlatMap will
   * be created from all the objects and groups within the passed PvlObject.
   *  
   * @see loadObject() 
   * 
   * @param pvl The PvlObject to use for creating the PvlFlatMap
   * @param constraints The PvlConstraints that determine which Pvl objects/groups are
   *                    included or excluded in the constructed PvlFlatMap
   */  
  PvlFlatMap::PvlFlatMap(const PvlObject &pvl, 
                         const PvlConstraints &constraints) : 
                         QMap<QString, PvlKeyword> () {
    loadObject(pvl, constraints);
  }
  
  
  /** 
   * Constructs a PvlFlatMap (with constraints) from the keywords in a PvlContainer (i.e 
   * PvlObject or PvlGroup). 
   * 
   * This constructor creates a PvlFlatMap from a PvlGroup object according to any constraints 
   * passed to the constructor. If no PvlConstraints object is provided, the PvlFlatMap will be 
   * created from all of the objects and groups within the passed PvlContainer. 
   *  
   * @see loadKeywords() 
   * 
   * @param pvl The PvlContainer to use for creating the PvlFlatMap
   * @param constraints The PvlConstraints that determine which Pvl objects/groups are
   *                    included or excluded in the constructed PvlFlatMap
   */  
  PvlFlatMap::PvlFlatMap(const PvlContainer &pvl, 
                         const PvlConstraints &constraints) : 
                         QMap<QString, PvlKeyword> () {
    loadKeywords(pvl, constraints);
  }
  
  
  /** 
   * Destructor
   */  
  PvlFlatMap::~PvlFlatMap() {
  }

 
  /** 
   * Determines whether a given keyword exists in the PvlFlatMap.
   * 
   * @param key The name of the keyword
   *  
   * @return bool True if the keyword exists in the PvlFlatMap
   */  
  bool PvlFlatMap::exists(const QString &key) const {
    return (contains(key.toLower()));
  }
  
  
  /** 
   * Returns the number of values associated with a given keyword.
   * 
   * If the keyword does not exist, this method returns 0.
   * 
   * @param key The name of the keyword
   *  
   * @return int The number of values associated with the keyword.
   */  
  int PvlFlatMap::count(const QString &key) const {
    if ( !exists(key) ) { 
      return (0); 
    }
    return (keyword(key).size());
  }
  
  
  /** 
   * Determines if the value of a keyword is Null.
   * 
   * By default, this method checks the first value associated with the
   * passed keyword. If the keyword does not exist, this method returns true.
   * 
   * @param key The name of the keyword
   * @param index The index of the value associated with the keyword
   *  
   * @return bool True if the keyword's value at the given index is Null
   *              or if the keyword does not exist
   */  
  bool PvlFlatMap::isNull(const QString &key, 
                          const int index) const {
    if (exists(key)) {
      return (keyword(key).isNull(index));
    }
  
    // Always returns true if the keyword doesn't exist
    return (true);
  }
  
  
  /** 
   * Adds PvlKeyword with the given name and value to the PvlFlatMap. This will
   * overwrite any existing keyword in the map with the same name. 
   * 
   * @param key The name of the keyword to add 
   * @param value The value of the added keyword
   */  
  void PvlFlatMap::add(const QString &key, 
                       const QString &value) {
    add(PvlKeyword(key.toStdString(), value.toStdString()));
  }
  
  
  /** 
   * Adds the given PvlKeyword to the PvlFlatMap. This will overwrite any 
   * existing keyword in the map keyword with the same name. 
   * 
   * @param key The PvlKeyword to add to the map.
   */  
  void PvlFlatMap::add(const PvlKeyword &key) {
    insert(QString::fromStdString(key.name()).toLower(),  key);
  }
  
  
  /** 
   * Appends a the given keyword and value to the PvlFlatMap.
   * 
   * If the keyword already exists in the PvlFlatMap, the provided value is appended to the 
   * keyword's list of values in the map. 
   * 
   * @param key The name of the keyword to be appended
   * @param value The value of the keyword to be appended
   */  
  void PvlFlatMap::append(const QString &key, 
                          const QString &value) {
    append(PvlKeyword(key.toStdString(), value.toStdString()));
    return;
  }
  
  
  /** 
   * Appends the given PvlKeyword's values to the PvlFlatMap.
   * 
   * If the keyword already exists in the PvlFlatMap, the values of the provided keyword are 
   * appended to the keyword's list of values in the map. 
   * 
   * @param key A PvlKeyword to append to the map.
   */  
  void PvlFlatMap::append(const PvlKeyword &key) {
    if ( exists(QString::fromStdString(key.name())) ) {
      PvlFlatMapIterator kw = find(QString::fromStdString(key.name()).toLower());
      // add all values to the map
      for (int i = 0; i < key.size(); i++) {
        kw.value().addValue(key[i]); 
      }
    }
    else {
      insert(QString::fromStdString(key.name()).toLower(),  PvlKeyword(key.name(), key[0]));
      // if there are more than one values in this keyword, insert the first
      // into the map and recursively call append to add the rest.
      for (int i = 1; i < key.size(); i++) {
        append(QString::fromStdString(key.name()), QString::fromStdString(key[i]));
      }
    }
    return;
  }


  /** 
   * Erases a keyword from the PvlFlatMap.
   * 
   * @param key The keyword to erase 
   *  
   * @return bool Indicates whether a keyword of the given name was found and 
   *              succesfully erased.
   */  
  bool PvlFlatMap::erase(const QString &key) {
    return (bool (remove(key.toLower())));
  }
  
  
  /** 
   * Gets the value of a keyword in the PvlFlatMap.
   * 
   * By default, this method gets the first value associated with the passed keyword name.
   * If the keyword does not exist, an exception is thrown.
   * 
   * @param key The name of the keyword 
   * @param index The index of the value to get
   * 
   * @return QString The keyword's value at the specified index
   * 
   * @throws IException::Programmer "Keyword does not exist."
   * @throws IException::Programmer "Index does not exist for the given keyword."
   */  
  QString PvlFlatMap::get(const QString &key, 
                          const int &index) const {
    QMap<QString, PvlKeyword>::const_iterator k = find(key.toLower());
    if (end() == k) {
      std::string mess = "Keyword " + key + " does not exist!";
      throw IException(IException::Programmer, mess, _FILEINFO_);
    }
    if (index >= k.value().size()) {
      std::string mess = "Index " + toString(index) + " does not exist for keyword " + key + "!";
      throw IException(IException::Programmer, mess, _FILEINFO_);
    }
  
    return QString::fromStdString(k.value()[index]);
  }
  
  
  /** 
   * Gets the value of a keyword in the PvlFlatMap.
   * 
   * By default, this method gets the first value associated with the passed keyword name.
   * If the keyword does not exist, this method returns the QString specified in defValue.
   * 
   * @param key The name of the keyword
   * @param defValue The default value to return if the keyword does not exist
   * @param index The index of the value to get
   * 
   * @return QString The keyword's value
   */  
  QString PvlFlatMap::get(const QString &key, 
                          const QString &defValue,
                          const int &index) const {
    QMap<QString, PvlKeyword>::const_iterator k = find(key.toLower());
    if (end() == k || index >= k.value().size()) {
      return (defValue);
    }
    else {
      return QString::fromStdString(k.value()[index]);
    }
  }
  

  /** 
   * Gets the first value of a keyword in the PvlFlatMap.
   * 
   * This method will get the first value of the keyword if the keyword has multiple values
   * associated with it.
   * 
   * @param name The name of the keyword
   * 
   * @return QString The value of the keyword
   */  
  QString PvlFlatMap::operator()(const QString &name) const {
    return (get(name));
  }
  
  
  /** 
   * Gets all the values associated with a keyword in the PvlFlatMap.
   * 
   * If the keyword does not exist, an empty QStringList is returned.
   * 
   * @param key The name of the keyword
   * 
   * @return QStringList The list of values associated with the keyword
   */  
  QStringList PvlFlatMap::allValues(const QString &key) const {
    QStringList values;
    QMap<QString, PvlKeyword>::const_iterator k = find(key.toLower());
    if (end() != k) {  
      values = keywordValues(k.value());  
    }
    return (values);
  }
  
  
  /** 
   * Finds a keyword in the PvlFlatMap.
   * 
   * @param key The name of the keyword to find
   * 
   * @return PvlKeyword The found keyword
   * 
   * @throws IException::Programmer "Keyword does not exist"
   */  
  PvlKeyword PvlFlatMap::keyword(const QString &key) const {
    QMap<QString, PvlKeyword>::const_iterator k = find(key.toLower());
    if (end() == k) {
      std::string mess = "Keyword " + key + " does not exist!";
      throw IException(IException::Programmer, mess, _FILEINFO_);
    }
    return (k.value());
  }
  
  
  /** 
   * Adds the keywords from another PvlFlatMap.
   * 
   * If there are keywords of the same in both maps, then the last loaded 
   * keyword in the "other" map will overwrite any previous values. 
   *  
   * @param other The PvlFlatMap to get keywords from
   * 
   * @return int The number of keywords added
   */  
  int PvlFlatMap::merge(const PvlFlatMap &other)  {
    int n = 0;
    QMap<QString, PvlKeyword>::const_iterator keys =  other.begin();
    while ( keys != other.end() ) {
      add(keys.value());
      n++;
      ++keys;
    }
  
    return (n);
  }
  
  
  /** 
   * Gets all of a PvlKeyword's values.
   * 
   * (Helper method for PvlFlatMap::allValues)
   * 
   * @param keyword The PvlKeyword to get values from
   * 
   * @return QStringList A list of the PvlKeyword's values
   */  
  QStringList PvlFlatMap::keywordValues(const PvlKeyword &keyword) {
    QStringList values;
    for (int i = 0 ; i < keyword.size() ; i++) {
      values << QString::fromStdString(keyword[i]);
    }
    return (values);
  }
  
  
  /** 
   * Loads PvlObjects into the PvlFlatMap
   * 
   * This method loads PvlObjects into the PvlFlatMap according to any provided PvlConstraints.
   * If a PvlObject is to be excluded, none of its contained PvlObjects, PvlGroups, or PvlKeywords
   * will be loaded. If the PvlObject is to be included, only it and any other included PvlObjects
   * will be loaded. When PvlObject can be loaded, any contained PvlKeywords, PvlGroups, and
   * PvlObjects will be loaded against any provided PvlConstraints. After finishing loading any
   * PvlKeywords and PvlGroups contained in the PvlObject, this method will try to load subsequent
   * PvlObjects. Returns the total number of PvlKeywords loaded into the PvlFlatMap 
   *  
   * @see loadKeywords() 
   * @see loadGroups() 
   *  
   * @param object The PvlObject to load PvlKeywords, PvlGroups, and PvlObjects from
   * @param constraints Any PvlConstraints that define what is loaded into the PvlFlatMap
   * 
   * @return int The total number of PvlKeywords loaded into the PvlFlatMap
   */  
  int PvlFlatMap::loadObject(const PvlObject &object, 
                             const PvlConstraints &constraints) {
    int total = 0;
  
    // Check constraints if specified
    int nconsts = constraints.excludeSize() + constraints.includeSize();
    if ( nconsts > 0 ) {
      bool isExcluded = constraints.isExcluded(QString::fromStdString(object.name()));
      bool isIncluded = constraints.isIncluded(QString::fromStdString(object.name()));
      bool hasBoth = (constraints.excludeSize() > 0) && 
                     (constraints.includeSize() > 0);

      // Check constraints
      // include object and its groups and keywords when both Includes and Excludes are specified
      // don't include an object if it isn't in Includes keyword
      if ( hasBoth ) {
        if ( !isIncluded ) { 
          return (total); 
        }  // At the object level
      } 
      else if ( isExcluded ) {
        return (total);
      }
      else if ( constraints.includeSize() > 0 ) {
        if ( !isIncluded ) { 
          return (total); 
        }
      }
    }
  
    // First load keys in the object, then existing groups followed by additional
    // objects
    total += loadKeywords(object, constraints);
    total += loadGroups(object, constraints);
  
    PvlObject::ConstPvlObjectIterator objs;
    for (objs = object.beginObject() ; objs != object.endObject() ; ++objs) {
      total += loadObject(*objs, constraints);
    }
    return (total);
  }
  
  
  /** 
   * Loads PvlGroups into the PvlFlatMap.
   * 
   * This method loads PvlGroups into the PvlFlatMap according to any provided PvlConstraints.
   * This method iterates through a provided PvlObject and calls its helper method, loadGroup(), 
   * on each PvlGroup contained in the PvlObject. Returns the number of PvlKeywords loaded
   * from the PvlGroups.
   * 
   * @see loadGroup() 
   *  
   * @param object The PvlObject to load PvlGroups from
   * @param constraints Any PvlConstraints that determine how PvlGroups should be loaded
   * 
   * @return int The number of PvlKeywords loaded from PvlGroups
   */  
  int  PvlFlatMap::loadGroups(const PvlObject &object,
                              const PvlConstraints &constraints) {
    int total = 0;
  
    //  Load the PvlGroups contained in the PvlObject
    PvlObject::ConstPvlGroupIterator group;
    for (group = object.beginGroup() ; group != object.endGroup() ; group++) {
      total += loadGroup(*group, constraints);
    }
    return total;
  }
  
  
  /**
   * Loads a PvlGroup into the PvlFlatMap
   * 
   * This method is the loadGroups() helper method that loads a PvlGroup into the PvlFlatMap
   * according to any provided PvlConstraints.
   * If the PvlGroup is to be excluded, none of its contained PvlKeywords will be loaded. If the
   * PvlGroup is to be included, only it and any other included PvlGroups will be loaded. When a
   * PvlGroup can be loaded, the group's PvlKeywords are loaded against any keyword constraints. 
   * Returns the number of PvlKeywords loaded from the PvlGroup. 
   *  
   * @see loadKeywords() 
   * 
   * @param group The PvlGroup to load into the PvlFlatmap
   * @param constraints Any PvlConstraints that determine how the PvlGroup should be loaded
   * 
   * @return int The number of PvlKewyords loaded from the PvlGroup
   */
  int PvlFlatMap::loadGroup(const PvlGroup &group,
                            const PvlConstraints &constraints) {
    int total = 0;
    // Check constraints if specified
    int nconsts = constraints.excludeSize() + constraints.includeSize();
    if ( nconsts > 0 ) {
      bool isExcluded = constraints.isExcluded(QString::fromStdString(group.name()));
      bool isIncluded = constraints.isIncluded(QString::fromStdString(group.name()));
      bool hasBoth = (constraints.excludeSize() > 0) && 
                     (constraints.includeSize() > 0);
      // Check constraints
      // When both Includes and Excludes provided, Excludes applies to groups
      // If an object is included but one of its group is excluded, exclude the group
      if ( hasBoth ) {
        if ( isExcluded ) { 
          return (total); 
        } // at group level
      } 
      else if ( isExcluded ) {
        return (total);
      }
      else if ( constraints.includeSize() > 0 ) {
        if ( !isIncluded ) { 
          return (total); 
        }
      }
    }
    
    // load the PvlGroup's PvlKeywords
    total += loadKeywords(group, constraints);
    return (total);
  }
  
  
  /** 
   * Loads PvlKeywords within a PvlContainer into the PvlFlatMap.
   * 
   * This method loads the PvlKeywords within a PvlContainer according to any PvlConstraints
   * given. 
   *  
   * If there is more than one PvlKeyword with the same name, then the last loaded keyword will 
   * overwrite the previous. 
   *  
   * If any keyword constraints exist, only the PvlKeywords included in the keyword constraints 
   * will be loaded into the PvlFlatMap. If no keyword constraints are given all PvlKeywords in 
   * the PvlContainer will be included in the PvlFlatMap. 
   *  
   * Returns the number of PvlKeywords in the provided PvlContainer that are loaded into the 
   * PvlFlatMap. 
   * 
   * @param pvl The PvlContainer (i.e. PvlObject or PvlGroup) to load PvlKeywords from
   * @param constraints The keyword constraints that define which PvlKeyword will be loaded
   * 
   * @return int The number of PvlKeywords loaded into the PvlFlatMap
   */  
  int PvlFlatMap::loadKeywords(const PvlContainer &pvl, 
                               const PvlConstraints &constraints) {
    PvlContainer::ConstPvlKeywordIterator key;
    int n = 0;
  
    // if there are any keyword constraints, only load those PvlKeywords
    if ( constraints.keyListSize() > 0 ) {
      for (key = pvl.begin() ; key != pvl.end() ; ++key) {
        if ( constraints.isKeyInList(QString::fromStdString(key->name())) ) {
          add(*key);
          n++;
        }
      }
    }
    // otherwise load all PvlKeywords in the PvlContainer provided
    else {
      for (key = pvl.begin() ; key != pvl.end() ; ++key) {
        add(*key);
        n++;
      }
    }
    return (n);
  }

} // namespace Isis

