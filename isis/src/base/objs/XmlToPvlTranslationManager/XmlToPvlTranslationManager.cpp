/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "LabelTranslationManager.h"

#include <ostream>

#include <QDebug>
#include <QFile>
#include <QString>

#include "IException.h"
#include "IString.h"
#include "Message.h"
#include "Pvl.h"
#include "PvlContainer.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "PvlObject.h"
#include "XmlToPvlTranslationManager.h"


using namespace std;
namespace Isis {

  /**
   * Constructs and initializes an XmlToPvlTranslationManager object from the given
   * Pvl translation file. If this constructor is used, the user will need to
   * set the input label before translating. This may be done by using
   * SetLabel(FileName inputLabel) or Auto(FileName inputLabel, Pvl
   * outputLabel).
   *
   * @param transFile The translation file to be used to tranlate keywords in
   *                  the input label.
   */
  XmlToPvlTranslationManager::XmlToPvlTranslationManager(const QString &transFile)
      : LabelTranslationManager() {
    AddTable(transFile);
  }


  /**
   * Constructs and initializes an XmlToPvlTranslationManager object from the given
   * input stream. If this constructor is used, the user will need to set the
   * input label before translating. This may be done by using SetLabel(FileName
   * inputLabel) or Auto(FileName inputLabel, Pvl outputLabel).
   *
   *
   * @param transStrm A stream containing the tranlation table to be used to
   *                  tranlate keywords in the input label.
   */
  XmlToPvlTranslationManager::XmlToPvlTranslationManager(std::istream &transStrm)
      : LabelTranslationManager() {
    AddTable(transStrm);
  }


  /**
   * Constructs and initializes an XmlToPvlTranslationManager object from the given
   * Pvl translation file and input label.
   *
   * @param inputLabel The Xml holding the input label.
   *
   * @param transFile The translation file to be used to tranlate keywords in
   *                  the input label.
   */
  XmlToPvlTranslationManager::XmlToPvlTranslationManager(FileName &inputLabel,
                                                         const QString &transFile)
      : LabelTranslationManager() {
    AddTable(transFile);
    parseFile(inputLabel);
  }


  /**
   * Constructs and initializes an XmlToPvlTranslationManager object from the given
   * input stream and input label.
   *
   * @param inputLabel The Xml holding the input label.
   *
   * @param transStrm A stream containing the tranlation table to be used to
   *                  tranlate keywords in the input label.
   */
  XmlToPvlTranslationManager::XmlToPvlTranslationManager(FileName &inputLabel,
                                                         std::istream &transStrm)
      : LabelTranslationManager() {
    AddTable(transStrm);
    parseFile(inputLabel);
  }


  /**
   * Destroys the XmlToPvlTranslationManager object.
   */
  XmlToPvlTranslationManager::~XmlToPvlTranslationManager() {
  }


  /**
   * Reads an Xml label file and internalizes it for translation.
   *
   * @param inputLabel The input label file
   */
  void XmlToPvlTranslationManager::SetLabel(FileName &inputLabel) {
    parseFile(inputLabel);
  }


  /**
   * Returns a vector of valid keyword names and their sizes.  A size of -1
   * indicates that the keyword can be any size.
   *
   * @return @b vector<pair<QString,int>> A vector of valid keyword names and their sizes.
   */
  vector< pair<QString, int> > XmlToPvlTranslationManager::validKeywords() const {

    vector< pair<QString, int> > validKeywords = PvlTranslationTable::validKeywords();
    validKeywords.push_back(pair<QString, int>("InputKeyAttribute",      -1));
    validKeywords.push_back(pair<QString, int>("InputKeyDependencies",   -1));
    validKeywords.push_back(pair<QString, int>("Debug",                   1));

    return validKeywords;
  }


