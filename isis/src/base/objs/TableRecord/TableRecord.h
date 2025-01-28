#ifndef TableRecord_h
#define TableRecord_h
/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <vector>
#include "TableField.h"

namespace Isis {
  /**
   * @brief
   *
   *
   *
   * @ingroup LowLevelCubeIO
   *
   * @author 2004-09-01 Jeff Anderson
   *
   * @internal
   *   @history 2005-03-18 Elizabeth Ribelin - Added documentation to class
   *   @history 2007-05-28 Steven Lambright - Added 4 byte float capablilities.
   *   @history 2008-06-19 Christopher Austin - Fixed the Packing of text TableFields
   *   @history 2008-06-25 Christopher Austin - Fixed the swapping of text
   *   @history 2012-10-04 Jeannie Backer Changed references to TableField methods in implementation
   *                           and unitTest files to lower camel case. Added and ordered includes.
   *                           Moved method implementation to cpp. Fixed header definition
   *                           statement. Fixed indentation of history entries. Ordered methods in
   *                           cpp file. Improved test coverage in all categories. Added padding to
   *                           control statements. References #1169.
   *   @history 2015-10-04 Jeannie Backer Improved coding standards. References #1178
   *  
   *   @todo Finish class documentation
   */
  class TableRecord {
    public:
      TableRecord();
      TableRecord(std::string tableRecordStr, char fieldDelimiter, 
                            std::vector<QString> fieldNames, int numOfFieldValues); 
      ~TableRecord();

      
      static QString toString(TableRecord record, QString fieldDelimiter = ",", bool fieldNames = false, bool endLine = true);
        
      void operator+=(Isis::TableField &field);
      TableField&operator [](const int field);
      TableField &operator[](const QString &field);

      int Fields() const;
      int RecordSize() const;

      void Pack(char *buf) const;
      void Unpack(const char *buf);
      void Swap(char *buf) const;

    private:
      std::vector<TableField> p_fields; /**< Vector of TableFields in the
                                                  record. */
  };
};

#endif
