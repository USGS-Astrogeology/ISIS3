/**
 * @file
 * $Revision: 1.1 $
 * $Date: 2007/08/09 18:24:24 $
 *
 *   Unless noted otherwise, the portions of Isis written by the USGS are public
 *   domain. See individual third-party library and package descriptions for
 *   intellectual property information,user agreements, and related information.
 *
 *   Although Isis has been used by the USGS, no warranty, expressed or implied,
 *   is made by the USGS as to the accuracy and functioning of such software
 *   and related material nor shall the fact of distribution constitute any such
 *   warranty, and no responsibility is assumed by the USGS in connection
 *   therewith.
 *
 *   For additional information, launch
 *   $ISISROOT/doc//documents/Disclaimers/Disclaimers.html in a browser or see
 *   the Privacy &amp; Disclaimers page on the Isis website,
 *   http://isis.astrogeology.usgs.gov, and the USGS privacy and disclaimers on
 *   http://www.usgs.gov/privacy.html.
 */

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "WriteTabular.h"
#include "IString.h"
#include "Message.h"
#include "IException.h"
#include "SpecialPixel.h"

using std::stringstream;

namespace Isis {

  /**
   * Constructor
   * @param filename The name of the file where the table will be written
   */
  WriteTabular::WriteTabular(std::ostream &strm) : p_outfile(strm) {
    p_rows = 0;
    p_delimiter = ",";
    p_curCol = 0;
  }

  /**
   * Constructor
   * @param filename The name of the target file to contain the table, once
   *                 formatted
   * @param cols The Column headers, containing information about the Columns
   */
  WriteTabular::WriteTabular(std::ostream &strm, std::vector<Column> cols) : p_outfile(strm) {
    p_rows = 0;
    p_delimiter = ",";
    p_curCol = 0;
    SetColumns(cols);
  }

  /**
   * Sets the vector of Columns and writes out the first row of the file.
   *
   * @param cols A vector of Columns, setting the format of the table
   */
  void WriteTabular::SetColumns(std::vector <Column> cols) {
    for(unsigned int index = 0; index < cols.size(); index++) {
      Column thisCol = cols[index];
      QString thisTitle = thisCol.Name();

      if((int)thisTitle.length() > (int)thisCol.Width()) {
        QString message = "Column header [" + thisTitle + "] is wider " +
                              "than the set width for column [" + toString((int)index) + "]";
        throw IException(IException::User, message, _FILEINFO_);
      }

      while((int)thisTitle.length() < (int)thisCol.Width()) {
        if(thisCol.Alignment() == Column::Left) {
          thisTitle += " ";
        }
        else if(thisCol.Alignment() == Column::Right ||
                thisCol.Alignment() == Column::Decimal) {
          thisTitle = " " + thisTitle;
        }
        else {
          QString message = "Alignment is improperly set";
          throw IException(IException::User, message, _FILEINFO_);
        }
      }//end while

      p_cols.push_back(thisCol);
      p_outfile << thisTitle;
      if(index < (cols.size() - 1)) {
        p_outfile << p_delimiter;
      }
    }//end for
    p_outfile << "\n";
  }//end function

  /**
   * Writes a blank space in the next column in the current row
   */
  void WriteTabular::Write() {
    Column thisCol = p_cols[p_curCol];

    QString item = "";

    stringstream tempStream;
    tempStream.width(thisCol.Width());
    tempStream.fill(' ');
    tempStream << item;
    item = tempStream.str().c_str();

    if(p_curCol == 0) {
      p_rows++;
    }

    if(p_curCol < (p_cols.size() - 1)) {
      item += p_delimiter;
      p_curCol++;
    }
    else {
      item += "\n";
      p_curCol = 0;
    }
    p_outfile << item;
  }

  /**
   * Add an integer value to the next column in this row
   *
   * @param item The integer value to put in this column.
   */
  void WriteTabular::Write(int item) {
    Column thisCol = p_cols[p_curCol];
    if(thisCol.DataType() != Column::Integer &&
        thisCol.DataType() != Column::Pixel) {
      if(thisCol.DataType() == Column::Real ||
          thisCol.DataType() == Column::Pixel) {
        Write((double)item);
        return;
      }
      QString message = "Wrong data type for this Column";
      throw IException(IException::User, message, _FILEINFO_);
    }
    QString thisItem(toString(item));
    if(thisItem.length() > (int)thisCol.Width()) {
      thisItem = "*";
      while(thisItem.length() < (int)thisCol.Width()) {
        thisItem += "*";
      }
    }
    stringstream tempStream;
    tempStream.width(thisCol.Width());
    tempStream.fill(' ');

    if(thisCol.Alignment() == Column::Left) {
      tempStream.setf(std::ios::left);
    }
    else tempStream.setf(std::ios::right);

    tempStream << thisItem;
    thisItem = tempStream.str().c_str();

    if(p_curCol == 0) {
      p_rows++;
    }

    if(p_curCol < (p_cols.size() - 1)) {
      thisItem += p_delimiter;
      p_curCol++;
    }
    else {
      thisItem += "\n";
      p_curCol = 0;
    }
    p_outfile << thisItem;
  }