  /**
   * Returns a translated value. The translation group name is
   * used to find the input group, keyword, default and
   * tranlations in the translation table. If the keyword does not
   * exist in the input label, the input default if available will
   * be used as the input value. This input value is then used to
   * search all of the translations. If a match is found the
   * translated value is returned.
   *
   * @param translationGroupName The name of the PVL translation
   *                        group used to identify the
   *                        input/output keywords to be
   *                        translated. Often, this is the
   *                        same as the output keyword name.
   *
   * @param index The index into the input keyword array.  Defaults to 0
   *
   * @return @b QString The translated output value to be
   *         placed in the ISIS cube label.
   *
   * @throws IException::Unknown "Failed to translate output value."
   * @throws IException::Unknown "Cannot translate value. Xml files can only
   *                              store a single value in each element."
   * @throws IException::Unknown "Unable to retrieve translation group from
   *                              translation table."
   * @throws IException::Unknown "Unable to retrieve [InputPosition] keyword
   *                              from translation group."
   * @throws IException::Unknown "Unable to retrieve [InputKey] keyword from
   *                              translation group."
   * @throws IException::Unknown "Failed traversing input position. Element
   *                              does not have the named child element."
   * @throws IException::Unknown "Could not find an input value or default value."
   * @throws IException::Unknown "Input element does not have the named attribute."
   */
  QString XmlToPvlTranslationManager::Translate(QString translationGroupName,
                                                int index) {
    try {
    if (index != 0) {
      QString msg = "Cannot translate value at index [" + toString(index) +
                    "]. Xml files can only store a single value in each element.";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }

    const Pvl &transTable = TranslationTable();
    PvlGroup transGroup;
    try {
      transGroup = transTable.findGroup(translationGroupName);
    }
    catch (IException &e){
      QString msg = "Unable to retrieve translation group from translation table.";
      throw IException(e, IException::Unknown, msg, _FILEINFO_);
    }

    // get input position values
    PvlKeyword inputPosition;
    try {
      inputPosition = transGroup["InputPosition"];
    }
    catch (IException &e){
      QString msg = "Unable to retrieve [InputPosition] keyword from "
                    "translation group.";
      throw IException(e, IException::Unknown, msg, _FILEINFO_);
    }
    QString inputParentName = inputPosition[inputPosition.size() - 1];

    // get input key (tag or att)
    QString inputKey;
    try {
      inputKey = transGroup["InputKey"][0];
    }
    catch (IException &e){
      QString msg = "Unable to retrieve [InputKey] keyword from "
                    "translation group.";
      throw IException(e, IException::Unknown, msg, _FILEINFO_);
    }
    QString attributeName;
    if (transGroup.hasKeyword("InputKeyAttribute")) {
      attributeName = transGroup["InputKeyAttribute"][0];
    }

    // get dependencies
    PvlKeyword keyDependencies;
    if (transGroup.hasKeyword("InputKeyDependencies")) {
      keyDependencies = transGroup["InputKeyDependencies"];
    }

    // Check for debug
    bool isDebug = transGroup.hasKeyword("Debug");

    // Notify what we are translating and what the translating group is.
    if (isDebug) {
      cout << endl << "          ====================          " << endl;
      cout << endl << "Translating output keyword: " << translationGroupName << endl;
      cout << endl << "Translation group:" << endl;
      cout << transGroup << endl << endl;
    }

    // read input value
    QDomElement inputParentElement = m_xmlLabel.documentElement();
    QString indent = "";
    if (isDebug) {
      cout << endl << "Finding input element:" << endl << endl;
      cout << inputParentElement.tagName() << endl;
    }
    // traverse the input position path
    Pvl::ConstPvlKeywordIterator it = transGroup.findKeyword("InputPosition",
                                      transGroup.begin(),
                                      transGroup.end());

    QDomElement oldInputParentElement = inputParentElement;
    QString childName;
    while(it != transGroup.end()) {
      const PvlKeyword &inputPosition = *it;
      inputParentElement = oldInputParentElement;

        for (int i = 0; i < inputPosition.size(); i++) {
          childName = inputPosition[i];
          inputParentElement = inputParentElement.firstChildElement(childName);
          if(inputParentElement.isNull()) {
            break;
          }
          if (isDebug) {
            indent += "  ";
            cout << indent << inputParentElement.tagName() << endl;
          }
        }
        if (!inputParentElement.isNull()) {
          break;
        }
        it = transGroup.findKeyword("InputPosition", it + 1, transGroup.end());
    }

    if (inputParentElement.isNull()) {
      if (hasInputDefault(translationGroupName)) {
        if (isDebug) {
          cout << endl << "Could not traverse input position, " <<
            "using default value: " <<
            InputDefault(translationGroupName) << endl;
        }
        return PvlTranslationTable::Translate( translationGroupName );
      }
      else {
        QString msg = "Failed traversing input position. [" +
          inputPosition.name() + "] element does not have a child element named [" +
          childName + "].";
        throw IException(IException::Unknown, msg, _FILEINFO_);
      }
    }
    // now get input value at given input position path
    QDomElement inputKeyElement = inputParentElement.firstChildElement(inputKey);
    if (isDebug) {
      indent += "  ";
      cout << indent << inputKeyElement.tagName() << endl;
    }

    // Check dependencies
    while ( !inputParentElement.isNull() &&
            !checkDependencies(inputKeyElement, keyDependencies, isDebug) ) {
      if (isDebug) {
        cout << endl << "Dependencies failed, checking next candidate." << endl;
      }
      // Check if a sibling satisfies the dependencies
      inputKeyElement = inputKeyElement.nextSiblingElement(inputKey);
      // If there are no siblings to check, try cousins.
      while ( inputKeyElement.isNull() ) {
        inputParentElement = inputParentElement.nextSiblingElement(inputParentName);
        // If there are no more siblings of the parent we've run out of things to check.
        if ( inputParentElement.isNull() ) {
          break;
        }
        inputKeyElement = inputParentElement.firstChildElement(inputKey);
      }
    }
    // If the parent element is NULL at this point then we traversed every
    // potential input element and none of them satisfied the dependencies.
    if ( inputParentElement.isNull() ) {
      if ( hasInputDefault(translationGroupName) ) {
        if (isDebug) {
          cout << endl << "No input value found, using default value: " <<
                          InputDefault(translationGroupName) << endl;
        }
        return PvlTranslationTable::Translate( translationGroupName );
      }
      else {
        QString msg = "Could not find an input or default value that fits the given input "
                      "keyword dependencies.";
        throw IException(IException::Unknown, msg, _FILEINFO_);
      }
    }

    // translate value to output value
    QString inputValue = inputKeyElement.text();
    // for attributes, overwrite inputValue
    if (attributeName.size() > 0) {
      if ( inputKeyElement.hasAttribute(attributeName) ) {
        inputValue = inputKeyElement.attribute(attributeName);
      }
      else if (hasInputDefault(translationGroupName) ) {
        if (isDebug) {
          cout << endl << "No input value found, using default value: " <<
                          InputDefault(translationGroupName) << endl;
        }
        return PvlTranslationTable::Translate( translationGroupName );
      }
      else {
        QString msg = "Input element [" + inputKeyElement.tagName() +
                      "] does not have an attribute named [" +
                      attributeName + "].";
        throw IException(IException::Unknown, msg, _FILEINFO_);
      }
    }
    if (isDebug) {
          cout << endl << "Translating input value: " << inputValue << endl;
        }
    return PvlTranslationTable::Translate( translationGroupName, inputValue.trimmed() );
    }
    catch (IException &e){
      QString msg = "Failed to translate output value for [" + translationGroupName + "].";
      throw IException(e, IException::Unknown, msg, _FILEINFO_);
    }
  }


