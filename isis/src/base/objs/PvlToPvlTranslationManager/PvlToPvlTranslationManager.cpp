/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "LabelTranslationManager.h"

#include "IException.h"
#include "IString.h"
#include "Message.h"
#include "Pvl.h"
#include "PvlContainer.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "PvlObject.h"
#include "PvlToPvlTranslationManager.h"

using namespace std;
namespace Isis {

  /**
   * Constructs and initializes a TranslationManager object from given the
   * Pvl translation file. If this constructor is used, the user will need to
   * set the input label before translating. This may be done by using
   * SetLabel(Pvl inputLabel) or Auto(Pvl inputLabel, Pvl outputLabel).
   *
   * @param transFile The translation file to be used to translate keywords in
   *                  the input label.
   */
  PvlToPvlTranslationManager::PvlToPvlTranslationManager(const QString &transFile)
      : LabelTranslationManager(transFile) {
  }

  /**
   * Constructs and initializes a TranslationManager object from the given
   * input stream. If this constructor is used, the user will need to set the
   * input label before translating. This may be done by using SetLabel(Pvl
   * inputLabel) or Auto(Pvl inputLabel, Pvl outputLabel).
   *
   * @param transStrm A stream containing the translation table to be used to
   *                  translate keywords in the input label.
   */
  PvlToPvlTranslationManager::PvlToPvlTranslationManager(std::istream &transStrm)
      : LabelTranslationManager(transStrm) {
  }


  /**
   * Constructs and initializes a TranslationManager object
   *
   * @param inputLabel The Pvl holding the input label.
   *
   * @param transFile The translation file to be used to translate keywords in
   *                  the input label.
   */
  PvlToPvlTranslationManager::PvlToPvlTranslationManager(Pvl &inputLabel,
                                               const QString &transFile)
      : LabelTranslationManager(transFile) {
    p_fLabel = inputLabel;
  }


  /**
   * Constructs and initializes a TranslationManager object
   *
   * @param inputLabel The Pvl holding the input label.
   *
   * @param transStrm A stream containing the translation table to be used to
   *                  translate keywords in the input label.
   */
  PvlToPvlTranslationManager::PvlToPvlTranslationManager(Pvl &inputLabel,
      std::istream &transStrm)
      : LabelTranslationManager(transStrm) {
    p_fLabel = inputLabel;
  }


  //! Destroys the TranslationManager object.
  PvlToPvlTranslationManager::~PvlToPvlTranslationManager() {
  }


  void PvlToPvlTranslationManager::SetLabel(Pvl &inputLabel) {
    p_fLabel = inputLabel;
  }


  /**
   * Returns a translated value. The output name is used to find the input
   * group, keyword, default and translations in the translation table. If the
   * keyword does not exist in the input label and an input default is available,
   * then this default will be used as the input value. This input value is
   * then used to search all of the translations. If a match is found the
   * translated value is returned. 
   *
   * @param translationGroupName The name of the PVL translation 
   *                             group used to identify the
   *                             input/output keywords to be
   *                             translated. Often, this is the
   *                             same as the output keyword name.
   *
   * @param findex The index into the input keyword array.  Defaults to 0
   *
   * @return string
   */
  QString PvlToPvlTranslationManager::Translate(QString translationGroupName, int findex) {
    const PvlContainer *con;
    int inst = 0;
    PvlKeyword grp;

    while((grp = InputGroup(translationGroupName, inst++)).name() != "") {
      if((con = GetContainer(grp)) != NULL) {
        if(con->hasKeyword(InputKeywordName(translationGroupName).toStdString())) {
          return PvlTranslationTable::Translate(translationGroupName,
                                                QString::fromStdString((*con)[InputKeywordName(translationGroupName).toStdString()][findex]));
        }
      }
    }

    return PvlTranslationTable::Translate(translationGroupName);
  }


  /**
   * Translate the requested output name to output values using the input name
   * and values or default value.
   *  
   * Note: This is a protected method used when automatically 
   * translating
   *  
   * @see Auto().
   *
   * @param translationGroupName The name of the PVL translation 
   *                             group used to identify the
   *                             input/output keywords to be
   *                             translated. Often, this is the
   *                             same as the output keyword name.
   *
   * @return PvlKeyword
   */
   PvlKeyword PvlToPvlTranslationManager::DoTranslation(const QString translationGroupName) {
     const PvlContainer *con = NULL;
     PvlKeyword key;

     int inst = 0;
     PvlGroup transGroup;
     PvlKeyword grp;

     while((grp = InputGroup(translationGroupName, inst++)).name() != "") {
       if((con = GetContainer(grp)) != NULL) {
         transGroup = TranslationTable().findGroup(translationGroupName.toStdString());
         Pvl::ConstPvlKeywordIterator it = transGroup.findKeyword("InputKey",
                                           transGroup.begin(),
                                           transGroup.end());
         // Loop through potential InputKeys in the translation file group currently beginning
         // translated.
         while(it != transGroup.end()) {
           const PvlKeyword &result = *it;
           if(con->hasKeyword(result[0])) {
             key.setName(OutputName(translationGroupName).toStdString());

             for(int v = 0; v < (*con)[(result[0])].size(); v++) {
               key.addValue(PvlTranslationTable::Translate(translationGroupName, QString::fromStdString((*con)[result[0]][v])).toStdString(),(*con)[result[0]].unit(v));
             }

             return key;
           }
           it = transGroup.findKeyword("InputKey", it + 1, transGroup.end());
         }
       }
     }

     return PvlKeyword(OutputName(translationGroupName).toStdString(),
                       PvlTranslationTable::Translate(translationGroupName, "").toStdString());
   }


