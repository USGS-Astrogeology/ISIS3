/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <sstream>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/TransService.hpp>
#include <xercesc/sax2/XMLReaderFactory.hpp>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "FileName.h"
#include "IException.h"
#include "IsisAml.h"
#include "IsisXMLChTrans.h"
#include "IString.h"
#include "Preference.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "PvlObject.h"

using namespace std;

/**
 * @internal
 *   @todo This namespace needs documentation.
 */
namespace XERCES = XERCES_CPP_NAMESPACE;

/**
 * Constructs an IsisAml object and internalizes the XML data in the given file
 * name.
 *
 * @param xmlfile Indicates the pull path of the XML file to be parsed.
 */
IsisAml::IsisAml(const QString &xmlfile) {
  StartParser(xmlfile.toLatin1().data());
}

/**
 * Destructs an IsisAml object.
 */
IsisAml::~IsisAml() {
}

/**
 * Allows the insertion of a value for any parameter. No validity check is
 * performed on the value passed in.
 *
 * @param paramName The partial or full name of the parameter to be modified.
 * @param value The QString representation of the value to be placed in the
 * parameters value data member. For parameters of type integer, the QString
 * must be convertable to an integer. For parameters of type double, the QString
 * must be convertable to a double. For parameters of type boolean, the QString
 * must be one of: (TRUE, FALSE, YES, NO, or a partial match of any of these
 * beginning with the first character).
 *
 * @throws IException The parameter already has a value in its
 * "value" data member.Overwriting an existing value is not allowed. Use "Clear"
 * to erase all values in the value data member instead of overwriting an
 * existing value.
 */
void IsisAml::PutAsString(const QString &paramName,
                          const QString &value) {

  IsisParameterData *param = const_cast <IsisParameterData *>(ReturnParam(paramName));

  if(param->values.size() > 0) {
    std::string message = "A value for this parameter [" + paramName.toStdString() + "] has "
                     "already been entered.";
    throw Isis::IException(Isis::IException::User, message, _FILEINFO_);
  }

  param->values.clear();
  param->values.push_back(value);

}

/**
 * Allows the insertion of a value for any parameter. No validity check is
 * performed on the value passed in.
 *
 * @param paramName The partial or full name of the parameter to be modified.
 * @param value The QString vector representation of the value to be placed in
 * the parameters value data member. For parameters of type integer, the QString
 * must be convertable to an integer. For parameters of type double, the QString
 * must be convertable to a double. For parameters of type boolean, the QString
 * must be one of: (TRUE, FALSE, YES, NO, or a partial match of any of these
 * beginning with the first character).
 */
void IsisAml::PutAsString(const QString &paramName,
                          const vector<QString> &value) {

  IsisParameterData *param = const_cast <IsisParameterData *>(ReturnParam(paramName));

  if(param->values.size() > 0) {
    std::string message = "A value for this parameter [" + paramName.toStdString() + "] has "
                     "already been entered.";
    throw Isis::IException(Isis::IException::User, message, _FILEINFO_);
  }

  param->values.resize(value.size());
  param->values = value;

}


// Public: Sets the value member of a parameter of type QString whose name
// starts with paramName

/**
 * Allows the insertion of a value for any parameter. No validity check
 * is
 * performed on the value passed in.
 *
 * @param paramName The partial or full name of the parameter to be modified.
 * @param value The QString representation of the value to be placed in the
 * parameters value data member. For parameters of type integer, the
 * QString
 * must be convertable to an integer. For parameters of type double, the QString
 * must be convertable to a double. For parameters of type boolean, the QString
 * must be one of: (TRUE, FALSE, YES, NO, or a partial match of any of these
 * beginning with the first character).
 *
 */
void IsisAml::PutString(const QString &paramName, const QString &value) {

  IsisParameterData *param = const_cast <IsisParameterData *>(ReturnParam(paramName));

  if(param->type != "string" && param->type != "combo") {
    std::string message = "Parameter [" + paramName.toStdString() + "] is not a string.";
    throw Isis::IException(Isis::IException::Programmer, message, _FILEINFO_);
  }

  if(param->values.size() > 0) {
    std::string message = "A value for this parameter [" + paramName.toStdString() + "] has "
                     "already been saved (possibly by IsisGui). If you need to "
                     "change the value use \"Clear\" before the Put.";
    throw Isis::IException(Isis::IException::Programmer, message, _FILEINFO_);
  }

  param->values.clear();
  param->values.push_back(value);

  Verify(param);
}


// Public: Sets the value member of a parameter of type QString whose name
// starts with paramName
/**
 * Allows the insertion of a value for a parameter of type "string". A validity
 * check is performed on the value passed in, but all QStrings are allowed.
 *
 * @param paramName The partial or full name of the parameter to be modified.
 * @param value The QString value to be placed in the QString's value data member.
 */
void IsisAml::PutString(const QString &paramName,
                        const vector<QString> &value) {

  IsisParameterData *param = const_cast <IsisParameterData *>(ReturnParam(paramName));

  if(param->type != "string" && param->type != "combo") {
    std::string message = "Parameter [" + paramName.toStdString() + "] is not a string.";
    throw Isis::IException(Isis::IException::Programmer, message, _FILEINFO_);
  }

  if(param->values.size() > 0) {
    std::string message = "A value for this parameter [" + paramName.toStdString() + "] has "
                     "already been saved (possibly by IsisGui). If you need to "
                     "change the value use \"Clear\" before the Put.";
    throw Isis::IException(Isis::IException::Programmer, message, _FILEINFO_);
  }

  param->values.resize(value.size());
  param->values = value;

  Verify(param);
}


// Public: Sets the value member of a parameter of type filename whose name
// starts with paramName

/**
 * Allows the insertion of a value for a parameter of type "filename". A
 * validity check is performed on the value passed in.
 *
 * @param paramName The partial or full name of the parameter to be modified.
 * @param value The QString representation of the value to be placed in the
 * filename's value data member.
 */
void IsisAml::PutFileName(const QString &paramName,
                          const QString &value) {

  IsisParameterData *param = const_cast <IsisParameterData *>(ReturnParam(paramName));

  if((param->type != "filename") && (param->type != "cube")) {
    std::string message = "Parameter [" + paramName.toStdString() + "] is not a filename.";
    throw Isis::IException(Isis::IException::Programmer, message, _FILEINFO_);
  }

  if(param->values.size() > 0) {
    std::string message = "A value for this parameter [" + paramName.toStdString() + "] has "
                     "already been saved (possibly by IsisGui). If you need to "
                     "change the value use \"Clear\" before the Put.";
    throw Isis::IException(Isis::IException::Programmer, message, _FILEINFO_);
  }

  param->values.clear();
  param->values.push_back(value);

  Verify(param);
}


// Public: Sets the value member of a parameter of type filename whose name
// starts with paramName

/**
 * Allows the insertion of a value for a parameter of type "filename". A
 * validity check is performed on the value passed in.
 *
 * @param paramName The partial or full name of the parameter to be modified.
 * @param value The QString vector representation of the value to be placed in
 * the filename's value data member.
 *
 * @throws iException (IsisProgrammerError) The parameter is not of type
 * "filename".
 * @throws iException (IsisProgrammerError) The parameter already has a value
 * in its "value" data member. Overwriting an existing value is not allowed.
 * Use "Clear" to erase all values in the value data member instead of
 * overwriting an existing value.
 */
void IsisAml::PutFileName(const QString &paramName,
                          const vector<QString> &value) {

  IsisParameterData *param = const_cast <IsisParameterData *>(ReturnParam(paramName));

  if((param->type != "filename") && (param->type != "cube")) {
    std::string message = "Parameter [" + paramName.toStdString() + "] is not a filename.";
    throw Isis::IException(Isis::IException::Programmer, message, _FILEINFO_);
  }

  if(param->values.size() > 0) {
    std::string message = "A value for this parameter [" + paramName.toStdString() + "] has "
                     "already been saved (possibly by IsisGui). If you need to "
                     "change the value use \"Clear\" before the Put.";
    throw Isis::IException(Isis::IException::Programmer, message, _FILEINFO_);
  }

  param->values.resize(value.size());
  param->values = value;

  Verify(param);
}


/**
 * Allows the insertion of a value for a parameter of type 
 * "cubename". A validity check is performed on the value passed
 * in. 
 *
 * @param paramName The partial or full name of the parameter to be modified.
 * @param value The QString representation of the value to be placed in the
 * cubename's value data member.
 */
void IsisAml::PutCubeName(const QString &paramName,
                          const QString &value) {

  IsisParameterData *param = const_cast <IsisParameterData *>(ReturnParam(paramName));

  if(param->type != "cube") {
    std::string message = "Parameter [" + paramName.toStdString() + "] is not a cubename.";
    throw Isis::IException(Isis::IException::Programmer, message, _FILEINFO_);
  }

  if(param->values.size() > 0) {
    std::string message = "A value for this parameter [" + paramName.toStdString() + "] has "
                     "already been saved (possibly by IsisGui). If you need to "
                     "change the value use \"Clear\" before the Put.";
    throw Isis::IException(Isis::IException::Programmer, message, _FILEINFO_);
  }

  param->values.clear();
  param->values.push_back(value);

  Verify(param);
}


// Public: Sets the value member of a parameter of type integer whose name
// starts with paramName

/**
 * Allows the insertion of a value for a parameter of type "integer". A
 * validity check is performed on the value passed in.
 *
 * @param paramName The partial or full name of the parameter to be modified.
 * @param value The integer value to be placed in the integer's value data
 * member.
 *
 * @throws iException (IsisProgrammerError) The parameter is not of type
 * "int".
 */
void IsisAml::PutInteger(const QString &paramName,
                         const int &value) {

  IsisParameterData *param = const_cast <IsisParameterData *>(ReturnParam(paramName));

  if(param->type != "integer") {
    std::string message = "Parameter [" + paramName.toStdString() + "] is not an integer.";
    throw Isis::IException(Isis::IException::Programmer, message, _FILEINFO_);
  }

  if(param->values.size() > 0) {
    std::string message = "A value for this parameter [" + paramName.toStdString() + "] has "
                     "already been saved (possibly by IsisGui). If you need to "
                     "change the value use \"Clear\" before the Put.";
    throw Isis::IException(Isis::IException::Programmer, message, _FILEINFO_);
  }

  param->values.clear();
  param->values.push_back(QString::number(value));

  Verify(param);
}


// Public: Sets the value member of a parameter of type integer whose name
// starts with paramName

/**
 * Allows the insertion of a value for a parameter of type "integer". A
 * validity check is performed on the value passed in.
 *
 * @param paramName The partial or full name of the parameter to be modified.
 * @param value The integer vector to be placed in the integer's value data
 * member.
 *
 * @throws iException (IsisProgrammerError) The parameter already has a value
 * in its "value" data member.Overwriting an existing value is not allowed. Use
 * "Clear" to erase all values in the value data member instead of overwriting
 * an existing value.
 * @throws iException (IsisProgrammerError) The parameter is not of type "int".
 */
void IsisAml::PutInteger(const QString &paramName,
                         const vector<int> &value) {

  IsisParameterData *param = const_cast <IsisParameterData *>(ReturnParam(paramName));

  if(param->type != "integer") {
    std::string message = "Parameter [" + paramName.toStdString() + "] is not an integer.";
    throw Isis::IException(Isis::IException::Programmer, message, _FILEINFO_);
  }

  if(param->values.size() > 0) {
    std::string message = "A value for this parameter [" + paramName.toStdString() + "] has "
                     "already been saved (possibly by IsisGui). If you need to "
                     "change the value use \"Clear\" before the Put.";
    throw Isis::IException(Isis::IException::Programmer, message, _FILEINFO_);
  }

  param->values.resize(value.size());
  for(unsigned int i = 0; i < value.size(); i++) {
    param->values[i] = QString::number(value[i]);
  }

  Verify(param);
}


// Public: Sets the value member of a parameter of type double whose name
// starts with paramName
/**
 * Allows the insertion of a value for a parameter of type "double". A
 * validity check is performed on the value passed in.
 *
 * @param paramName The partial or full name of the parameter to be modified.
 * @param value The double value to be placed in the double's value data
 * member.
 *
 * @throws iException (IsisProgrammerError) The parameter already has a value
 * in its "value" data member.Overwriting an existing value is not allowed. Use
 * "Clear" to erase all values in the value data member instead of overwriting
 * an existing value.
 * @throws iException (IsisProgrammerError) The parameter is not of type
 * "double".
 */
