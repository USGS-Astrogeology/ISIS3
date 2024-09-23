/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "FileName.h"

#include <cmath>

#include <QDate>
#include <QDir>
#include <QLocale>
#include "Preference.h"
#include "PvlGroup.h"
#include "IException.h"
#include "IString.h"
#include <regex>
#include <memory>
#include <sstream>

using namespace std;
namespace fs = std::filesystem;

namespace Isis {

  /**
   * Constructs an empty FileName object.
   */
  FileName::FileName() : m_d(std::make_shared<FileData>()) {
    
  }

  /**
   * Constructs a FileName object using a char pointer as a file name.
   *
   * @param file char pointer representing new filename
   */
    FileName::FileName(const char *file) : m_d(std::make_shared<FileData>()) {
    m_d->setOriginal(file);
  }

  /**
   * Constructs a FileName object using a std::string as a file name.
   *
   * @param file Qstring representing new filename
   */
  FileName::FileName(const std::string &file) : m_d(std::make_shared<FileData>()) {
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
   * @return std::string of the path portion of the original filename.
   */
  std::string FileName::originalPath() const {
    std::string ogPath = fs::path(m_d->original(false)).parent_path().string();
    if (ogPath.empty()) {
      return ".";
    }
    return ogPath;
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
   * @return std::string of the path portion of the filename.
   */
  std::string FileName::path() const {
    std::string path = fs::path(expanded()).parent_path().string();
    if (path.empty()) {
      return ".";
    }
    return path;
  }

  /**
   * Returns a std::string of the attributes in a filename, attributes are expected to be of type
   * CubeAttributeInput or CubeAttributeOutput. Filenames without any attributes return an
   * empty std::string.
   *
   * <pre>
   *   for a full file specification of:
   *    "/tmp/Peaks.cub+Bsq"
   *   attributes() gives:
   *    "Bsq"
   * </pre>
   *
   * @return std::string of the attributes specified in the filename.
   */
  std::string FileName::attributes() const {
    std::string result;
    std::string fileNameWithAttribs = fs::path(m_d->original(true)).filename();

    size_t attribStartPos = fileNameWithAttribs.find("+");

    if (attribStartPos != std::string::npos)
      result = fileNameWithAttribs.substr(attribStartPos + 1);

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
   * @return std::string containing every character excluding the path and all extensions.
   */
  std::string FileName::baseName() const {
    std::string stem = fs::path(m_d->original(false)).filename().stem().string();
    if (stem.erase(0, 1) == extension()){
      return "";
    }
    return fs::path(m_d->original(false)).filename().stem().string();
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
   * @return std::string containing every character in the file name exluding the path and attributes
   * of the file.
   */
  std::string FileName::name() const {
    return fs::path(m_d->original(false)).filename();
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
   * @return std::string containing every character in the file name after the last "." character.
   */
  std::string FileName::extension() const {
    std::string ext = fs::path(m_d->original(false)).extension().string();
    if (ext.empty() && m_d->original(false)[0] == '.') {
      return fs::path(m_d->original(false)).filename().string().erase(0, 1);
    }
    return ext.erase(0, 1);
  }

  /**
   * Returns a std::string of the full file name including the file path, excluding the attributes.
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
   * @return std::string
   */
  std::string FileName::expanded() const {
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
   * @return std::string containing every character in the file name and the path
   */
  std::string FileName::original() const {
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
  FileName FileName::addExtension(const std::string &newExtension) const {
    FileName result = *this;

    if (result.extension() != newExtension) {
      std::string attributesStr = result.attributes();

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
    std::string attributesStr = attributes();

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
  FileName FileName::setExtension(const std::string &newExtension) const {
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
    return (FileName(expanded()).name().find("?") != std::string::npos);
  }

  /**
   * Checks if the file name is versioned by date. Returns true if the file is versioned
   * by date; returns false otherwise.
   *
   * @return Boolean
   */
  bool FileName::isDateVersioned() const {
    std::regex datePattern("\\{.*\\}");
    return (std::regex_search(FileName(expanded()).name(), datePattern));
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
      std::string msg = "Asked for highest version of file named [" + name() + 
                              "] in [" + originalPath() + "] but there are no version sequences in the name";
      throw IException(IException::Unknown, msg, _FILEINFO_);
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
      std::string msg = "Asked for new version of file named [" + name() + "] in [" + originalPath() + "] but there are no version sequences in the name";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }

    // Look for date
    if (isDateVersioned()) {
      auto now = std::chrono::system_clock::now();
      std::time_t now_c = std::chrono::system_clock::to_time_t(now);
      std::tm now_tm = *std::localtime(&now_c);

      result = result.version(now_tm);
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
      std::string msg = "Could not generate unique new version of file named [" + name() + "] in [" + originalPath() + "] because the file [" + result.name() + "] exists";
      throw IException(IException::Unknown, msg, _FILEINFO_);
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
    std::string file = FileName(expanded()).name();

    int width = std::count(file.begin(), file.end(), '?');

    if (versionNumber < 0) {
      std::string msg = "FileName does not support negative version numbers in the file name, tried to get version [" + std::to_string(versionNumber) + "] in file named " + originalPath() + "/" + file;
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }

    if (versionNumber >= pow(10.0, width)) {
      std::string msg = "FileName does not support version numbers greater than what would fit in the file name, tried to get version [" + std::to_string(versionNumber) + "] in file named [" + originalPath() + "/" + file + "]";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }

    std::pair<std::string, std::string> splitName = splitNameAroundVersionNum();
    std::string &before = splitName.first;
    std::string &after = splitName.second;

    std::ostringstream formattedVersion;
    formattedVersion << std::setw(width) << std::setfill('0') << versionNumber;

    file = before + formattedVersion.str() + after;

    return FileName(originalPath() + "/" + file);
  }

  /**
   * Returns a FileName object of the same file name but versioned by the
   * date passed in as a parameter.
   *
   * @param versionDate std::tm to version the new FileName object
   *
   * @return FileName object with the new version file name.
   *
   */
  FileName FileName::version(const std::tm& versionDate) const {
      std::ostringstream oss;
      oss << std::put_time(&versionDate, fileNameDatePattern().c_str());

      std::string newName = oss.str();
      newName.erase(std::remove(newName.begin(), newName.end(), ' '), newName.end());

    return FileName(originalPath() + "/" + newName);
  }

  /**
   * Returns true if the file exists; false otherwise.
   * If the file is a symlink that points to a nonexistent file, false is returned.
   *
   * @return Boolean
   */
  bool FileName::fileExists() const {
    return fs::exists(expanded());
  }

  /**
   * Returns the path of the file's parent directory as a std::filesystem::path
   *
   * <pre>
   *   for a full file specification of:
   *    "/tmp/Peaks.cub+Bsq"
   *   dir() gives:
   *    "/tmp/"
   * </pre>
   *
   * @return fs::path
   */
  std::filesystem::path FileName::dir() const {
    fs::path dir = fs::path(expanded()).parent_path();
    if (dir.empty()){
      return ".";
    }
    return dir;
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
    fs::path tempPath = templateFileName.path();
    std::string baseName = templateFileName.baseName();
    std::string extension = templateFileName.extension();
    
    std::string uniqueFileName = baseName + "XXXXXX." + extension;
    fs::path tempFilePath = tempPath / uniqueFileName;
    
    std::ofstream tempFile(tempFilePath.string());
    if (!tempFile) {
      throw IException(IException::Io,
            "Could not create a unique temporary file name based on [" + templateFileName.original() + "]",
          _FILEINFO_);
    }

    FileName result;
    std::string newTempFileNameStr = templateFileName.originalPath() + "/" + uniqueFileName;
    result = FileName(newTempFileNameStr);

    return result;
  }

  /**
   * Returns a std::string of the full file name including the file path, excluding the attributes
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
   * @return std::string
   */
  std::string FileName::toString() const {
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
      std::filesystem::path expandedOfThis = expanded();
      std::filesystem::path expandedOfRhs = rhs.expanded();

      std::filesystem::path canonicalOfThis = std::filesystem::absolute(expandedOfThis);
      std::filesystem::path canonicalOfRhs = std::filesystem::absolute(expandedOfRhs);

      bool equal = !canonicalOfThis.empty() && canonicalOfThis == canonicalOfRhs;

    if (!equal) {
          equal = canonicalOfThis.empty() && canonicalOfRhs.empty() &&
                  expandedOfThis == expandedOfRhs;
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
   * This function iterates over all the files in the specified directory and returns a vector of filenames
   * that match the pattern formed by the `before` and `after` strings. The pattern is constructed as:
   * `before + ".*" + after`, where `.*` matches any sequence of characters.
   *
   * @param directory The directory in which to search for files.
   * @param pattern The pattern string.
   * @return std::vector<std::string> A vector containing the filenames that match the specified pattern.
   */
  std::vector<std::string> FileName::getFilesMatchingFilters(fs::path &directory, std::string &pattern) const {
    std::vector<std::string> files;
    std::regex regex(pattern);

    for (const auto& entry : fs::directory_iterator(directory)) {
        if (fs::is_regular_file(entry.path())) {
            std::string filename = entry.path().filename().string();
            if (std::regex_match(filename, regex)) {
                files.push_back(filename);
            }
        }
    }

    return files;
  }
  /**
   * This function verifies that the individual fields of a `std::tm` structure
   * are within their respective valid ranges. Specifically, it checks the following:
   * - `tm_year` should be non-negative (i.e., year >= 1900).
   * - `tm_mon` should be in the range [0, 11], representing months January through December.
   * - `tm_mday` should be in the range [1, 31], representing days of the month.
   *
   * @param tm The `std::tm` structure representing the date to be validated.
   * @return `true` if the date is valid, `false` otherwise.
   */
  bool FileName::isValidDate(std::tm &tm) const {
    if (tm.tm_year < 0 || tm.tm_mon < 0 || tm.tm_mon > 11 || tm.tm_mday < 1 || tm.tm_mday > 31) {
        return false;
    }
    return true;
}

  /**
   * This looks through the directory of the file and checks for the highest version date of
   * the file that is versioned date.
   *
   * @return std::tm
   */
  std::tm FileName::highestVersionDate() const {
    std::string fileDatePattern = fileNameDatePattern();

    std::pair<int, int> truncateRange(-1, -1);
    if (fileDatePattern.find('?') != std::string::npos) {
      std::string trueLengthName = name();
      trueLengthName.erase(std::remove(trueLengthName.begin(), trueLengthName.end(), '{'), trueLengthName.end());
      trueLengthName.erase(std::remove(trueLengthName.begin(), trueLengthName.end(), '}'), trueLengthName.end());
      truncateRange.first = trueLengthName.find('?');
      truncateRange.second = trueLengthName.rfind('?');
      fileDatePattern.erase(std::remove(fileDatePattern.begin(), fileDatePattern.end(), '?'), fileDatePattern.end());
    }

    std::string file = name();

    std::tm result = {};
    std::tm sputnikLaunch = {};
    sputnikLaunch.tm_year = 1957 - 1900;
    sputnikLaunch.tm_mon = 10 - 1;
    sputnikLaunch.tm_mday = 4;

    std::string after = file.substr(file.rfind("}") + 1);

    std::string regexPattern = fileDatePattern;
    std::regex pattern("%d");
    regexPattern = std::regex_replace(regexPattern, pattern, R"(\d{2})");

    pattern = "%e";
    regexPattern = std::regex_replace(regexPattern, pattern, R"(\d{1})");
    
    pattern = "%b";
    regexPattern = std::regex_replace(regexPattern, pattern, R"([a-zA-Z]{3})");

    pattern = "%B";
    regexPattern = std::regex_replace(regexPattern, pattern, R"([a-zA-Z]{4,})");

    pattern = "%Y";
    regexPattern = std::regex_replace(regexPattern, pattern, R"(\d{4})");

    pattern = "%y";
    regexPattern = std::regex_replace(regexPattern, pattern, R"(\d{2})");

    pattern = "\'";
    regexPattern = std::regex_replace(regexPattern, pattern, "^");

    pattern = "_v";
    regexPattern = std::regex_replace(regexPattern, pattern, R"(_v\d+)");

    std::size_t pos = regexPattern.rfind('.');
    if (pos != std::string::npos) {
        regexPattern.insert(pos, 1, '\\');
    }

    fs::path directory = dir();
    std::vector<std::string> files = getFilesMatchingFilters(directory, regexPattern);

    // We can't sort the files to get our answer, so we need to go through every possible file.
    for (std::string foundFile : files) {
        // Toss any numerical versioning sequence
        if (truncateRange.first >= 0 && truncateRange.second > truncateRange.first) {
          foundFile = foundFile.substr(0, truncateRange.first) +
                      foundFile.substr(truncateRange.second + 1);

        }
        std::istringstream ss(foundFile);
        std::tm fileDate = {};
        ss >> std::get_time(&fileDate, fileDatePattern.c_str());

      if (isValidDate(fileDate)) {
        // No missions before Sputnik 1, so we must be in the new millenium
        std::time_t fileDateTime = std::mktime(const_cast<std::tm*>(&fileDate));
        std::time_t sputnikLaunchTime = std::mktime(const_cast<std::tm*>(&sputnikLaunch));
        std::time_t resultTime = std::mktime(const_cast<std::tm*>(&result));
        if (fileDateTime < sputnikLaunchTime) {
          fileDate.tm_year += 100;
          fileDateTime = std::mktime(&fileDate);
        }
        if (!isValidDate(result) || fileDateTime > resultTime) {
          result = fileDate;
        }
      }
    }

    if (!isValidDate(result)) {
      std::string msg = "No existing files found with a date version matching [" + FileName(expanded()).name() + "] in [" + path() + "]";
      throw IException(IException::Unknown,msg, _FILEINFO_);
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
    std::string file = FileName(expanded()).name();
    int result = 0;

    int width = std::count(file.begin(), file.end(), '?');

    std::pair<std::string, std::string> splitName = splitNameAroundVersionNum();
    std::string &before = splitName.first;
    std::string &after = splitName.second;

    std::ostringstream oss;
    oss << "^" << before << "\\d{" << width << "}" << after << "$";
    std::string pattern = oss.str();

    fs::path directory = dir();
    std::vector<std::string> files = getFilesMatchingFilters(directory, pattern);

    long foundValue = -1;
    long highestValue = 0;
    bool success = false;

    for (std::string foundFile : files) {
      try {
        foundValue = IString::ToInteger(foundFile.substr(before.length(), width));
        success = true;
      }
      catch (...) {
        success = false;
      }
      if (foundValue > highestValue) {
        highestValue = foundValue;
      }
    }

    if (success) {
      result = highestValue;
    }
    else {
      std::string msg = "No existing files found with a numerical version matching [" + FileName(expanded()).name() + "] in [" + path() + "]";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }

    return result;
  }

  /**
   * This verifies the class invariant when using versioning - that the FileName is in an acceptable
   * state to find file version numbers.
   */
  void FileName::validateVersioningState() const {
    std::string file = fs::path(expanded()).filename();

    std::regex regex("\\?\\?*[^?][^?]*\\?+");
    if(std::regex_match(file, regex)) {
      std::string msg = "Only one numerical version sequence is allowed in a filename; "
                      "there are multiple in [" + file + "]";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }

    if (isDateVersioned()) {
      std::string fileDatePattern = fileNameDatePattern();
      const std::time_t t_c = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
      std::tm buf;
      localtime_r(&t_c, &buf);

      std::stringstream ss;
      ss << std::put_time(&buf, fileDatePattern.c_str());

      std::string dated = ss.str();

      if (file.find("'") != std::string::npos) {
        std::string msg = "Date version sequenced file names cannot have single quotes in them; "
                        "the file named [" + file + "] is not usable";
        throw IException(IException::Unknown, msg, _FILEINFO_);
      }
      else if (dated.empty()) {
        std::string msg = "The date version sequence is not usable in the file named [" + file + "]"; 
        throw IException(IException::Unknown, msg, _FILEINFO_);
      }
      else if (fileDatePattern.find('{') != std::string::npos && fileDatePattern.find('}') != std::string::npos) {
        std::string msg = "The date version sequences are not recognized in the file named [" + file + "]";
        throw IException(IException::Unknown, msg, _FILEINFO_);
      }
    }
  }

  /**
   * This changes the files format. Specifically quotes everything not in {} with single quotes
   * and removes the {} from the file name.
   *
   * @return std::string
   */
  std::string FileName::fileNameDatePattern() const {
    // We need to quote everything not in {} with single quotes.
    std::string file = FileName(expanded()).name();

    std::regex regex("\\{MMM\\}");
    file = std::regex_replace(file, regex, "%b");

    regex = "\\{MMMM\\}";
    file = std::regex_replace(file, regex, "%B");

    regex = "\\{dd\\}";
    file = std::regex_replace(file, regex, "%d");

    regex = "\\{d\\}";
    file = std::regex_replace(file, regex, "%e");

    regex = "\\{yy\\}";
    file = std::regex_replace(file, regex, "%y");

    regex = "\\{yyyy\\}";
    file = std::regex_replace(file, regex, "%Y");

    regex = "\\{ddMMMyyyy\\}";
    file = std::regex_replace(file, regex, "%d%b%Y");

    return file;
  }

  /**
   * This returns a std::pair of the text (before, after) a version number in a file. Before being
   * the text before the version number and after being the text after the version number.
   *
   * @return std::pair
   */
  std::pair<std::string, std::string> FileName::splitNameAroundVersionNum() const {
    std::string file = FileName(expanded()).name();
    std::string before;
    std::string after;

    if (!isNumericallyVersioned()) {
      before = file;
    }
    else {
      before = file.substr(0, file.find("?"));
      after = file.substr(file.find_last_of("?") + 1);
    }

    return std::pair<std::string, std::string>(before, after);
  }

   /**
    * Data constructor, creates a new Data object.
    */
  FileName::FileData::FileData() {
    m_originalFileNameString = NULL;
    m_expandedFileNameString = NULL;

    m_originalFileNameString = new std::string;
    m_expandedFileNameString = new std::string;
  }

  /**
   * FileData copy constructor, creates a copy of a FileData object.
   *
   * @param &other FileData object to copy
   */
  FileName::FileData::FileData(const FileData &other) {
    m_originalFileNameString = NULL;
    m_expandedFileNameString = NULL;

    m_originalFileNameString = new std::string(*other.m_originalFileNameString);
    m_expandedFileNameString = new std::string(*other.m_expandedFileNameString);
  }

  /**
   * Destroys the FileData object.
   */
  FileName::FileData::~FileData() {
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
  std::string FileName::FileData::original(bool includeAttributes) const {
    std::string result = *m_originalFileNameString;

    if (!includeAttributes) {
      size_t attributesPos = result.find("+");

      if (attributesPos != string::npos)
        result = result.substr(0, attributesPos);
    }

    return result;
  }

  /**
   * Sets the original file name, stored in m_originalFileNameString. std::string parameter
   * is the new file name to store in m_originalFileNameString. The expanded verison is also
   * set and stored in m_expandedFileNameString when this method is called.
   *
   * @param originalStr the new file name
   */
  void FileName::FileData::setOriginal(const std::string &originalStr) {
    *m_originalFileNameString = originalStr;

    // Expand the file name and store that too.
    std::string expandedStr = original(true);

    int varSearchStartPos = 0;
    int varStartPos = -1;
    
    // Loop while there are any "$" at the current position or after
    // Some "$" might be skipped if no translation can be found
    while((varStartPos = expandedStr.find("$", varSearchStartPos)) != -1) {
      std::regex pattern("[^a-zA-Z{}0-9_]");
      std::smatch match;

      auto search_start = expandedStr.begin() + varStartPos + 1;
      auto it = std::sregex_iterator(search_start, expandedStr.end(), pattern);

      int varEndPos = (it != std::sregex_iterator()) ? std::distance(expandedStr.begin(), it->position() + search_start) : -1;
      if (varEndPos == -1)
        varEndPos = expandedStr.length();

      bool variableValid = false;
      int varNameLength = varEndPos - varStartPos;

      if (varNameLength > 0) {
        std::string varName = expandedStr.substr(varStartPos + 1, varEndPos - varStartPos - 1);

        if (varName.length()) {
          if (varName[0] =='{' && varName[varName.length() - 1] == '}')
            varName = varName.substr(1, varName.length() - 2);

          std::string varValue;

          // Find the corresponding Isis Preference if one exists
          if(Preference::Preferences().hasGroup("DataDirectory")) {
            PvlGroup &dataDir = Preference::Preferences().findGroup("DataDirectory");
            if(dataDir.hasKeyword(varName)) {
              varValue = dataDir[varName][0];
            }
          }

          // Find the corresponding environment variable if one exists
          if (varValue.empty()) {
            char *val;
            val = getenv(varName.c_str());
            if(val != NULL) varValue = val;
          }

          // Replace the $xxxx with the pref/env, but don't move
          // the pointer. We may have replaced one $ for another.
          // Note: May need to put a test for circular replaces in here
          if (!varValue.empty()) {
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
  std::string FileName::FileData::expanded(bool includeAttributes) const {
    std::string result = *m_expandedFileNameString;

    if (!includeAttributes) {
      int attributesPos = result.find("+");

      if (attributesPos != -1)
        result = result.substr(0, attributesPos);
    }

    return result;
  }
}