  /**
   * Automatically translate all the output names found in the translation table
   * If a output name does not translate an error will be thrown by one
   * of the support members
   * Store the translated key, value pairs in the argument pvl
   */
  void PvlToPvlTranslationManager::Auto(Pvl &inputLabel, Pvl &outputLabel) {
    p_fLabel = inputLabel;
    Auto(outputLabel);
  }


  /**
   * Automatically translate all the output names found in the translation table
   * If a output name does not translate an error will be thrown by one
   * of the support members
   * Store the translated key, value pairs in the argument pvl
   */
  void PvlToPvlTranslationManager::Auto(Pvl &outputLabel) {
    // Attempt to translate every group in the translation table
    for(int i = 0; i < TranslationTable().groups(); i++) {
      PvlGroup &g = TranslationTable().group(i);
      if(IsAuto(QString::fromStdString(g.name()))) {
        try {
          PvlContainer *con = CreateContainer(QString::fromStdString(g.name()), outputLabel);
          (*con) += DoTranslation(QString::fromStdString(g.name()));
        }
        catch(IException &e) {
          if(!IsOptional(QString::fromStdString(g.name()))) {
            throw;
          }
        }
      }
    }
  }


  /**
   * Returns the ith input value associated with the output name argument.
   *
   * @param translationGroupName The name of the PVL translation 
   *                             group used to identify the
   *                             input/output keywords to be
   *                             translated. Often, this is the
   *                             same as the output keyword name.
   *
   * @param findex The index into the input keyword array.  Defaults to 0
   *
   * @throws IException::Programmer
   */
  const PvlKeyword &PvlToPvlTranslationManager::InputKeyword(const QString translationGroupName) const {

    int instanceNumber = 0;
    PvlKeyword inputGroupKeyword = InputGroup(translationGroupName, instanceNumber);
    bool anInputGroupFound = false;

    while(inputGroupKeyword.name() != "") {
      const PvlContainer *containingGroup = GetContainer(inputGroupKeyword);
      if(containingGroup != NULL) {
        anInputGroupFound = true;

        if(containingGroup->hasKeyword(InputKeywordName(translationGroupName).toStdString())) {
          return containingGroup->findKeyword(InputKeywordName(translationGroupName).toStdString());
        }
      }

      instanceNumber ++;
      inputGroupKeyword = InputGroup(translationGroupName, instanceNumber);
    }

    if(anInputGroupFound) {
      std::string msg = "Unable to find input keyword [" + InputKeywordName(translationGroupName).toStdString() +
                   "] for output name [" + translationGroupName.toStdString() + "] in file [" + TranslationTable().fileName() + "]";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    else {
      QString container = "";

      for(int i = 0; i < InputGroup(translationGroupName).size(); i++) {
        if(i > 0) container += ",";

        container += QString::fromStdString(InputGroup(translationGroupName)[i]);
      }

      std::string msg = "Unable to find input group [" + container.toStdString() +
                   "] for output name [" + translationGroupName.toStdString() + "] in file [" + TranslationTable().fileName() + "]";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }


  /**
   * Indicates if the input keyword corresponding to the output name exists in
   * the label
   *
   * @param translationGroupName The name of the PVL translation 
   *                             group used to identify the
   *                             input/output keywords to be
   *                             translated. Often, this is the
   *                             same as the output keyword name.
   */
  bool PvlToPvlTranslationManager::InputHasKeyword(const QString translationGroupName) {

    // Set the current position in the input label pvl
    // by finding the input group corresponding to the output group
    const PvlContainer *con;
    int inst = 0;
    //while ((con = GetContainer(InputGroup(translationGroupName, inst++))) != NULL) {
    //if ((con = GetContainer (InputGroup(translationGroupName))) != NULL) {

    PvlKeyword grp;
    while((grp = InputGroup(translationGroupName, inst++)).name() != "") {
      if((con = GetContainer(grp)) != NULL) {
        if(con->hasKeyword(InputKeywordName(translationGroupName).toStdString())) return true;
      }
    }

    return false;
  }


  //! Return a container from the input label according tund
  const PvlContainer *PvlToPvlTranslationManager::GetContainer(const PvlKeyword &inputGroup) const {


    // Return the root container if "ROOT" is the ONLY thing in the list
    if(inputGroup.size() == 1 &&
        PvlKeyword::stringEqual(inputGroup[0], "ROOT")) {
      return &p_fLabel;
    }

    const PvlObject *currentObject = &p_fLabel;

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
   * Create the requsted container and any containers above it and
   * return a reference to the container. List is a PvlKeyword 
   * with an array of container types and their names. 
   *  
   * @param translationGroupName The name of the PVL translation 
   *                        group used to identify the
   *                        input/output keywords to be
   *                        translated. Often, this is the
   *                        same as the output keyword name.
   * @param pvl Pvl 
   */
  PvlContainer *PvlToPvlTranslationManager::CreateContainer(const QString translationGroupName, Pvl &pvl) {
    return LabelTranslationManager::CreateContainer(translationGroupName, pvl);
  }
} // end namespace isis
