#ifndef PvlToPvlTranslationManager_h
#define PvlToPvlTranslationManager_h
/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "LabelTranslationManager.h"

#include <fstream>
#include <string>

#include "FileName.h"
#include "PvlTokenizer.h"

namespace Isis {
  class Pvl;
  class PvlContainer;
  class PvlKeyword;
  /**
   * @brief Allows applications to translate simple text files
   *
   * This class allows the translation of text files which can be parsed by the
   * Pvl class.
   *
   * @ingroup Parsing
   *
   * @author 2003-05-29 Stuart Sides
   *
   * @internal
   *  @history 2003-09-03 Stuart Sides - Modified to work with new isis label format.
   *  @history 2003-09-25 Stuart Sides - Added the Translate member.
   *  @history 2005-02-15 Elizabeth Ribelin - Modified file to support Doxygen documentation.
   *  @history 2006-08-09 Brendan George - Modified to support Optional keyword translation.
   *  @history 2006-10-01 Stuart Sides - Fixed bug with Optional keyword.
   *                          Non-optional keywords were being reported instantly.
   *  @history 2006-11-16 Brendan George - Changed instances of "Foreign" to "Input"
   *                          and "Native" to "Output".
   *  @history 2007-06-22 Stuart Sides - Added ability to have more than one input location
   *                          keyword for a translation. The first one found which contains
   *                          the input keyword is used.
   *  @history 2008-05-09 Steven Lambright - Added ability to change input label without
   *                          re-reading the translation file.
   *  @history 2008-07-10 Noah Hilt - Changed while loops to continue searching other groups
   *                          if a group has been found, but the keyword does not exist in
   *                          that group.
   *  @history 2008-07-10 Steven Lambright - Changed to use new accessors.
   *  @history 2010-01-04 Steven Lambright - Added InputKeyword method and removed
   *                          InputSize, InputUnits, InputValue. Renamed private Translate() method
   *                          to DoTranslation() to remove ambiguity with a parent method,
   *                          instead of using a dummy parameter.
   *  @history 2017-01-11 Jeannie Backer - Moved several methods to a generic parent class,
   *                          LabelTranslationManager. Fixes #4584.
   *  @history 2017-06-13 Adam Paquette - Changed PvlTranslationManager file name to
   *                          PvlToPvlTranslationManager. Fixes #4901.
   *  @history 2018-01-10 Christopher Combs - Changed ProcessDataFilePointer call to reflect 
   *                          changes made to voy2isis. Fixes #4345, #4421.
   *  @history 2018-04-16 Jeannie Backer - Fixed indentation of history comments and
   *                          brought code closer to coding standards.
   *  @todo 2005-02-15 Stuart Sides - add coded example and implementation example
   *                          to class documentation, and finish
   *                          documentation.
   */
  class PvlToPvlTranslationManager : public LabelTranslationManager {
    public:
      PvlToPvlTranslationManager(const QString &transFile);

      PvlToPvlTranslationManager(std::istream &transStrm);

      PvlToPvlTranslationManager(Pvl &inputLabel,
                            const QString &transFile);

      PvlToPvlTranslationManager(Pvl &inputLabel,
                            std::istream &transStrm);

      virtual ~PvlToPvlTranslationManager();

      // Attempt to translate the requested output name to output value
      // using the input name and value/default value
      virtual QString Translate(QString translationGroupName, int findex = 0);

      // Translate all translation table groups which contain "Auto"
      void Auto(Pvl &outputLabel);
      void Auto(Pvl &inputLabel, Pvl &outputLabel);

      // Return the ith input value associated with a output name
      virtual const PvlKeyword &InputKeyword(const QString translationGroupName) const;

      // Return true if the input lable contains the translated group and key names
      virtual bool InputHasKeyword(const QString translationGroupName);

      void SetLabel(Pvl &inputLabel);

    protected:
      virtual PvlKeyword DoTranslation(const QString translationGroupName);
      virtual const PvlContainer *GetContainer(const PvlKeyword &inputGroup) const;
      virtual PvlContainer *CreateContainer(const QString translationGroupName, Pvl &pvl);
    private:
      Pvl p_fLabel; //!< A Pvl object for the input label file
  };
};

#endif