  /**
   * @brief Checks if a element in the xml label satisfies a list of
   *        dependencies.
   *
   * Checks if a element in the xml label satisfies a list of dependencies. The
   * dependencies are requirements on the values of attributes of the element
   * and/or the values of sibling elements. The dependencies are specified by
   * strings that are formatted as follows
   * <code>[tag/att]\@[tagName/attName]|[value]</code> or
   * <code>[tagName/attName]|[value]</code>
   *
   * @param element The element to check dependencies on.
   * @param dependencies A multi-valued keyword were every entry specifies a
   *                     requirement upon either an attribute of the element or
   *                     a sibling of the element.
   *
   * @return @b bool If the element passed the dependencies check
   *
   * @throws IException::Unknown "Parsing error, dependency type is not [att] or [tag]."
   */
  bool XmlToPvlTranslationManager::checkDependencies(QDomElement element,
                                                     PvlKeyword dependencies,
                                                     bool isDebug) const{

    if (isDebug) {
      cout << endl << "Testing dependencies:" << endl;
    }
    for (int i = 0; i < dependencies.size(); i++) {
      QStringList specification = parseSpecification(dependencies[i]);

      if (specification.size() != 3) { // the specification is not a dependancy
        return true;
      }
      if (isDebug) {
        cout << endl << "Testing dependency number " << toString(i+1) << endl;
        cout << "  Specification:    " << dependencies[i] << endl;
        cout << endl;
        cout << "  Dependency type:  " << specification[0] << endl;
        cout << "  Dependency name:  " << specification[1] << endl;
        cout << "  Dependency value: " << specification[2] << endl;
      }
      if (specification[0] == "att") {
        if ( element.hasAttributes() ) {
          QDomNamedNodeMap atts = element.attributes();
          QString attributeValue = atts.namedItem(specification[1]).nodeValue();
          if (isDebug) {
            cout << endl;
            cout << "  Attribute name:   " << atts.namedItem(specification[1]).nodeName();
            cout << "  Attribute value:  " << attributeValue << endl;
          }
          if ( attributeValue != specification[2] ) {
            // attribute value does not match specification or
            // element does not have the named attribute
            return false;
          }
        }
        else {
          // element does not have any attributes
          return false;
        }
      }

      else if (specification[0] == "tag") {
        QDomElement candidateSibling = element.parentNode().firstChildElement(specification[1]);
        QString siblingValue = candidateSibling.text();
        if (isDebug) {
          cout << endl;
          cout << "  Tag name:         " << candidateSibling.tagName() << endl;
          cout << "  Tag value:        " << siblingValue << endl;
        }
        if (siblingValue != specification[2] ) {
          // sibling tag value does not match specification or
          // named sibling tag does not exist
          return false;
        }
      }

      else {
        QString msg = "Parsing error, dependency type [" + specification[0] +
                      "] is not [att] or [tag].";
        throw IException(IException::Unknown, msg, _FILEINFO_);
      }
    }

    // No dependencies failed!
    return true;
  }


