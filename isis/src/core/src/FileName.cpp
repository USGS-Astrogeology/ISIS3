/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "FileName.h"

#include <cmath>

#include <QDate>
#include <QDebug>
#include <QDir>
#include <QLocale>
#include <QPair>
#include <QString>
#include <QTemporaryFile>

#include "Preference.h"
#include "IException.h"
#include "IString.h"

using namespace std;

namespace Isis {

  /**
   * Constructs an empty FileName object.
   */
  FileName::FileName() {
    m_d = new Data;
  }

  /**
   * Constructs a FileName object using a char pointer as a file name.
   *
   * @param file char pointer representing new filename
   */
  FileName::FileName(const char *file) {
    m_d = new Data;
    m_d->setOriginal(file);
  }

  /**
   * Constructs a FileName object using a QString as a file name.
   *
   * @param file Qstring representing new filename
   */
  FileName::FileName(const QString &file) {
    m_d = new Data;
    m_d->setOriginal(file);
  }

  /**
   * Constructs a copy of a FileName object using another FileName object.
   *
   * @param &other FileName object to copy.
   */
  FileName::FileName(const FileName &other) : m_d(other.m_d) {
  }

  /**
   * Destroys the FileName object.
   */
  FileName::~FileName() {
  }

  /**
   * Returns the path of the original file name. For *nix operating
   * systems this includes everything up to but not including the
   * last slash "/". For filenames created without any slashes
   * the current working directory will be returned.
   *
   * <pre>
   *   for a full file specification of:
   *    "/home/me/img/picture.jpg"
   *   originalPath() gives:
   *    "/home/me/img"
   * </pre>
   *
   * @return QString of the path portion of the original filename.
   */
  QString FileName::originalPath() const {
    return QFileInfo(m_d->original(false)).path();
  }

  /**
   * Returns the path of the file name. For *nix operating
   * systems this includes everything up to but not including the
   * last slash "/". For filenames created without any slashes
   * the current working directory will be returned.
   *
   * <pre>
   *   for a full file specification of:
   *    "/home/me/img/picture.jpg"
   *   path() gives:
   *    "/home/me/img"
   * </pre>
   *
   * @return QString of the path portion of the filename.
   */
  QString FileName::path() const {
    return QFileInfo(expanded()).path();
  }

  /**
   * Returns a QString of the attributes in a filename, attributes are expected to be of type
   * CubeAttributeInput or CubeAttributeOutput. Filenames without any attributes return an
   * empty QString.
   *
   * <pre>
   *   for a full file specification of:
   *    "/tmp/Peaks.cub+Bsq"
   *   attributes() gives:
   *    "Bsq"
   * </pre>
   *
   * @return QString of the attributes specified in the filename.
   */
  QString FileName::attributes() const {
    QString result;
    QString fileNameWithAttribs = QFileInfo(m_d->original(true)).fileName();

    int attribStartPos = fileNameWithAttribs.indexOf("+");

    if (attribStartPos != -1)
      result = fileNameWithAttribs.mid(attribStartPos + 1);

    return result;
  }

  /**
   * Returns the name of the file without the path and without extensions.
   *
   * <pre>
   *   for a full file specification of:
   *    "/tmp/Peaks.cub.gz"
   *   baseName() gives:
   *    "Peaks"
   * </pre>
   *
   * @return QString containing every character excluding the path and all extensions.
   */
  QString FileName::baseName() const {
    return QFileInfo(m_d->original(false)).completeBaseName();
  }

  /**
   * Returns the name of the file excluding the path and the attributes in the file name.
   *
   * <pre>
   *   for a full file specification of:
   *    "/tmp/Peaks.cub+Bsq"
   *   name() gives:
   *    "Peaks.cub"
   * </pre>
   *
   * @return QString containing every character in the file name exluding the path and attributes
   * of the file.
   */
  QString FileName::name() const {
    return QFileInfo(m_d->original(false)).fileName();
  }

  /**
   * Returns the last extension of the file name.
   *
   * <pre>
   *   for a full file specification of:
   *    "/tmp/Peaks.cub.gz"
   *   extension() gives:
   *    "gz"
   * </pre>
   *
   * @return QString containing every character in the file name after the last "." character.
   */
  QString FileName::extension() const {
    return QFileInfo(m_d->original(false)).suffix();
  }

