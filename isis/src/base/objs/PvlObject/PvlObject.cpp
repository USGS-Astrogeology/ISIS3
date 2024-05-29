/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "PvlObject.h"

#include "FileName.h"
#include "Pvl.h"
#include "IException.h"
#include "IString.h"
#include "Message.h"
#include "PvlFormat.h"

#include <QList>

#include <iostream>
#include <sstream>

using namespace std;
using json = nlohmann::json;
namespace Isis {

  //! Creates a blank PvlObject.
  PvlObject::PvlObject() : Isis::PvlContainer("Object") {
  }


  /**
   * Creates a PvlObject with a specified name.
   *
   * @param name The name of the PvlObject.
   */
  PvlObject::PvlObject(const QString &name) :
    Isis::PvlContainer("Object", name) {
  }


  //! Copy constructor
  PvlObject::PvlObject(const PvlObject &other) :
    PvlContainer::PvlContainer(other) {
    m_objects = other.m_objects;
    m_groups = other.m_groups;
  }



  /**
   * Creates a PvlObject with a specified name converted
   * from a json object.
   *
   * TODO: Needs to be more robust, right now exists in order to
   * convert an Ale ISD NaifKeywords group over to an ISIS
   * compatible version.
   *
   * @param name The name of the PvlObject.
   * @param json Json object to convert to PVL
   */
  PvlObject::PvlObject(const QString &name, const json &jsonobj) :
    PvlContainer("Object", name) {

    for(auto it = jsonobj.begin(); it != jsonobj.end(); it++) {
        PvlKeyword keyword;
        keyword.setName(QString::fromStdString(it.key()));
        if (it.value().is_array()) {
          for(auto ar = it.value().begin(); ar!=it.value().end(); ar++) {
            try {
              keyword.addJsonValue(*ar);
            }
            catch (IException &e) {
              QString msg = "While attempting to parse " + name + " the following occured";
              throw IException(e, IException::Unknown, msg, _FILEINFO_);
            }
          }
        }
        else {
          keyword.setJsonValue(*it);
        }
        addKeyword(keyword);
    }
  }

  /**
   * Finds a group within the current PvlObject.
   *
   * @param name The name of the group to look for.
   * @param opts The FindOptions option (None or Traverse).
   *
   * @return The PvlGroup object sought for.
   *
   * @throws IException
   */
  Isis::PvlGroup &PvlObject::findGroup(const QString &name,
                                       PvlObject::FindOptions opts) {
    vector<PvlObject *> searchList;
    searchList.push_back(this);

    while(searchList.size() > 0) {
      PvlGroupIterator it =
        searchList[0]->findGroup(name,
                                 searchList[0]->beginGroup(),
                                 searchList[0]->endGroup());
      if(it != searchList[0]->endGroup()) return *it;
      if(opts == Traverse) {
        for(int i = 0; i < searchList[0]->objects(); i++) {
          searchList.push_back(&searchList[0]->object(i));
        }
      }
      searchList.erase(searchList.begin());
    }

    QString msg = "Unable to find PVL group [" + name + "]";
    if(m_filename.size() > 0) msg += " in file [" + m_filename + "]";
    throw IException(IException::Unknown, msg, _FILEINFO_);
  }


  /**
   * Finds a group within the current PvlObject.
   *
   * @param name The name of the group to look for.
   * @param opts The FindOptions option (None or Traverse).
   *
   * @return The PvlGroup object sought for.
   *
   * @throws IException
   */
  const Isis::PvlGroup &PvlObject::findGroup(const QString &name,
      PvlObject::FindOptions opts) const {
    vector<const PvlObject *> searchList;
    searchList.push_back(this);

    while(searchList.size() > 0) {
      ConstPvlGroupIterator it =
        searchList[0]->findGroup(name,
                                 searchList[0]->beginGroup(),
                                 searchList[0]->endGroup());
      if(it != searchList[0]->endGroup()) return *it;
      if(opts == Traverse) {
        for(int i = 0; i < searchList[0]->objects(); i++) {
          searchList.push_back(&searchList[0]->object(i));
        }
      }
      searchList.erase(searchList.begin());
    }

    QString msg = "Unable to find PVL group [" + name + "]";
    if(m_filename.size() > 0) msg += " in file [" + m_filename + "]";
    throw IException(IException::Unknown, msg, _FILEINFO_);
  }

