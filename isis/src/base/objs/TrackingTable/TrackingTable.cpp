/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "TrackingTable.h"

#include <QList>
#include <QString>

#include "FileName.h"
#include "IException.h"
#include "Pvl.h"
#include "SpecialPixel.h"
#include "Table.h"
#include "TableField.h"
#include "TableRecord.h"

using namespace std;
namespace Isis {


  /**
  * Default constructor
  */
  TrackingTable::TrackingTable() {

  }


  /**
  * Constructs a TrackingTable given a Table object. The Table is used to populate
  *         m_fileList.
  *
  * @param table The Table object to pull the filenames and serial numbers from
  */
  TrackingTable::TrackingTable(Table table) {

    for (int i=0; i < table.Records(); i++) {
      TableRecord record = table[i];
      QString nameField = QString::fromStdString(record["FileName"]).split("/").last();
      QString extension(FileName(nameField).extension());
      int found = nameField.lastIndexOf(extension);
      if (found != -1) {
        // clear the packing characters - get only the file name
        nameField.remove(found + 3);
      }
      FileName fileName(nameField);
      std::string serialNumber = record["SerialNumber"];
      m_fileList.append(QPair<FileName, QString>(fileName, QString::fromStdString(serialNumber)));
    }
  }


  /**
  * Destroys the TrackingTable object
  */
  TrackingTable::~TrackingTable() {

  }


  /**
  * Constrcts and returns a Table object based on values in m_fileList.
  *
  * @return Table The constructed table to be returned
  */
  Table TrackingTable::toTable() {

    // Begin by establishing the length of the fields within the table. This would be the longest
    // length that is needed to be stored.
    int fieldLength = 0;

    for (int i=0; i < m_fileList.size(); i++) {
      if (m_fileList[i].first.name().length() > fieldLength) {
        fieldLength = m_fileList[i].first.name().length();
      }
      if (m_fileList[i].second.length() > fieldLength) {
        fieldLength = m_fileList[i].second.length();
      }
    }

    // This record is never being used. It is simply to construct the Table object.
    TableRecord dummyRecord;
    TableField fileNameField("FileName", TableField::Text, fieldLength);
    TableField serialNumberField("SerialNumber", TableField::Text, fieldLength);
    TableField indexField("PixelValue", TableField::Integer);
    dummyRecord += fileNameField;
    dummyRecord += serialNumberField;
    dummyRecord += indexField;
    Table table(trackingTableName.toStdString(), dummyRecord);

    // Loop through m_fileList and add records to the table with the proper information.
    for (int i=0; i < m_fileList.size(); i++) {

      fileNameField = m_fileList[i].first.name().toStdString();
      serialNumberField = m_fileList[i].second.toStdString();
      indexField = (int) (i + VALID_MINUI4);

      TableRecord record;
      record += fileNameField;
      record += serialNumberField;
      record += indexField;

      table += record;

    }

    return table;
  }


  /**
  * Returns the FileName that corresponds to a pixel value.
  *
  * @param pixel The pixel value to find the filename for
  * @returns @b FileName The FileName represented by the pixel value
  */
  FileName TrackingTable::pixelToFileName(unsigned int pixel) {
    if (pixel < VALID_MINUI4) {
      QString msg = "Cannot convert pixel [" + toString(pixel)
                  + "] to a filename, pixel is below valid minimum ["
                  + toString(VALID_MINUI4) + "].";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    unsigned int index = pixel - VALID_MINUI4;
    if (index >= (unsigned int)m_fileList.size()) {
      QString msg = "Cannot convert pixel [" + toString(pixel)
                  + "] to a filename, pixel is above valid maximum ["
                  + toString(VALID_MINUI4 + m_fileList.size()) + "].";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    return m_fileList[index].first;
  }


  /**
  * Returns the pixel value of the filename/serialnumber combination.
  *
  * @param file The FileName within m_fileList to find the pixel value of
  * @param serialNumber The QString of the serial number within m_fileList
  *                     to find the pixel value of
  *
  * @return  @b unsigned  @b int The pixel value corresponding to the
  *                              filename/serialnumber combination
  */
  unsigned int TrackingTable::fileNameToPixel(FileName file, QString serialNumber) {
    for (int i = 0; i < m_fileList.size(); i++) {
      if (QString::compare(m_fileList[i].first.toString(), file.name()) == 0) {
        return i + VALID_MINUI4;
      }
    }

    // At this point, the file is not in the internal file list so append it
    // and return its new index.
    m_fileList.append(QPair<FileName, QString>(file, serialNumber));
    return m_fileList.size() - 1 + VALID_MINUI4;
  }


  /**
  * Returns the serial number that corresponds to a pixel value.
  *
  * @param pixel The pixel value to find the serial number for
  * @returns @b QString The serial number represented by the pixel value
  */
  QString TrackingTable::pixelToSN(unsigned int pixel) {
    if (pixel < VALID_MINUI4) {
      QString msg = "Cannot convert pixel [" + toString(pixel)
                  + "] to a serial number, pixel is below valid minimum ["
                  + toString(VALID_MINUI4) + "].";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    unsigned int index = pixel - VALID_MINUI4;
    if (index >= (unsigned int)m_fileList.size()) {
      QString msg = "Cannot convert pixel [" + toString(pixel)
                  + "] to a serial number, pixel is above valid maximum ["
                  + toString(VALID_MINUI4 + m_fileList.size()) + "].";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    return m_fileList[index].second;
  }


  /**
  * Returns the index of the filename/serialnumber combination.
  *
  * @param file The FileName within m_fileList to find the index of
  * @param serialNumber The QString of the serial number within m_fileList
  *                     to find the index of
  *
  * @return @b int The index corresponding to the
  *                              filename/serialnumber combination
  */
  int TrackingTable::fileNameToIndex(FileName file, QString serialNumber) {
    for (int i = 0; i < m_fileList.size(); i++) {
      if (m_fileList[i].first == file) {
        return i;
      }
    }

    // At this point, the file is not in the internal file list so append it
    // and return its new index.
    m_fileList.append(QPair<FileName, QString>(file, serialNumber));
    return m_fileList.size() - 1;
  }
}