void IsisAml::PutDouble(const QString &paramName,
                        const double &value) {

  IsisParameterData *param = const_cast <IsisParameterData *>(ReturnParam(paramName));

  if(param->type != "double") {
    std::string message = "Parameter [" + paramName.toStdString() + "] is not a double.";
    throw Isis::IException(Isis::IException::Programmer, message, _FILEINFO_);
  }

  if(param->values.size() > 0) {
    std::string message = "A value for this parameter [" + paramName.toStdString() + "] has "
                     "already been saved (possibly by IsisGui). If you need to "
                     "change the value use \"Clear\" before the Put.";
    throw Isis::IException(Isis::IException::Programmer, message, _FILEINFO_);
  }

  param->values.clear();
  param->values.push_back(QString::number(value));

  Verify(param);
}


// Public: Sets the value member of a parameter of type double whose name
// starts with paramName
/**
 * Allows the insertion of a value for a parameter of type "double". A
 * validity check is performed on the value passed in.
 *
 * @param paramName The partial or full name of the parameter to be modified.
 * @param value The double vector to be placed in the double's value data
 * member.
 *
 * @throws iException (IsisProgrammerError) The parameter already has a value
 * in its "value" data member.Overwriting an existing value is not allowed. Use
 * "Clear" to erase all values in the value data member instead of overwriting
 * an existing value.
 * @throws iException (IsisProgrammerError) The parameter is not of type
 * "double".
 */
void IsisAml::PutDouble(const QString &paramName,
                        const vector<double> &value) {

  IsisParameterData *param = const_cast <IsisParameterData *>(ReturnParam(paramName));

  if(param->type != "double") {
    std::string message = "Parameter [" + paramName.toStdString() + "] is not a double.";
    throw Isis::IException(Isis::IException::Programmer, message, _FILEINFO_);
  }

  if(param->values.size() > 0) {
    std::string message = "A value for this parameter [" + paramName.toStdString() + "] has "
                     "already been saved (possibly by IsisGui). If you need to "
                     "change the value use \"Clear\" before the Put.";
    throw Isis::IException(Isis::IException::Programmer, message, _FILEINFO_);
  }

  param->values.resize(value.size());
  for(unsigned int i = 0; i < value.size(); i++) {
    param->values[i] = QString::number(value[i]);
  }

  Verify(param);
}


// Public: Sets the value member of a parameter of type boolean whose name
// starts with paramName
/**
 * Allows the insertion of a value for a parameter of type "boolean". A
 * validity check is performed on the value passed in.
 *
 * @param paramName The partial or full name of the parameter to be modified.
 * @param value The boolean value to be placed in the boolean's value data
 * member.
 *
 * @throws iException (IsisProgrammerError) The parameter already has a value
 * in its "value" data member.Overwriting an existing value is not allowed. Use
 * "Clear" to erase all values in the value data member instead of overwriting
 * an existing value.
 * @throws iException (IsisProgrammerError) The parameter is not of type
 * "boolean".
 */
void IsisAml::PutBoolean(const QString &paramName,
                         const bool &value) {

  IsisParameterData *param = const_cast <IsisParameterData *>(ReturnParam(paramName));

  if(param->type != "boolean") {
    std::string message = "Parameter [" + paramName.toStdString() + "] is not a boolean.";
    throw Isis::IException(Isis::IException::Programmer, message, _FILEINFO_);
  }

  if(param->values.size() > 0) {
    std::string message = "A value for this parameter [" + paramName.toStdString() + "] has "
                     "already been saved (possibly by IsisGui). If you need to "
                     "change the value use \"Clear\" before the Put.";
    throw Isis::IException(Isis::IException::Programmer, message, _FILEINFO_);
  }

  param->values.clear();
  if(value) {
    param->values.push_back("YES");
  }
  else {
    param->values.push_back("NO");
  }

  Verify(param);
}


// Public: Sets the value member of a parameter of type boolean whose name
// starts with paramName
/**
 * Allows the insertion of a value for a parameter of type "boolean". A
 * validity check is performed on the value passed in.
 *
 * @param paramName The partial or full name of the parameter to be modified.
 * @param value The boolean vector to be placed in the boolean's value data
 * member.
 *
 * @throws iException (IsisProgrammerError) The parameter already has a value
 * in its "value" data member.Overwriting an existing value is not allowed. Use
 * "Clear" to erase all values in the value data member instead of overwriting
 * an existing value.
 * @throws iException (IsisProgrammerError) The parameter is not of type
 * "boolean".
 */
void IsisAml::PutBoolean(const QString &paramName,
                         const vector<bool> &value) {

  IsisParameterData *param = const_cast <IsisParameterData *>(ReturnParam(paramName));

  if(param->type != "boolean") {
    std::string message = "Parameter [" + paramName.toStdString() + "] is not a boolean.";
    throw Isis::IException(Isis::IException::Programmer, message, _FILEINFO_);
  }

  if(param->values.size() > 0) {
    std::string message = "A value for this parameter [" + paramName.toStdString() + "] has "
                     "already been saved (possibly by IsisGui). If you need to "
                     "change the value use \"Clear\" before the Put.";
    throw Isis::IException(Isis::IException::Programmer, message, _FILEINFO_);
  }

  param->values.resize(value.size());
  for(unsigned int i = 0; i < value.size(); i++) {
    if(value[i]) {
      param->values.push_back("YES");
    }
    else {
      param->values.push_back("NO");
    }
  }

  Verify(param);
}


// Accessor methods for getting the value(s) of a parameter

// Public: Returns the first element of the value member of a parameter whos
// name starts with paramName as a QString. Any type can be retrieve with this member.
/**
 * Allows the retrieval of a value for a parameter of any type. The value will
 * be returned as a QString no matter what the parameter type is.
 *
 * @param paramName The partial or full name of the parameter to be retrieved.
 *
 * @return A QString representation of the value for the specified parameter.
 *
 * @throws iException (IsisProgrammerError) The parameter has no value.
 */
QString IsisAml::GetAsString(const QString &paramName) const {

  const IsisParameterData *param = ReturnParam(paramName);
  QString value;
  if(param->values.size() == 0) {
    if(param->defaultValues.size() == 0) {
      std::string message = "Parameter [" + paramName.toStdString() + "] has no value.";
      throw Isis::IException(Isis::IException::Programmer, message, _FILEINFO_);
    }
    else {
      value = param->defaultValues[0];
    }
  }
  else {
    value = param->values[0];
  }

  return value;
}


// Public: Returns the value member of a parameter whose name starts with paramName
// as a vector<QString>
/**
 * Allows the retrieval of a value for a parameter of any type. The value will
 * be returned as a QString no matter what the parameter type is.
 *
 * @param paramName The partial or full name of the parameter to be retrieved.
 * @param values The value member of the parameter whose name starts with
 * paramName.
 *
 * @throws iException (IsisProgrammerError) The parameter has no value.
 */
void IsisAml::GetAsString(const QString &paramName,
                          vector<QString> &values) const {

  const IsisParameterData *param = ReturnParam(paramName);

  values.clear();
  QString value;
  if(param->values.size() == 0) {
    if(param->defaultValues.size() == 0) {
      std::string message = "Parameter [" + paramName.toStdString() + "] has no value.";
      throw Isis::IException(Isis::IException::User, message, _FILEINFO_);
    }
    else {
      for(unsigned int i = 0; i < param->defaultValues.size(); i++)
        values.push_back(param->defaultValues[i]);
    }
  }
  else {
    for(unsigned int i = 0; i < param->values.size(); i++)
      values.push_back(param->values[i]);
  }

  return;
}


// Public: Returns the first element of the value member of a parameter whose
// name starts with paramName as a QString/filename
/**
 * Allows the retrieval of a value for a parameter of type "filename".
 *
 * @param paramName The partial or full name of the parameter to be retrieved.
 * @param extension A default extension to add if it does not already exist on
 * the file name.  For example, "txt" will make /mydir/myfile into
 * /mydir/myfile.txt
 *
 * @return The value of the parameter.
 */
QString IsisAml::GetFileName(const QString &paramName, QString extension) const {

  const IsisParameterData *param = ReturnParam(paramName);
  if(param->type != "filename") {
    std::string message = "Parameter [" + paramName.toStdString() + "] is not a filename.";
    throw Isis::IException(Isis::IException::Programmer, message, _FILEINFO_);
  }

  QString value;
  if(param->values.size() == 0) {
    if(param->defaultValues.size() == 0) {
      std::string message = "Parameter [" + paramName.toStdString() + "] has no value.";
      throw Isis::IException(Isis::IException::User, message, _FILEINFO_);
    }
    else {
      value = param->defaultValues[0];
    }
  }
  else {
    value = param->values[0];
  }

  Isis::FileName name(value.toStdString());
  if(extension != "") name = name.addExtension(extension.toStdString());
  value = QString::fromStdString(name.expanded());

  return value;
}


// Public: Returns the value member of a parameter whose name starts with paramName
// as a vector<QString/filename>
/**
 * Allows the retrieval of a value for a parameter of type "filename".
 *
 * @param paramName The partial or full name of the parameter to be retrieved.
 * @param values The value membet of the parameter whose name starts with
 * paramName.
 */
void IsisAml::GetFileName(const QString &paramName,
                          vector<QString> &values) const {

  const IsisParameterData *param = ReturnParam(paramName);

  if(param->type != "filename") {
    std::string message = "Parameter [" + paramName.toStdString() + "] is not a filename.";
    throw Isis::IException(Isis::IException::Programmer, message, _FILEINFO_);
  }

  values.clear();
  if(param->values.size() == 0) {
    if(param->defaultValues.size() == 0) {
      std::string message = "Parameter [" + paramName.toStdString() + "] has no value.";
      throw Isis::IException(Isis::IException::User, message, _FILEINFO_);
    }
    else {
      for(unsigned int i = 0; i < param->defaultValues.size(); i++) {
        Isis::FileName name(param->defaultValues[i].toStdString());
        values.push_back(QString::fromStdString(name.expanded()));
      }
    }
  }
  else {
    for(unsigned int i = 0; i < param->values.size(); i++) {
      Isis::FileName name(param->values[i].toStdString());
      values.push_back(QString::fromStdString(name.expanded()));
    }
  }

  return;
}


/**
 * Retrieves of a value for a parameter of type "cubename".
 *
 * @param paramName The partial or full name of the parameter to be retrieved.
 * @param extension A default extension to add if it does not already exist on
 * the file name.  For example, "txt" will make /mydir/myfile into
 * /mydir/myfile.txt
 *
 * @return The value of the parameter.
 */
QString IsisAml::GetCubeName(const QString &paramName, QString extension) const {

  const IsisParameterData *param = ReturnParam(paramName);

  if (param->type != "cube") {
    std::string message = "Parameter [" + paramName.toStdString() + "] is not a cubename.";
    throw Isis::IException(Isis::IException::Programmer, message, _FILEINFO_);
  }

  QString value;
  if (param->values.size() == 0) {
    if (param->defaultValues.size() == 0) {
      std::string message = "Parameter [" + paramName.toStdString() + "] has no value.";
      throw Isis::IException(Isis::IException::User, message, _FILEINFO_);
    }
    else {
      value = param->defaultValues[0];
    }
  }
  else {
    value = param->values[0];
  }

  Isis::FileName name(value.toStdString());
  if (extension != "") name = name.addExtension(extension.toStdString());
  value = QString::fromStdString(name.expanded());
  if (name.attributes().length() > 0) {
    value += "+" + QString::fromStdString(name.attributes());
  }

  return value;
}


// Public: Returns the first element of the value member of a parameter whos
// name starts with paramName as a QString
/**
 * Allows the retrieval of a value for a parameter of type "string".
 *
 * @param paramName The partial or full name of the parameter to be retrieved.
 *
 * @return The value of the parameter.
 *
 * @throws iException::Programmer (IsisErrorUser) The parameter has no value.
 */
QString IsisAml::GetString(const QString &paramName) const {

  const IsisParameterData *param = ReturnParam(paramName);
  QString value;

  if(param->type != "string" && param->type != "combo") {
    std::string message = "Parameter [" + paramName.toStdString() + "] is not a string.";
    throw Isis::IException(Isis::IException::Programmer, message, _FILEINFO_);
  }

  if(param->values.size() == 0) {
    if(param->defaultValues.size() == 0) {
      std::string message = "Parameter [" + paramName.toStdString() + "] has no value.";
      throw Isis::IException(Isis::IException::User, message, _FILEINFO_);
    }
    else {
      value = param->defaultValues[0];
    }
  }
  else {
    value = param->values[0];
    // If there is a list of legal values return the list option that matches
    // or begins with what was entered rather than exactly what was entered
    if(param->listOptions.size() > 0) {
      value = value.toUpper();
      int found = -1;
      int foundcount = 0;
      for(unsigned int p = 0; p < param->listOptions.size(); p++) {
        QString option = param->listOptions[p].value;
        option = option.toUpper();
        if(value == option) {
          return value;
        }
        else if(value.startsWith(option) || option.startsWith(value)) {
          found = p;
          foundcount = foundcount + 1;
        }
      }
      if(foundcount == 0) {
        std::string message = "Value [" + value.toStdString() + "] for parameter [" +
                          paramName.toStdString() + "] is not a valid value.";
        throw Isis::IException(Isis::IException::User, message, _FILEINFO_);
      }
      if(foundcount > 1) {
        std::string message = "Value [" + value.toStdString() + "] for parameter [" +
                          paramName.toStdString() + "] is not unique.";
        throw Isis::IException(Isis::IException::User, message, _FILEINFO_);
      }
      return param->listOptions[found].value;
    }

    // Just return what is in the value
    else {
      value = param->values[0];
    }
  }
  return value;
}


