/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"

#include <locale>
#include <fstream>
#include <sstream>

#include "FileName.h"
#include "IException.h"
#include "Message.h"
#include "PvlTokenizer.h"
#include "PvlFormat.h"

using namespace std;
namespace Isis {
  //! Constructs an empty Pvl object.
  Pvl::Pvl() : Isis::PvlObject("Root") {
    init();
  }


  /**
   * Constructs a Pvl from a file
   *
   * @param file The file containing the pvl formatted information
   */
  Pvl::Pvl(const QString &file) : Isis::PvlObject("Root") {
    init();
    read(file);
  }


  //! Copy constructor
  Pvl::Pvl(const Pvl &other) : PvlObject::PvlObject(other) {
    m_internalTemplate = false;
    m_terminator = other.m_terminator;
  }


  //! initializes the class
  void Pvl::init() {
    m_filename = "";
    m_terminator = "End";
    m_internalTemplate = false;
  }


  /**
   * Load PVL information from a string 
   *  
   * TODO: fully document 
   */
  void Pvl::fromString(const std::string &str) {
    stringstream stm;
    stm << str;
    if(!stm) {
      QString message = "Failed to use string: " + QString::fromStdString(str) + "to create Pvl";
      throw IException(IException::Io, message, _FILEINFO_);
    }

    // Read it
    try {
      stm >> *this;
    }
    catch(IException &e) {
      QString message = "Unable to create PVL from string: " + QString::fromStdString(str);
      throw IException(e, IException::Unknown, message, _FILEINFO_);
    }
    catch(...) {
      QString message = "Unable to create PVL from string: " + QString::fromStdString(str);
      throw IException(IException::Unknown, message, _FILEINFO_);
    }
  }


  /**
   * Loads PVL information from a stream
   *
   * @param file A file containing PVL information
   *
   * @throws Isis::iException::Io
   */
  void Pvl::read(const QString &file) {
    // Expand the filename
    Isis::FileName temp(file);
    m_filename = temp.expanded();

    // Open the file
    ifstream istm;
    istm.open(m_filename.toLatin1().data(), std::ios::in);
    if(!istm) {
      QString message = Message::FileOpen(temp.expanded());
      throw IException(IException::Io, message, _FILEINFO_);
    }

    // Read it
    try {
      istm >> *this;
    }
    catch(IException &e) {
      istm.close();
      QString message = "Unable to read PVL file [" + temp.expanded() + "]";
      throw IException(e, IException::Unknown, message, _FILEINFO_);
    }
    catch(...) {
      istm.close();
      QString message = "Unable to read PVL file [" + temp.expanded() + "]";
      throw IException(IException::Unknown, message, _FILEINFO_);
    }
    istm.close();
  }


  /**
   * Opens and writes PVL information to a file and handles the end of line
   * sequence
   *
   * @param file Name of the file to create. The method will overwrite any
   *             existing file.
   *
   * @throws Isis::iException::Io
   */
  void Pvl::write(const QString &file) {
    // Expand the filename
    Isis::FileName temp(file);

    // Set up a Formatter
    bool removeFormatter = false;
    if(format() == NULL) {
      setFormat(new PvlFormat());
      removeFormatter = true;
    }

    // Open the file
    ofstream ostm;
    QString tempName(temp.expanded());
    ostm.open(tempName.toLatin1().data(), std::ios::out);
    ostm.seekp(0, std::ios::beg);
    if(!ostm) {
      QString message = Isis::Message::FileCreate(temp.expanded());
      throw IException(IException::Io, message, _FILEINFO_);
    }

    // Write the labels
    try {
      ostm << *this;
      if(terminator() != "") ostm << format()->formatEOL();
    }
    catch(IException &e) {
      ostm.close();
      QString message = "Unable to write PVL to file [" + temp.expanded() + "]";
      throw IException(e, IException::Io, message, _FILEINFO_);
    }
    catch(...) {
      ostm.close();
      QString message = "Unable to write PVL to file [" + temp.expanded() + "]";
      throw IException(IException::Io, message, _FILEINFO_);
    }

    if(removeFormatter) {
      delete format();
      setFormat(NULL);
    }

    // Close the file
    ostm.close();
  }


