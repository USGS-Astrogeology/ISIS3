/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "LabelTranslationManager.h"

#include <QDebug>
#include <QDomDocument>
#include <QDomElement>
#include <QString>

#include "IException.h"
#include "IString.h"
#include "Message.h"
#include "Pvl.h"
#include "PvlContainer.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "PvlObject.h"
#include "PvlToXmlTranslationManager.h"

using namespace std;
namespace Isis {

  /**
   * Constructs and initializes a TranslationManager object from given the
   * Pvl translation file. If this constructor is used, the user will need to
   * set the input label before translating. This may be done by using
   * SetLabel(Pvl inputLabel) or Auto(Pvl inputLabel, Pvl outputLabel).
   *
   * @param transFile The translation file to be used to tranlate keywords in
   *                  the input label.
   */
  PvlToXmlTranslationManager::PvlToXmlTranslationManager(const QString &transFile)
      : LabelTranslationManager(transFile) {
  }


  /**
   * Constructs and initializes a TranslationManager object
   *
   * @param inputLabel The Pvl holding the input label.
   *
   * @param transFile The translation file to be used to tranlate keywords in
   *                  the input label.
   */
  PvlToXmlTranslationManager::PvlToXmlTranslationManager(Pvl &inputLabel,
                                                         const QString &transFile)
      : LabelTranslationManager(transFile) {
    m_inputLabel = inputLabel;
  }


  //! Destroys the TranslationManager object.
  PvlToXmlTranslationManager::~PvlToXmlTranslationManager() {
  }


  /**
   * Internalizes a Pvl formatted label for translation.
   * 
   * @param inputLabel The Pvl formatted label to be translated.
   */
  void PvlToXmlTranslationManager::SetLabel(Pvl &inputLabel) {
    m_inputLabel = inputLabel;
  }


  /**
   * Returns a vector of valid keyword names and their sizes.  A size of -1
   * indicates that the keyword can be any size.
   *
   * @return vector<pair<QString,int>> A vector of valid keyword names and their sizes.
   *
   * @see PvlTranslationTable::validKeywords()
   */
  vector< pair<QString, int> > PvlToXmlTranslationManager::validKeywords() const {

    vector< pair<QString, int> > validKeywords = PvlTranslationTable::validKeywords();
    validKeywords.push_back(pair<QString, int>("OutputAttributes",   -1));
    validKeywords.push_back(pair<QString, int>("OutputSiblings",   -1));

    return validKeywords;
  }


  /**
   * Returns a translated value. The output name is used to find the input
   * group, keyword, default and tranlations in the translation table. If the
   * keyword does not exist in the input label, the input default if
   * available will be used as the input value. This input value
   * is then used to search all of the translations. If a match is
   * found the translated value is returned.
   *
   * @param transGroupName The output name used to identify the input keyword to be
   *              translated.
   *
   * @param findex The index into the input keyword array.  Defaults to 0
   *
   * @return @b QString The translated value.
   */
  QString PvlToXmlTranslationManager::Translate(QString transGroupName,
                                                int inputIndex) {
    const PvlContainer *con;
    int inst = 0;
    PvlKeyword grp;

    while((grp = InputGroup(transGroupName, inst++)).name() != "") {
      if((con = GetContainer(grp)) != NULL) {
        if(con->hasKeyword(InputKeywordName(transGroupName))) {
          return PvlTranslationTable::Translate(
              transGroupName, (*con)[InputKeywordName(transGroupName)][inputIndex]);
        }
      }
    }
    return PvlTranslationTable::Translate(transGroupName); 
  }