// Public: Returns the value member of a parameter whose name starts with paramName
// as a vector<QString>
/**
 * Allows the retrieval of a value for a parameter of type "string".
 *
 * @param paramName The partial or full name of the parameter to be retrieved.
 *
 * @param values The value member of a parameter whose name starts with
 *                paramName.
 *
 * @throws iException::Programmer (IsisErrorUser) The parameter has no value.
 */
void IsisAml::GetString(const QString &paramName,
                        vector<QString> &values) const {

  const IsisParameterData *param = ReturnParam(paramName);

  if(param->type != "string" && param->type != "combo") {
    std::string message = "Parameter [" + paramName.toStdString() + "] is not a string.";
    throw Isis::IException(Isis::IException::Programmer, message, _FILEINFO_);
  }

  values.clear();
  QString value;
  if(param->values.size() == 0) {
    if(param->defaultValues.size() == 0) {
      std::string message = "Parameter [" + paramName.toStdString() + "] has no value.";
      throw Isis::IException(Isis::IException::User, message, _FILEINFO_);
    }
    else {
      for(unsigned int i = 0; i < param->defaultValues.size(); i++)
        values.push_back(param->defaultValues[i]);
    }
  }
  else {
    for(unsigned int i = 0; i < param->values.size(); i++)
      values.push_back(param->values[i]);
  }

  return;
}


// Public: Returns the first element of the value member of a parameter whos
// name starts with paramName as an integer
/**
 * Allows the retrieval of a value for a parameter of type "integer".
 *
 * @param paramName The partial or full name of the parameter to be retrieved.
 *
 * @return The value of the parameter.
 *
 * @throws iException::Programmer (IsisErrorUser) The parameter has no value.
 */
int IsisAml::GetInteger(const QString &paramName) const {

  const IsisParameterData *param = ReturnParam(paramName);

  if(param->type != "integer") {
    std::string message = "Parameter [" + paramName.toStdString() + "] is not an integer.";
    throw Isis::IException(Isis::IException::Programmer, message, _FILEINFO_);
  }

  Isis::IString value;
  if(param->values.size() == 0) {
    if(param->defaultValues.size() == 0) {
      std::string message = "Parameter [" + paramName.toStdString() + "] has no value.";
      throw Isis::IException(Isis::IException::User, message, _FILEINFO_);
    }
    else {
      value = param->defaultValues[0].toStdString();
    }
  }
  else {
    value = param->values[0].toStdString();
  }

  return value.ToInteger();
}


// Public: Returns the value member of a parameter whose name starts with paramName
// as a vector<int>
/**
 * Allows the retrieval of a value for a parameter of type "integer".
 *
 * @param paramName The partial or full name of the parameter to be retrieved.
 *
 * @param values The value member of a parameter whose name starts with
 * paramName.
 *
 * @throws iException::Programmer (IsisErrorUser) The parameter has no value.
 */
void IsisAml::GetInteger(const QString &paramName,
                         vector<int> &values) const {

  const IsisParameterData *param = ReturnParam(paramName);

  if(param->type != "integer") {
    std::string message = "Parameter [" + paramName.toStdString() + "] is not an integer.";
    throw Isis::IException(Isis::IException::Programmer, message, _FILEINFO_);
  }

  values.clear();
  Isis::IString value;
  if(param->values.size() == 0) {
    if(param->defaultValues.size() == 0) {
      std::string message = "Parameter [" + paramName.toStdString() + "] has no value.";
      throw Isis::IException(Isis::IException::User, message, _FILEINFO_);
    }
    else {
      for(unsigned int i = 0; i < param->defaultValues.size(); i++)
        value = param->defaultValues[i].toStdString();
      values.push_back(value.ToInteger());
    }
  }
  else {
    for(unsigned int i = 0; i < param->values.size(); i++)
      value = param->values[i].toStdString();
    values.push_back(value.ToInteger());
  }

  return;
}


// Public: Returns the first element of the value member of a parameter whos
// name starts with paramName as a doubble
/**
 * Allows the retrieval of a value for a parameter of type "double".
 *
 * @param paramName The partial or full name of the parameter to be retrieved.
 *
 * @return The value of the parameter.
 *
 * @throws iException::Programmer (IsisErrorUser) The parameter has no value.
 */
double IsisAml::GetDouble(const QString &paramName) const {

  const IsisParameterData *param = ReturnParam(paramName);

  if(param->type != "double") {
    std::string message = "Parameter [" + paramName.toStdString() + "] is not a double.";
    throw Isis::IException(Isis::IException::Programmer, message, _FILEINFO_);
  }

  Isis::IString value;
  if(param->values.size() == 0) {
    if(param->defaultValues.size() == 0) {
      std::string message = "Parameter [" + paramName.toStdString() + "] has no value.";
      throw Isis::IException(Isis::IException::User, message, _FILEINFO_);
    }
    else {
      value = param->defaultValues[0].toStdString();
    }
  }
  else {
    value = param->values[0].toStdString();
  }

  return value.ToDouble();
}


// Public: Returns the value member of a parameter whose name starts with paramName
// as a vector<doubble>
/**
 * Allows the retrieval of a value for a parameter of type "double".
 *
 * @param paramName The partial or full name of the parameter to be retrieved.
 *
 * @param values The value member of a parameter whose name starts with
 *                paramName.
 *
 * @throws iException::Programmer (IsisErrorUser) The parameter has no value.
 */
void IsisAml::GetDouble(const QString &paramName,
                        vector<double> &values) const {

  const IsisParameterData *param = ReturnParam(paramName);

  if(param->type != "double") {
    std::string message = "Parameter [" + paramName.toStdString() + "] is not a double.";
    throw Isis::IException(Isis::IException::Programmer, message, _FILEINFO_);
  }

  values.clear();
  Isis::IString value;
  if(param->values.size() == 0) {
    if(param->defaultValues.size() == 0) {
      std::string message = "Parameter [" + paramName.toStdString() + "] has no value.";
      throw Isis::IException(Isis::IException::User, message, _FILEINFO_);
    }
    else {
      for(unsigned int i = 0; i < param->defaultValues.size(); i++)
        value = param->defaultValues[i].toStdString();
      values.push_back(value.ToDouble());
    }
  }
  else {
    for(unsigned int i = 0; i < param->values.size(); i++)
      value = param->values[i].toStdString();
    values.push_back(value.ToDouble());
  }

  return;
}


// Public: Returns the first element of the value member of a parameter whos
// name starts with paramName as a bool
/**
 * Allows the retrieval of a value for a parameter of type "boolean".
 *
 * @param paramName The partial or full name of the parameter to be retrieved.
 *
 * @return The value of the parameter.
 *
 * @throws iException::Programmer (IsisErrorUser) The parameter has no value.
 */
bool IsisAml::GetBoolean(const QString &paramName) const {

  const IsisParameterData *param = ReturnParam(paramName);

  if(param->type != "boolean") {
    std::string message = "Parameter [" + paramName.toStdString() + "] is not a boolean.";
    throw Isis::IException(Isis::IException::Programmer, message, _FILEINFO_);
  }

  QString value;
  if(param->values.size() == 0) {
    if(param->defaultValues.size() == 0) {
      std::string message = "Parameter [" + paramName.toStdString() + "] has no value.";
      throw Isis::IException(Isis::IException::User, message, _FILEINFO_);
    }
    else {
      value = param->defaultValues[0];
    }
  }
  else {
    value = param->values[0];
  }

  return Isis::toBool(value.toStdString());

}


// Public: Returns the value member of a parameter whose name starts with paramName
// as a vector<bool>
/**
 * Allows the retrieval of a value for a parameter of type "boolean".
 *
 * @param paramName The partial or full name of the parameter to be retrieved.
 *
 * @param values The member value of a parameter whose name starts with
 * paramName..
 *
 * @throws iException::Programmer (IsisErrorUser) The parameter has no value.
 */
void IsisAml::GetBoolean(const QString &paramName,
                         vector<bool> &values) const {

  const IsisParameterData *param = ReturnParam(paramName);

  if(param->type != "boolean") {
    std::string message = "Parameter [" + paramName.toStdString() + "] is not a boolean.";
    throw Isis::IException(Isis::IException::Programmer, message, _FILEINFO_);
  }

  values.clear();
  vector <QString> value;
  QString tmp;
  if(param->values.size() == 0) {
    if(param->defaultValues.size() == 0) {
      std::string message = "Parameter [" + paramName.toStdString() + "] has no value.";
      throw Isis::IException(Isis::IException::User, message, _FILEINFO_);
    }
    else {
      for(unsigned int i = 0; i < param->defaultValues.size(); i++) {
        tmp = param->defaultValues[i].toUpper();
        value.push_back(tmp);
      }
    }
  }
  else {
    for(unsigned int i = 0; i < param->values.size(); i++) {
      tmp = param->values[i].toUpper();
      value.push_back(tmp);
    }
  }

  for(unsigned int i = 0; i < value.size(); i++) {
    values.push_back(StringToBool(value[i]));

  }

  return;
}
/**
 * Returns the Program name.
 *
 * @return The name of the program.
 */
QString IsisAml::ProgramName() const {
  QString tmp = name;
  return tmp;
}


/**
 * Returns the brief description of the program.
 *
 * @return The brief description.
 */
QString IsisAml::Brief() const {
  return brief;
}


/**
 * Returns the full description of the program.
 *
 * @return The full description.
 */
QString IsisAml::Description() const {
  return description;
}


/**
 * Returns the number of groups found in the XML.
 *
 * @return The number of groups.
 */
int IsisAml::NumGroups() const {
  return groups.size();
}


/**
 * Returns the group name of group[index].
 *
 * @param index The array index of the group.
 *
 * @return The group name.
 */
QString IsisAml::GroupName(const int &index) const {
  QString s = groups[index].name;
  return s;
}


/**
 * Given group name return its index in the Gui
 *
 * @author Sharmila Prasad (8/11/2011)
 *
 * @param grpName
 *
 * @return int
 */
int IsisAml::GroupIndex(const QString & grpName) const {
  for(int i=0; i<(int)groups.size(); i++) {
    if(Isis::IString(grpName.toStdString()).DownCase() == Isis::IString(groups[i].name.toStdString()).DownCase()) {
      return i;
    }
  }
  return -1;
}


/**
 * Creates a json object which could be used as a command line
 *
 */
json IsisAml::GetParams() { 
  json ret = {}; 
  Isis::PvlGroup group("UserParameters");

  // Add appropriate keywords
  for(unsigned int g = 0; g < groups.size(); g++) {
    for(unsigned int p = 0; p < groups[g].parameters.size(); p++) {
      const IsisParameterData *param = ReturnParam(ParamName(g, p));
      // If this param has a value add it to the command line
      if(param->values.size() > 0) {
        std::vector<std::string> values(param->values.size());

        for(unsigned int value = 0; value < param->values.size(); value++) {
          values[value] = param->values[value].toStdString();
        }

        if (values.size() == 0) { 
          ret[param->name.toStdString()] = "NULL";
        }
        if (values.size() > 1) {  
          ret[param->name.toStdString()] = values;
        }
        else { 
          ret[param->name.toStdString()] = values.at(0); 
        }

      }

      // Or if it has a default value add it to the command line
      else if(param->defaultValues.size() > 0) {
        std::vector<std::string> values(param->defaultValues.size());
        
        for(unsigned int value = 0;
           value < param->defaultValues.size();
           value++) {
          values[value] = param->defaultValues[value].toStdString();
        }

        if (values.size() == 0) { 
          ret[param->name.toStdString()] = "NULL";
        }
        else if (values.size() > 1) {  
          ret[param->name.toStdString()] = values;
        }
        else { 
          ret[param->name.toStdString()] = values.at(0); 
        }
      }
    }
  }

  return ret;  
}


/**
 * Create a PVL file from the parameters in a Group given the Gui group name,
 * Pvl Object and Group names and the list of parameters to be included in the
 * Pvl
 *
 * @author Sharmila Prasad (8/11/2011)
 *
 * @param pvlDef      - output PVL
 * @param guiGrpName  - Gui Group name
 * @param pvlObjName  - output PVL Object name
 * @param pvlGrpName  - output PVL Group name
 * @param include     - vector of parameter names to be part of the output PVL
 */
