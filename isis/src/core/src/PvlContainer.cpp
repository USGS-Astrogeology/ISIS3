/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <algorithm>

#include "PvlContainer.h"
#include "Pvl.h"
#include "FileName.h"
#include "IException.h"
#include "Message.h"
#include "PvlFormat.h"

using namespace std;

namespace Isis {

  /**
   * Constructs a PvlContainer object with a type.
   * @param type The type of the container.
   */
  PvlContainer::PvlContainer(const std::string &type) {
    init();
    m_name.setName(type);
  }


  /**
   * Constructs a PvlContainer object with a keyword name and a container name.
   * @param type The type of container.
   * @param name The name of the container.
   */
  PvlContainer::PvlContainer(const std::string &type, const std::string &name) {
    init();
    m_name.setName(type);
    setName(name);
  }


  PvlContainer::PvlContainer(const PvlContainer &other) {
    *this = other;
  }



  //! Sets the filename to blank.
  void PvlContainer::init() {
    m_filename = "";
    m_formatTemplate = NULL;
  }

  /**
   * Find a keyword with a specified name.
   * @param name The name of the keyword to look for.
   * @return The PvlKeyword object.
   * @throws iException::Pvl The keyword doesn't exist.
   */
  Isis::PvlKeyword &PvlContainer::findKeyword(const std::string &name) {
    PvlKeywordIterator key = findKeyword(name, begin(), end());
    if(key == end()) {
      std::string msg = "PVL Keyword [" + name + "] does not exist in [" +
                   type() + " = " + this->name() + "]";
      if(m_filename.size() > 0) msg += " in file [" + m_filename + "]";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }

    return *key;
  }

  /**
   * Find a keyword with a specified name.
   * @param name The name of the keyword to look for.
   * @return The PvlKeyword object.
   * @throws IException The keyword doesn't exist.
   */
  const Isis::PvlKeyword &PvlContainer::findKeyword(const std::string &name) const {
    ConstPvlKeywordIterator key = findKeyword(name, begin(), end());
    if(key == end()) {
      std::string msg = "PVL Keyword [" + name + "] does not exist in [" +
                   type() + " = " + this->name() + "]";
      if(m_filename.size() > 0) msg += " in file [" + m_filename + "]";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }

    return *key;
  }

  /**
   * Remove a specified keyword.
   * @param name The name of the keyword to remove.
   * @throws iException::Pvl Keyword doesn't exist.
   */
  void PvlContainer::deleteKeyword(const std::string &name) {
    PvlKeywordIterator key = findKeyword(name, begin(), end());
    if(key == end()) {
      std::string msg = "PVL Keyword [" + name + "] does not exist in [" +
                   type() + " = " + this->name() + "]";
      if(m_filename.size() > 0) msg += " in file [" + m_filename + "]";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }

    m_keywords.erase(key);
  }


  /**
   * Remove the specified keyword.
   * @param index The index of the keyword to remove.
   * @throws iException::Pvl Keyword doesn't exist.
   */
  void PvlContainer::deleteKeyword(const int index) {
    if(index >= (int)m_keywords.size() || index < 0) {
      std::string msg = "The specified index is out of bounds in PVL [" +
                   type() + " = " + name() + "]";
      if(m_filename.size() > 0) msg += " in file [" + m_filename + "]";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }

    PvlKeywordIterator key = begin();
    for(int i = 0; i < index; i++) key++;

    m_keywords.erase(key);
  }


  /**
   * Removes keywords from the container that have BOTH the same name and value.
   *
   * @return bool True if one or more keywords were deleted; False if no keywords
   *         were deleted.
   */
  bool PvlContainer::cleanDuplicateKeywords() {
    bool keywordDeleted = false;
    for (auto current = this->begin(); current!=this->end(); ++current){
      auto &comp = current;
      for (std::next(comp); comp!=this->end(); ++comp){
        if (*current == *comp){
          comp = m_keywords.erase(comp);
          keywordDeleted = true;
        }
      }
    }
    return keywordDeleted;
  }


  /**
   * Check to see if a keyword exists.
   * @param name The name of the keyword to check for.
   * @return True if the keyword exists, false if it doesn't.
   */
  bool PvlContainer::hasKeyword(const std::string &name) const {
    ConstPvlKeywordIterator key = findKeyword(name, begin(), end());
    if(key == end()) return false;
    return true;
  }