  /**
   * Translate the requested output name to output values using the input name
   * and values or default value
   *
   * @param transGroupName The output name used to identify the input keyword to be
   *              translated.
   *
   * @param parentElement The element that is the parent of the new QDomElement, if a new element is
   *                      created. If a new element is not created, then parentElement will have an
   *                      attribute added to it.
   */
  void PvlToXmlTranslationManager::doTranslation(PvlGroup transGroup,
                                                 QDomElement &parentElement) {

    int inst = 0;
    QString transGroupName = transGroup.name();
    PvlKeyword grp = InputGroup(transGroupName, inst);

    while (grp.name() != "") {

      const PvlContainer *con = GetContainer(grp);
      if (con != NULL) {
        if (con->hasKeyword(InputKeywordName(transGroupName))) {

          QStringList outputName = parseSpecification(OutputName(transGroupName));
          // Get the InputKey from the input label.
          PvlKeyword inputKeyword = (*con)[InputKeywordName(transGroupName)];
          // Translate input keyword value and set the qdomelement
          // NOTE: We are assuming this is a single valued keyword since
          //       xml does not allow multiple values
          QString untranslatedValue = inputKeyword[0];
          QString translatedValue = PvlTranslationTable::Translate(transGroupName, 
                                                                   untranslatedValue);
          QString units = inputKeyword.unit(); 
          if (outputName.size() == 2 && outputName[0] == "att") {
            parentElement.setAttribute(outputName[1], translatedValue);
            if (transGroup.hasKeyword("OutputAttributes")) {
              addAttributes(transGroup.findKeyword("OutputAttributes"), parentElement);
            }
          }
          else {
            QDomElement newElement = parentElement.ownerDocument().createElement(outputName[0]);
            setElementValue(newElement, translatedValue, units);
            parentElement.appendChild(newElement);
            if (transGroup.hasKeyword("OutputAttributes")) {
              addAttributes(transGroup.findKeyword("OutputAttributes"), newElement);
            }
          }

          if (transGroup.hasKeyword("OutputSiblings")) {
            addSiblings(transGroup.findKeyword("OutputSiblings"), parentElement);
          }
          return;

        }
      }
      grp = InputGroup(transGroupName, ++inst);
    }

    // Look for default
    QString translatedValue = PvlTranslationTable::Translate(transGroupName, "");
    QDomElement newElement = parentElement.ownerDocument().createElement(OutputName(transGroupName));
    setElementValue(newElement, translatedValue);
    parentElement.appendChild(newElement);
    if (transGroup.hasKeyword("OutputAttributes")) {
      addAttributes(transGroup.findKeyword("OutputAttributes"), newElement);
    }
    if (transGroup.hasKeyword("OutputSiblings")) {
      addSiblings(transGroup.findKeyword("OutputSiblings"), parentElement);
    }

  }


  /**
   * Set the inputLabel and automatically translate all the output names found in the translation
   * table.
   * If a output name does not translate an error will be thrown by one of the support members.
   * If the output name is translated, store the translated key, value pairs in the QDomDocument.
   *
   * @param inputLabel The input Pvl label
   * @param outputLabel The output QDomDocument label
   *
   * @see Auto(QDomDocument &outputLabel)
   */
  void PvlToXmlTranslationManager::Auto(Pvl &inputLabel,
                                        QDomDocument &outputLabel) {
    m_inputLabel = inputLabel;
    Auto(outputLabel);
  }


  /**
   * Automatically translate all the output names found in the translation table.
   * If a output name does not translate an error will be thrown by one of the support members.
   * If the output name is translated, store the translated key, value pairs in the QDomDocument.
   *
   * @param outputLabel A reference to the output QDomDocument label
   */
  void PvlToXmlTranslationManager::Auto(QDomDocument &outputLabel) {
    Pvl pvl;
    // Attempt to translate every group in the translation table
    for(int i = 0; i < TranslationTable().groups(); i++) {
      PvlGroup &g = TranslationTable().group(i);
      if(IsAuto(QString::fromStdString(g.name()))) {
        try {
          QDomElement element = outputLabel.documentElement();
          QDomElement *parentElement = createParentElements(QString::fromStdString(g.name()), element);
          // deal with siblings and attributes
          doTranslation(g, *parentElement);
        }
        catch(IException &e) {
          if(!IsOptional(QString::fromStdString(g.name()))) {
            throw;//???
          }
        }
      }
    }
  }