void IsisAml::CreatePVL(Isis::Pvl &pvlDef , QString guiGrpName, QString pvlObjName, QString pvlGrpName, vector<QString> & include) {

  Isis::PvlObject *pvlObj = NULL;
  if (pvlObjName != "") {
    pvlObj = new Isis::PvlObject(pvlObjName.toStdString());
  }

  // Get Gui Group index
  int grpIndex= GroupIndex(guiGrpName);

  if (pvlGrpName == "" || grpIndex == -1 ) {
    std::string errMsg = "Must provide Group Name\n";
    throw Isis::IException(Isis::IException::User, errMsg, _FILEINFO_);
  }

  Isis::PvlGroup grp(pvlGrpName.toStdString());
  for(int i=0; i<NumParams(grpIndex); i++) {
    QString paramName = ParamName(grpIndex, i);

    if(IsParamInPvlInclude(paramName,include)) {
      Isis::IString paramType = Isis::IString(ParamType(grpIndex, i).toStdString()).DownCase();
      if(paramType == "double") {
        grp += Isis::PvlKeyword(paramName.toStdString(), Isis::toString(GetDouble(paramName)));
      }
      if(paramType == "integer") {
        grp += Isis::PvlKeyword(paramName.toStdString(), Isis::toString(GetInteger(paramName)));
      }
      if(paramType == "boolean") {
        grp += Isis::PvlKeyword(paramName.toStdString(), Isis::toString(GetBoolean(paramName)));
      }
      if(paramType == "string" || paramType == "filename" || paramType == "combo") {
        grp += Isis::PvlKeyword(paramName.toStdString(), GetAsString(paramName).toStdString());
      }
    }
  }

  if(pvlObj != NULL) {
    *pvlObj += grp;
    pvlDef  += *pvlObj;
    delete (pvlObj);
    pvlObj = NULL;
  }
  else {
    pvlDef += grp;
  }
}


/**
 * Verify if the Parameter is in the Included list
 *
 * @author Sharmila Prasad (8/11/2011)
 *
 * @param paramName - parameter name
 * @param include   - include list
 *
 * @return bool
 */
bool IsisAml::IsParamInPvlInclude(QString & paramName, vector<QString> & include) {

  for(int i=0; i<(int)include.size(); i++) {
    if(Isis::IString(paramName.toStdString()).DownCase() == Isis::IString(include[i].toStdString()).DownCase()) {
      return true;
    }
  }
  return false;
}


/**
 * Returns the number of parameters in a group.
 *
 * @param group The group to measure.
 *
 * @return The number of parameters.
 */
int IsisAml::NumParams(const int &group) const {
  return groups[group].parameters.size();
}


/**
 * Returns the parameter name.
 *
 * @param group The group index where the parameter can be found.
 * @param param The index of the parameter to name.
 *
 * @return The name of the parameter.
 */
QString IsisAml::ParamName(const int &group, const int &param) const {
  QString s = groups[group].parameters[param].name;
  return s;
}


/**
 * Returns the brief description of a parameter in a specified group.
 *
 * @param group The group index
 * @param param The parameter index
 *
 * @return The brief description.
 */
QString IsisAml::ParamBrief(const int &group, const int &param) const {
  QString s = groups[group].parameters[param].brief;
  return s;
}


/**
 * Returns the long description of a parameter in a specified group.
 *
 * @param group The group index
 * @param param The parameter index
 *
 * @return The long description
 */
QString IsisAml::ParamDescription(const int &group, const int &param) const {
  QString s = groups[group].parameters[param].description;
  return s;
}


/**
 * Returns the minimum value of a parameter in a specified group.
 *
 * @param group The group index
 * @param param The parameter index
 *
 * @return The minimum
 */
QString IsisAml::ParamMinimum(const int &group, const int &param) const {
  QString s = groups[group].parameters[param].minimum;
  return s;
}


/**
 * Returns the maximum value of a parameter in a specified group.
 *
 * @param group The group index
 * @param param The parameter index
 *
 * @return The maximum
 */
QString IsisAml::ParamMaximum(const int &group, const int &param) const {
  QString s = groups[group].parameters[param].maximum;
  return s;
}


/**
 * Returns whether the minimum value is inclusive or not.
 *
 * @param group The group index
 * @param param The parameter index
 *
 * @return Whether minimum is inclusive or not
 */
QString IsisAml::ParamMinimumInclusive(const int &group, const int &param) const {
  QString s = groups[group].parameters[param].minimum_inclusive;
  return s;
}


/**
 * Returns whether the maximum value is inclusive or not.
 *
 * @param group The group index
 * @param param The parameter index
 *
 * @return Whether maximum is inclusive or not
 */
QString IsisAml::ParamMaximumInclusive(const int &group, const int &param) const {
  QString s = groups[group].parameters[param].maximum_inclusive;
  return s;
}


/**
 * Returns whether the selected parameter has a restriction on odd values or
 * not.
 *
 * @param group The group index
 * @param param The parameter index
 *
 * @return Whether the parameter restricts odd values or not
 */
QString IsisAml::ParamOdd(const int &group, const int &param) const {
  QString s = groups[group].parameters[param].odd;
  return s;
}


/**
 * Returns the number of values in the parameters greater than list.
 *
 * @param group The group index
 * @param param The parameter index
 *
 * @return Number of values in the parameters greater than list
 */
int IsisAml::ParamGreaterThanSize(const int &group, const int &param) const {
  return groups[group].parameters[param].greaterThan.size();
}


/**
 * Returns the number of values in the parameters greater than or equal list.
 *
 * @param group The group index
 * @param param The parameter index
 *
 * @return Number of values in the parameters greater than or equal list
 */
int IsisAml::ParamGreaterThanOrEqualSize(const int &group,
    const int &param) const {
  return groups[group].parameters[param].greaterThanOrEqual.size();
}


/**
 * Returns the number of values in the parameters less than list.
 *
 * @param group The group index
 * @param param The parameter index
 *
 * @return Number of values in the parameters less than list
 */
int IsisAml::ParamLessThanSize(const int &group, const int &param) const {
  return groups[group].parameters[param].lessThan.size();
}


/**
 * Returns the number of values in the parameters less than or equal list.
 *
 * @param group The group index
 * @param param The parameter index
 *
 * @return Number of values in the parameters less than or equal list
 */
int IsisAml::ParamLessThanOrEqualSize(const int &group,
                                      const int &param) const {
  return groups[group].parameters[param].lessThanOrEqual.size();
}


/**
 * Returns the number of values in the not equal list.
 *
 * @param group The group index
 * @param param The parameter index
 *
 * @return Number of values in the parameters not equal list
 */
int IsisAml::ParamNotEqualSize(const int &group, const int &param) const {
  return groups[group].parameters[param].notEqual.size();
}


/**
 * Returns the name of the specified greaterThan parameter
 *
 * @param group The group index
 * @param param The parameter index
 * @param great The greaterThan index
 *
 * @return The name of the greaterThan parameter
 */
QString IsisAml::ParamGreaterThan(const int &group, const int &param,
                                 const int &great) const {
  QString s = groups[group].parameters[param].greaterThan[great];
  return s;
}


/**
 * Returns the name of the specified greaterThanOrEqual parameter
 *
 * @param group The group index
 * @param param The parameter index
 * @param great The greaterThanOrEqual index
 *
 * @return The name of the greaterThanOrEqual parameter
 */
QString IsisAml::ParamGreaterThanOrEqual(const int &group, const int &param,
                                        const int &great) const {
  QString s = groups[group].parameters[param].greaterThanOrEqual[great];
  return s;
}


/**
 * Returns the name of the specified lessThan parameter
 *
 * @param group The group index
 * @param param The parameter index
 * @param les The lessThan index
 *
 * @return The name of the lessThan parameter
 */
QString IsisAml::ParamLessThan(const int &group, const int &param,
                              const int &les) const {
  QString s = groups[group].parameters[param].lessThan[les];
  return s;
}


/**
 * Returns the name of the specified lessThanOrEqual parameter
 *
 * @param group The group index
 * @param param The parameter index
 * @param les The lessThanOrEqual index
 *
 * @return The name of the lessThanOrEqual parameter
 */
QString IsisAml::ParamLessThanOrEqual(const int &group, const int &param,
                                     const int &les) const {
  QString s = groups[group].parameters[param].lessThanOrEqual[les];
  return s;
}


/**
 * Returns the name of the specified notEqual parameter
 *
 * @param group The group index
 * @param param The parameter index
 * @param notEq The notEqual index
 *
 * @return The name of the notEqual parameter
 */
QString IsisAml::ParamNotEqual(const int &group, const int &param,
                              const int &notEq) const {
  QString s = groups[group].parameters[param].notEqual[notEq];
  return s;
}


/**
 * Returns the name of the specified excluded parameter
 *
 * @param group The group index
 * @param param The parameter index
 * @param exclude The exclude index
 *
 * @return The name of the excluded parameter
 */
QString IsisAml::ParamExclude(const int &group, const int &param,
                             const int &exclude) const {
  QString s = groups[group].parameters[param].exclude[exclude];
  return s;
}


/**
 * Returns the name of the specified included parameter
 *
 * @param group The group index
 * @param param The parameter index
 * @param include The include index
 *
 * @return The name of the included parameter
 */
QString IsisAml::ParamInclude(const int &group, const int &param,
                             const int &include) const {
  QString s = groups[group].parameters[param].include[include];
  return s;
}


/**
 * Returns the parameter type of a parameter in a specified group.
 *
 * @param group The group index
 * @param param The parameter index
 *
 * @return The type of parameter.
 */
QString IsisAml::ParamType(const int &group, const int &param) const {
  QString s = groups[group].parameters[param].type;
  return s;
}


/**
 * Returns the default for a parameter in a specified group.
 *
 * @param group The group index
 * @param param The parameter index
 *
 * @return The default for the specified parameter.
 */
QString IsisAml::ParamDefault(const int &group, const int &param) const {
  QString s;
  if(groups[group].parameters[param].defaultValues.size() == 0) {
    s = "";
  }
  else {
    s = groups[group].parameters[param].defaultValues[0];
  }
  return s;
}


/**
 * Returns the internal default for a parameter in a specified group
 *
 *.@param group The group index
 * @param param The parameter index
 *
 * @return The internal default for the specified parameter.
 */
QString IsisAml::ParamInternalDefault(const int &group, const int &param) const {
  QString s;
  if(groups[group].parameters[param].internalDefault.size() == 0) {
    s = "";
  }
  else {
    s = groups[group].parameters[param].internalDefault;
  }
  return s;
}


/**
 * Returns the parameter filter for a parameter in a specified group.
 *
 * @param group The group index
 * @param param The parameter index
 *
 * @return The paramter filter.
 */
QString IsisAml::ParamFilter(const int &group, const int &param) const {
  QString s;
  if(groups[group].parameters[param].filter.size() == 0) {
    s = "";
  }
  else {
    s = groups[group].parameters[param].filter;
  }
  return s;
}


/**
 * Returns the default path for a filename/cube parameter
 *
 * @param group The group index
 * @param param The parameter index
 *
 * @return The default path.
 */
QString IsisAml::ParamPath(const int &group, const int &param) const {
  QString s;
  if(groups[group].parameters[param].path.size() == 0) {
    s = "";
  }
  else {
    s = groups[group].parameters[param].path;
  }
  return s;
}


/**
 * Returns the file mode for a parameter in a specified group.
 *
 * @param group The group index
 * @param param The parameter index
 *
 * @return The file mode for the parameter.
 */
QString IsisAml::ParamFileMode(const int &group, const int &param) const {
  QString s;
  if(groups[group].parameters[param].fileMode.size() == 0) {
    s = "";
  }
  else {
    s = groups[group].parameters[param].fileMode;
  }
  return s;
}


/**
 * Returns the number of options in the specified parameter's list.
 *
 * @param group The group index
 * @param param The parameter index
 *
 * @return The number of options contained in the parameter's list. If the
 * parameter does not contain a list, zero will be returned.
 */
int IsisAml::ParamListSize(const int &group, const int &param) const {
  return groups[group].parameters[param].listOptions.size();
}


/**
 * Returns the option value for a specific option to a parameter.
 *
 * @param group The group index
 * @param param The parameter index
 * @param option The option number within the parameters list.
 *
 * @return The value of the option.
 */
QString IsisAml::ParamListValue(const int &group, const int &param,
                               const int &option) const {
  QString s = groups[group].parameters[param].listOptions[option].value;
  return s;
}


/**
 * Returns the brief description for a specific option to a parameter.
 *
 * @param group The group index
 * @param param The parameter index
 * @param option The option number within the parameters list.
 *
 * @return The brief description of the option.
 */