  /**
   * Appends PVL information to a file and handles the end of line sequence
   *
   * @param file Name of the file to append to.
   *
   * @throws Isis::iException::Io
   */
  void Pvl::append(const QString &file) {
    // Set up for opening and writing
    Isis::FileName temp(file);

    // Set up a Formatter
    bool removeFormatter = false;
    if(format() == NULL) {
      setFormat(new PvlFormat());
      removeFormatter = true;
    }

    // Open the file
    ofstream ostm;
    QString tempName(temp.expanded());
    ostm.open(tempName.toLatin1().data(), std::ios::app);
    ostm.seekp(0, std::ios::end);
    if(!ostm) {
      QString message = Message::FileOpen(temp.expanded());
      throw IException(IException::Io, message, _FILEINFO_);
    }

    // Write the labels
    try {
      ostm << *this;
      if(terminator() != "") ostm << format()->formatEOL();
    }
    catch(...) {
      ostm.close();
      QString message = "Unable to append PVL infomation to file [" +
                       temp.expanded() + "]";
      throw IException(IException::Io, message, _FILEINFO_);
    }

    if(removeFormatter) {
      delete format();
      setFormat(NULL);
    }

    // Close the file
    ostm.close();
  }


  void Pvl::setFormatTemplate(Isis::Pvl &temp) {
    if(m_internalTemplate) delete m_formatTemplate;
    m_internalTemplate = false;
    Isis::PvlObject::setFormatTemplate(temp);
  }


  void Pvl::setFormatTemplate(const QString &file) {
    if(m_internalTemplate) delete m_formatTemplate;
    m_internalTemplate = true;
    m_formatTemplate = new Isis::Pvl(file);
  }


