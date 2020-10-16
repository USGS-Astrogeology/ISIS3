#ifndef LabelTranslationManager_h
#define LabelTranslationManager_h
/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <string>
#include <fstream>

#include "FileName.h"
#include "PvlTokenizer.h"
#include "PvlTranslationTable.h"

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
   * @author 2017-01-11 Jeannie Backer
   *
   * @internal
   *  @history 2017-01-11 Jeannie Backer - Original Version. Code moved out of
   *                          PvlTranslationManager to make a generic parent
   *                          class. Fixes #4584.
   *  @history 2017-01-20 Jesse Mapel - Updated documentation and unit test.
   *                          Fixes #4584.
   *  @history 2017-05-26 Cole Neubauer - Moved parseDependancy from children
   *                          class. Fixes #5167.
   *  @history 2017-10-26 Kristin Berry - Modified parseSpecification to switch
   *                          from parsing translation table dependency specifications of the form
   *                          name:value to strings of the form name|value. Colons are
   *                          now used for namespaces only.
   */
  class LabelTranslationManager : public PvlTranslationTable {
    public:
      LabelTranslationManager();

      LabelTranslationManager(const QString &transFile);

      LabelTranslationManager(std::istream &transStrm);

      virtual ~LabelTranslationManager();

      // Attempt to translate the requested output name to output value
      // using the input name and value/default value
      virtual QString Translate(QString translationGroupName, int findex = 0) = 0;

      // Translate all translation table groups which contain "Auto"
      virtual void Auto(Pvl &outputLabel);

      virtual QStringList parseSpecification(QString specification) const;
    protected:

      virtual PvlKeyword DoTranslation(const QString translationGroupName);
      virtual PvlContainer *CreateContainer(const QString translationGroupName, Pvl &pvl);
  };
};

#endif