QString IsisAml::ParamListBrief(const int &group, const int &param,
                               const int &option) const {
  QString s = groups[group].parameters[param].listOptions[option].brief;
  return s;
}


/**
 * Returns the full description for a specific option to a parameter.
 *
 * @param group The group index
 * @param param The parameter index
 * @param option The option number within the parameters list.
 *
 * @return The full description of the option.
 */
QString IsisAml::ParamListDescription(const int &group, const int &param,
                                     const int &option) const {
  QString s = groups[group].parameters[param].listOptions[option].description;
  return s;
}


/**
 * Returns the number of items in a parameters list exclude section.
 *
 * @param group The group index
 * @param param The parameter index
 * @param option The option number within the parameters list.
 *
 * @return The number of items in the parameters list exclude section.
 */
int IsisAml::ParamListExcludeSize(const int &group, const int &param,
                                  const int &option) const {
  return groups[group].parameters[param].listOptions[option].exclude.size();
}


/**
 * Returns the parameter name to be excluded if this option is selected.
 *
 * @param group The group index
 * @param param The parameter index
 * @param option The option number within the parameters list.
 * @param exclude The exclusion number within the parameters list.
 *
 * @return The parameter name to be excluded if this option is selected.
 */
QString IsisAml::ParamListExclude(const int &group, const int &param,
                                 const int &option, const int &exclude) const {
  QString s = groups[group].parameters[param].listOptions[option].exclude[exclude];
  return s;
}


/**
 * Returns the number of items in a parameters list include section.
 *
 * @param group The group index
 * @param param The parameter index
 * @param option The option number within the parameters list.
 *
 * @return The number of items in the parameters list include section.
 */
int IsisAml::ParamListIncludeSize(const int &group, const int &param,
                                  const int &option) const {
  return groups[group].parameters[param].listOptions[option].include.size();
}


/**
 * Returns the parameter name to be included if this option is selected.
 *
 * @param group The group index
 * @param param The parameter index
 * @param option The option number within the parameters list.
 * @param include The inclusion number within the parameters list.
 *
 * @return The parameter name to be included if this option is selected.
 */
QString IsisAml::ParamListInclude(const int &group, const int &param,
                                 const int &option, const int &include) const {
  QString s = groups[group].parameters[param].listOptions[option].include[include];
  return s;
}


/**
 * Returns the number of parameters excluded in this parameter's exclusions
 *
 * @param group The group index
 * @param param The parameter index
 *
 * @return The number of items in the parameters exclude list
 */
int IsisAml::ParamExcludeSize(const int &group, const int &param) const {
  return groups[group].parameters[param].exclude.size();
}


/**
 * Returns the number of parameters included in this parameter's inclusions
 *
 * @param group The group index
 * @param param The parameter index
 *
 * @return The number of items in the parameters include list
 */
int IsisAml::ParamIncludeSize(const int &group, const int &param) const {
  return groups[group].parameters[param].include.size();
}


/**
 * Returns the default pixel type from the XML
 *
 * @param group The group index
 * @param param The parameter index
 *
 * @return The default pixel type
 */
QString IsisAml::PixelType(const int &group, const int &param) const {
  return groups[group].parameters[param].pixelType;
}


/**
 * Returns the number of helpers the parameter has
 *
 * @param group The group index
 * @param param The parameter index
 *
 * @return int The number of helpers the parameter has
 */
int IsisAml::HelpersSize(const int &group, const int &param) const {
  return groups[group].parameters[param].helpers.size();
}


/**
 * Returns the name of the helper button
 *
 * @param group The group index
 * @param param The parameter index
 * @param helper The helper index
 *
 * @return QString The name of the helper
 */
QString IsisAml::HelperButtonName(const int &group, const int &param,
                                 const int &helper) const {
  return groups[group].parameters[param].helpers[helper].name;
}


/**
 * Returns the name of the helper function
 *
 * @param group The group index
 * @param param The parameter index
 * @param helper The helper index
 *
 * @return QString The name of the helper function
 */
QString IsisAml::HelperFunction(const int &group, const int &param,
                               const int &helper) const {
  return groups[group].parameters[param].helpers[helper].function;
}


/**
 * Returns the brief description of the helper button
 *
 * @param group The group index
 * @param param The parameter index
 * @param helper The helper index
 *
 * @return QString The brief description of the helper button
 */
QString IsisAml::HelperBrief(const int &group, const int &param,
                            const int &helper) const {
  return groups[group].parameters[param].helpers[helper].brief;
}


/**
 * Returns the long description of the helper button
 *
 * @param group The group index
 * @param param The parameter index
 * @param helper The helper index
 *
 * @return QString The long description of the helper button
 */
QString IsisAml::HelperDescription(const int &group, const int &param,
                                  const int &helper) const {
  return groups[group].parameters[param].helpers[helper].description;
}


/**
 * Returns the name of the icon for the helper button
 *
 * @param group The group index
 * @param param The parameter index
 * @param helper The helper index
 *
 * @return QString The name of the helper icon
 */
QString IsisAml::HelperIcon(const int &group, const int &param,
                           const int &helper) const {
  return groups[group].parameters[param].helpers[helper].icon;
}


/**
 * Returns a true if the parameter has a value, and false if it does not
 *
 * @param paramName The name of the parameter to check if it was entered
 *
 * @return True if the parameter was entered, and false if it was not
 */
bool IsisAml::WasEntered(const QString &paramName) const {

  const IsisParameterData *param = ReturnParam(paramName);

  if(param->values.size() == 0) {
    return false;
  }

  return true;
}


/**
 * Clears the value(s) in the named parameter
 *
 * @param paramName The name of the parameter to clear
 */
void IsisAml::Clear(const QString &paramName) {

  IsisParameterData *param = const_cast <IsisParameterData *>(ReturnParam(paramName));
  param->values.clear();

  param->outCubeAtt.setAttributes("+" + param->pixelType.toStdString());
  param->inCubeAtt.setAttributes("");

  return;
}


/**
 * Gets the attributes for an input cube
 *
 * @param paramName The name of the parameter to get the attributes for
 *
 * @return CubeAttributeInput
 *
 * @throws iException::Programmer "Parameter is not a cube."
 * @throws iException::Programmer "Parameter in not an input cube"
 */
Isis::CubeAttributeInput &IsisAml::GetInputAttribute(const QString &paramName) {

  IsisParameterData *param = const_cast <IsisParameterData *>(ReturnParam(paramName));

  if(param->type != "cube") {
    std::string message = "Unable to get input cube attributes.  Parameter ["
      + paramName.toStdString() + "] is not a cube. Parameter type = [" + param->type.toStdString() + "].";
    throw Isis::IException(Isis::IException::Programmer, message, _FILEINFO_);
  }

  QString value;
  if(param->values.size() == 0) {
    if(param->defaultValues.size() == 0) {
      value.clear();
//      std::string message = "Parameter [" + paramName.toStdString() + "] has no value.";
//      throw Isis::IException(Isis::IException::User,message, _FILEINFO_);
    }
    else {
      value = param->defaultValues[0];
    }
  }
  else {
    value = param->values[0];
  }
  if(param->fileMode == "input") {
    param->inCubeAtt.setAttributes(value.toStdString());
  }
  else {
    std::string message = "Unable to get input cube attributes.  Parameter ["
      + paramName.toStdString() + "] is not an input. Parameter fileMode = [" + param->fileMode.toStdString() + "].";
    throw Isis::IException(Isis::IException::Programmer, message, _FILEINFO_);
  }
  return param->inCubeAtt;
}


/**
 * Gets the attributes for an output cube
 *
 * @param paramName The name of the parameter to get the attributes for
 *
 * @return CubeAttributeOutput
 *
 * @throws iException::Programmer "Parameter is not a cube"
 * @throws iException::Programmer "Parameter in not an output"
 */
Isis::CubeAttributeOutput &IsisAml::GetOutputAttribute(const QString &paramName) {

  IsisParameterData *param = const_cast <IsisParameterData *>(ReturnParam(paramName));

  if(param->type != "cube") {
    std::string message = "Unable to get output cube attributes.  Parameter ["
      + paramName.toStdString() + "] is not a cube. Parameter type = [" + param->type.toStdString() + "].";
    throw Isis::IException(Isis::IException::Programmer, message, _FILEINFO_);
  }

  QString value;
  if(param->values.size() == 0) {
    if(param->defaultValues.size() == 0) {
      value.clear();
//      std::string message = "Parameter [" + paramName.toStdString() + "] has no value.";
//      throw Isis::IException(Isis::IException::User,message, _FILEINFO_);
    }
    else {
      value = param->defaultValues[0];
    }
  }
  else {
    value = param->values[0];
  }
  if(param->fileMode == "output") {
    param->outCubeAtt.setAttributes("+" + param->pixelType.toStdString());
    param->outCubeAtt.addAttributes(Isis::FileName(value.toStdString()));
  }
  else {
    std::string message = "Unable to get output cube attributes.  Parameter ["
      + paramName.toStdString() + "] is not an output. Parameter fileMode = [" + param->fileMode.toStdString() + "].";
    throw Isis::IException(Isis::IException::Programmer, message, _FILEINFO_);
  }
  return param->outCubeAtt;
}


/**
 * Returns a pointer to a parameter whose name starts with paramName
 *
 * @param paramName The name of the parameter to return
 *
 * @return IsisParameterData
 *
 * @throws iException::User (Parameter is not unique)
 * @throws iException::User (Unknown Parameter)
 */
const IsisParameterData *IsisAml::ReturnParam(const QString &paramName) const {
  Isis::IString pn = paramName.toStdString();
  pn.UpCase();
  int found = 0;
  bool exact = false;
  const IsisParameterData *param = NULL;
  Isis::IString cur_pn;

  for(unsigned int g = 0; g < groups.size(); g++) {
    for(unsigned int p = 0; p < groups[g].parameters.size(); p++) {
      cur_pn = groups[g].parameters[p].name.toStdString();
      cur_pn.UpCase();
      if(cur_pn.find(pn) == 0) {
        if(cur_pn == pn) {
          if(exact) {
            std::string message = "Parameter [" + paramName.toStdString() + "] is not unique.";
            throw Isis::IException(Isis::IException::User, message, _FILEINFO_);
          }
          else {
            exact = true;
            found = 0;
            param = &(groups[g].parameters[p]);
          }
        }
        else if(!exact) {
          found++;
          param = &(groups[g].parameters[p]);
        }
      }
    }
  }
  if(param == NULL) {
    std::string message = "Unknown parameter [" + paramName.toStdString() + "].";
    throw Isis::IException(Isis::IException::User, message, _FILEINFO_);
  }
  else if((found > 1) && (!exact)) {
    std::string message = "Parameter [" + paramName.toStdString() + "] is not unique.";
    throw Isis::IException(Isis::IException::User, message, _FILEINFO_);
  }
  return param;
}


/**
 * Throws an Isis::iExceptionXxxxxxxx if the parameter value(s) is invalid
 *
 * @param param The parameter data
 *
 * @throws iException::User
 * @internal
 *   @history 2010-07-19 Jeannie Walldren - Added check for FileCustomization
 *                          preference if an existing output file is selected
 */