  /**
   * Returns a QString of the full file name including the file path, excluding the attributes.
   * Any Isis Preferences or environment variables indicated by $, are changed to what they
   * represent.
   *
   * <pre>
   *   for a full file specification of:
   *    "$ISISROOT/tmp/Peaks.cub+Bsq"
   *   expanded() gives:
   *    "/usgs/pkgs/isis3/isis/tmp/Peaks.cub"
   * </pre>
   *
   * @return QString
   */
  QString FileName::expanded() const {
    return m_d->expanded(false);
  }

  /**
   * Returns the full file name including the file path
   *
   * <pre>
   *   for a full file specification of:
   *    "$ISISROOT/tmp/Peaks.cub+Bsq"
   *   original() gives:
   *    "$ISISROOT/tmp/Peaks.cub+Bsq"
   * </pre>
   *
   * @return QString containing every character in the file name and the path
   */
  QString FileName::original() const {
    return m_d->original(true);
  }

  /**
   * Adds a new extension to the file name. If the current extension is the same as the
   * new extension it will return an unchanged FileName object.
   *
   * @param newExtension The new file extension to be added at the end of the file name after all
   * exisiting extensions.
   *
   * @return FileName object with added extension
   */
  FileName FileName::addExtension(const QString &newExtension) const {
    FileName result = *this;

    if (result.extension() != newExtension) {
      QString attributesStr = result.attributes();

      if (attributesStr == "")
        result = FileName(result.originalPath() + "/" + result.name() + "." + newExtension);
      else
        result = FileName(result.originalPath() + "/" + result.name() + "." + newExtension
                          + "+" + attributesStr);
    }

    return result;
  }

  /**
   * Removes all extensions in the file name
   *
   * @return FileName object with all extensions removed
   */
  FileName FileName::removeExtension() const {
    QString attributesStr = attributes();

    FileName result;
    if (attributesStr == "")
      result = FileName(originalPath() + "/" + baseName());
    else
      result = FileName(originalPath() + "/" + baseName() + "+" + attributesStr);

    return result;
  }

  /**
   * Sets all current file extensions to a new extension in the file name.
   *
   * @param newExtension The new file extension to replace any current file extensions with
   *
   * @return FileName object with all existing extensions replaced by the new extension
   */
  FileName FileName::setExtension(const QString &newExtension) const {
    FileName result = *this;

    if (extension() != newExtension) {
      result = result.removeExtension().addExtension(newExtension);
    }

    return result;
  }

  /**
   * Checks to see if a file name is versioned by date or numerically. Returns true if file is
   * versioned by date or numerically; returns false otherwise.
   *
   * @return Boolean
   */
  bool FileName::isVersioned() const {
    validateVersioningState();

    return isNumericallyVersioned() || isDateVersioned();
  }

  /**
   * Checks if the file name is versioned numerically. Returns true if the file is versioned
   * numerically; returns false otherwise.
   *
   * @return Boolean
   */
  bool FileName::isNumericallyVersioned() const {
    return FileName(expanded()).name().contains("?");
  }

  /**
   * Checks if the file name is versioned by date. Returns true if the file is versioned
   * by date; returns false otherwise.
   *
   * @return Boolean
   */
  bool FileName::isDateVersioned() const {
    return FileName(expanded()).name().contains(QRegExp("\\{.*\\}"));
  }

  /**
   * Searches the directory specified in the file name for the highest version of the file name.
   * Returns a FileName object with the file name changed to reflect the highest version.
   *
   * @return FileName object
   */
  FileName FileName::highestVersion() const {
    validateVersioningState();

    FileName result = *this;

    if (!isVersioned()) {
      throw IException(IException::Unknown,
                       QObject::tr("Asked for highest version of file named [%1] in [%2] but there "
                                   "are no version sequences in the name")
                         .arg(name()).arg(originalPath()),
                       _FILEINFO_);
    }

    // Look for dates
    if (isDateVersioned()) {
      result = result.version(result.highestVersionDate());
    }

    // Look for number's
    if (isNumericallyVersioned()) {
      result = result.version(result.highestVersionNum());
    }

    return result;
  }