  /**
   * This stream will not handle the end of line sequence
   *
   * @param os
   * @param pvl
   *
   * @return ostream&
   */
  ostream &operator<<(std::ostream &os, Pvl &pvl) {
    // Set up a Formatter
    bool removeFormatter = false;
    if(pvl.format() == NULL) {
      pvl.setFormat(new PvlFormat());
      removeFormatter = true;
    }

    Isis::Pvl outTemplate;
    if(pvl.hasFormatTemplate()) outTemplate = *(Isis::Pvl *)pvl.formatTemplate();

    // Look for and process all include files and remove duplicates from the
    // format template. Include files take precedence over all other objects and
    // groups
    Isis::Pvl newTemp;
    for(int i = 0; i < outTemplate.keywords(); i++) {
      if(outTemplate[i].isNamed("Isis:PvlTemplate:File")) {
        QString filename = outTemplate[i];
        Isis::FileName file(filename);
        if(!file.fileExists()) {
          QString message = "Could not open the template file [" + filename + "]";
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
      // If it is not an include file add it in place
      else if(!newTemp.hasKeyword(outTemplate[i].name()))
        newTemp.addKeyword(outTemplate[i]);
    }

    // copy over the objects
    for(int i = 0; i < outTemplate.objects(); i++) {
      if(!newTemp.hasObject(outTemplate.object(i).name()))
        newTemp.addObject(outTemplate.object(i));
    }

    // copy over the groups
    for(int i = 0; i < outTemplate.groups(); i++) {
      if(!newTemp.hasGroup(outTemplate.group(i).name()))
        newTemp.addGroup(outTemplate.group(i));
    }

    outTemplate = newTemp;

    // Output the pvl's comments
    for(int i = 0; i < pvl.comments(); i++) {
      os << pvl.comment(i) << pvl.format()->formatEOL();
      if(i == (pvl.comments() - 1)) os << pvl.format()->formatEOL();
    }

    // Output the keywords
    if(pvl.keywords() > 0) {
      os << (Isis::PvlContainer &) pvl << pvl.format()->formatEOL();
    }

    // this number keeps track of the number of objects that have been written
    int numObjects = 0;

    // Output the objects using the format template
    for(int i = 0; i < outTemplate.objects(); i++) {
      for(int j = 0; j < pvl.objects(); j++) {
        if(outTemplate.object(i).name() != pvl.object(j).name()) continue;
        if(numObjects == 0 && pvl.keywords() > 0) os << pvl.format()->formatEOL();
        pvl.object(j).setIndent(pvl.indent());
        pvl.object(j).setFormatTemplate(outTemplate.object(i));
        pvl.object(j).setFormat(pvl.format());
        os << pvl.object(j) << pvl.format()->formatEOL();
        pvl.object(j).setFormat(NULL);
        pvl.object(j).setIndent(0);
        if(++numObjects < pvl.objects()) os << pvl.format()->formatEOL();
      }
    }

    // Output the objects that were not included in the format template pvl
    for(int i = 0; i < pvl.objects(); i++) {
      if(outTemplate.hasObject(pvl.object(i).name())) continue;
      if(numObjects == 0 && pvl.keywords() > 0) os << pvl.format()->formatEOL();
      pvl.object(i).setIndent(pvl.indent());
      pvl.object(i).setFormat(pvl.format());
      os << pvl.object(i) << pvl.format()->formatEOL();
      pvl.object(i).setFormat(NULL);
      pvl.object(i).setIndent(0);
      if(++numObjects < pvl.objects()) os << pvl.format()->formatEOL();
    }

    // this number keeps track of the number of groups that have been written
    int numGroups = 0;

    // Output the groups using the format template
    for(int i = 0; i < outTemplate.groups(); i++) {
      for(int j = 0; j < pvl.groups(); j++) {
        if(outTemplate.group(i).name() != pvl.group(j).name()) continue;
        if((numGroups == 0) &&
            (pvl.objects() > 0 || pvl.keywords() > 0)) os << pvl.format()->formatEOL();
        pvl.group(j).setIndent(pvl.indent());
        pvl.group(j).setFormatTemplate(outTemplate.group(i));
        pvl.group(j).setFormat(pvl.format());
        os << pvl.group(j) << pvl.format()->formatEOL();
        pvl.group(j).setFormat(NULL);
        pvl.group(j).setIndent(0);
        if(++numGroups < pvl.groups()) os << pvl.format()->formatEOL();
      }
    }

    // Output the groups that were not in the format template
    for(int i = 0; i < pvl.groups(); i++) {
      if(outTemplate.hasGroup(pvl.group(i).name())) continue;
      if((numGroups == 0) &&
          (pvl.objects() > 0 || pvl.keywords() > 0)) os << pvl.format()->formatEOL();
      pvl.group(i).setIndent(pvl.indent());
      pvl.group(i).setFormat(pvl.format());
      os << pvl.group(i) << pvl.format()->formatEOL();
      pvl.group(i).setFormat(NULL);
      pvl.group(i).setIndent(0);
      if(++numGroups < pvl.groups()) os << pvl.format()->formatEOL();
    }

    // Output the terminator
    if(pvl.terminator() != "") {
      os << pvl.terminator();
    }

    if(removeFormatter) {
      delete pvl.format();
      pvl.setFormat(NULL);
    }

    return os;
  }


  /**
   * Reads keywords from the instream and appends them to the Pvl object.
   *
   * @param is A specified instream to read from.
   * @param pvl The Pvl object to append to.
   * @throws iException Invalid PVL format specified.
   * @return Returns the entered instream after reading from it.
   */
  istream &operator>>(std::istream &is, Pvl &pvl) {
    if(!is.good()) {
      string msg = "Tried to read input stream with an error state into a Pvl";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    try {
      PvlKeyword termination("End");

      PvlKeyword errorKeywords[] = {
        PvlKeyword("EndGroup"),
        PvlKeyword("EndObject")
      };

      PvlKeyword readKeyword;
      istream::pos_type beforeKeywordPos = is.tellg();

      is >> readKeyword;

      while(readKeyword != termination) {
        for(unsigned int errorKey = 0;
            errorKey < sizeof(errorKeywords) / sizeof(PvlKeyword);
            errorKey++) {
          if(readKeyword == errorKeywords[errorKey]) {
            is.seekg(beforeKeywordPos, ios::beg);

            QString msg = "Unexpected [";
            msg += readKeyword.name();
            msg += "] in PVL Object [ROOT]";
            throw IException(IException::Unknown, msg, _FILEINFO_);
          }
        }

        if(readKeyword == PvlKeyword("Group")) {
          is.seekg(beforeKeywordPos);
          PvlGroup newGroup;
          is >> newGroup;
          pvl.addGroup(newGroup);
        }
        else if(readKeyword == PvlKeyword("Object")) {
          is.seekg(beforeKeywordPos);
          PvlObject newObject;
          is >> newObject;
          pvl.addObject(newObject);
        }
        else {
          pvl.addKeyword(readKeyword);
        }

        readKeyword = PvlKeyword();
        beforeKeywordPos = is.tellg();

        // non-whitespace non-ascii says we're done
        if(is.good() && (is.peek() < 32 || is.peek() > 126)) {
          // fake eof (binary data)
          break;
        }

        if(is.good()) {
          is >> readKeyword;
        }
        else {
          // eof
          break;
        }
      }

      return is;
    }
    catch(IException &e) {
      if(is.eof() && !is.bad()) {
        is.clear();
        is.unget();
      }

      istream::pos_type errorPos = is.tellg();
      if((int)errorPos == -1) throw;

      is.seekg(0, ios::beg);
      long lineNumber = 1;

      if((int)is.tellg() == -1) throw;

      while(is.good() && is.tellg() < errorPos) {
        char next = is.get();

        if(!isprint(next) && !isspace(next)) {
          is.seekg(errorPos, ios::beg);
        }
        else if(next == '\n') {
          lineNumber ++;
        }
      }

      string msg;
      if(lineNumber > 0) {
        msg = "Error in PVL file on line [";
        msg += IString((Isis::BigInt)lineNumber);
        msg += "]";
      }

      throw IException(e, IException::Unknown, msg, _FILEINFO_);
    }
  }


  //! This is an assignment operator
  const Pvl &Pvl::operator=(const Pvl &other) {
    this->PvlObject::operator=(other);

    m_internalTemplate = other.m_internalTemplate;
    m_terminator = other.m_terminator;

    return *this;
  }


  /**
   * Validate a Pvl, comparing against corresponding Template Pvl
   * It validates all the Objects, Groups and Keywords
   *
   * @author Sharmila Prasad (9/22/2010)
   *
   * @param pPvl - User Pvl to be validated
   * @param pPvlResults - Contains the unvalidated Pvl
   */
  void Pvl::validatePvl(const Pvl & pPvl, Pvl & pPvlResults)
  {
    pPvlResults=Pvl(pPvl);

    // Validate Objects
    int iTmplObjSize = objects();

    for(int i=0; i<iTmplObjSize; i++) {
      PvlObject & pvlTmplObj = object(i);

      QString sObjName = pvlTmplObj.name();
      bool bObjFound = false;

      // Pvl contains the Object Name
      if(pPvl.hasObject(sObjName)) {
        PvlObject & pvlObj = pPvlResults.findObject(sObjName);
        pvlTmplObj.validateObject(pvlObj);
        if(pvlObj.objects()==0 && pvlObj.groups()==0 && pvlObj.keywords()==0) {
          pPvlResults.deleteObject(sObjName);
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

    // Validate Groups
    int iTmplGrpSize = groups();
    for(int i=0; i<iTmplGrpSize; i++) {
      PvlGroup & pvlTmplGrp = group(i);

      QString sGrpName  = pvlTmplGrp.name();
      bool bGrpFound = false;

      // Pvl contains the Object Name
      if(pPvl.hasGroup(sGrpName)) {
        PvlGroup & pvlGrp = pPvlResults.findGroup(sGrpName);
        pvlTmplGrp.validateGroup(pvlGrp);
        if(pvlGrp.keywords()==0) {
          pPvlResults.deleteGroup(sGrpName);
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

    // Validate all the Keywords
    validateAllKeywords((PvlContainer &)pPvlResults);
  }

} //end namespace isis
