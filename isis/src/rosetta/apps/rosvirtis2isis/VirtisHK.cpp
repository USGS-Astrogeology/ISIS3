/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "VirtisHK.h"

namespace Isis {


/**
 * Constructs a VIRTIS HK object.
 *
 * @param hkName Name to be used for the housekeeping data
 * @param tableType The type the data is (double, int)
 * @param one first conversion coefficient
 * @param two second conversion coefficient
 * @param three third conversion coefficient
 */
 VirtisHK::VirtisHK(QString hkName, QString tableType, QString one, QString two, QString three) {
    m_name = hkName;

    if (QString::compare(tableType.trimmed(), "double") == 0) {
      m_tableType = TableField::Double;
    }
    else if (QString::compare(tableType.trimmed(), "int") == 0) {
      m_tableType = TableField::Integer;
    }

    // Initialize equation
    one = one.remove("{").trimmed();
    three = three.remove("}").trimmed();

    m_coefficients.push_back(three.toDouble());
    m_coefficients.push_back(two.toDouble());
    m_coefficients.push_back(one.toDouble());
  }


 /**
 * Default destructor
 *
 */
  VirtisHK::~VirtisHK() {}


/**
 * The name of the HK.
 *
 * @return QString The name of the HK.
 */
  QString VirtisHK::name(){
    return m_name;
  }


/**
 * Returns the TableField Type for this HK.
 *
 * @return TableField::Type The field type for this HK.
 */
  TableField::Type VirtisHK::tableType(){
    return m_tableType;
  }


/**
 * Creates and returns the appropriate TableField for this HK.
 *
 * @return TableField The entire appropriate TableField for this HK
 */
  TableField VirtisHK::tableField(){
    return TableField(m_name.toStdString(), m_tableType);
  }


/**
 * Returns the coefficients used to convert the HK to physical units.
 *
 * @return std::vector<double> A vector of coefficients used to convert this HK to physical units
 */
  std::vector<double> VirtisHK::coefficients(){
    return m_coefficients;
  }

};