  /**
   * Updates the file name to be the latest version. If the file is versioned by date the
   * newest version will be the current date. If the file is versioned numerically, the newest
   * version will be the current version plus one.
   *
   * @return FileName object with the new version file name.
   *
   * @throws Isis::IException::Unknown
   */
  FileName FileName::newVersion() const {
    validateVersioningState();

    FileName result = *this;

    if (!isVersioned()) {
      throw IException(IException::Unknown,
                       QObject::tr("Asked for new version of file named [%1] in [%2] but there "
                                   "are no version sequences in the name")
                         .arg(name()).arg(originalPath()),
                       _FILEINFO_);
    }

    // Look for date
    if (isDateVersioned()) {
      result = result.version(QDate::currentDate());
    }

    // Look for #'s
    if (isNumericallyVersioned()) {
      try {
        result = result.version(result.highestVersionNum() + 1);
      }
      catch (IException &) {
        result = result.version(1);
      }
    }

    if (result.fileExists()) {
      throw IException(IException::Unknown,
                       QObject::tr("Could not generate unique new version of file named [%1] in "
                                   "[%2] because the file [%3] exists")
                         .arg(name()).arg(originalPath()).arg(result.name()),
                       _FILEINFO_);

    }

    return result;
  }

  /**
   * Returns a FileName object of the same file name but versioned numerically by the
   * number passed in as a parameter.
   *
   * @param versionNumber number to version the new FileName object
   *
   * @return FileName object with the new version file name.
   *
   * @throws Isis::IException::Unknown
   */
  FileName FileName::version(long versionNumber) const {
    QString file = FileName(expanded()).name();

    int width = file.count("?");

    if (versionNumber < 0) {
      throw IException(IException::Unknown,
          QObject::tr("FileName does not support negative version numbers in the file name, "
                      "tried to get version [%1] in file named [%2]")
            .arg(versionNumber).arg(originalPath() + "/" + file),
          _FILEINFO_);
    }

    if (versionNumber >= pow(10.0, width)) {
      throw IException(IException::Unknown,
          QObject::tr("FileName does not support version numbers greater than what would fit in "
                      "the file name, tried to get version [%1] in file named [%2]")
            .arg(versionNumber).arg(originalPath() + "/" + file),
          _FILEINFO_);
    }

    QPair<QString, QString> splitName = splitNameAroundVersionNum();
    QString &before = splitName.first;
    QString &after = splitName.second;

    file = before + QString("%1").arg(QString::number(versionNumber), width, '0') + after;

    return FileName(originalPath() + "/" + file);
  }

  /**
   * Returns a FileName object of the same file name but versioned by the
   * date passed in as a parameter.
   *
   * @param versionDate QDate to version the new FileName object
   *
   * @return FileName object with the new version file name.
   *
   */
  FileName FileName::version(QDate versionDate) const {
    QString newName = versionDate.toString(fileNameQDatePattern());

    return FileName(originalPath() + "/" + newName);
  }

  /**
   * Returns true if the file exists; false otherwise.
   * If the file is a symlink that points to a nonexistent file, false is returned.
   *
   * @return Boolean
   */
  bool FileName::fileExists() const {
    return QFileInfo(expanded()).exists();
  }

  /**
   * Returns the path of the file's parent directory as a QDir object
   *
   * <pre>
   *   for a full file specification of:
   *    "/tmp/Peaks.cub+Bsq"
   *   dir() gives:
   *    "/tmp/"
   * </pre>
   *
   * @return QDir
   */
  QDir FileName::dir() const {
    return QFileInfo(expanded()).dir();
  }

  /**
   * Creates a temporary file and returns a FileName object created using the temporary file.
   *
   * @param templateFileName the file name used to create the temporary file.
   *
   * @return FileName object created using the temporary file
   *
   * @throws Isis::IException::Io
   */
  FileName FileName::createTempFile(FileName templateFileName) {
    QString preppedFileName = QString("%1/%2XXXXXX.%3").arg(templateFileName.path())
        .arg(templateFileName.baseName()).arg(templateFileName.extension());
    QTemporaryFile tempFile(preppedFileName);
    tempFile.setAutoRemove(false);

    if (!tempFile.open()) {
      throw IException(IException::Io,
          QObject::tr("Could not create a unique temporary file name based on [%1]")
            .arg(templateFileName.original()),
          _FILEINFO_);
    }

    // We want to set the 'original' path as correctly as possible. So let's use the input original
    //   path with the output temp file's file name in our result.
    FileName result;
    QString newTempFileNameStr = templateFileName.originalPath() + "/" +
        QFileInfo(tempFile.fileName()).fileName();
    result = FileName(newTempFileNameStr);

    return result;
  }