  /**
   * Automatically translate all the output names flagged with the Auto keyword
   * in the translation table and store the translated key, value pairs in the
   * argument pvl.
   *
   * @param inputLabel The input label file to be translated.
   * @param outputLabel The output translated Pvl.
   */
  void XmlToPvlTranslationManager::Auto(FileName &inputLabel,
                                        Pvl &outputLabel) {
    parseFile(inputLabel);
    Auto(outputLabel);
  }


  /**
   * Opens, parses, and internalizes an Xml label file.
   *
   * @param xmlFileName The Xml label file.
   *
   * @throws IException::Unknown "Could not open label file."
   * @throws IException::Unknown "XML read/parse error in file."
   */
  void XmlToPvlTranslationManager::parseFile(const FileName &xmlFileName) {
     QFile xmlFile(xmlFileName.expanded());
     if ( !xmlFile.open(QIODevice::ReadOnly) ) {
       QString msg = "Could not open label file [" + xmlFileName.expanded() +
                     "].";
       throw IException(IException::Unknown, msg, _FILEINFO_);
     }

     QString errmsg;
     int errline, errcol;
     if ( !m_xmlLabel.setContent(&xmlFile, false, &errmsg, &errline, &errcol) ) {
       xmlFile.close();
       QString msg = "XML read/parse error in file [" + xmlFileName.expanded()
            + "] at line [" + toString(errline) + "], column [" + toString(errcol)
            + "], message: " + errmsg;
       throw IException(IException::Unknown, msg, _FILEINFO_);
     }

     xmlFile.close();
     return;
  }
} // end namespace isis