  /**
   * Finds a keyword in the current PvlObject, or deeper inside
   * other PvlObjects and Pvlgroups within this one. Note: This
   * member has the same name as the PvlContainer and hides those
   * implementations, but with the using statement the parents
   * FindKeyword members ar made visible. Note: If more than one
   * occurance of a Keyword appears below this Object no
   * guarantee is made as to which one is returned.
   *
   * @param kname The name of the keyword to look for.
   * @param opts The FindOptions option (None or Traverse).
   *
   * @return The keyword sought
   *
   * @throws IException
   */
  PvlKeyword &PvlObject::findKeyword(const QString &kname,
                                     FindOptions opts) {

    // Call the parent's version if they don't want to dig deeper
    if(opts == None) return findKeyword(kname);

    // Search this PvlObject, and all PvlObjects and PvlContainers within
    // it for the first occurrence of the requested keyword.
    vector<PvlObject *> searchList;
    searchList.push_back(this);

    while(searchList.size() > 0) {
      PvlKeywordIterator it =
        searchList[0]->findKeyword(kname, searchList[0]->begin(),
                                   searchList[0]->end());
      if(it != searchList[0]->end()) {
        return *it;
      }

      // See if the keyword is inside a Group of this Object
      for(int g = 0; g < searchList[0]->groups(); g++) {
        PvlKeywordIterator it =
          searchList[0]->group(g).findKeyword(kname,
                                              searchList[0]->group(g).begin(),
                                              searchList[0]->group(g).end());
        if(it != searchList[0]->group(g).end()) {
          return *it;
        }
      }

      // It's not in this Object or any groups in this Object, so
      // add all Objects inside this Object to the search list
      for(int i = 0; i < searchList[0]->objects(); i++) {
        searchList.push_back(&searchList[0]->object(i));
      }

      // This Object has been searched to remove it from the list
      searchList.erase(searchList.begin());
    }

    // No where else to look for the Keyword so throw an error
    QString msg = "Unable to find PVL keyword [" + kname + "]";
    if(m_filename.size() > 0) msg += " in file [" + m_filename + "]";
    throw IException(IException::Unknown, msg, _FILEINFO_);
  }


  /**
   * See if a keyword is in the current PvlObject, or deeper inside
   * other PvlObjects and Pvlgroups within this one. Note: This
   * member has the same name as the PvlContainer and hides those
   * implementations, but with the using statement the parents
   * FindKeyword members ar made visible.
   *
   * @param kname The name of the keyword to look for.
   * @param opts The FindOptions option (None or Traverse).
   *
   * @return True if the Keyword exists False otherwise.
   */
  bool PvlObject::hasKeyword(const QString &kname,
                             FindOptions opts) const {

    // Call the parent's version if they don't want to dig deeper
    if(opts == None) return hasKeyword(kname);

    // Search this PvlObject, and all PvlObjects and PvlContainers within
    // it for the first occurrence of the requested keyword.
    vector<const PvlObject *> searchList;
    searchList.push_back(this);

    while(searchList.size() > 0) {
      ConstPvlKeywordIterator it =
        searchList[0]->findKeyword(kname, searchList[0]->begin(),
                                   searchList[0]->end());
      if(it != searchList[0]->end()) {
        return true;
      }

      // See if the keyword is inside a Group of this Object
      for(int g = 0; g < searchList[0]->groups(); g++) {
        ConstPvlKeywordIterator it =
          searchList[0]->group(g).findKeyword(kname,
                                              searchList[0]->group(g).begin(),
                                              searchList[0]->group(g).end());

        if(it != searchList[0]->group(g).end()) {
          return true;
        }
      }

      // It's not in this Object or any groups in this Object, so
      // add all Objects inside this Object to the search list
      for(int i = 0; i < searchList[0]->objects(); i++) {
        searchList.push_back(&searchList[0]->object(i));
      }

      // This Object has been searched to remove it from the list
      searchList.erase(searchList.begin());
    }
    return false;
  }


  /**
   * Find an object within the current PvlObject.
   *
   * @param name The object name to look for.
   * @param opts The FindOptions option (None or Traverse).
   *
   * @return The PvlObject sought for.
   *
   * @throws IException
   */
  PvlObject &PvlObject::findObject(const QString &name,
                                   PvlObject::FindOptions opts) {
    vector<PvlObject *> searchList;
    searchList.push_back(this);

    while(searchList.size() > 0) {
      PvlObjectIterator it =
        searchList[0]->findObject(name,
                                  searchList[0]->beginObject(),
                                  searchList[0]->endObject());
      if(it != searchList[0]->endObject()) return *it;
      if(opts == Traverse) {
        for(int i = 0; i < searchList[0]->objects(); i++) {
          searchList.push_back(&searchList[0]->object(i));
        }
      }
      searchList.erase(searchList.begin());
    }

    QString msg = "Unable to find PVL object [" + name + "]";
    if(m_filename.size() > 0) msg += " in file [" + m_filename + "]";
    throw IException(IException::Unknown, msg, _FILEINFO_);
  }