  /**
   * Returns a QString of the full file name including the file path, excluding the attributes
   * with any Isis Preferences or environment variables indicated by $, changed to what they
   * represent.
   *
   * <pre>
   *   for a full file specification of:
   *    "$ISISROOT/tmp/Peaks.cub+Bsq"
   *   toString() gives:
   *    "/usgs/pkgs/isis3/isis/tmp/Peaks.cub"
   * </pre>
   *
   * @return QString
   */
  QString FileName::toString() const {
    return expanded();
  }

  /**
   * Clears the current contents of the FileName object and reinitializes it with
   * the argument.
   *
   * @param rhs FileName to replace the current contents of the object.
   *
   * @return void
   */
  FileName &FileName::operator=(const FileName &rhs) {
    m_d = rhs.m_d;
    return *this;
  }

  /**
   * Compares equality of two FileName objects. Returns true if the two objects are equal
   * and false otherwise.
   *
   * @param rhs FileName to compare the current FileName object to.
   *
   * @return Boolean
   */
  bool FileName::operator==(const FileName &rhs) {
    QString expandedOfThis = expanded();
    QString canonicalOfThis = QFileInfo(expandedOfThis).canonicalFilePath();

    QString expandedOfRhs = rhs.expanded();
    QString canonicalOfRhs = QFileInfo(expandedOfRhs).canonicalFilePath();

    // Cononical file paths return empty strings if the file does not exist. Either both canonicals
    //   are valid and the same (equal is initialized to true), or neither canonical is valid but
    //   the expandeds are the same (equal is set to true when it isn't initialized to true).
    bool equal = (!canonicalOfThis.isEmpty() && canonicalOfThis == canonicalOfRhs);

    if (!equal) {
      equal = (canonicalOfThis.isEmpty() && canonicalOfRhs.isEmpty() &&
               expandedOfThis == expandedOfRhs);
    }

    return equal;
  }

  /**
   * Compares equality of two FileName objects. Returns false if the two objects are equal
   * and true otherwise.
   *
   * @param rhs FileName to compare the current FileName object to.
   *
   * @return Boolean
   */
  bool FileName::operator!=(const FileName &rhs) {
    return !(*this == rhs);
  }

  /**
   * This looks through the directory of the file and checks for the highest version date of
   * the file that is versioned date.
   *
   * @return QDate
   */
  QDate FileName::highestVersionDate() const {
    QString fileQDatePattern = fileNameQDatePattern();

    QPair<int, int> truncateRange(-1, -1);
    if (fileQDatePattern.contains("?")) {
      QString trueLengthName = name().replace(QRegExp("[{}]"), "");
      truncateRange.first = trueLengthName.indexOf("?");
      truncateRange.second = trueLengthName.lastIndexOf("?");
      fileQDatePattern = fileQDatePattern.replace("?", "");
    }

    QString file = name();

    QDate result;
    QDate sputnikLaunch(1957, 10, 4);

    QString before = file.mid(0, file.indexOf("{"));
    QString after = file.mid(file.lastIndexOf("}") + 1);

    QStringList nameFilters;

    nameFilters.append(before + "*" + after);
    QStringList files = dir().entryList(nameFilters);

    // We can't sort the files to get our answer, so we need to go through every possible file.
    foreach (QString foundFile, files) {
      // Toss any numerical versioning sequence
      if (truncateRange.first >= 0 && truncateRange.second > truncateRange.first) {
        foundFile = foundFile.mid(0, truncateRange.first) +
                    foundFile.mid(truncateRange.second + 1);

      }
      QDate fileDate = QLocale(QLocale::English, QLocale::UnitedStates).toDate(foundFile, fileQDatePattern);

      if (fileDate.isValid()) {
        // No missions before Sputnik 1, so we must be in the new millenium
        if (fileDate < sputnikLaunch)
          fileDate = fileDate.addYears(100);

        if (!result.isValid() || fileDate > result) {
          result = fileDate;
        }
      }
    }

    if (!result.isValid()) {
      throw IException(IException::Unknown,
                       QObject::tr("No existing files found with a date version matching [%1] in "
                                   "[%2]")
                         .arg(FileName(expanded()).name()).arg(path()),
                       _FILEINFO_);
    }

    return result;
  }