void IsisAml::Verify(const IsisParameterData *param) {

  // Check to make sure the value QString can be converted to the correct type
  for(unsigned int i = 0; i < param->values.size(); i++) {
    if(param->type == "integer") {
      try {
        Isis::IString value(param->values[i].toStdString());
        value.ToInteger();
      }
      catch(Isis::IException &e) {
        std::string message = "Unable to convert [" + param->values[i].toStdString() + "] to an integer,"
                         " parameter [" + param->name.toStdString() + "].";
        throw Isis::IException(e, Isis::IException::User, message, _FILEINFO_);
      }
    }
    else if(param->type == "double") {
      try {
        Isis::IString value(param->values[i].toStdString());
        value.ToDouble();
      }
      catch(Isis::IException &e) {
        std::string message = "Unable to convert [" + param->values[i].toStdString() + "] to a double,"
                         " parameter [" + param->name.toStdString() + "].";
        throw Isis::IException(e, Isis::IException::User, message, _FILEINFO_);
      }
    }
    else if(param->type == "boolean") {
      QString v = param->values[i].toUpper();

      try {
        StringToBool(v);
      }
      catch(Isis::IException &e) {
        std::string message = "Illegal value for [" + param->name.toStdString() + "], [" + param->values[i].toStdString() + "].";
        throw Isis::IException(e, Isis::IException::User, message, _FILEINFO_);
      }
    }
    else if(param->type == "filename") {
      // If this is an output file and a file with this name already exists,
      // check user filename customization preferences.
      QString value(param->values[i]);
      Isis::FileName name(value.toStdString());
      value = QString::fromStdString(name.expanded());
      if(name.fileExists() && param->fileMode == "output") {
        CheckFileNamePreference(value, param->name);
      }
    }
    // THIS IS CURRENTLY HANDLED IN THE CUBE CLASS, see CubeIoHandler.cpp
    // 2010-07-15 Jeannie Walldren
    //
    //  else if(param->type == "cube") {
    //    Isis::IString value(param->values[i]);
    //    Isis::FileName name(value);
    //    value = name.expanded();
    //    if (name.Exists() && param->fileMode == "output"
    //        && Isis::Preference::Preferences().findGroup("CubeCustomization").findKeyword("Overwrite")[0] == "Error") {
    //      std::string message = "Invalid output cube for [" + param->name + "]. The cube file [" + value + "] already exists.  " +
    //                       "The user preference cube customization group is set to disallow cube overwrites.";
    //      throw Isis::IException(Isis::IException::User, message, _FILEINFO_);
    //    }
    //  }
  }

  // Check the default values if there were no values
  if(param->values.size() == 0) {
    for(unsigned int i = 0; i < param->defaultValues.size(); i++) {
      // Check to make sure the DefaultValue QString can be converted to the
      // correct type
      if(param->type == "integer") {
        try {
          Isis::IString value(param->defaultValues[i].toStdString());
          value.ToInteger();
        }
        catch(Isis::IException &e) {
          std::string message = "Unable to convert default [" + param->defaultValues[i].toStdString() +
                           "] to an integer, parameter [" + param->name.toStdString() + "].";
          throw Isis::IException(e, Isis::IException::Programmer, message, _FILEINFO_);
        }
      }
      else if(param->type == "double") {
        try {
          Isis::IString value(param->defaultValues[i].toStdString());
          value.ToDouble();
        }
        catch(Isis::IException &e) {
          std::string message = "Unable to convert default [" + param->defaultValues[i].toStdString() +
                           "] to a double, parameter [" + param->name.toStdString() + "].";
          throw Isis::IException(e, Isis::IException::Programmer, message, _FILEINFO_);
        }
      }
      else if(param->type == "boolean") {
        QString v = param->defaultValues[i].toUpper();

        try {
          StringToBool(v);
        }
        catch(Isis::IException &e) {
          std::string message = "Illegal default value for [" + param->name.toStdString() + "], ["
                           + param->defaultValues[i].toStdString() + "].";
          throw Isis::IException(e, Isis::IException::User, message, _FILEINFO_);
        }
      }
      else if(param->type == "filename") {
        // Put something here once we figure out what to do with filenames
        QString value(param->defaultValues[i]);
        Isis::FileName name(value.toStdString());
        value = QString::fromStdString(name.expanded());
        if(name.fileExists() && param->fileMode == "output") {
          CheckFileNamePreference(value, param->name);
        }
      }
    }
  }

  // Check the values against the values list if there is one
  if(param->listOptions.size() > 0) {
    for(unsigned int i = 0; i < param->values.size(); i++) {
      Isis::IString value = param->values[i].toStdString();
      value = value.UpCase();
      int partial = 0;
      bool exact = false;
      for(unsigned int p = 0; p < param->listOptions.size(); p++) {
        Isis::IString option = param->listOptions[p].value.toStdString();
        option = option.UpCase();
        // Check to see if the value matches the list option exactly
        if(value == option) {
          // If we already have one exact match then there is an error
          if(exact) {
            std::string message = "Duplicate list options [" +
                             param->listOptions[p].value.toStdString() +
                             "] in parameter [" + param->name.toStdString() + "].";
            throw Isis::IException(Isis::IException::Programmer, message, _FILEINFO_);
          }
          exact = true;
        }
        // Check for a partial match
        // Compare the shorter of the two (values[i]) to a subQString of the
        // longer QString (listOptions[p].value)
        else if(option.compare(0, min(value.size(), option.size()),
                               value, 0, min(value.size(), option.size())) == 0) {
          partial++;
        }
      }
      if(!exact && partial == 0) {
        std::string message = "Value of [" + param->name.toStdString() + "] must be one of [" +
                         param->listOptions[0].value.toStdString();
        for(unsigned int p = 1; p < param->listOptions.size(); p++) {
          message += ", " + param->listOptions[p].value.toStdString();
        }
        message += "].";
        throw Isis::IException(Isis::IException::User, message, _FILEINFO_);
      }
      else if(!exact && partial > 1) {
        std::string msg = "Value of [" + param->name.toStdString() +
                     "] does not match a list option uniquely.";
        throw Isis::IException(Isis::IException::Programmer, msg, _FILEINFO_);
      }
    }
  }

  // Check the values against the minimum
  if(param->minimum.length() > 0) {
    QString incl = param->minimum_inclusive;
    for(unsigned int i = 0; i < param->values.size(); i++) {
      if(param->type == "integer") {
        QString value(param->values[i]);
        int temp = Isis::toInt(value.toStdString());
        value = param->minimum;
        int min = Isis::toInt(value.toStdString());
        if(StringToBool(incl) && (temp < min)) {
          std::string message = "Parameter [" + param->name.toStdString() +
                           "] must be greater than or equal to [" + param->minimum.toStdString() + "].";
          throw Isis::IException(Isis::IException::User, message, _FILEINFO_);
        }
        else if(!StringToBool(incl) && (temp <= min)) {
          std::string message = "Parameter [" + param->name.toStdString() +
                           "] must be greater than [" + param->minimum.toStdString() + "].";
          throw Isis::IException(Isis::IException::User, message, _FILEINFO_);
        }
      }
      else if(param->type == "double") {
        Isis::IString value(param->values[i].toStdString());
        double temp = value.ToDouble();
        value = param->minimum.toStdString();
        double min = value.ToDouble();
        if(StringToBool(incl) && (temp < min)) {
          std::string message = "Parameter [" + param->name.toStdString() +
                           "] must be greater than or equal to [" + param->minimum.toStdString() + "].";
          throw Isis::IException(Isis::IException::User, message, _FILEINFO_);
        }
        else if(!StringToBool(incl) && (temp <= min)) {
          std::string message = "Parameter [" + param->name.toStdString() +
                           "] must be greater than [" + param->minimum.toStdString() + "].";
          throw Isis::IException(Isis::IException::User, message, _FILEINFO_);
        }
      }
    }
    if(param->values.size() == 0) {
      for(unsigned int i = 0; i < param->defaultValues.size(); i++) {
        if(param->type == "integer") {
          Isis::IString value(param->defaultValues[i].toStdString());
          int temp = value.ToInteger();
          value = param->minimum.toStdString();
          int min = value.ToInteger();
          if(StringToBool(incl) && (temp < min)) {
            std::string message = "Parameter [" + param->name.toStdString() +
                             "] must be greater than or equal to [" + param->minimum.toStdString() + "].";
            throw Isis::IException(Isis::IException::User, message, _FILEINFO_);
          }
          else if(!StringToBool(incl) && (temp <= min)) {
            std::string message = "Parameter [" + param->name.toStdString() +
                             "] must be greater than [" + param->minimum.toStdString() + "].";
            throw Isis::IException(Isis::IException::User, message, _FILEINFO_);
          }
        }
        else if(param->type == "double") {
          Isis::IString value(param->defaultValues[i].toStdString());
          double temp = value.ToDouble();
          value = param->minimum.toStdString();
          double min = value.ToDouble();
          if(StringToBool(incl) && (temp < min)) {
            std::string message = "Parameter [" + param->name.toStdString() +
                             "] must be greater than or equal to [" + param->minimum.toStdString() + "].";
            throw Isis::IException(Isis::IException::User, message, _FILEINFO_);
          }
          else if(!StringToBool(incl) && (temp <= min)) {
            std::string message = "Parameter [" + param->name.toStdString() +
                             "] must be greater than [" + param->minimum.toStdString() + "].";
            throw Isis::IException(Isis::IException::User, message, _FILEINFO_);
          }
        }
      }
    }
  }

  // Check the values against the maximum
  if(param->maximum.length() > 0) {
    QString incl = param->maximum_inclusive.toLower();
    for(unsigned int i = 0; i < param->values.size(); i++) {
      if(param->type == "integer") {
        QString value(param->values[i]);
        int temp = Isis::toInt(value.toStdString());
        value = param->maximum;
        int max = Isis::toInt(value.toStdString());
        if(StringToBool(incl) && (temp > max)) {
          std::string message = "Parameter [" + param->name.toStdString() +
                           "] must be less than or equal to [" + param->maximum.toStdString() + "].";
          throw Isis::IException(Isis::IException::User, message, _FILEINFO_);
        }
        else if(!StringToBool(incl) && (temp >= max)) {
          std::string message = "Parameter [" + param->name.toStdString() +
                           "] must be less than [" + param->maximum.toStdString() + "].";
          throw Isis::IException(Isis::IException::User, message, _FILEINFO_);
        }
      }
      else if(param->type == "double") {
        Isis::IString value(param->values[i].toStdString());
        double temp = value.ToDouble();
        value = param->maximum.toStdString();
        double max = value.ToDouble();
        if(StringToBool(incl) && (temp > max)) {
          std::string message = "Parameter [" + param->name.toStdString() +
                           "] must be less than or equal to [" + param->maximum.toStdString() + "].";
          throw Isis::IException(Isis::IException::User, message, _FILEINFO_);
        }
        else if(!StringToBool(incl) && (temp >= max)) {
          std::string message = "Parameter [" + param->name.toStdString() +
                           "] must be less than [" + param->maximum.toStdString() + "].";
          throw Isis::IException(Isis::IException::User, message, _FILEINFO_);
        }
      }
    }
    if(param->values.size() == 0) {
      for(unsigned int i = 0; i < param->defaultValues.size(); i++) {
        if(param->type == "integer") {
          Isis::IString value(param->defaultValues[i].toStdString());
          int temp = value.ToInteger();
          value = param->maximum.toStdString();
          int max = value.ToInteger();
          if(StringToBool(incl) && (temp > max)) {
            std::string message = "Parameter [" + param->name.toStdString() +
                             "] must be less than or equal to [" + param->maximum .toStdString()+ "].";
            throw Isis::IException(Isis::IException::User, message, _FILEINFO_);
          }
          else if(!StringToBool(incl) && (temp >= max)) {
            std::string message = "Parameter [" + param->name.toStdString() +
                             "] must be less than [" + param->maximum.toStdString() + "].";
            throw Isis::IException(Isis::IException::User, message, _FILEINFO_);
          }
        }
        else if(param->type == "double") {
          Isis::IString value(param->defaultValues[i].toStdString());
          double temp = value.ToDouble();
          value = param->maximum.toStdString();
          double max = value.ToDouble();
          if(StringToBool(incl) && (temp > max)) {
            std::string message = "Parameter [" + param->name.toStdString() +
                             "] must be less than or equal to [" + param->maximum.toStdString() + "].";
            throw Isis::IException(Isis::IException::User, message, _FILEINFO_);
          }
          else if(!StringToBool(incl) && (temp >= max)) {
            std::string message = "Parameter [" + param->name.toStdString() +
                             "] must be less than [" + param->maximum.toStdString() + "].";
            throw Isis::IException(Isis::IException::User, message, _FILEINFO_);
          }
        }
      }
    }
  }

  // Check the value for an odd test
  QString odd = param->odd.toLower();

  if((odd != "") || StringToBool(odd)) {
    if(param->type != "integer") {
      std::string message = "Parameter [" + param->name.toStdString() +
                       "] must be of type integer to have an [odd] test.";
      throw Isis::IException(Isis::IException::Programmer, message, _FILEINFO_);
    }
    else {
      for(unsigned int i = 0; i < param->values.size(); i++) {
        Isis::IString value(param->values[i].toStdString());
        if((value.ToInteger() % 2) != 1) {
          std::string message = "Value for [" + param->name.toStdString() + "] must be odd.";
          throw Isis::IException(Isis::IException::User, message, _FILEINFO_);
        }
      }
    }
  }
  return;
}


/**
 * This method checks whether the user preferences are set to allow overwrites
 * of existing files.  It should be called if the parameter is an output and the
 * given file name exists.
 *
 * @param filename Name of the file to be overwritten.
 * @param paramname Name of the output file parameter.
 *
 * @throw iException::User -  "The file already exists. The user preference file
 *        customization group is set to disallow file overwrites."
 * @throw iException::User - "Invalid entry in user preference file
 *        FileCustomization group."
 *
 * @author 2010-07-19 Jeannie Walldren
 * @internal
 *   @history 2010-07-19 Jeannie Walldren - Original version.
 */
