#ifndef PvlTranslationTable_h
#define PvlTranslationTable_h
/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <iostream>
#include <vector>
#include <string>
#include <QString>

#include "FileName.h"
#include "Pvl.h"

namespace Isis {
  /**
   * @brief Internalizes a translation table
   *
   * This class internalizes an Isis translation table and provides group/key
   * searches and value translations. The group names listed are the output names
   * used by applications to get the input names and values. There is only one
   * level of groups and groups are not allowed to be nested. The Group names can
   * NOT be repeated. OutputGroup is a comma delimited list of objects and/or
   * groups in the input label. Traversing this list within the input label
   * will lead to the correct level to find the input keyword. OutputKey is the
   * keyword within the group which holds the information. OutputDefault is the
   * value used if there is no value for the keyword. Translation is the output
   * and corresponding input values. Translation may be repeated as needed.
   * An example piece of a tranlation file:
   *   @code
   *     Group = DataStart
   *       OutputKey = ^IMAGE
   *       OutputDefault = 1
   *       Translation = (*,*)
   *     EndGroup
   *     Group = DataFileRecordBytes
   *       OutputKey = RECORD_BYTES
   *       Translation = (*,*)
   *     EndGroup
   *     Group = CoreSamples
   *       OutputGroup = IMAGE
   *       OutputKey = LINE_SAMPLES
   *       Translation = (*,*)
   *     EndGroup
   *     Group = CorePixelType
   *       OutputGroup = IMAGE
   *       OutputKey = SAMPLE_TYPE
   *       OutputDefault = LSB_INTEGER
   *       Translation = (Integer,LSB_INTEGER)
   *       Translation = (Integer,MSB_INTEGER)
   *       Translation = (Integer,PC_INTEGER)
   *       Translation = (Integer,MAC_INTEGER)
   *       Translation = (Integer,SUN_INTEGER)
   *       Translation = (Integer,VAX_INTEGER)
   *       Translation = (Natural,UNSIGNED_INTEGER)
   *       Translation = (Natural,LSB_UNSIGNED_INTEGER)
   *       Translation = (Natural,MSB_UNSIGNED_INTEGER)
   *     EndGroup
   *     Group = CoreOrganization
   *       OutputGroup = IMAGE
   *       OutputKey = BAND_STORAGE_TYPE
   *       OutputDefault = BAND_SEQUENTIAL
   *       Translation = (BSQ,BAND_SEQUENTIAL)
   *       Translation = (BIL,LINE_INTERLEAVED)
   *       Translation = (BIP,SAMPLE_INTERLEAVED)
   *     EndGroup
   *    End
   *   @endcode
   *
   * @ingroup Parsing
   *
   * @author 2003-05-01 Stuart Sides
   *
   * @internal
   *  @history 2003-09-03 Stuart Sides - Modified to work with new isis label format.
   *  @history 2005-02-15 Elizabeth Ribelin - Modified file to support Doxygen documentation.
   *  @history 2005-09-08 Stuart Sides - Modified Translate member to handle translating any
   *                          input value i.e., "*" to a specific output value (Thanks Kris
   *                          Becker).
   *  @history 2006-08-09 Brendan George - Added IsOptional function as part of
   *                          support for Optional keyword translations
   *  @history 2006-11-16 Brendan George - Changed instances of "Foreign" to "Input" and "Native"
   *                          to "Output".
   *  @history 2007-06-22 Stuart Sides - Added instance parameter to InputGroup member.
   *  @history 2006-12-05 Brendan George - Changed OutputPosition keyword to be case insensitive,
   *                          and added verification to the AddTable method so that
   *                          Translation Tables are verified when read in.
   *  @history 2008-07-10 Steven Lambright - Made trnsTbl member into a non-pointer
   *  @history 2010-01-04 Steven Lambright - Now using files instead of streams where possible
   *                          to improve errors and added code that checks the integrity of
   *                          translation files. Also now looking for "InputPosition" keyword
   *                          instead of "InputGroup." The InputGroup method (which needs
   *                          renamed later) now returns a PvlKeyword.
   *  @history 2017-11-04 Jeannie Backer - Modified Translation input value to make a case
   *                          insensitive comparison.
   *  @history 2018-04-16 Jeannie Backer - Updated documentation and improved
   *                          coding standards/style.
   *  
   *  
   *  
   *  @todo 2005-02-15 Stuart Sides - add coded and implementation example to
   *                       class documentation and finish documentation
   *                       for protected methods and variable
   */
  class PvlTranslationTable {

    public:
      // Constructors
      PvlTranslationTable(FileName transFile);
      PvlTranslationTable(std::istream &istr);
      PvlTranslationTable();

      virtual ~PvlTranslationTable();

      // Return the associated input group from the trans table
      virtual PvlKeyword InputGroup(const QString translationGroupName, const int inst = 0) const;

      // Return the associated input keyword name from the trans table
      virtual QString InputKeywordName(const QString translationGroupName) const;

      // Return the associated input default value from the trans table
      QString InputDefault(const QString translationGroupName) const;

      // Translate a single input value associated with a output name to a output value
      QString Translate(const QString translationGroupName, const QString inputKeyValue = "") const;

      // Add more table entries to the translation table data
      void AddTable(std::istream &transStm);
      void AddTable(const QString &transFile);

    protected:
      Pvl &TranslationTable();
      const Pvl &TranslationTable() const;
      virtual std::vector< std::pair<QString, int> > validKeywords() const;

      bool hasInputDefault(const QString translationGroupName);
      bool IsAuto(const QString translationGroupName);
      bool IsOptional(const QString translationGroupName);
      PvlKeyword OutputPosition(const QString translationGroupName);
      QString OutputName(const QString translationGroupName);
      const PvlGroup &findTranslationGroup(const QString translationGroupName) const;

    private:
      Pvl p_trnsTbl;
  };
};

#endif