  /**
   * Find an object within the current PvlObject.
   *
   * @param name The object name to look for.
   * @param opts The FindOptions option (None or Traverse).
   *
   * @return The PvlObject sought for.
   *
   * @throws IException
   */
  const PvlObject &PvlObject::findObject(const QString &name,
                                         FindOptions opts) const {
    vector<const PvlObject *> searchList;
    searchList.push_back(this);

    while(searchList.size() > 0) {
      ConstPvlObjectIterator it =
        searchList[0]->findObject(name,
                                  searchList[0]->beginObject(),
                                  searchList[0]->endObject());

      if(it != searchList[0]->endObject()) {
        return *it;
      }

      if(opts == Traverse) {
        for(int i = 0; i < searchList[0]->objects(); i++) {
          searchList.push_back(&searchList[0]->object(i));
        }
      }

      searchList.erase(searchList.begin());
    }

    QString msg = "Unable to find PVL object [" + name + "]";

    if(m_filename.size() > 0) {
      msg += " in file [" + m_filename + "]";
    }

    throw IException(IException::Unknown, msg, _FILEINFO_);
  }


  /**
   * Remove an object from the current PvlObject.
   *
   * @param name The name of the PvlObject to remove.
   *
   * @throws IException
   */
  void PvlObject::deleteObject(const QString &name) {
    PvlObjectIterator key = findObject(name, beginObject(), endObject());
    if(key == endObject()) {
      QString msg = "Unable to find PVL object [" + name + "] in " + type() +
                   " [" + this->name() + "]";
      if(m_filename.size() > 0) msg += " in file [" + m_filename + "]";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }

    m_objects.erase(key);
  }


  /**
   * Remove an object from the current PvlObject.
   *
   * @param index The index of the PvlObject to remove.
   *
   * @throws IException
   */
  void PvlObject::deleteObject(const int index) {
    if(index >= (int)m_objects.size() || index < 0) {
      QString msg = "The specified index is out of bounds in PVL " + type() +
                   " [" + name() + "]";
      if(m_filename.size() > 0) msg += " in file [" + m_filename + "]";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }

    PvlObjectIterator key = beginObject();
    for(int i = 0; i < index; i++)  key++;

    m_objects.erase(key);
  }


  /**
   * Remove a group from the current PvlObject.
   *
   * @param name The name of the PvlGroup to remove.
   *
   * @throws IException
   */
  void PvlObject::deleteGroup(const QString &name) {
    PvlGroupIterator key = findGroup(name, beginGroup(), endGroup());
    if(key == endGroup()) {
      QString msg = "Unable to find PVL group [" + name + "] in " + type() +
                   " [" + this->name() + "]";
      if(m_filename.size() > 0) msg += " in file [" + m_filename + "]";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }

    m_groups.erase(key);
  }


  /**
   * Remove a group from the current PvlObject.
   *
   * @param index The index of the PvlGroup to remove.
   *
   * @throws IException
   */
  void PvlObject::deleteGroup(const int index) {
    if(index >= (int)m_groups.size() || index < 0) {
      QString msg = "The specified index is out of bounds in PVL " + type() +
                   " [" + name() + "]";
      if(m_filename.size() > 0) msg += " in file [" + m_filename + "]";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }

    PvlGroupIterator key = beginGroup();
    for(int i = 0; i < index; i++)  key++;

    m_groups.erase(key);
  }