void IsisAml::CheckFileNamePreference(QString filename, QString paramname) {
  Isis::PvlGroup fileCustomization = Isis::Preference::Preferences().findGroup("FileCustomization");
  QString overwritePreference = QString::fromStdString(fileCustomization.findKeyword("Overwrite")[0]).simplified().trimmed();
  QString temp = overwritePreference;
  if(overwritePreference.toUpper() == "ERROR") {
    std::string message = "Invalid output filename for [" + paramname.toStdString() + "]. The file [" + filename.toStdString() + "] already exists.  " +
                     "The user preference file customization group is set to disallow file overwrites.";
    throw Isis::IException(Isis::IException::User, message, _FILEINFO_);
  }
  else if(overwritePreference.toUpper() != "ALLOW") { // not set to ERROR or ALLOW
    std::string message = "Invalid entry in user preference file FileCustomization group.";
    message += "  Overwrite = [" + temp.toStdString() + "].  Valid values: [Allow] or [Error].";
    throw Isis::IException(Isis::IException::User, message, _FILEINFO_);
  }
}


/**
 * Verify all parameters
 */
void IsisAml::VerifyAll() {
  for(unsigned int g = 0; g < groups.size(); g++) {
    for(unsigned int p = 0; p < groups[g].parameters.size(); p++) {
      IsisParameterData *param = &(this->groups[g].parameters[p]);

      Verify(param);

      // Check the values for inclusive clauses
      for(unsigned int item = 0; item < param->include.size(); item++) {
        // If this parameter is a boolean and it is true/yes
        // all included parameters must have some type of value
        if(param->type == "boolean") {
          if(((param->values.size() > 0) && StringToBool(param->values[0])) ||
              ((param->values.size() == 0) && (param->defaultValues.size() > 0)
               && StringToBool(param->defaultValues[0]))) {

            const IsisParameterData *param2 = ReturnParam(param->include[item]);
            if((param2->values.size()) == 0 &&
                (param2->defaultValues.size() == 0) &&
                (param2->internalDefault.size() == 0)) {
              std::string message = "Parameter [" + param2->name.toStdString() +
                               "] must be used if parameter [" +
                               param->name.toStdString() + "] equates to true.";
              throw Isis::IException(Isis::IException::User, message, _FILEINFO_);
            }
          }
        }
        // This parameter is NOT a boolean but the other one might be
        else {
          // If the other parameter is a boolean and it is true/yes
          // then this parameter must have some type of value
          const IsisParameterData *param2 = ReturnParam(param->include[item]);
          if(param2->type == "boolean") {
            if(((param2->values.size() > 0) && StringToBool(param2->values[0])) ||
                ((param2->values.size() == 0) && (param2->defaultValues.size() > 0) &&
                 StringToBool(param2->defaultValues[0]))) {
              if((param->values.size()) == 0 &&
                  (param->defaultValues.size() == 0) &&
                  (param->internalDefault.size() == 0)) {
                std::string message = "Parameter [" + param2->name.toStdString() +
                                 "] must be used if parameter [" +
                                 param->name.toStdString() + "] is used.";
                throw Isis::IException(Isis::IException::User, message, _FILEINFO_);
              }
            }
          }
          // Neithter parameter is a boolean so
          // If this one has a value the other parameter must have some type of value
          else {
            if(param->values.size() > 0 &&
                param2->values.size() == 0 &&
                param2->defaultValues.size() == 0 &&
                param2->internalDefault.size() == 0) {
              std::string message = "Parameter [" + param2->name.toStdString() +
                               "] must be used if parameter [" +
                               param->name.toStdString() + "] is used.";
              throw Isis::IException(Isis::IException::User, message, _FILEINFO_);
            }
          }
        }
      }
      // Check the values for exclusive clauses
      for(unsigned int item = 0; item < param->exclude.size(); item++) {
        // If this parameter is a boolean that is true/yes
        // the other parameter should not have a value
        if(param->type == "boolean") {
          if(((param->values.size() > 0) && StringToBool(param->values[0])) ||
              ((param->values.size() == 0) && (param->defaultValues.size() > 0) &&
               StringToBool(param->defaultValues[0]))) {

            const IsisParameterData *param2 = ReturnParam(param->exclude[item]);
            if(param2->values.size() > 0) {
              std::string message = "Parameter [" + param2->name.toStdString() +
                               "] must NOT be used if parameter [" +
                               param->name.toStdString() + "] equates to true.";
              throw Isis::IException(Isis::IException::User, message, _FILEINFO_);
            }
          }
        }
        // This parameter is NOT a boolean but the other one might be
        else {
          const IsisParameterData *param2 = ReturnParam(param->exclude[item]);
          if(param2->type == "boolean") {
            if(((param2->values.size() > 0) && StringToBool(param2->values[0])) ||
                ((param2->values.size() == 0) && (param2->defaultValues.size() > 0) &&
                 StringToBool(param2->defaultValues[0]))) {
              if(param->values.size() > 0) {
                std::string message = "Parameter [" + param2->name.toStdString() +
                                 "] must be used if parameter [" +
                                 param->name.toStdString() + "] is used.";
                throw Isis::IException(Isis::IException::User, message, _FILEINFO_);
              }
            }
          }
          // Neither parameter is a boolean
          else {
            if(param->values.size() > 0 && param2->values.size() > 0) {
              std::string message = "Parameter [" + param2->name.toStdString() +
                               "] must NOT be used if parameter [" +
                               param->name.toStdString() + "] is used.";
              throw Isis::IException(Isis::IException::User, message, _FILEINFO_);
            }
          }
        }
      }

      // Check the values for greaterThan clauses
      if(param->values.size() > 0) {
        for(unsigned int item = 0; item < param->greaterThan.size(); item++) {
          const IsisParameterData *param2 = ReturnParam(param->greaterThan[item]);
          if(param2->values.size() != 0) {
            double double1, double2;
            if(param->type == "integer") {
              double1 = (double) GetInteger(param->name);
            }
            else if(param->type == "double") {
              double1 = GetDouble(param->name);
            }
            else {
              std::string msg = "Parameter is not INTEGER or DOUBLE type [" +
                           param->name.toStdString() + "]";
              throw Isis::IException(Isis::IException::Programmer, msg, _FILEINFO_);
            }

            if(param2->type == "integer") {
              double2 = GetInteger(param2->name);
            }
            else if(param2->type == "double") {
              double2 = GetDouble(param2->name);
            }
            else {
              std::string msg = "Parameter is not INTEGER or DOUBLE type [" +
                           param->name.toStdString() + "]";
              throw Isis::IException(Isis::IException::Programmer, msg, _FILEINFO_);
            }

            if(double2 >= double1) {
              std::string message = "Parameter [" + param->name.toStdString() +
                               "] must be greater than parameter [" +
                               param2->name.toStdString() + "].";
              throw Isis::IException(Isis::IException::User, message, _FILEINFO_);
            }
          }
        }
      }

      // Check the values for greaterThanOrEqual clauses
      if(param->values.size() > 0) {
        for(unsigned int item = 0; item < param->greaterThanOrEqual.size(); item++) {
          const IsisParameterData *param2 =
            ReturnParam(param->greaterThanOrEqual[item]);
          if(param2->values.size() != 0) {
            double double1, double2;
            if(param->type == "integer") {
              double1 = (double) GetInteger(param->name);
            }
            else if(param->type == "double") {
              double1 = GetDouble(param->name);
            }
            else {
              std::string msg = "Parameter is not INTEGER or DOUBLE type [" +
                           param->name.toStdString() + "]";
              throw Isis::IException(Isis::IException::Programmer, msg, _FILEINFO_);
            }

            if(param2->type == "integer") {
              double2 = GetInteger(param2->name);
            }
            else if(param2->type == "double") {
              double2 = GetDouble(param2->name);
            }
            else {
              std::string msg = "Parameter is not INTEGER or DOUBLE type [" +
                           param->name.toStdString() + "]";
              throw Isis::IException(Isis::IException::Programmer, msg, _FILEINFO_);
            }

            if(double2 > double1) {
              std::string message = "Parameter [" + param->name.toStdString() +
                               "] must be greater than or equal to parameter [" +
                               param2->name.toStdString() + "].";
              throw Isis::IException(Isis::IException::User, message, _FILEINFO_);
            }
          }
        }
      }
      // Check the values for lessThan clauses
      if(param->values.size() > 0) {
        for(unsigned int item = 0; item < param->lessThan.size(); item++) {
          const IsisParameterData *param2 = ReturnParam(param->lessThan[item]);
          if(param2->values.size() != 0) {
            double double1, double2;
            if(param->type == "integer") {
              double1 = (double) GetInteger(param->name);
            }
            else if(param->type == "double") {
              double1 = GetDouble(param->name);
            }
            else {
              std::string msg = "Parameter is not INTEGER or DOUBLE type [" +
                           param->name.toStdString() + "]";
              throw Isis::IException(Isis::IException::Programmer, msg, _FILEINFO_);
            }

            if(param2->type == "integer") {
              double2 = GetInteger(param2->name);
            }
            else if(param2->type == "double") {
              double2 = GetDouble(param2->name);
            }
            else {
              std::string msg = "Parameter is not INTEGER or DOUBLE type [" +
                           param->name.toStdString() + "]";
              throw Isis::IException(Isis::IException::Programmer, msg, _FILEINFO_);
            }

            if(double2 <= double1) {
              std::string message = "Parameter [" + param->name.toStdString() +
                               "] must be less than parameter [" +
                               param2->name.toStdString() + "].";
              throw Isis::IException(Isis::IException::User, message, _FILEINFO_);
            }
          }
        }
      }

      // Check the values for lessThanOrEqual clauses
      if(param->values.size() > 0) {
        for(unsigned int item = 0; item < param->lessThanOrEqual.size(); item++) {
          const IsisParameterData *param2 =
            ReturnParam(param->lessThanOrEqual[item]);
          if(param2->values.size() != 0) {
            double double1, double2;
            if(param->type == "integer") {
              double1 = (double) GetInteger(param->name);
            }
            else if(param->type == "double") {
              double1 = GetDouble(param->name);
            }
            else {
              std::string msg = "Parameter is not INTEGER or DOUBLE type [" +
                           param->name.toStdString() + "]";
              throw Isis::IException(Isis::IException::Programmer, msg, _FILEINFO_);
            }

            if(param2->type == "integer") {
              double2 = GetInteger(param2->name);
            }
            else if(param2->type == "double") {
              double2 = GetDouble(param2->name);
            }
            else {
              std::string msg = "Parameter is not INTEGER or DOUBLE type [" +
                           param->name.toStdString() + "]";
              throw Isis::IException(Isis::IException::Programmer, msg, _FILEINFO_);
            }

            if(double2 < double1) {
              std::string message = "Parameter [" + param->name.toStdString() +
                               "] must be less than or equal to parameter [" +
                               param2->name.toStdString() + "].";
              throw Isis::IException(Isis::IException::User, message, _FILEINFO_);
            }
          }
        }
      }

      // Check the values for notEqual clauses
      if(param->values.size() > 0) {
        for(unsigned int item = 0; item < param->notEqual.size(); item++) {
          const IsisParameterData *param2 = ReturnParam(param->notEqual[item]);
          if(param2->values.size() != 0) {
            double double1, double2;
            if(param->type == "integer") {
              double1 = (double) GetInteger(param->name);
            }
            else if(param->type == "double") {
              double1 = GetDouble(param->name);
            }
            else {
              std::string msg = "Parameter is not INTEGER or DOUBLE type [" +
                           param->name.toStdString() + "]";
              throw Isis::IException(Isis::IException::Programmer, msg, _FILEINFO_);
            }

            if(param2->type == "integer") {
              double2 = GetInteger(param2->name);
            }
            else if(param2->type == "double") {
              double2 = GetDouble(param2->name);
            }
            else {
              std::string msg = "Parameter is not INTEGER or DOUBLE type [" +
                           param->name.toStdString() + "]";
              throw Isis::IException(Isis::IException::Programmer, msg, _FILEINFO_);
            }

            if(double2 == double1) {
              std::string message = "Parameter [" + param->name.toStdString() +
                               "] must NOT be equal to parameter [" +
                               param2->name.toStdString() + "].";
              throw Isis::IException(Isis::IException::User, message, _FILEINFO_);
            }
          }
        }
      }

      // If this parameter has a value, and a list/option/exclude, make sure
      // the excluded parameter has NO value
      if(((param->values.size() > 0) || (param->defaultValues.size())) > 0) {
        for(unsigned int o2 = 0; o2 < param->listOptions.size(); o2++) {
          QString value, option;
          if(param->type == "string"  || param->type == "combo") {
            value = GetString(param->name);
            value = value.toUpper();
            option = param->listOptions[o2].value;
            option = option.toUpper();
          }
          else if(param->type == "integer") {
            value = GetAsString(param->name);
            value = value.trimmed();
            option = param->listOptions[o2].value;
            option = option.trimmed();
          }
          if(value == option) {
            for(unsigned int e2 = 0; e2 < param->listOptions[o2].exclude.size(); e2++) {
              const IsisParameterData *param2 =
                ReturnParam(param->listOptions[o2].exclude[e2]);
              if(param2->values.size() > 0) {
                std::string message = "Parameter [" + param2->name.toStdString() +
                                 "] can not be entered if parameter [" +
                                 param->name.toStdString() + "] is equal to [" +
                                 value.toStdString() + "]";
                throw Isis::IException(Isis::IException::User, message, _FILEINFO_);
              }
            }
          }
        }
      }

      // If this parameter has a value, and a list/option/include, make sure
      // the included parameter has a value
      if(((param->values.size() > 0) || (param->defaultValues.size())) > 0) {
        for(unsigned int o2 = 0; o2 < param->listOptions.size(); o2++) {
          QString value, option;
          if(param->type == "string"  || param->type == "combo") {
            value = GetString(param->name);
            value = value.toUpper();
            option = param->listOptions[o2].value;
            option = option.toUpper();
          }
          else if(param->type == "integer") {
            value = GetAsString(param->name);
            value = value.trimmed();
            option = param->listOptions[o2].value;
            option = option.trimmed();
          }
          if(value == option) {
            for(unsigned int e2 = 0; e2 < param->listOptions[o2].include.size(); e2++) {
              const IsisParameterData *param2 =
                ReturnParam(param->listOptions[o2].include[e2]);
              if((param2->values.size() == 0) &&
                  (param2->defaultValues.size() == 0)) {
                std::string message = "Parameter [" + param2->name.toStdString() +
                                 "] must be entered if parameter [" +
                                 param->name.toStdString() + "] is equal to [" +
                                 value.toStdString() + "]";
                throw Isis::IException(Isis::IException::User, message, _FILEINFO_);
              }
            }
          }
        }
      }

      // If this parameter has no value, default value or internal default
      // value then it must be excluded by some other parameter with an
      // exclude or list/option/exclude
      //   OR
      // it must include a boolean which is false
      if((param->values.size() == 0) && (param->defaultValues.size() == 0) &&
          (param->internalDefault.size() == 0)) {
        bool excluded = false;
        // See if another parameter has a list option excluding this parameter
        for(unsigned int g2 = 0; g2 < groups.size(); g2++) {
          for(unsigned int p2 = 0; p2 < groups[g2].parameters.size(); p2++) {
            for(unsigned int o2 = 0;
                o2 < groups[g2].parameters[p2].listOptions.size(); o2++) {
              for(unsigned int e2 = 0;
                  e2 < groups[g2].parameters[p2].listOptions[o2].exclude.size();
                  e2++) {
                QString excl =
                  this->groups[g2].parameters[p2].listOptions[o2].exclude[e2];
                if(excl == param->name) {
                  excluded = true;
                  break;
                }
              }
            }

            if(groups[g2].parameters[p2].type == "boolean") {
              const IsisParameterData *param2 = &groups[g2].parameters[p2];
              if(((param2->values.size() > 0) && !StringToBool(param2->values[0])) ||
                  ((param2->values.size() == 0) && (param2->defaultValues.size() > 0) &&
                   !StringToBool(param2->defaultValues[0]))) {
                for(unsigned int e2 = 0; e2 < groups[g2].parameters[p2].include.size();
                    e2++) {
                  QString incl =
                    this->groups[g2].parameters[p2].include[e2];
                  if(incl == param->name) {
                    excluded = true;
                  }
                }
              }
              else if(((param2->values.size() > 0) && StringToBool(param2->values[0])) ||
                      ((param2->values.size() == 0) && (param2->defaultValues.size() > 0) &&
                       StringToBool(param2->defaultValues[0]))) {
                for(unsigned int e2 = 0; e2 < groups[g2].parameters[p2].exclude.size();
                    e2++) {
                  QString excl =
                    this->groups[g2].parameters[p2].exclude[e2];
                  if(excl == param->name) {
                    excluded = true;
                  }
                }
              }
            }
          }
        }

        // See if this parameter excludes any other (this implies the other
        // one also excludes this one too
        for(unsigned int item = 0; item < param->exclude.size(); item++) {
          const IsisParameterData *param2 = ReturnParam(param->exclude[item]);
          if((param2->values.size() != 0) ||
              (param2->defaultValues.size() != 0) ||
              (param2->internalDefault.size() != 0)) {
            if(param2->type != "boolean") {
              excluded = true;
            }
            else {
              if(((param2->values.size() > 0) && !StringToBool(param2->values[0])) ||
                  ((param2->values.size() == 0) && (param2->defaultValues.size() > 0) &&
                   !StringToBool(param2->defaultValues[0]))) {
                excluded = true;
              }
            }
          }
        }

        // See if this parameter includes a boolean that is false
        // then it doesn't need a value
        for(unsigned int item = 0; item < param->include.size(); item++) {
          const IsisParameterData *param2 = ReturnParam(param->include[item]);
          if(param2->type == "boolean") {
            if(((param2->values.size() > 0) && !StringToBool(param2->values[0])) ||
                ((param2->values.size() == 0) && (param2->defaultValues.size() > 0) &&
                 !StringToBool(param2->defaultValues[0]))) {
              excluded = true;
            }
          }
        }

        if(!excluded) {
          std::string message = "Parameter [" + param->name.toStdString() + "] must be entered.";
          throw Isis::IException(Isis::IException::User, message, _FILEINFO_);
        }
      }
    }
  }
}


