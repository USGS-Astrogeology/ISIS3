/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <vector>
#include <string>
#include <iostream>

#include "Column.h"
#include "IException.h"
#include "IString.h"

namespace Isis {

  /**
   * Constructor. Sets the precision for decimal-aligned columns to 4
   */
  Column::Column() {
    p_precision = 4;
    p_width = 0;
    p_name = "";
    p_align = Column::NoAlign;
    p_type = Column::NoType;
  }

  /**
   * Constructor with parameter
   *
   * @param name The name of the column, used as the header
   * @param width The width (in characters) to make the column
   * @param type The type of information the column is to represent
   * @param align The alignment, within the column, the data is to conform to
   */
  Column::Column(QString name, int width, Column::Type type, Column::Align align) {
    p_precision = 4;
    p_width = 0;
    p_name = "";
    p_align = Column::NoAlign;
    p_type = Column::NoType;

    //Set the parameters with function calls, to make use of pre-existing error checks
    SetWidth(width);
    SetName(name);
    SetType(type);
    SetAlignment(align);
  }

  /**
   * Sets the Column name, or header
   *
   * @param name The name of the Column
   */
  void Column::SetName(QString name) {
    if (p_width != 0 && name.length() > (int)p_width) {
      std::string message = "Name [" + name.toStdString() + "] is wider than width";
      throw IException(IException::User, message, _FILEINFO_);
    }
    p_name = name;
  }

  /**
   * Sets the width of the Column, in text columns
   *
   * @param width The number of text columns the Column will hold
   */
  void Column::SetWidth(unsigned int width) {
    if (p_name.size() > 0 && p_name.size() > (int)width) {
      std::string message = "Width is insufficient to contain name[";
      message += p_name.toStdString() + "]";
      throw IException(IException::User, message, _FILEINFO_);
    }
    p_width = width;
  }

  /**
   * Sets the data type of the Column
   *
   * @param type The data type for the Column
   */
  void Column::SetType(Column::Type type) {
    if (p_align == Column::Decimal &&
        (type == Column::Integer || type == Column::String)) {
      std::string message = "Integer or string type is not sensible if ";
      message += "alignment is Decimal.";
      throw IException(IException::User, message, _FILEINFO_);
    }
    p_type = type;
  }

  /**
   * Sets the alignment of the Column
   *
   * The text in the Column will be aligned according to this parameter,
   * which is Right, Left, or, possible only with real-number values,
   * aligned by the decimal point
   *
   * @param alignment The alignment of the text in the Column
   */
  void Column::SetAlignment(Column::Align alignment) {
    if (alignment == Column::Decimal &&
        (p_type == Column::Integer || p_type == Column::String)) {
      std::string message = "Decimal alignment does not make sense for ";
      message += "integer or string values.";
      throw IException(IException::Programmer, message, _FILEINFO_);
    }
    p_align = alignment;
  }

  /**
   * Sets the precision of the Column, for real number values
   *
   * This sets the number of digits after the decimal point, for decimal aligned
   * values. If the Column's alignment is anything else, an error is thrown.
   *
   * @param precision The number of digits after the decimal point to be shown
   */
  void Column::SetPrecision(unsigned int precision) {
    if (DataType() != Column::Real &&
        DataType() != Column::Pixel) {
      std::string message = "Setting precision only makes sense for Decimal Alignment";
      throw IException(IException::User, message, _FILEINFO_);
    }
    p_precision = precision;
  }

  /**
   * Get the Column's name
   * 
   * @returns The column's name
   */
  QString Column::Name() const {
    return p_name;
  }

  /**
   * Get the Column's width
   * 
   * @returns The column's width
   */
  unsigned int Column::Width() const {
    return p_width;
  }


  /**
   * Returns the type of data this column will contain
   *
   * @return Column::Type The data type of this column
   */
  Column::Type Column::DataType() const {
    return p_type;
  }
  
  
  /** 
   * Get the Column's alignment
   * 
   * @returns The column's alignment
   */
  Column::Align Column::Alignment() const {
    return p_align;
  }


  /**
   * Get the Column's precision
   * 
   * @returns The column's presicion
   */
  unsigned int Column::Precision() const {
    return p_precision;
  }
}