  /**
   * This looks through the directory of the file and checks for the highest version number of
   * the file that is versioned numerically.
   *
   * @return long
   */
  long FileName::highestVersionNum() const {
    QString file = FileName(expanded()).name();
    int result = 0;

    int width = file.count("?");

    QPair<QString, QString> splitName = splitNameAroundVersionNum();
    QString &before = splitName.first;
    QString &after = splitName.second;

    QStringList nameFilters;
    nameFilters.append(before + QString("%1").arg("", width, '?') + after);
    QStringList files = dir().entryList(nameFilters, QDir::NoFilter, QDir::Name);

    long foundValue = -1;
    bool success = false;

    for (int i = files.count() - 1; !success && i >= 0; i--) {
      foundValue = files[i].mid(before.count(), width).toLong(&success);
    }

    if (success) {
      result = foundValue;
    }
    else {
      throw IException(IException::Unknown,
                       QObject::tr("No existing files found with a numerical version matching [%1] "
                                   "in [%2]")
                         .arg(FileName(expanded()).name()).arg(path()),
                       _FILEINFO_);
    }

    return result;
  }

  /**
   * This verifies the class invariant when using versioning - that the FileName is in an acceptable
   * state to find file version numbers.
   */
  void FileName::validateVersioningState() const {
    QString file = QFileInfo(expanded()).fileName();

    if (file.contains(QRegExp("\\?\\?*[^?][^?]*\\?"))) {
      throw IException(IException::Unknown,
          QObject::tr("Only one numerical version sequence is allowed in a filename; "
                      "there are multiple in [%1]").arg(file),
          _FILEINFO_);
    }

    if (isDateVersioned()) {
      QString fileDatePattern = FileName(expanded()).name();

      // {} needs to be removed from the fileDatePattern (i.e. an empty date version sequence).
      // This prevents the replacement of {} with '' in the pattern, since
      // Qt5's QDate.toString(pattern) handles these two adjacent single quotes in the pattern
      // differently than Qt4 did.
      fileDatePattern.replace(QRegExp("\\{\\}"), "");

      fileDatePattern = "'" + fileDatePattern.replace(QRegExp("[{}]"), "'") + "'";

      QString dated = QDate::currentDate().toString(fileDatePattern);
      if (file.contains("'")) {
        throw IException(IException::Unknown,
            QObject::tr("Date version sequenced file names cannot have single quotes in them; "
                        "the file named [%1] is not usable").arg(file),
            _FILEINFO_);
      }
      else if (dated == "") {
        throw IException(IException::Unknown,
            QObject::tr("The date version sequence is not usable in the file named [%1]").arg(file),
            _FILEINFO_);
      }
      else if (dated == fileDatePattern.replace(QRegExp("'"), "")) {
        throw IException(IException::Unknown,
            QObject::tr("The date version sequences are not recognized in the file named [%1]")
              .arg(file),
            _FILEINFO_);
      }
    }
  }

  /**
   * This changes the files format. Specifically quotes everything not in {} with single quotes
   * and removes the {} from the file name.
   *
   * @return QString
   */
  QString FileName::fileNameQDatePattern() const {
    // We need to quote everything not in {} with single quotes.
    QString file = FileName(expanded()).name();

    // Current Text: {VAR}XXX{VAR}XXX{VAR} or XXX{VAR}XXX{VAR} or XXX{VAR}XXX or {VAR}XXX
    file = file.replace(QRegExp("[{}]"), "'");

    // Current Text: 'VAR'XXX'VAR'XXX'VAR' or XXX'VAR'XXX'VAR' or XXX'VAR'XXX or 'VAR'XXX
    if (file.startsWith("'"))
      file = file.mid(1);
    else
      file = "'" + file;

    // Current Text: VAR'XXX'VAR'XXX'VAR' or 'XXX'VAR'XXX'VAR' or 'XXX'VAR'XXX or VAR'XXX
    if (file.endsWith("'"))
      file = file.mid(0, file.length() - 1);
    else
      file = file + "'";

    // Current Text: VAR'XXX'VAR'XXX'VAR or 'XXX'VAR'XXX'VAR or 'XXX'VAR'XXX'  or VAR'XXX' -- VAR's
    //   are the only text not quoted; this is success.
    return file;
  }

  /**
   * This returns a QPair of the text (before, after) a version number in a file. Before being
   * the text before the version number and after being the text after the version number.
   *
   * @return QPair
   */
  QPair<QString, QString> FileName::splitNameAroundVersionNum() const {
    QString file = FileName(expanded()).name();
    QString before;
    QString after;

    if (!isNumericallyVersioned()) {
      before = file;
    }
    else {
      before = file.mid(0, file.indexOf("?"));
      after = file.mid(file.lastIndexOf("?") + 1);
    }

    return QPair<QString, QString>(before, after);
  }