/**
 * Returns a boolean value based on the QString contents
 *
 * @param value The value to convert to a boolean
 *
 * @return boolean value based on QString contents
 *
 * @throws iException::Programmer (Invalid boolean value)
 */
bool IsisAml::StringToBool(QString value) const {

  value = value.toUpper();
  if(value == "") {
    return false;
  }
  else if(!value.compare("NO")) {
    return false;
  }
  else if(!value.compare("FALSE")) {
    return false;
  }
  else if(!value.compare("F")) {
    return false;
  }
  else if(!value.compare("N")) {
    return false;
  }
  else if(!value.compare("YES")) {
    return true;
  }
  else if(!value.compare("TRUE")) {
    return true;
  }
  else if(!value.compare("Y")) {
    return true;
  }
  else if(!value.compare("T")) {
    return true;
  }
  else {
    std::string message = "Invalid boolean value [" + value.toStdString() + "].";
    throw Isis::IException(Isis::IException::Programmer, message, _FILEINFO_);
  }
  return false;
}


/**
 * Creates a QString which could be used as a command line
 *
 * @param cont Pvl to put command line information in
 */
void IsisAml::CommandLine(Isis::Pvl &cont) const {
  Isis::PvlGroup group("UserParameters");

  // Add appropriate keywords
  for(unsigned int g = 0; g < groups.size(); g++) {
    for(unsigned int p = 0; p < groups[g].parameters.size(); p++) {
      const IsisParameterData *param = ReturnParam(ParamName(g, p));
      // If this param has a value add it to the command line
      if(param->values.size() > 0) {
        Isis::PvlKeyword paramKeyword(param->name.toStdString());

        for(unsigned int value = 0; value < param->values.size(); value++) {
          paramKeyword.addValue(param->values[value].toStdString());
        }

        group += paramKeyword;
      }

      // Or if it has a default value add it to the command line
      else if(param->defaultValues.size() > 0) {
        Isis::PvlKeyword paramKeyword(param->name.toStdString());

        for(unsigned int value = 0;
           value < param->defaultValues.size();
           value++) {
          paramKeyword.addValue(param->defaultValues[value].toStdString());
        }

        group += paramKeyword;
      }
    }
  }
  // Remove excluded keywords
  for(unsigned int g = 0; g < groups.size(); g++) {
    for(unsigned int p = 0; p < groups[g].parameters.size(); p++) {
      const IsisParameterData *param = ReturnParam(ParamName(g, p));

      if(((param->values.size() > 0) || (param->defaultValues.size())) > 0) {
        for(unsigned int o2 = 0; o2 < param->listOptions.size(); o2++) {
          Isis::IString value, option;
          if(param->type == "string"  || param->type == "combo") {
            value = GetAsString(param->name).toStdString();
            value = value.UpCase();
            option = param->listOptions[o2].value.toStdString();
            option = option.UpCase();
          }
          else if(param->type == "integer") {
            value = GetAsString(param->name).toStdString();
            value = value.Trim("\n\r\t\f\v\b");
            option = param->listOptions[o2].value.toStdString();
            option = option.Trim("\n\r\t\f\v\b");
          }
          if(value == option) {
            for(unsigned int e2 = 0; e2 < param->listOptions[o2].exclude.size(); e2++) {
              const IsisParameterData *param2 =
                ReturnParam(param->listOptions[o2].exclude[e2]);
              if(group.hasKeyword(param2->name.toStdString())) {
                group.deleteKeyword(param2->name.toStdString());
              }
            }
          }
        }
      }
    }
  }

  cont.clear();
  cont.addGroup(group);
  return;
}


/**
 * Returns the application version date
 *
 * @return The application version date
 */
QString IsisAml::Version() const {
  QString st = "000-00-00";
  for(unsigned int i = 0; i < changes.size(); i++) {
    if(changes[i].date > st) st = changes[i].date;
  }
  return st;
}


/**
 * Starts parsing an application xml file
 *
 * @param xmlfile The xml file to parse
 *
 * @throws iException::Programmer (Error during XML parser initialization)
 */
void IsisAml::StartParser(const char *xmlfile) {

  // Initialize the XML system
  try {
    XERCES::XMLPlatformUtils::Initialize();
  }

  catch(const XERCES::XMLException &toCatch) {

    std::string message = "Error during XML parser initialization" +
                     (std::string)XERCES::XMLString::transcode(toCatch.getMessage());
    throw Isis::IException(Isis::IException::Programmer, message, _FILEINFO_);
    return;
  }

  //
  //  Create a SAX parser object. Then, set it to validate for an IsisAml file
  //
  parser = XERCES::XMLReaderFactory::createXMLReader();

//  SAX2XMLReader::ValSchemes valScheme = SAX2XMLReader::Val_Auto;
  XERCES::SAX2XMLReader::ValSchemes valScheme = XERCES::SAX2XMLReader::Val_Never;
  if(valScheme == XERCES::SAX2XMLReader::Val_Auto) {
    parser->setFeature(XERCES::XMLString::transcode("http://xml.org/sax/features/validation"), true);
    parser->setFeature(XERCES::XMLString::transcode("http://apache.org/xml/features/validation/dynamic"), true);
  }
  else if(valScheme == XERCES::SAX2XMLReader::Val_Never) {
    parser->setFeature(XERCES::XMLString::transcode("http://xml.org/sax/features/validation"), false);
  }

  else if(valScheme == XERCES::SAX2XMLReader::Val_Always) {
    parser->setFeature(XERCES::XMLString::transcode("http://xml.org/sax/features/validation"), true);
    parser->setFeature(XERCES::XMLString::transcode("http://apache.org/xml/features/validation/dynamic"), false);
  }

//  bool doSchema = true;
  bool doSchema = false;
  parser->setFeature(XERCES::XMLString::transcode("http://apache.org/xml/features/validation/schema"), doSchema);

  bool schemaFullChecking = false;
  parser->setFeature(XERCES::XMLString::transcode("http://apache.org/xml/features/validation/schema-full-checking"), schemaFullChecking);

  //  Create the handler object for an application
  //  Then parse the file
  char *encodingName = const_cast<char *>("LATIN1");
  bool expandNamespaces = false ;

  try {
    IsisAmlData *mydata = this;
    appHandler = new IsisXMLApplication(encodingName, expandNamespaces, parser, mydata);
    parser->parse(xmlfile);
  }
  catch (const XERCES::XMLException &toCatch) {
    std::string message = "Error in application XML file: " + 
                    (std::string)XERCES::XMLString::transcode(toCatch.getMessage());
    throw Isis::IException(Isis::IException::Programmer, message, _FILEINFO_);
    XERCES::XMLPlatformUtils::Terminate();
    return;
  }
  // This Exception is thrown whenever an error is encountered while parsing an XML file.
  // Stacked error messages are generated containing the file path and additional data about where the error occurred.
  catch (Isis::IException &e) {
      QString filePath = (QString) xmlfile;
      std::string previousErrorMessage = e.toString();
      std::string additionalErrorMessage = "Error while parsing application XML file [" + filePath.toStdString() + "]";

      throw Isis::IException(Isis::IException::Programmer, additionalErrorMessage + "\n" + previousErrorMessage, _FILEINFO_);
      XERCES::XMLPlatformUtils::Terminate();
      return;
  }

  //  Delete the parser
  delete parser;
  XERCES::XMLPlatformUtils::Terminate();
  delete appHandler;
  return;
}
