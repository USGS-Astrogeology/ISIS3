/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "TableField.h"

#include "IException.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"

using namespace std;
namespace Isis {

  /**
   * Constructs a TableField object with the given field name, field value type,
   * and field size. The size defaults to 1 entry value per field.
   *
   * @param name The name of the field.
   * @param type The type of the field value.
   * @param size The size of the field. This is the number of values for a 
   *             single table entry. Defaults to 1.
   */
  TableField::TableField(const std::string name, TableField::Type type,
                         int size) {
    m_name = name;
    m_type = type;
    m_size = size;
    if (m_type == TableField::Integer) {
      m_bytes = 4 * m_size;
      m_ivalues.resize(m_size);
    }
    else if (m_type == TableField::Double) {
      m_bytes = 8 * m_size;
      m_dvalues.resize(m_size);
    }
    else if (m_type == TableField::Text) {
      m_bytes = 1 * m_size;
      m_svalue.resize(m_size);
    }
    else if (m_type == TableField::Real) {
      m_bytes = 4 * m_size;
      m_rvalues.resize(m_size);
    }
  }

  /**
   * Constructs a TableField object from a PvlGroup. 
   * The given group must contain the PvlKeywords "Name", "Size", and "Type". 
   * "Size" must be an integer value and valid values for "Type" are "Integer", 
   * "Double", "Text", or "Real". 
   *
   * @param field PvlGroup containing Name, Size, and Type for new TableField
   *              object
   *
   * @throws IException::Programmer - Invalid field type
   */
  TableField::TableField(PvlGroup &field) {
    QString name = (QString)field["Name"];
    m_name = name.toStdString();
    m_size = (int) field["Size"];
    if ((QString) field["Type"] == "Integer") {
      m_type = TableField::Integer;
      m_bytes = 4 * m_size;
      m_ivalues.resize(m_size);
    }
    else if ((QString) field["Type"] == "Double") {
      m_type = TableField::Double;
      m_bytes = 8 * m_size;
      m_dvalues.resize(m_size);
    }
    else if ((QString) field["Type"] == "Text") {
      m_type = TableField::Text;
      m_bytes = 1 * m_size;
      m_svalue.resize(m_size);
    }
    else if ((QString) field["Type"] == "Real") {
      m_type = TableField::Real;
      m_bytes = 4 * m_size;
      m_rvalues.resize(m_size);
    }
    else {
      std::string msg = "Field [" + m_name + "] has invalid type.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }

  //! Destroys the TableField object
  TableField::~TableField() {
  }

  /**
   * Returns the name of the TableField
   *
   * @return @b string Name of TableField
   */
  std::string TableField::name() const {
    return m_name;
  }

  /**
   * Returns the enumerated value of the TableField value's type. 
   * <ul> 
   *   <li>0 = Integer</li>
   *   <li>1 = Double</li>
   *   <li>2 = Text</li>
   *   <li>3 = Real</li>
   * </ul>
   *
   * @return @b TableField::Type Name of TableField
   */
  TableField::Type TableField::type() const {
    return m_type;
  }

  /**
   * Determines whether the field type is Integer
   *
   * @return @b bool Returns true if field type is Integer, and false if it is 
   *         not.
   */
  bool TableField::isInteger() const {
    return (m_type == TableField::Integer);
  }

  /**
   * Determines whether the field type is Double
   *
   * @return @b bool Returns true if field type is Double, and false if it is 
   *         not.
   */
  bool TableField::isDouble() const {
    return (m_type == TableField::Double);
  }

  /**
   * Determines whether the field type is Text
   *
   * @return @b bool Returns true if field type is Text, and false if it is not.
   */
  bool TableField::isText() const {
    return (m_type == TableField::Text);
  }

  /**
   * Determines whether the field type is Text
   *
   * @return @b bool Returns true if field type is Text, and false if it is not.
   */
  bool TableField::isReal() const {
    return (m_type == TableField::Real);
  }

  /**
   * Returns the number of bytes in the field value
   *
   * @return @b int The number of bytes in the TableField
   */
  int TableField::bytes() const {
    return m_bytes;
  }

  /**
   * Returns the number of values stored for the field at each record.
   *
   * @return @b int The size of the TableField
   */
  int TableField::size() const {
    return m_size;
  }

  /**
   * Casts the table field value to int if the type is Integer and the size is
   * 1. 
   *  
   * <code> 
   *   TableField field("Field 1", FieldType::Integer, 1);
   *   // The following lines are equivalent
   *   int value = int(field);
   *   int value = (int) field;
   * </code> 
   *
   * @return @b int The value of the field.
   *  
   * @throws IException::Programmer - Field is not an Integer
   * @throws IException::Programmer - Field has multiple Integer values
   */
  TableField::operator int() const {
    if (m_type != TableField::Integer) {
      std::string msg = "Field [" + m_name + "] is not Integer.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    if (m_ivalues.size() > 1) {
      std::string msg = "Field [" + m_name + "] has multiple Integer values. "
                   "Use std::vector<int>().";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    return m_ivalues[0];
  }

  /**
   * Casts the table field value to double if the type is Double and the size is
   * 1. 
   *  
   * <code> 
   *   TableField field("Field 1", FieldType::Double, 1);
   *   // The following lines are equivalent
   *   double value = double(field);
   *   double value = (double) field;
   * </code> 
   *
   * @return @b double The value of the field.
   *
   * @throws IException::Programmer - Field is not a Double
   * @throws IException::Programmer - Field has multiple Double values
   */
  TableField::operator double() const {
    if (m_type != TableField::Double) {
      std::string msg = "Field [" + m_name + "] is not a Double.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    if (m_dvalues.size() > 1) {
      std::string msg = "Field [" + m_name + "] has multiple Double values. "
                   "Use std::vector<double>().";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    return m_dvalues[0];
  }

  /**
   * Casts the table field value to float if the type is Real and the size is
   * 1. 
   *  
   * <code> 
   *   TableField field("Field 1", FieldType::Real, 1);
   *   // The following lines are equivalent
   *   float value = float(field);
   *   float value = (float) field;
   * </code> 
   *
   * @return @b float The value of the field.
   *
   * @throws IException::Programmer - Field is not a Real
   * @throws IException::Programmer - Field has multiple Real values
   */
  TableField::operator float() const {
    if (m_type != TableField::Real) {
      std::string msg = "Field [" + m_name + "] is not Real.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    if (m_rvalues.size() > 1) {
      std::string msg = "Field [" + m_name + "] has multiple Real values. "
                   "Use std::vector<float>().";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    return m_rvalues[0];
  }

  /**
   * Casts the table field value to string if the type is Text. 
   *  
   * <code> 
   *   TableField field("Field 1", FieldType::Text, 9);
   *   // The following lines are equivalent
   *   string value = std::string(field);
   *   string value = (std::string) field;
   * </code> 
   *  
   * @return @b string The value of the field.
   *
   * @throws IException::Programmer - Field is not a string
   */
  TableField::operator std::string() const {
    if (m_type != TableField::Text) {
      std::string msg = "Field [" + m_name + "] is not Text.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    return m_svalue;
  }

  /**
   * Casts the table field value to a vector of ints if the type is Integer. 
   *  
   * <code> 
   *   TableField field("Field 1", FieldType::Integer, 3);
   *   vector<int> values = std::vector<int>(field);
   * </code> 
   *
   * @return The value of the field.
   *
   * @throws IException::Programmer - Field is not an Integer array
   */
  TableField::operator std::vector<int>() const {
    if (m_type != TableField::Integer) {
      std::string msg = "Field [" + m_name + "] is not an Integer array.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    return m_ivalues;
  }

  /**
   * Casts the table field value to a vector of doubles if the type is Double. 
   *  
   * <code> 
   *   TableField field("Field 1", FieldType::Double, 3);
   *   vector<double> values = std::vector<double>(field);
   * </code> 
   *
   * @return The value of the field.
   *
   * @throws IException::Programmer - Field is not a Double array
   */
  TableField::operator std::vector<double>() const {
    if (m_type != TableField::Double) {
      std::string msg = "Field [" + m_name + "] is not a Double array.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    return m_dvalues;
  }

  /**
   * Casts the table field value to a vector of floats if the type is Real. 
   *  
   * <code> 
   *   TableField field("Field 1", FieldType::Real, 3);
   *   vector<float> values = std::vector<float>(field);
   * </code> 
   *
   * @return The value of the field.
   *
   * @throws IException::Programmer - Field is not an Integer array
   */
  TableField::operator std::vector<float>() const {
    if (m_type != TableField::Real) {
      std::string msg = "Field [" + m_name + "] is not a Real array.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    return m_rvalues;
  }

  /**
   * Sets field value equal to input if TableField::Type is Integer and size 
   * is 1.
   *
   * @param value Integer to be assigned to field value
   *
   * @throws IException::Programmer - Field is not an Integer
   * @throws IException::Programmer - Field has multiple Integer values
   */
  void TableField::operator=(const int value) {
    if (m_type != TableField::Integer) {
      std::string msg = "Unable to set field to the given int value. Field [" 
                   + m_name + "] Type is not Integer.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    if (m_size > 1) {
      std::string msg = "Unable to set field to the given int value. "
                    "Field [" + m_name + "] has [" + std::to_string(m_size) + "] "
                    "Integer values. Use operator=(vector<int>).";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    m_ivalues[0] = value;
  }

  /**
   * Sets field value equal to input if TableField::Type is Double and size 
   * is 1.
   *
   * @param value Double to be assigned to field value
   *
   * @throws IException::Programmer - Field is not a Double
   * @throws IException::Programmer - Field has multiple Double values
   */
  void TableField::operator=(const double value) {
    if (m_type != TableField::Double) {
      std::string msg = "Unable to set field to the given double value. Field [" 
                   + m_name + "] Type is not Double.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    if (m_size > 1) {
      std::string msg = "Unable to set field to the given double value. "
                    "Field [" + m_name + "] has [" + std::to_string(m_size) + "] "
                    "Double values. Use operator=(vector<double>).";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    m_dvalues[0] = value;
  }

  /**
   * Sets field value equal to input if the TableField::Type is Real and the 
   * size is 1.
   *
   * @param value float to be assigned to field value
   *
   * @throws IException::Programmer - Field is not a Real
   * @throws IException::Programmer - Field has multiple Real values
   */
  void TableField::operator=(const float value) {
    if (m_type != TableField::Real) {
      std::string msg = "Unable to set field to the given float value. Field [" 
                   + m_name + "] Type is not Real.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    if (m_size > 1) {
      std::string msg = "Unable to set field to the given float value. "
                    "Field [" + m_name + "] has [" + std::to_string(m_size) + "] "
                    "Real values. Use operator=(vector<float>).";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    m_rvalues[0] = value;
  }

  /**
   * Sets field value equal to input if the TableField::Type is Text.
   *
   * @param value string to be assigned to field value
   *
   * @throws IException::Programmer - Field is not a string
   */
  void TableField::operator=(const std::string value) {
    std::string val = value;
    if (m_type != TableField::Text) {
      std::string msg = "Unable to set field to the given string value. Field [" 
                    + m_name + "] Type is not Text.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    if (m_size < (int) val.size()) {// automos with tracking ???
      for (int i = m_size; i < val.size(); i++) {
        // if the extra characters are not spaces or nulls, throw an erro
        if (val[i] != ' ' && val[i] != '\0') {
          std::string msg = "Unable to set the Text TableField to the given string. "
                        "The number of bytes allowed for this field value [" 
                        + std::to_string(m_size) + "] is less than the length of the "
                        "given string [" + value + "].";
          throw IException(IException::Unknown, msg, _FILEINFO_);
        }
      }
      // if the extra characters are spaces and nulls, concatenate the string 
      val.resize(m_size);
    }
    m_svalue = val;
  }

  /**
   * Sets field value equal to input if the TableField::Type is Integer and the 
   * input vector size matches the field size.
   *
   * @param values Integer vector of values to be assigned to field value
   *
   * @throws IException::Programmer - Field is not an Integer
   * @throws IException::Programmer - Vector is not the correct size
   */
  void TableField::operator=(const std::vector<int> &values) {
    if (m_type != TableField::Integer) {
      std::string msg = "Unable to set field to the given vector of int values. "
                    "Field [" + m_name + "] Type is not Integer.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    else if ((int) values.size() != m_size) {
      std::string msg = "Unable to set field to the given vector of int values. "
                    "Field [" + m_name + "] values has size [" 
                    + std::to_string(m_size) + "].";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    m_ivalues = values;
  }

  /**
   * Sets field value equal to the input if the TableField::Type is Double and 
   * the input vector size matches the field size.
   *
   * @param values Double vector of values to be assigned to field value
   *
   * @throws IException::Programmer - Field is not a Double
   * @throws IException::Programmer - Vector is not the correct size
   */
  void TableField::operator=(const std::vector<double> &values) {
    if (m_type != TableField::Double) {
      std::string msg = "Unable to set field to the given vector of double values. "
                    "Field [" + m_name + "] Type is not Double.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    else if ((int) values.size() != m_size) {
      std::string msg = "Unable to set field to the given vector of double values. "
                    "Field [" + m_name + "] values has size [" 
                    + std::to_string(m_size) + "].";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    m_dvalues = values;
  }

  /**
   * Sets field value equal to the input if the TableField::Type is Real and the 
   * input vector size matches the field size.
   *
   * @param values Float vector of values to be assigned to field
   *               value
   *
   * @throws IException::Programmer - Field is not a Real
   * @throws IException::Programmer - Vector is not the correct size
   */
  void TableField::operator=(const std::vector<float> &values) {
    if (m_type != TableField::Real) {
      std::string msg = "Unable to set field to the given vector of float values. "
                    "Field [" + m_name + "] Type is not Real.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    else if ((int) values.size() != m_size) {
      std::string msg = "Unable to set field to the given vector of float values. "
                    "Field [" + m_name + "] values has size [" 
                    + std::to_string(m_size) + "].";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    m_rvalues = values;
  }

  /**
   * Sets field value equal to the values found in the input binary 
   * buffer.  This method reads the buffer by assuming that the buffer 
   * values are stored in the same number of bytes as the 
   * TableField::Type.
   *
   * @param ibuf Binary buffer containing values to be assigned to the 
   *             field value.
   *
   * @throws iException::Programmer - Undefined field type.
   */
  void TableField::operator=(const void *ibuf) {
    char *buf = (char *) ibuf;
    if (m_type == TableField::Double) {
      for (unsigned int i = 0; i < m_dvalues.size(); i++) {
        // m_dvalues[i] = ((double *) buf)[i];
        double bufDouble;
        memmove(&bufDouble, buf + i * 8, 8);
        m_dvalues[i] = bufDouble;
      }
    }
    else if (m_type == TableField::Integer) {
      for (unsigned int i = 0; i < m_ivalues.size(); i++) {
        // m_ivalues[i] = ((int *) buf)[i];
        int bufInt;
        memmove(&bufInt, buf + i * 4, 4);
        m_ivalues[i] = bufInt;
      }
    }
    else if (m_type == TableField::Text) {
      m_svalue.resize(bytes());
      for (int i = 0; i < bytes(); i++) {
        m_svalue[i] = buf[i];
      }
    }
    else if (m_type == TableField::Real) {
      for (unsigned int i = 0; i < m_rvalues.size(); i++) {
        // m_ivalues[i] = ((int *) buf)[i];
        float bufFloat;
        memmove(&bufFloat, buf + i * 4, 4);
        m_rvalues[i] = bufFloat;
      }
    }
    else {
      string msg = "Undefined field type [" + IString(m_type) + "].";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }

  /**
   * Sets field value equal to the values found in the character string array if
   * the TableField::Type is Text. 
   *
   * @param buf Character array conataining text values to be assigned to the 
   *            field value.
   *
   * @throws IException::Programmer - Field is not Text
   */
  void TableField::operator=(const char *buf) {
    if (m_type != TableField::Text) {
      std::string msg = "Unable to set field to the given string value. Field [" + m_name + "] Type is not Text.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    m_svalue = buf;
  }

  /**
   * Creates and returns a PvlGroup named "Field" containing the following
   * keywords and their respective values: "Name", "Type", and "Size"
   *
   * @return PvlGroup containing field Name, Type, and Size
   */
  PvlGroup TableField::pvlGroup() {
    PvlGroup group("Field");
    group += PvlKeyword("Name", QString::fromStdString(m_name));
    if (m_type == TableField::Double) {
      group += PvlKeyword("Type", "Double");
    }
    else if (m_type == TableField::Integer) {
      group += PvlKeyword("Type", "Integer");
    }
    else if (m_type == TableField::Text) {
      group += PvlKeyword("Type", "Text");
    }
    else if (m_type == TableField::Real) {
      group += PvlKeyword("Type", "Real");
    }
    group += PvlKeyword("Size", QString::number(m_size));

    return group;
  }


  std::string TableField::toString(const TableField &field, std::string delimiter){
    std::string fieldValues = "";
    if (field.size()== 1){
      if (field.isText()){
        fieldValues = (std::string)field;
      }
      else if (field.isInteger()){
        fieldValues = std::to_string((int)field);
      }
      else if (field.isDouble()){
        fieldValues = std::to_string((double)field);
      }
      else { //real
        fieldValues = std::to_string((float)field);
      }
    }
    // Otherwise, build a vector to contain the entries
    else {
      if (field.isText()){
        fieldValues +=(std::string)field;
      }
      else if (field.isInteger()){
        vector< int > currField = field;
        for (int i = 0;i <(int)currField.size();i++){
          fieldValues += std::to_string(currField[i]);
          if (i <(int)currField.size()- 1){
            // add delimiter for all but the last element of the field
            fieldValues += delimiter;
          }
        }
      }
      else if (field.isDouble()){
        vector< double > currField = field;
        for (int i = 0;i <(int)currField.size();i++){
          fieldValues += std::to_string(currField[i]);
          if (i <(int)currField.size()- 1){
            fieldValues += delimiter;
          }
        }
      }
      else { //if (field.isReal()) {
        vector< float > currField = field;
        for (int i = 0;i <(int)currField.size();i++){
          fieldValues += std::to_string(currField[i]);
          if (i <(int)currField.size()- 1){
            fieldValues += delimiter;
          }
        }
      }
    }
    return fieldValues;
  }
} // end namespace isis