  /**
   * Return the PvlKeyword object at the specified index.
   * @param index The index to use.
   * @return The PvlKeyword at the specified index.
   * @throws iException::Message The index is out of bounds.
   */
  PvlKeyword &PvlContainer::operator[](const int index) {
    if(index < 0 || index >= (int)m_keywords.size()) {
      std::string msg = Message::ArraySubscriptNotInRange(index);
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    PvlKeywordIterator it = m_keywords.begin();
    std::advance(it, index);
    return *(it);
  };


  /**
   * Return the PvlKeyword object at the specified index.
   * @param index The index to use.
   * @return The PvlKeyword at the specified index.
   * @throws iException::Message The index is out of bounds.
   */
  const Isis::PvlKeyword &PvlContainer::operator[](const int index) const {
    if(index < 0 || index >= (int)m_keywords.size()) {
      std::string msg = Message::ArraySubscriptNotInRange(index);
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    ConstPvlKeywordIterator it = m_keywords.begin();
    std::advance(it, index);
    return *(it);
  }

  /**
   * Add a keyword to the PvlContainer object.
   * @param key The PvlKeyword object to add.
   * @param mode The enum InsertMode has two possible values, Append or Replace.
   * Use Append if you just want to add it to the end, Replace if you want to
   * replace it.
   */
  void PvlContainer::addKeyword(const Isis::PvlKeyword &key,
                                const InsertMode mode) {
    if(mode == Append) {
      m_keywords.push_back(key);
    }
    else if(hasKeyword(key.name())) {
      Isis::PvlKeyword &outkey = findKeyword(key.name());
      outkey = key;
    }
    else {
      m_keywords.push_back(key);
    }
  }

  /**
   * @brief Insert a keyword at the specified iterator position
   *
   * This method provides the capability to insert a keyword at the specified
   * iterator position.  The process follows the description of the STL vector
   * definition along with all the caveats (e.g., invalidation of iterators upon
   * insert operations).
   *
   * This method will not perform any checks for the existance of the keyword.
   * This could lead to multiple instances of the same keyword in the same
   * container.  It is up to the caller to manage this issue.
   *
   * @param key Keyword to insert
   * @param pos Iterator position where to insert the new keyword
   * @return PvlContainer::PvlKeywordIterator Returns the position of the
   *          inserted keyword per the STL vector documentation.
   */
  PvlContainer::PvlKeywordIterator PvlContainer::addKeyword(const Isis::PvlKeyword &key,
      PvlKeywordIterator pos) {
    return (m_keywords.insert(pos, key));
  }

  /**
   * Output the PvlContainer information.
   *
   * @param os The preferred output stream.
   * @param container The PvlContainer object to output.
   */
  ostream &operator<<(std::ostream &os, PvlContainer &container) {

    // Set up a Formatter (This should not be necessary for a container because
    // Object or Group should have done this already, but just in case.
    bool removeFormatter = false;
    if(container.format() == NULL) {
      container.setFormat(new PvlFormat());
      removeFormatter = true;
    }

    Isis::PvlContainer outTemplate("DEFAULT_TEMPLATE");
    if(container.hasFormatTemplate()) outTemplate = *(container.formatTemplate());

    // Look for and process all include files inside the template
    Isis::PvlContainer newTemp(outTemplate.type());

    // Include files take precedence over all other objects and groups
    for(int i = 0; i < outTemplate.keywords(); i++) {
      if(outTemplate[i].isNamed("Isis:PvlTemplate:File")) {
        std::string filename = outTemplate[i];
        Isis::FileName file(filename);
        if(!file.fileExists()) {
          std::string message = "Could not open the template file [" + filename + "]";
          throw IException(IException::Io, message, _FILEINFO_);
        }
        Isis::Pvl include(file.expanded());

        for(int j = 0; j < include.keywords(); j++) {
          if(!newTemp.hasKeyword(include[j].name()))
            newTemp.addKeyword(include[j]);
        }
      }
      // If it is not an include file keyword add it in place
      else if(!newTemp.hasKeyword(outTemplate[i].name())) {
        newTemp.addKeyword(outTemplate[i]);
      }
    }

    outTemplate = newTemp;

    // Figure out the longest keyword
    int width = 0;
    for(int i = 0; i < container.keywords(); i++) {
      if(container[i].name().length() > width) width = container[i].name().length();
    }

    // This number keeps track of the number of keywords written
    int numKeywords = 0;

    // Write out the container using the output format template
    for(int i = 0; i < outTemplate.keywords(); i++) {
      for(int j = 0; j < container.keywords(); j++) {
        if(outTemplate[i].name() != container[j].name()) continue;
        container[j].setIndent(container.indent());
        container[j].setWidth(width);
        container[j].setFormat(container.format());
        // Add a blank line before keyword comments
        if(outTemplate[i].comments() + container[j].comments() > 0) os << container.format()->formatEOL();
        if(outTemplate[i].comments() > 0) {
          for(int k = 0; k < outTemplate[i].comments(); k++) {
            for(int l = 0; l < outTemplate[i].indent() + container[j].indent(); l++) os << " ";
            os << outTemplate[i].comment(k) << container.format()->formatEOL();
          }
        }
        os << container[j];
        container[j].setFormat(NULL);
        container[j].setIndent(0);
        container[j].setWidth(0);
        if(++numKeywords < container.keywords()) {
//          if (j+1 < container.Keywords() && container[j+1].comments() > 0) os << container.format()->formatEOL();
          os << container.format()->formatEOL();
        }
      }
    }

    // Output the keywords in the container that were not specified in the template
    for(int i = 0; i < container.keywords(); i++) {
      if(outTemplate.hasKeyword(container[i].name())) continue;
      container[i].setIndent(container.indent());
      container[i].setWidth(width);
      container[i].setFormat(container.format());
      os << container[i];
      container[i].setFormat(NULL);
      container[i].setIndent(0);
      container[i].setWidth(0);
      if(++numKeywords < container.keywords()) {
        if(i + 1 < container.keywords() && container[i+1].comments() > 0) os << container.format()->formatEOL();
        os << container.format()->formatEOL();
      }
    }

    if(removeFormatter) {
      delete container.format();
      container.setFormat(NULL);
    }

    return os;
  }


  /**
   * Find the index of a keyword, using iterators.
   * @param name The name of the keyword.
   * @param beg The beginning iterator.
   * @param end The ending iterator.
   * @return The keyword index.
   */
  PvlContainer::PvlKeywordIterator PvlContainer::findKeyword(const std::string &name,
      PvlContainer::PvlKeywordIterator beg,
      PvlContainer::PvlKeywordIterator end) {
    PvlKeyword temp(name);
    return find(beg, end, temp);
  };


  /**
   * Find the index of a keyword, using iterators.
   * @param name The name of the keyword.
   * @param beg The beginning iterator.
   * @param end The ending iterator.
   * @return The keyword index.
   */
  PvlContainer::ConstPvlKeywordIterator PvlContainer::findKeyword(const std::string &name,
      PvlContainer::ConstPvlKeywordIterator beg,
      PvlContainer::ConstPvlKeywordIterator end) const {
    PvlKeyword temp(name);
    return find(beg, end, temp);
  };


  //! This is an assignment operator
  const PvlContainer &PvlContainer::operator=(const PvlContainer &other) {
    m_filename = other.m_filename;
    m_name = other.m_name;
    m_keywords = other.m_keywords;
    m_formatTemplate = other.m_formatTemplate;

    return *this;
  }

  /**
   * Validate all the PvlKeywords in this container
   *
   * @author Sharmila Prasad (9/24/2010)
   *
   * @param pPvlCont - Container to be Validated
   *
   * @history 2010-10-18 Sharmila Prasad - Added options "Type", "Range", "Value"
   *                                       for the keyword validation
   */
  void PvlContainer::validateAllKeywords(PvlContainer & pPvlCont)
  {
    // Validate the Keywords in the current Object
    int iTmplKeySize = keywords();
    for(int i=0; i<iTmplKeySize; i++) {
      PvlKeyword & pvlTmplKwrd = (*this)[i];
      std::string sKeyName = pvlTmplKwrd.name();
      bool bKwrdFound = false;

      // These are reserved keywords for properties like "Range", "Value", "Type",
      // "Required" or "Repeated"
      if (sKeyName.find("__Required") != std::string::npos || 
          sKeyName.find("__Repeated") != std::string::npos ||
          sKeyName.find("__Range") != std::string::npos ||
          sKeyName.find("__Type") != std::string::npos) {
        continue;
      }

      if(pPvlCont.hasKeyword(sKeyName)) {
        PvlKeyword & pvlKwrd = pPvlCont.findKeyword(sKeyName);
        std::string sTmplKwrdRange = sKeyName + "__Range";
        std::string sTmplKwrdValue = sKeyName + "__Value";
        std::string sTmplKwrdType  = sKeyName + "__Type";
        std::string sType="";
        PvlKeyword pvlTmplKwrdRange, pvlTmplKwrdValue;

        // Check if Type is specified (positive or negative for numbers)
        if(hasKeyword(sTmplKwrdType)) {
          sType = findKeyword(sTmplKwrdType)[0];
        }
        // Check for Range
        if(hasKeyword(sTmplKwrdRange)) {
          pvlTmplKwrdRange = findKeyword(sTmplKwrdRange);
          pvlTmplKwrd.validateKeyword(pvlKwrd, sType, &pvlTmplKwrdRange);
        }
        // Check for Value
        else if(hasKeyword(sTmplKwrdValue)) {
          pvlTmplKwrdValue = findKeyword(sTmplKwrdValue);
          pvlTmplKwrd.validateKeyword(pvlKwrd, sType, &pvlTmplKwrdValue);
        }
        else {
          pvlTmplKwrd.validateKeyword(pvlKwrd, sType);
        }
        pPvlCont.deleteKeyword(pvlKwrd.name());
        bKwrdFound = true;
      }
      else {
        bKwrdFound = true;
        std::string sOption = sKeyName + "__Required";
        if(hasKeyword(sOption)) {
          PvlKeyword pvlKeyOption = findKeyword(sOption);
          if(pvlKeyOption[0] == "true") { // Required is true
            bKwrdFound = false;
          }
        }
      }
      if (bKwrdFound == false) {
        std::string sErrMsg = "Keyword \"" + sKeyName + "\" Not Found in the Template File\n";
        throw IException(IException::User, sErrMsg, _FILEINFO_);
      }

      // Check for "Repeated" Option
      validateRepeatOption(pvlTmplKwrd, pPvlCont);
    }
  }

  /**
   * Validate Repeat Option in the Template Group. This option indicates
   * that a particular keyname can be repeated several times
   *
   * @author Sharmila Prasad (9/24/2010)
   *
   * @param pPvlTmplKwrd - Template Keyword wit
   * @param pPvlCont - Container with all the Keywords
   *
   * @history 2010-10-18 Sharmila Prasad - Added option "Type" for the keyword validation
   */
  void PvlContainer::validateRepeatOption(PvlKeyword & pPvlTmplKwrd, PvlContainer & pPvlCont)
  {
    std::string sTmplKeyName = pPvlTmplKwrd.name();

    // Check for the Type
    std::string sType = sTmplKeyName + "__Type";
    std::string sValueType ="";
    if(hasKeyword(sType)) {
      sValueType = findKeyword(sType)[0];
    }
    std::string sRepeatOption = sTmplKeyName + "__Repeated";
    bool bRepeat =false;
    if(hasKeyword(sRepeatOption)) {
      PvlKeyword pvlKeyOption = findKeyword(sRepeatOption);
      if(pvlKeyOption[0] == "true") { // Required is true
        bRepeat = true;
      }
    }
    if(bRepeat) {
      int iKeySize = pPvlCont.keywords();
      for(int j=(iKeySize-1); j>=0; j--) {
        PvlKeyword & pvlKwrd = pPvlCont[j];
        std::string sKeyName = pvlKwrd.name();
        if(sTmplKeyName == sKeyName) {
          pPvlTmplKwrd.validateKeyword(pvlKwrd, sValueType);
          pPvlCont.deleteKeyword(pvlKwrd.name());
        }
      }
    }
  }

} // end namespace isis
