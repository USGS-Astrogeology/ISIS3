/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "PvlTranslationTable.h"

#include "IException.h"
#include "IString.h"
#include "LabelTranslationManager.h"
#include "Message.h"
#include "Pvl.h"
#include "PvlContainer.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "PvlObject.h"

#include <QString>
#include <QStringList>

using namespace std;
namespace Isis {

  /**
   * Constructs a default LabelTranslationManager.
   */
  LabelTranslationManager::LabelTranslationManager()
      : PvlTranslationTable() {
  }


  /**
   * Constructs a LabelTranslationManager with a given translation table.
   *
   * @param transfile The translation table file.
   */
  LabelTranslationManager::LabelTranslationManager(const QString &transFile)
      : PvlTranslationTable() {
    AddTable(transFile);
  }


  /**
   * Constructs and initializes a LabelTranslationManager object
   *
   * @param transStrm A stream containing the tranlation table to be used to
   *                  tranlate keywords in the input label.
   */
  LabelTranslationManager::LabelTranslationManager(std::istream &transStrm)
      : PvlTranslationTable() {
    AddTable(transStrm);
  }


  /**
   * Destroys the LabelTranslationManager object.
   */
  LabelTranslationManager::~LabelTranslationManager() {
  }


  /**
   * Automatically translate all the output names tagged as Auto in the
   * translation table If a output name does not translate an error will be
   * thrown by one of the support members.
   *
   * The results of the translations will be stored in the outputLabel PVL
   * based on the OutputPosition keywords in the translation table.
   *
   * @param outputLabel The PVL to add the translated keywords to.
   */
  void LabelTranslationManager::Auto(Pvl &outputLabel) {
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
            throw e;//??? is this needed???
          }
        }
      }
    }
  }


  /**
   * Creates all parent PVL containers for an output keyword. If any parent
   * containers already exist then they will not be recreated.
   *
   * @param translationGroupName The name of the output keyword. The OutputPosition keyword
   *              in the translation group for translationGroupName will be used to determine
   *              which containers are made.
   * @param pvl The PVL file to create the containers in.
   *
   * @return @b PvlContainer The immediate parent container for translationGroupName.
   */
  PvlContainer *LabelTranslationManager::CreateContainer(const QString translationGroupName,
                                                         Pvl &pvl) {

    // Get the array of Objects/Groups from the OutputName keyword
    PvlKeyword np = OutputPosition(translationGroupName);

    PvlObject *obj = &pvl;

    // Look at every pair in the output position
    for(int c = 0; c < np.size(); c += 2) {
      // If this pair is an object
      if(QString::fromStdString(np[c]).toUpper() == "OBJECT") {
        // If the object doesn't exist create it
        if(!obj->hasObject(np[c+1])) {
          obj->addObject(np[c+1]);
        }
        obj = &(obj->findObject(np[c+1]));
      }
      // If this pair is a group
      else if(QString::fromStdString(np[c]).toUpper() == "GROUP") {
        // If the group doesn't exist create it
        if(!obj->hasGroup(np[c+1])) {
          obj->addGroup(np[c+1]);
        }
        return (PvlContainer *) & (obj->findGroup(np[c+1]));

      }
    }

    return (PvlContainer *) obj;
  }


  /**
   * Translate the requested output name to output values using the input name
   * and values or default value
   *
   * @param outputName The output name used to identify the input keyword to be
   *                   translated.
   *
   * @return @b PvlKeyword A keyword containing the output name and output value.
   *
   * @TODO output units
   */
  PvlKeyword LabelTranslationManager::DoTranslation(const QString outputName) {
    PvlKeyword outputKeyword( outputName.toStdString(), Translate(outputName).toStdString() );
    return outputKeyword;
  }


  /**
 * Parses and validates a dependency specification.
 *
 * @param specification The dependency specification string.
 *
 * @return @b QStringList The dependency split into 3 components
 *                        <ol>
 *                          <li>the type (att or tag)</li>
 *                          <li>the name of what to check</li>
 *                          <li>the value to check for</li>
 *                        </ol>
 *
 * @throws IException::Programmer "Malformed dependency specification."
 * @throws IException::Programmer "Specification does not have two components
 *                                 separated by [@], the type of dependency and
 *                                 the name-value pair.
 * @throws IException::Programmer "Dependency type specification is invalid.
 *                                 Valid types are [att] and [tag]"
 * @throws IException::Programmer "Name-value specification does not have two
 *                              components separated by [|]."
 *
 */
QStringList LabelTranslationManager::parseSpecification(QString specification) const {

  QStringList parsedSpecification;

  try {
    QStringList typeSplit = specification.split("@", Qt::SkipEmptyParts); 
    QStringList barSplit = specification.split("|", Qt::SkipEmptyParts);
   
    if (typeSplit.size() == 2) { //handle tag@elementname|value
      if (typeSplit[0].toLower() != "att" &&
          typeSplit[0].toLower() != "tag" &&
          typeSplit[0].toLower() != "new") {
        std::string msg = "Dependency type specification [" + typeSplit[0].toStdString() +
                      "] is invalid. Valid types are [att], [tag] and [new]";
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }
      parsedSpecification.append(typeSplit[0].toLower());

      QStringList nameValueSplit = typeSplit[1].split("|", Qt::SkipEmptyParts);
      if (nameValueSplit.size() == 2) {
        parsedSpecification.append(nameValueSplit);
      }
      else if (nameValueSplit.size() == 1) {
        parsedSpecification.append(nameValueSplit);
      }
      else { //nameValueSplit is an unexpected value
        std::string msg = "Malformed dependency specification [" + specification.toStdString() + "].";
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }
    }
    else if (barSplit.size() == 2) { //handle elementname|value
      parsedSpecification = barSplit;
    }
    else if (barSplit.size() == 1 && typeSplit.size() == 1) { //handle value with no "@" or "|" characters
      parsedSpecification = barSplit;
    }
    else { //nameValueSplit is an unexpected value
      std::string msg = " [" + specification.toStdString() + "] has unexpected number of '@' or '|' delimiters";
      throw IException(IException::Programmer,msg, _FILEINFO_);
    }
  }

  catch (IException &e) {
    std::string msg = "Malformed dependency specification [" + specification.toStdString() + "].";
    throw IException(e, IException::Programmer, msg, _FILEINFO_);
  }

  return parsedSpecification;
}

} // end namespace isis