  /**
   * Return the group at the specified index.
   *
   * @param index The index of the group.
   *
   * @return The PvlGroup sought for.
   *
   * @throws IException
   */
  Isis::PvlGroup &PvlObject::group(const int index) {
    if(index < 0 || index >= (int)m_groups.size()) {
      QString msg = Message::ArraySubscriptNotInRange(index);
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    return m_groups[index];
  }


  /**
   * Return the group at the specified index.
   *
   * @param index The index of the group.
   *
   * @return The PvlGroup sought for.
   *
   * @throws IException
   */
  const Isis::PvlGroup &PvlObject::group(const int index) const {
    if(index < 0 || index >= (int)m_groups.size()) {
      QString msg = Message::ArraySubscriptNotInRange(index);
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    return m_groups[index];
  }

  /**
   * Return the object at the specified index.
   *
   * @param index The index of the object.
   *
   * @return The PvlObject sought for.
   *
   * @throws IException::Programmer
   */
  PvlObject &PvlObject::object(const int index) {
    if(index < 0 || index >= (int)m_objects.size()) {
      QString msg = Message::ArraySubscriptNotInRange(index);
      throw IException(Isis::IException::Programmer, msg, _FILEINFO_);
    }

    return m_objects[index];
  }

  /**
   * Return the object at the specified index.
   *
   * @param index The index of the object.
   *
   * @return The PvlObject sought for.
   *
   * @throws IException::Programmer
   */
  const PvlObject &PvlObject::object(const int index) const {
    if(index < 0 || index >= (int)m_objects.size()) {
      QString msg = Message::ArraySubscriptNotInRange(index);
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    return m_objects[index];
  }


  /**
   * Outputs the PvlObject data to a specified output stream.
   *
   * @param os The output stream to write to.
   * @param object The PvlObject to send to the output stream.
   */
  ostream &operator<<(std::ostream &os, PvlObject &object) {

    // Set up a Formatter
    bool removeFormatter = false;
    if(object.format() == NULL) {
      object.setFormat(new PvlFormat());
      removeFormatter = true;
    }

    Isis::PvlObject outTemplate("DEFAULT");
    if(object.hasFormatTemplate())
      outTemplate = *(Isis::PvlObject *)object.formatTemplate();

    // Look for and process all include files and remove duplicates
    Isis::PvlObject newTemp(outTemplate.name());

    // Make sure the new template has all the original's comments
    for(int i = 0; i < outTemplate.comments(); i++) {
      newTemp.addComment(outTemplate.comment(i));
    }

    // Include files take precedence to all other objects and groups
    for(int i = 0; i < outTemplate.keywords(); i++) {
      if(outTemplate[i].isNamed("Isis:PvlTemplate:File")) {
        QString filename = outTemplate[i];
        Isis::FileName file(filename);
        if(!file.fileExists()) {
          QString message = "Could not open the following PVL template file: ";
          message += filename;
          throw IException(IException::Io, message, _FILEINFO_);
        }

        Isis::Pvl include(file.expanded());

        for(int j = 0; j < include.keywords(); j++) {
          if(!newTemp.hasKeyword(include[j].name()))
            newTemp.addKeyword(include[j]);
        }

        for(int j = 0; j < include.objects(); j++) {
          if(!newTemp.hasObject(include.object(j).name()))
            newTemp.addObject(include.object(j));
        }

        for(int j = 0; j < include.groups(); j++) {
          if(!newTemp.hasGroup(include.group(j).name()))
            newTemp.addGroup(include.group(j));
        }
      }
      // If it is not an include file keyword add it in place
      else if(!newTemp.hasKeyword(outTemplate[i].name()))
        newTemp.addKeyword(outTemplate[i]);
    }

    // Copy over the objects
    for(int i = 0; i < outTemplate.objects(); i++) {
      if(!newTemp.hasObject(outTemplate.object(i).name()))
        newTemp.addObject(outTemplate.object(i));
    }

    // Copy over the groups
    for(int i = 0; i < outTemplate.groups(); i++) {
      if(!newTemp.hasGroup(outTemplate.group(i).name()))
        newTemp.addGroup(outTemplate.group(i));
    }

    outTemplate = newTemp;

    // Write out comments for this Object that were in the template
    if(outTemplate.comments() > 0) {
      for(int k = 0; k < outTemplate.comments(); k++) {
        for(int l = 0; l < object.indent(); l++) os << " ";
        os << outTemplate.comment(k) << object.format()->formatEOL();
      }
      //os << object.format()->FormatEOL();
    }

    // Output the object comments and name
    os << object.nameKeyword() << object.format()->formatEOL();
    object.setIndent(object.indent() + 2);

    // Output the keywords in this Object
    if(object.keywords() > 0) {
      os << (Isis::PvlContainer &) object << object.format()->formatEOL();
    }

    // This number keeps track of the number of objects have been written
    int numObjects = 0;

    // Output the Objects within this Object using the format template
    for(int i = 0; i < outTemplate.objects(); i++) {
      for(int j = 0; j < object.objects(); j++) {
        if(outTemplate.object(i).name() != object.object(j).name()) continue;
        if(j == 0 && object.keywords() > 0)
          os << object.format()->formatEOL();

        object.object(j).setIndent(object.indent());
        object.object(j).setFormatTemplate(outTemplate.object(i));
        object.object(j).setFormat(object.format());
        os << object.object(j) << object.format()->formatEOL();
        object.object(j).setFormat(NULL);
        object.object(j).setIndent(0);

        if(++numObjects < object.objects())
          os << object.format()->formatEOL();
      }
    }

    // Output the Objects within this Object that were not included in the
    // format template pvl
    for(int i = 0; i < object.objects(); i++) {
      if(outTemplate.hasObject(object.object(i).name())) continue;
      if(i == 0 && object.keywords() > 0)
        os << object.format()->formatEOL();

      object.object(i).setIndent(object.indent());
      object.object(i).setFormat(object.format());
      os << object.object(i) << object.format()->formatEOL();
      object.object(i).setFormat(NULL);
      object.object(i).setIndent(0);

      if(++numObjects < object.objects())
        os << object.format()->formatEOL();

    }

    // This number keeps track of the number of groups that have been written
    int numgroups = 0;

    // Output the groups within this Object using the format template
    for(int i = 0; i < outTemplate.groups(); i++) {
      for(int j = 0; j < object.groups(); j++) {
        if(outTemplate.group(i).name() != object.group(j).name()) continue;
        if((numgroups == 0) &&
            (object.objects() > 0 || object.keywords() > 0))
          os << object.format()->formatEOL();

        object.group(j).setIndent(object.indent());
        object.group(j).setFormatTemplate(outTemplate.group(i));
        object.group(j).setFormat(object.format());
        os << object.group(j) << object.format()->formatEOL();
        object.group(j).setFormat(NULL);
        object.group(j).setIndent(0);
        if(++numgroups < object.groups()) os << object.format()->formatEOL();
      }
    }

    // Output the groups that were not in the format template
    for(int i = 0; i < object.groups(); i++) {
      if(outTemplate.hasGroup(object.group(i).name())) continue;
      if((numgroups == 0) &&
          (object.objects() > 0 || object.keywords() > 0))
        os << object.format()->formatEOL();

      object.group(i).setIndent(object.indent());
      object.group(i).setFormat(object.format());
      os << object.group(i) << object.format()->formatEOL();
      object.group(i).setFormat(NULL);
      object.group(i).setIndent(0);

      if(++numgroups < object.groups())
        os << object.format()->formatEOL();
    }

    // Output the end of the object
    object.setIndent(object.indent() - 2);
    for(int i = 0; i < object.indent(); i++) os << " ";
    os << object.format()->formatEnd("End_Object", object.nameKeyword());

    if(removeFormatter) {
      delete object.format();
      object.setFormat(NULL);
    }

    return os;
  }


  /**
   * This method reads a PvlObject from the input stream
   *
   */
  std::istream &operator>>(std::istream &is, PvlObject &result) {
    PvlKeyword termination("EndObject");

    PvlKeyword errorKeywords[] = {
      PvlKeyword("EndGroup")
    };

    PvlKeyword readKeyword;

    istream::pos_type beforeKeywordPos = is.tellg();
    is >> readKeyword;

    if(readKeyword != PvlKeyword("Object")) {
      if(is.eof() && !is.bad()) {
        is.clear();
      }

      is.seekg(beforeKeywordPos, ios::beg);

      QString msg = "Expected PVL keyword named [Object], found keyword named [";
      msg += readKeyword.name();
      msg += "]";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    if(readKeyword.size() == 1) {
      result.setName(readKeyword[0]);
    }
    else {
      is.seekg(beforeKeywordPos, ios::beg);

      QString msg = "Expected a single value for PVL object name, found [(";

      for(int i = 0; i < readKeyword.size(); i++) {
        if(i != 0) msg += ", ";

        msg += readKeyword[i];
      }

      msg += ")]";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }

    for(int comment = 0; comment < readKeyword.comments(); comment++) {
      result.addComment(readKeyword.comment(comment));
    }

    readKeyword = PvlKeyword();
    beforeKeywordPos = is.tellg();

    is >> readKeyword;
    while(readKeyword != termination) {
      for(unsigned int errorKey = 0;
          errorKey < sizeof(errorKeywords) / sizeof(PvlKeyword);
          errorKey++) {
        if(readKeyword == errorKeywords[errorKey]) {
          if(is.eof() && !is.bad()) {
            is.clear();
          }

          is.seekg(beforeKeywordPos, ios::beg);

          QString msg = "Unexpected [";
          msg += readKeyword.name();
          msg += "] in PVL Object [";
          msg += result.name();
          msg += "]";
          throw IException(IException::Unknown, msg, _FILEINFO_);
        }
      }

      if(readKeyword == PvlKeyword("Group")) {
        is.seekg(beforeKeywordPos);
        PvlGroup newGroup;
        is >> newGroup;
        result.addGroup(newGroup);
      }
      else if(readKeyword == PvlKeyword("Object")) {
        is.seekg(beforeKeywordPos);
        PvlObject newObject;
        is >> newObject;
        result.addObject(newObject);
      }
      else {
        result.addKeyword(readKeyword);
      }

      readKeyword = PvlKeyword();
      beforeKeywordPos = is.tellg();

      if(is.good()) {
        is >> readKeyword;
      }
      else {
        // eof found
        break;
      }
    }

    if(readKeyword != termination) {
      if(is.eof() && !is.bad()) {
        is.clear();
      }

      is.seekg(beforeKeywordPos, ios::beg);

      QString msg = "PVL Object [" + result.name();
      msg += "] EndObject not found before end of file";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }

    return is;
  }


  //! This is an assignment operator
  const PvlObject &PvlObject::operator=(const PvlObject &other) {
    this->PvlContainer::operator=(other);

    m_objects = other.m_objects;
    m_groups = other.m_groups;

    return *this;
  }

  /**
   * Validate a PvlObject, comparing against corresponding Template PvlObject.
   * If the Objects are nested, it will recursively validate the PvlObject.
   *
   * Template PvlObject has the format:
   * Object = (objectName, optional/required)
   *
   * @author Sharmila Prasad (9/22/2010)
   *
   * @param pvlObj- PvlObject to be validated
   */
  void PvlObject::validateObject(PvlObject & pPvlObj)
  {
    // Validate the current object
    int iObjSize = objects();

    for(int i=0; i<iObjSize; i++) {
      PvlObject & pvlTmplObj = object(i);

      QString sObjName = pvlTmplObj.name();
      bool bObjFound = false;

      // Pvl contains the Object Name
      if(pPvlObj.hasObject(sObjName)) {
        PvlObject & pvlObj = pPvlObj.findObject(sObjName);
        pvlTmplObj.validateObject(pvlObj);
        if(pvlObj.objects()==0 && pvlObj.groups()==0 && pvlObj.keywords()==0) {
          pPvlObj.deleteObject(pvlObj.name());
        }
        bObjFound = true;
      }
      else {
        QString sOption = sObjName + "__Required";
        bObjFound = true; // optional is the default
        if(pvlTmplObj.hasKeyword(sOption)) {
          PvlKeyword pvlKeyOption = pvlTmplObj.findKeyword(sOption);
          if(pvlKeyOption[0] == "true") { // Required is true
            bObjFound = false;
          }
        }
      }
      if (bObjFound == false) {
        QString sErrMsg = "Object \"" + sObjName + "\" Not Found in the Template File\n";
        throw IException(IException::User, sErrMsg, _FILEINFO_);
      }
    }

    // Validate the groups in the current object
    int iTmplGrpSize = groups();
    for(int i=0; i<iTmplGrpSize; i++) {
      PvlGroup & pvlTmplGrp = group(i);
      bool bGrpFound = false;
      QString sGrpName = pvlTmplGrp.name();

      // Pvl contains the Object Name
      if(pPvlObj.hasGroup(sGrpName)) {
        PvlGroup & pvlGrp = pPvlObj.findGroup(sGrpName);
        pvlTmplGrp.validateGroup(pvlGrp);
        if(pvlGrp.keywords()==0) {
          pPvlObj.deleteGroup(pvlGrp.name());
        }
        bGrpFound = true;
      }
      else {
        bGrpFound = true;
        QString sOption = sGrpName + "__Required";
        if(pvlTmplGrp.hasKeyword(sOption)) {
          PvlKeyword pvlKeyOption = pvlTmplGrp.findKeyword(sOption);
          if(pvlKeyOption[0] == "true") { // Required is true
            bGrpFound = false;
          }
        }
      }
      if (bGrpFound == false) {
        QString sErrMsg = "Group \"" + sGrpName + "\" Not Found in the Template File\n";
        throw IException(IException::User, sErrMsg, _FILEINFO_);
      }
    }

    // Validate the Keywords in the current Object
    validateAllKeywords((PvlContainer &)pPvlObj);
  }

} // end namespace isis