  /**
   * Uses the translation file group name to find the input label's PvlKeyword
   * that corresponds to the InputKey value of the translation table and returns
   * it, if found.
   *
   * @param transGroupName The name of the translation group for this InputKey.
   * 
   * @return @b PvlKeyword The input PvlKeyword for the translation group.
   *
   * @throws IException::Programmer
   */
  const PvlKeyword &PvlToXmlTranslationManager::InputKeyword(const QString transGroupName) const {

    int instanceNumber = 0;
    PvlKeyword inputGroupKeyword = InputGroup(transGroupName, instanceNumber);
    bool anInputGroupFound = false;

    while(inputGroupKeyword.name() != "") {
      const PvlContainer *containingGroup = GetContainer(inputGroupKeyword);
      if(containingGroup != NULL) {
        anInputGroupFound = true;

        if(containingGroup->hasKeyword(InputKeywordName(transGroupName))) {
          return containingGroup->findKeyword(InputKeywordName(transGroupName));
        }
      }

      instanceNumber ++;
      inputGroupKeyword = InputGroup(transGroupName, instanceNumber);
    }

    if(anInputGroupFound) {
      QString msg = "Unable to find input keyword [" + InputKeywordName(transGroupName) +
                     "] for output name [" + transGroupName + "] in file [" + 
                     TranslationTable().fileName() + "]";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    else {
      QString container = "";

      for(int i = 0; i < InputGroup(transGroupName).size(); i++) {
        if(i > 0) container += ",";

        container += InputGroup(transGroupName)[i];
      }

      QString msg = "Unable to find input group [" + container + "] for output name [" + 
                     transGroupName + "] in file [" + TranslationTable().fileName() + "]";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }


  /**
   * Indicates if the input keyword corresponding to the output name exists in
   * the label.
   *
   * @param transGroupName The output name used to identify the input keyword.
   * 
   * @return @b bool If the input keyword exists in the input label.
   */
  bool PvlToXmlTranslationManager::InputHasKeyword(const QString transGroupName) {

    // Set the current position in the input label pvl
    // by finding the input group corresponding to the output group
    const PvlContainer *con;
    int inst = 0;

    PvlKeyword grp;
    while((grp = InputGroup(transGroupName, inst++)).name() != "") {
      if((con = GetContainer(grp)) != NULL) {
        if(con->hasKeyword(InputKeywordName(transGroupName))) return true;
      }
    }

    return false;
  }


  /**
   * Return a container from the input label with the path given by the "InputPosition" keyword of
   * the translation table.
   *
   * @param inputGroup The InputPosition keyword
   *
   * @return @b PvlContainer* Pointer to the PvlContainer found
   */
  const PvlContainer *PvlToXmlTranslationManager::GetContainer(const PvlKeyword &inputGroup) const {


    // Return the root container if "ROOT" is the ONLY thing in the list
    if(inputGroup.size() == 1 &&
        PvlKeyword::stringEqual(inputGroup[0], "ROOT")) {
      return &m_inputLabel;
    }

    const PvlObject *currentObject = &m_inputLabel;

    // Search for object containing our solution
    int objectIndex;
    for(objectIndex = 0;
        objectIndex < inputGroup.size() - 1;
        objectIndex ++) {
      if(currentObject->hasObject(inputGroup[objectIndex])) {
        currentObject = &currentObject->findObject(inputGroup[objectIndex]);
      }
      else {
        return NULL;
      }
    }

    // Our solution can be an object or a group
    if(currentObject->hasObject(inputGroup[objectIndex])) {
      return &currentObject->findObject(inputGroup[objectIndex]);
    }
    else if(currentObject->hasGroup(inputGroup[objectIndex])) {
      return &currentObject->findGroup(inputGroup[objectIndex]);
    }
    else {
      return NULL;
    }
  }


  /**
   * Read the OutputPosition for the translation group name passed and create any parent elements
   * specified by OutputPosition to prepare for translation.
   *
   * @param const QString translationGroupName Name of the translation group we will grab
   *                                           OutputPosition from.
   * @param QDomElement &xmlRootElement The XML document root element to add to.
   *
   * @return @b QDomElement * Returns a pointer to the parent element right above the element
   *                          we are going to create when the caller starts the translation.
   */
  QDomElement *PvlToXmlTranslationManager::createParentElements(const QString translationGroupName,
                                                                QDomElement &xmlRootElement) {

    // Get the OutputPosition array using the name of the translation group
    PvlKeyword containers = OutputPosition(translationGroupName);

    QDomElement *currentElement = &xmlRootElement;

    int i = 0;
    // Check if the root node (e.g. Product_Observational) exits in the OutputPosition values
    // If so, skip over that OutputPosition value so we don't add it as a child of itself
    if (containers.size() > 0 && currentElement->tagName() == containers[0]) {
     i = 1;
    }

    // Look at all the containers and add any missing ones or ones explicitly requested with new@
    while (i < containers.size()) {

      // Parse current value in the OuputPosition
      // (i.e. parse into string tokens using "@" and ":" as delimiters)
      QStringList specifications = parseSpecification(containers[i]);

      bool addNewElement = false;
      // After parsing, if the first token is "new", then add a new child element
      if (specifications.size() == 2 && specifications[0] == "new") {
        addNewElement = true;
      }

      // Check if the specification says we need a new element
      if (addNewElement) {

        QDomElement childElement = xmlRootElement.ownerDocument().createElement(specifications[1]);
        *currentElement = currentElement->appendChild(childElement).toElement();
      }

      // If the current element does not have a direct child with the name at containers[i]
      else if (currentElement->namedItem(containers[i]).isNull()) {
        QDomElement childElement = xmlRootElement.ownerDocument().createElement(specifications[0]);
        *currentElement = currentElement->appendChild(childElement).toElement();
      }

      // Otherwise, if we are not requesting a container with @new, grab the child container
      else {
        *currentElement = currentElement->firstChildElement(containers[i]);
      }
      i++;
    }
    return currentElement;
  }


  /**
   * Take in outputSiblings PvlKeyword and turn each sibling into its corresponding QDomElement.
   * Then add the QDomElement to the parent as a child.
   *
   * @param outputSiblings The PvlKeyword that holds the list of siblings
   * @param parent The parent QDomElement
   *
   * @throws IException::Programmer "Malformed OutputSibling [" + outputSiblings[i] + "]"
   */
  void PvlToXmlTranslationManager::addSiblings(PvlKeyword outputSiblings,
                                               QDomElement &parent) {

    for (int i = 0; i < outputSiblings.size(); i++) {
      QStringList parsedSibling;
      parsedSibling.reserve(5);
      parsedSibling = parseSpecification(outputSiblings[i]);
      if (parsedSibling.size() != 2) {
        // If the sibling does not have a tag name AND a tag value
        QString msg = "Malformed OutputSibling [" + outputSiblings[i] + "]. OutputSiblings must" +
                      " be in the form of tag|value";
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }

      if (parent.namedItem(parsedSibling[0]).isNull()) {
        // parsedSibling[0] is the tag name, parsedSibling[1] is the tag value
        QDomElement childElement = parent.ownerDocument().createElement(parsedSibling[0]);
        setElementValue(childElement, parsedSibling[1]);
        parent.appendChild(childElement).toElement();;
      }
    }
  }


  /**
  * Take in the outputAttributes PvlKeyword and add each attribute to the appropriate
  * element given as an argument.
  *
  * @param outputAttributes The PvlKeyword that holds the list of siblings
  * @param element QDomElement to be modified
  *
  * @throws IException::Programmer "Malformed output attribute [" + outputAttributes[i] + ]."
  */
  void PvlToXmlTranslationManager::addAttributes(PvlKeyword outputAttributes,
                                                QDomElement &element) {
    QStringList parsedAttribute;

    for (int i = 0; i < outputAttributes.size(); i++) {
      parsedAttribute = parseSpecification(outputAttributes[i]);

      if (parsedAttribute.size() != 2) {
        QString msg = "Malformed output attribute [" + outputAttributes[i] +
                        "]. OutputAttributes must be in the form of att@attribute_name|value";
        throw IException(IException::Programmer,msg ,_FILEINFO_);
      }
      element.setAttribute(parsedAttribute[0], parsedAttribute[1]);
    }
  }


  /**
   * Add a QDomElement to the given parent with the indicated 
   * value and units. 
   *
   * @param parent The parent QDomElement of the new element.
   * @param name The name of the new element.
   * @param value The value of the new element.
   * @param units A string containing the unit specification for 
   *              the new element.
   */
  void PvlToXmlTranslationManager::addElement(QDomElement &parent,
                                              QString name,
                                              QString value,
                                              QString units) {
    QDomElement newElement = parent.ownerDocument().createElement(name);
    setElementValue(newElement, value, units);
    // append element to parent node???
    parent.appendChild(newElement).toElement();;
  }


  /**
   * Set the QDomElement's value, and units, if units != "".
   *
   * @param element The QDomElement whose value needs to be set.
   * @param value The value to set.
   * @param units A string containing the unit specification.
   */
  void PvlToXmlTranslationManager::setElementValue(QDomElement &element,
                                                   QString value,
                                                   QString units) {
    QDomText valueText = element.ownerDocument().createTextNode(value);
    // append value to element???
    element.appendChild(valueText);

    if (units != "") {
      element.setAttribute("unit", units);
    }
  }


  /**
   * Reset the QDomElement's value, and units, if units != "".
   *
   * @param element The QDomElement whose value needs to be reset.
   * @param value The value to set.
   * @param units A string containing the unit specification.
   */
  void PvlToXmlTranslationManager::resetElementValue(QDomElement &element,
                                                     QString value,
                                                     QString units) {
    element.firstChild().setNodeValue(value);
    if (units != "") {
      element.setAttribute("unit", units);
    }
  }
} // end namespace isis