  /**
   * Writes a string to the next column in the current row
   *
   * @param item The string to write out
   */
  void WriteTabular::Write(const char *itemCStr) {
    Column thisCol = p_cols[p_curCol];
    if(thisCol.DataType() != Column::String &&
        thisCol.DataType() != Column::Pixel) {
      QString message = "Wrong data type for this Column";
      throw IException(IException::User, message, _FILEINFO_);
    }

    QString item(itemCStr);
    if(item.length() > (int)thisCol.Width()) {
      item = "*";
      while(item.length() < (int)thisCol.Width()) {
        item += "*";
      }
    }
    stringstream tempStream;
    tempStream.width(thisCol.Width());
    tempStream.fill(' ');

    if(thisCol.Alignment() == Column::Left) {
      tempStream.setf(std::ios::left);
    }
    else tempStream.setf(std::ios::right);

    tempStream << item;
    item = tempStream.str().c_str();

    if(p_curCol == 0) {
      p_rows++;
    }

    if(p_curCol < (p_cols.size() - 1)) {
      item += p_delimiter;
      p_curCol++;
    }
    else {
      item += "\n";
      p_curCol = 0;
    }
    p_outfile << item;
  }

  /**
   * Writes a floating-point value out to the next column in the current row
   *
   * @param item The value to be printed out
   */
  void WriteTabular::Write(double item) {
    Column thisCol = p_cols[p_curCol];
    if(thisCol.DataType() != Column::Real &&
        thisCol.DataType() != Column::Pixel) {
      QString message = "Wrong data type for this Column";
      throw IException(IException::User, message, _FILEINFO_);
    }

    //Check for special pixels, if it's a pixel column
    if(thisCol.DataType() == Column::Pixel && IsSpecial(item)) {
      if(IsNullPixel(item)) {
        Write("Null");
        return;
      }
      if(IsHisPixel(item)) {
        Write("His");
        return;
      }
      if(IsHrsPixel(item)) {
        Write("Hrs");
        return;
      }
      if(IsLisPixel(item)) {
        Write("Lis");
        return;
      }
      if(IsLrsPixel(item)) {
        Write("Lrs");
        return;
      }
    }

    QString thisItem(toString(item));


    if(thisCol.Alignment() == Column::Decimal) {

      //Format and round the number

      //First, split the number at the decimal point
      QStringList tempString = thisItem.split(".");
      QString intPart = tempString.takeFirst();

      //Make the fractional portion appear as such, so the iomanipulators
      //handle it properly
      if(!tempString.isEmpty()) {
        tempString.prepend("0");
      }
      else {
        tempString.append("0");
        tempString.append("0");
      }

      //Put the fractional portion into a stringstream, and use
      //stream manipulators to round it properly
      stringstream b;
      b << std::showpoint
        << std::setprecision(thisCol.Precision())
        << toDouble(tempString.join("."));

      //if the rounding causes a rollover (i.e. the decimal portion is greater
      //than 0.95) increment the integer portion
      if(toDouble(QString(b.str().c_str())) >= 1) {
        intPart = toString(toInt(intPart) + 1);
      }

      //Put it back into an QString, for easier manipulation
      QString tempString2 = b.str().c_str();
      tempString2.remove(QRegExp("[^.]*\\."));
      //Add any zeros necessary to pad the number
      while(tempString2.size() < (int)thisCol.Precision()) {
        tempString2 += "0";
      }

      //Put the number back together, adding the decimal point in the right location
      thisItem = intPart + "." + tempString2;
    }
    stringstream tempStream;
    tempStream.width(thisCol.Width());
    tempStream.fill(' ');

    if(thisCol.Alignment() == Column::Left) {
      tempStream.setf(std::ios::left);
    }
    else tempStream.setf(std::ios::right);

    tempStream << thisItem;
    thisItem = tempStream.str().c_str();

    if(p_curCol == 0) {
      p_rows++;
    }

    //If the number is too wide for the column, replace with a string of stars
    if(thisItem.length() > (int)thisCol.Width()) {
      thisItem = "*";
      while(thisItem.length() < (int)thisCol.Width()) {
        thisItem += "*";
      }
    }

    if(p_curCol < (p_cols.size() - 1)) {
      thisItem += p_delimiter;
      p_curCol++;
    }
    else {
      thisItem += "\n";
      p_curCol = 0;
    }
    p_outfile << thisItem;

  }

  /**
   * Sets the string to be put between columns for this table
   *
   * @param delim The string to separate columns
   */
  void WriteTabular::SetDelimiter(QString delim) {
    p_delimiter = delim;
  }

}
