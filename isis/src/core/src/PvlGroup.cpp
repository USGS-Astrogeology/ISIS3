/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "IException.h"
#include "PvlFormat.h"

using namespace std;
namespace Isis {
  //! Creates a blank PvlGroup object.
  PvlGroup::PvlGroup() : Isis::PvlContainer("Group", "") {}

  /**
   * Creates a PvlGroup object with a name.
   * @param name The group name.
   */
  PvlGroup::PvlGroup(const QString &name) :
    Isis::PvlContainer("Group", name) {
  }


  //! Copy constructor
  PvlGroup::PvlGroup(const PvlGroup &other) : PvlContainer(other) {}


  /**
   * Read in a group
   *
   * @param is The input stream
   * @param result The PvlGroup to read into (OUTPUT)
   *
   */
  std::istream &operator>>(std::istream &is, PvlGroup &result) {
    PvlKeyword termination("EndGroup");

    PvlKeyword errorKeywords[] = {
      PvlKeyword("Group"),
      PvlKeyword("Object"),
      PvlKeyword("EndObject")
    };

    PvlKeyword readKeyword;

    istream::pos_type beforeKeywordPos = is.tellg();
    is >> readKeyword;

    if(readKeyword != PvlKeyword("Group")) {
      if(is.eof() && !is.bad()) {
        is.clear();
      }

      is.seekg(beforeKeywordPos, ios::beg);

      QString msg = "Expected PVL keyword named [Group], found keyword named [";
      msg += readKeyword.name();
      msg += "] when reading PVL";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }

    if(readKeyword.size() == 1) {
      result.setName(readKeyword[0]);
    }
    else {
      if(is.eof() && !is.bad()) {
        is.clear();
      }

      is.seekg(beforeKeywordPos, ios::beg);

      QString msg = "Expected a single value for group name, found [(";

      for(int i = 0; i < readKeyword.size(); i++) {
        if(i != 0) msg += ", ";

        msg += readKeyword[i];
      }

      msg += ")] when reading PVL";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }


    for(int comment = 0; comment < readKeyword.comments(); comment++) {
      result.addComment(readKeyword.comment(comment));
    }

    readKeyword = PvlKeyword();
    beforeKeywordPos = is.tellg();

    is >> readKeyword;
    while(is.good() && readKeyword != termination) {
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
          msg += "] in Group [";
          msg += result.name();
          msg += "] when reading PVL";
          throw IException(IException::Unknown, msg, _FILEINFO_);
        }
      }

      result.addKeyword(readKeyword);
      readKeyword = PvlKeyword();
      beforeKeywordPos = is.tellg();

      is >> readKeyword;
    }

    if(readKeyword != termination) {
      if(is.eof() && !is.bad()) {
        is.clear();
        is.unget();
      }

      is.seekg(beforeKeywordPos, ios::beg);

      QString msg = "Group [" + result.name();
      msg += "] EndGroup not found before end of file when reading PVL";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }

    return is;
  }

  /**
   * Outputs the PvlGroup data to a specified output stream.
   * @param os The output stream to output to.
   * @param group The PvlGroup object to output.
   */
  ostream &operator<<(std::ostream &os, PvlGroup &group) {

    // Set up a Formatter
    bool removeFormatter = false;
    if(group.format() == NULL) {
      group.setFormat(new PvlFormat());
      removeFormatter = true;
    }

    Isis::PvlGroup temp("DEFAULT");
    if(group.hasFormatTemplate()) temp = *(Isis::PvlGroup *)group.formatTemplate();

    // Output comment from the template
    if(temp.comments() > 0) {
      for(int k = 0; k < temp.comments(); k++) {
        for(int l = 0; l < group.indent(); l++) os << " ";
        os << temp.comment(k) << group.format()->formatEOL();
      }
//      os << group.format()->formatEOL();
    }

    // Output the group comments and name
    os << group.nameKeyword() << group.format()->formatEOL();
    group.setIndent(group.indent() + 2);

    // Output the keywords in this group
    if(group.keywords() > 0) {
      os << (Isis::PvlContainer &) group << group.format()->formatEOL();
    }

    // Output the end of the group
    group.setIndent(group.indent() - 2);
    for(int i = 0; i < group.indent(); i++) os << " ";
    os << group.format()->formatEnd("End_Group", group.nameKeyword());

    if(removeFormatter) {
      delete group.format();
      group.setFormat(NULL);
    }

    return os;
  }


  //! This is an assignment operator
  const PvlGroup &PvlGroup::operator=(const PvlGroup &other) {
    this->PvlContainer::operator=(other);

    return *this;
  }

  /**
   * Validate a PvlGroup, comparing against the corresponding
   * PvlGroup in the Template file
   *
   * Template PvlGroup has the format:
   * Group = (groupName, optional/required)
   *
   * @author Sharmila Prasad (9/22/2010)
   *
   * @param pPvlGrp - PvlGroup to be validated
   */
  void PvlGroup::validateGroup(PvlGroup & pPvlGrp)
  {
    // Group cannot be empty - needs to have a keyword
    if(pPvlGrp.keywords() <= 0) {
      QString sErrMsg = "Group \"" + pPvlGrp.name() + "\" has no Keywords\n";
      throw IException(IException::User, sErrMsg, _FILEINFO_);
    }

    validateAllKeywords((PvlContainer &)pPvlGrp);
  }

} // end namespace isis