   /**
    * Data constructor, creates a new Data object.
    */
  FileName::Data::Data() {
    m_originalFileNameString = NULL;
    m_expandedFileNameString = NULL;

    m_originalFileNameString = new QString;
    m_expandedFileNameString = new QString;
  }

  /**
   * Data copy constructor, creates a copy of a Data object.
   *
   * @param &other Data object to copy
   */
  FileName::Data::Data(const Data &other) : QSharedData(other) {
    m_originalFileNameString = NULL;
    m_expandedFileNameString = NULL;

    m_originalFileNameString = new QString(*other.m_originalFileNameString);
    m_expandedFileNameString = new QString(*other.m_expandedFileNameString);
  }

  /**
   * Destroys the Data object.
   */
  FileName::Data::~Data() {
    delete m_originalFileNameString;
    m_originalFileNameString = NULL;

    delete m_expandedFileNameString;
    m_expandedFileNameString = NULL;
  }

  /**
   * Returns the original file name, stored in m_originalFileNameString. Boolean
   * parameter includeAttributes determines if the returned file name has the variables
   * included.
   *
   * @param includeAttributes boolean to represent whether the attricubtes should be included.
   *
   * @return Qstring
   */
  QString FileName::Data::original(bool includeAttributes) const {
    QString result = *m_originalFileNameString;


    if (!includeAttributes) {
      int attributesPos = result.indexOf("+");

      if (attributesPos != -1)
        result = result.left(attributesPos);
    }

    return result;
  }

  /**
   * Sets the original file name, stored in m_originalFileNameString. QString parameter
   * is the new file name to store in m_originalFileNameString. The expanded verison is also
   * set and stored in m_expandedFileNameString when this method is called.
   *
   * @param originalStr the new file name
   */
  void FileName::Data::setOriginal(const QString &originalStr) {
    *m_originalFileNameString = originalStr;

    // Expand the file name and store that too.
    QString expandedStr = original(true);

    int varSearchStartPos = 0;
    int varStartPos = -1;
    // Loop while there are any "$" at the current position or after
    // Some "$" might be skipped if no translation can be found
    while((varStartPos = expandedStr.indexOf("$", varSearchStartPos)) != -1) {
      int varEndPos = expandedStr.indexOf(QRegExp("[^a-zA-Z{}0-9_]"), varStartPos + 1);
      if (varEndPos == -1)
        varEndPos = expandedStr.length();

      bool variableValid = false;
      int varNameLength = varEndPos - varStartPos;

      if (varNameLength > 0) {
        QString varName = expandedStr.mid(varStartPos + 1, varEndPos - varStartPos - 1);

        if (varName.length()) {
          if (varName[0] =='{' && varName[varName.length() - 1] == '}')
            varName = varName.mid(1, varName.length() - 2);

          QString varValue;

          // Find the corresponding Isis Preference if one exists
          if(Preference::Preferences().hasGroup("DataDirectory")) {
            PvlGroup &dataDir = Preference::Preferences().findGroup("DataDirectory");
            if(dataDir.hasKeyword(varName)) {
              varValue = ((QString)dataDir[varName.toStdString().c_str()][0]);
            }
          }

          // Find the corresponding environment variable if one exists
          if (varValue.isEmpty()) {
            char *val;
            val = getenv(varName.toStdString().c_str());
            if(val != NULL) varValue = val;
          }

          // Replace the $xxxx with the pref/env, but don't move
          // the pointer. We may have replaced one $ for another.
          // Note: May need to put a test for circular replaces in here
          if (!varValue.isEmpty()) {
            expandedStr = expandedStr.replace(varStartPos, varNameLength, varValue);
            variableValid = true;
          }
        }
      }

      if (variableValid) {
        // We could expand multiple times...
        varSearchStartPos = varStartPos;
      }
      else {
        // We failed to understand this variable, move along
        varSearchStartPos = varStartPos + 1;
      }
    }

    *m_expandedFileNameString = expandedStr;
  }

  /**
   * Returns the expanded file name, stored in m_expandedFileNameString. Boolean
   *  parameter includeAttributes determines if the returned file name has the variables
   * included.
   *
   * @param includeAttributes boolean to represent whether the attricubtes should be included.
   *
   * @return Qstring
   */
  QString FileName::Data::expanded(bool includeAttributes) const {
    QString result = *m_expandedFileNameString;

    if (!includeAttributes) {
      int attributesPos = result.indexOf("+");

      if (attributesPos != -1)
        result = result.left(attributesPos);
    }

    return result;
  }
}
