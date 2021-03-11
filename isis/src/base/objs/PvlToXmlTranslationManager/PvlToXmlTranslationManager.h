#ifndef PvlToXmlTranslationManager_h
#define PvlToXmlTranslationManager_h
/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "LabelTranslationManager.h"

#include <string>

#include <QDomDocument>

#include "FileName.h"
#include "Pvl.h"

class QDomElement;
class QString;

namespace Isis {
  class Pvl;
  class PvlContainer;
  class PvlKeyword;
  /**
   * @brief Allows applications to translate simple text files
   *
   * This class allows the translation of text files which can be parsed by the
   * Pvl class into Xml files.
   *
   * @ingroup Parsing
   *
   * @author 2017-05-24 Jeannie Backer and Ian Humphrey
   *
   * @internal
   *  @history 2017-05-24 Jeannie Backer and Ian Humphrey - Original version.
   *  @history 2017-05-26 Ian Humphrey & Makayla Shepherd - Added createParentElements.
   *  @history 2017-05-26 Makayla Shepherd & Adam Paquette - Added addSiblings and setElementValue.
   *  @history 2017-05-30 Makayla Shepherd - Renamed p_fLabel to m_inputLabel, and updated
   *                          documentation.
   *  @history 2017-05-31 Adam Paquette - Added addAttributes method.
   *  @history 2017-05-31 Ian Humphrey & Makayla Shepherd - Fixed duplicate root xml element in
   *                          output xml file.
   *  @history 2017-06-02 Makayla Shepherd - Made setElementValue public.
   *  @history 2017-10-18 Jeannie Backer & Makayla Shepherd - Added convenience methods, addElement
   *                          and resetElementValue, and made setElementValue static. See #5202.
   *  @history 2017-10-31 Jeannie Backer - Moved creation of sibling elements in doTranslation()
   *                          so that this only happens if the translation is succesful.
   */
  class PvlToXmlTranslationManager : public LabelTranslationManager {
    public:
      PvlToXmlTranslationManager(const QString &transFile);

      PvlToXmlTranslationManager(Pvl &inputLabel,
                            const QString &transFile);

      virtual ~PvlToXmlTranslationManager();

      // Attempt to translate the requested output name to output value
      // using the input name and value/default value
      virtual QString Translate(QString translationGroupName, int inputIndex = 0);

      // Translate all translation table groups which contain "Auto"
      void Auto(QDomDocument &outputLabel);
      void Auto(Pvl &inputLabel, QDomDocument &outputLabel);

      // Return the ith input value associated with a output name
      virtual const PvlKeyword &InputKeyword(const QString translationGroupName) const;

      // Return true if the input lable contains the translated group and key names
      virtual bool InputHasKeyword(const QString translationGroupName);

      void SetLabel(Pvl &inputLabel);

      static void addElement(QDomElement &parent, QString name, QString value, QString units = "");
      static void setElementValue(QDomElement &element, QString value, QString units = "");
      static void resetElementValue(QDomElement &element, QString value, QString units = "");


    protected:
      void doTranslation(PvlGroup transGroup, QDomElement &parent);
      virtual const PvlContainer *GetContainer(const PvlKeyword &inputGroup) const;

      virtual std::vector< std::pair<QString, int> > validKeywords() const;
      bool checkDependencies(QDomElement element, PvlKeyword dependencies, bool isDebug) const;
      QDomElement *createParentElements(const QString translationGroupName, QDomElement &xml);
      void addSiblings(PvlKeyword outputSiblings, QDomElement &parent);
      void addAttributes(PvlKeyword something, QDomElement &parent);

    private:
      Pvl m_inputLabel; //!< A Pvl object for the input label file
  };
};

#endif
