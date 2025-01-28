#include "TestUtilities.h"

#include <cmath>
#include <string>

namespace Isis {

  /**
   * Custom IException assertion that checks that the exception message contains
   * a substring.
   */
  ::testing::AssertionResult AssertIExceptionMessage(
          const char* e_expr,
          const char* contents_expr,
          IException &e,
          QString contents) {
    if (e.toString().contains(contents)) return ::testing::AssertionSuccess();

    return ::testing::AssertionFailure() << "IExeption "<< e_expr << "\'s error message (\""
       << e.toString().toStdString() << "\") does not contain " << contents_expr << " (\""
       << contents.toStdString() << "\").";
  }


  /**
   * Custom IException assertion that checks that the exception is the expected
   * IException::ErrorType.
   */
  ::testing::AssertionResult AssertIExceptionError(
          const char* e_expr,
          const char* errorType_expr,
          IException &e,
          IException::ErrorType errorType) {
    if (e.errorType() == errorType) return ::testing::AssertionSuccess();

    return ::testing::AssertionFailure() << "IExeption "<< e_expr << "\'s error type ("
        << std::to_string(e.errorType()) << ") does not match expected error type ("
        << std::to_string(errorType) << ").";
  }


  /**
   * Custom QString assertion that properly outputs them as string if they
   * are different.
   */
  ::testing::AssertionResult AssertQStringsEqual(
        const char* string1_expr,
        const char* string2_expr,
        QString string1,
        QString string2) {
    if (string1 != string2) {
      return ::testing::AssertionFailure() << "QStrings " << string1_expr
          << " (" << string1.toStdString() << ") and " << string2_expr
          << " (" << string2.toStdString() << ") are not the same.";
    }

    return ::testing::AssertionSuccess();
  }


  /**
   * Custom PvlGroup assertion that compares the group names and
   * all of the PvlKeywords in the groups.
   */
  ::testing::AssertionResult AssertPvlGroupEqual(
      const char* group1_expr,
      const char* group2_expr,
      PvlGroup group1,
      PvlGroup group2) {
    if (group1.name() != group2.name()) {
      return ::testing::AssertionFailure() << "PvlGroup " << group1_expr
          << " has name (" << group1.name().toStdString() << ") and PvlGroup "
          << group2_expr << " has name (" << group2.name().toStdString() << ").";
    }

    for (auto grp1KeyIt = group1.begin(); grp1KeyIt != group1.end(); grp1KeyIt++) {
      if (!group2.hasKeyword(grp1KeyIt->name())) {
        return ::testing::AssertionFailure() << "PvlGroup " << group1_expr
            << " contains keyword " << grp1KeyIt->name().toStdString()
            << " that is not in PvlGroup " << group2_expr;
      }
      const PvlKeyword &group2Key = group2.findKeyword(grp1KeyIt->name());
      if (grp1KeyIt->size() != group2Key.size()) {
        return ::testing::AssertionFailure() << "Keyword (" << grp1KeyIt->name().toStdString()
            << ") has size (" << grp1KeyIt->size() << ") in PvlGroup " << group1_expr
            << " and size (" << group2Key.size() << ") in PvlGroup " << group2_expr;
      }
      for (int i = 0; i < grp1KeyIt->size(); i++) {
        if (!grp1KeyIt->isEquivalent(group2Key[i], i)) {
          return ::testing::AssertionFailure() << "Keyword (" << grp1KeyIt->name().toStdString()
              << ") has value (" << (*grp1KeyIt)[i].toStdString() << ") in PvlGroup "
              << group1_expr << " and value (" << group2Key[i].toStdString()
              << ") in PvlGroup " << group2_expr << " at index " << i;
        }
        if (grp1KeyIt->unit(i) != group2Key.unit(i)) {
          return ::testing::AssertionFailure() << "Keyword (" << grp1KeyIt->name().toStdString()
              << ") has units (" << grp1KeyIt->unit(i).toStdString() << ") in PvlGroup "
              << group1_expr << " and units (" << group2Key.unit(i).toStdString()
              << ") in PvlGroup " << group2_expr << " at index " << i;
        }
      }
    }

    // The second time through you only have to check that the keys in group 2 exist in group 1
    for (auto grp2KeyIt = group2.begin(); grp2KeyIt != group2.end(); grp2KeyIt++) {
      if (!group1.hasKeyword(grp2KeyIt->name())) {
        return ::testing::AssertionFailure() << "PvlGroup " << group2_expr
            << " contains keyword " << grp2KeyIt->name().toStdString()
            << " that is not in PvlGroup " << group1_expr;
      }
    }
    return ::testing::AssertionSuccess();
  }


  /**
   * Custom PvlGroup assertion that compares only the PvlKeywords in the groups.
   */
  ::testing::AssertionResult AssertPvlGroupKeywordsEqual(
      const char* group1_expr,
      const char* group2_expr,
      PvlGroup group1,
      PvlGroup group2) {

    for (auto grp1KeyIt = group1.begin(); grp1KeyIt != group1.end(); grp1KeyIt++) {
      if (!group2.hasKeyword(grp1KeyIt->name())) {
        return ::testing::AssertionFailure() << "PvlGroup " << group1_expr
            << " contains keyword " << grp1KeyIt->name().toStdString()
            << " that is not in PvlGroup " << group2_expr;
      }
      const PvlKeyword &group2Key = group2.findKeyword(grp1KeyIt->name());
      if (grp1KeyIt->size() != group2Key.size()) {
        return ::testing::AssertionFailure() << "Keyword (" << grp1KeyIt->name().toStdString()
            << ") has size (" << grp1KeyIt->size() << ") in PvlGroup " << group1_expr
            << " and size (" << group2Key.size() << ") in PvlGroup " << group2_expr;
      }
      for (int i = 0; i < grp1KeyIt->size(); i++) {
        if (!grp1KeyIt->isEquivalent(group2Key[i], i)) {
          return ::testing::AssertionFailure() << "Keyword (" << grp1KeyIt->name().toStdString()
              << ") has value (" << (*grp1KeyIt)[i].toStdString() << ") in PvlGroup "
              << group1_expr << " and value (" << group2Key[i].toStdString()
              << ") in PvlGroup " << group2_expr << " at index " << i;
        }
        if (grp1KeyIt->unit(i) != group2Key.unit(i)) {
          return ::testing::AssertionFailure() << "Keyword (" << grp1KeyIt->name().toStdString()
              << ") has units (" << grp1KeyIt->unit(i).toStdString() << ") in PvlGroup "
              << group1_expr << " and units (" << group2Key.unit(i).toStdString()
              << ") in PvlGroup " << group2_expr << " at index " << i;
        }
      }
    }

    // The second time through you only have to check that the keys in group 2 exist in group 1
    for (auto grp2KeyIt = group2.begin(); grp2KeyIt != group2.end(); grp2KeyIt++) {
      if (!group1.hasKeyword(grp2KeyIt->name())) {
        return ::testing::AssertionFailure() << "PvlGroup " << group2_expr
            << " contains keyword " << grp2KeyIt->name().toStdString()
            << " that is not in PvlGroup " << group1_expr;
      }
    }

    return ::testing::AssertionSuccess();
  }


  /**
   * Asserts that two vectors are within a given tolerance of each other.
   * If the vectors are not the same size, then they are not assumed to be equal.
   * The difference between two vectors is the maximum elementwise difference,
   * the infinity norm.
   */
  ::testing::AssertionResult AssertVectorsNear(
      const char* vec1_expr,
      const char* vec2_expr,
      const char* tolerance_expr,
      const std::vector<double> &vec1,
      const std::vector<double> &vec2,
      double tolerance) {
    if (vec1.size() != vec2.size()) {
      return ::testing::AssertionFailure() << "Vector " << vec1_expr
          << " and Vector " << vec2_expr << " have different sizes "
          << vec1.size() << " and " << vec2.size() << ".";
    }
    std::vector<size_t> differences;
    for (size_t i = 0; i < vec1.size(); i++) {
      if (abs(vec1[i] - vec2[i]) > tolerance) {
        differences.push_back(i);
      }
    }
    if (differences.empty()) {
      return ::testing::AssertionSuccess();
    }
    ::testing::AssertionResult failure = ::testing::AssertionFailure()
        << "Vector " << vec1_expr << " and Vector " << vec2_expr
        << " differ by more than tolerance " << tolerance_expr
        << " which evaluates to " << tolerance << ".\n";
    for (size_t &index : differences) {
      failure = failure << " Index: " << index << " values: "
          << vec1[index] << " and " << vec2[index] << ".\n";
    }
    return failure;
  }

  /**
   * Helper recursive comparison function for two JSON objects
   * Logic is modified from nlohmann::json::diff function
   */
  std::vector<std::string> compareJsons(
      const nlohmann::json &json1,
      const nlohmann::json &json2,
      std::string jsonPointer,
      double tolerance) {
    std::vector<std::string> differences;
    // Basic check for equality to short-circuit behavior
    if (json1 == json2) {
      return differences;
    }

    // Check types
    // If both are numeric, then don't check the type so we can compare ints to floats
    if (!(json1.is_number() && json2.is_number()) && json1.type() != json2.type()) {
      differences.push_back("JSONs have different types at [" + jsonPointer + "]");
      return differences;
    }

    switch (json1.type()) {
      // Handle arrays
      case nlohmann::detail::value_t::array: {
        // Check for same size
        if (json1.size() != json2.size()) {
          differences.push_back(
              "JSONs have different sized arrays [" + std::to_string(json1.size())
              + "] and [" + std::to_string(json2.size()) + "] at [" + jsonPointer + "]");
          return differences;
        }

        // Check values
        for (size_t i = 0; i < json1.size(); i++) {
          std::string newPointer = jsonPointer + "/" + std::to_string(i);
          std::vector<std::string> tempDiffs = compareJsons(json1[i], json2[i], newPointer, tolerance);
          differences.insert(differences.end(), tempDiffs.begin(), tempDiffs.end());
        }

        break;
      }

      // Handle objects
      case nlohmann::detail::value_t::object: {
        // Check for keys from the first JSON
        for (auto it = json1.cbegin(); it != json1.cend(); ++it) {
          // Check for presence
          if (json2.contains(it.key())) {
            // Check values
            std::string newPointer = jsonPointer + "/" + it.key();
            std::vector<std::string> tempDiffs = compareJsons(it.value(), json2[it.key()], newPointer, tolerance);
            differences.insert(differences.end(), tempDiffs.begin(), tempDiffs.end());
          }
          else {
            differences.push_back(
                "Key [" + it.key() + "] is present in the first JSON but not the second at ["
                + jsonPointer + "]");
          }
        }

        // Check for keys from the second JSON
        // This time only check for presence because if they are present in the first JSON
        // we have already checked their value
        for (auto it = json2.cbegin(); it != json2.cend(); ++it) {
          if (!json1.contains(it.key())) {
            differences.push_back(
                "Key [" + it.key() + "] is present in the second JSON but not the first at ["
                + jsonPointer + "]");
          }
        }

        break;
      }

      // Handle numeric types
      case nlohmann::detail::value_t::number_integer:
      case nlohmann::detail::value_t::number_unsigned:
      case nlohmann::detail::value_t::number_float: {
        double numDiff = abs(json1.get<double>() - json2.get<double>());
        if (numDiff > tolerance) {
          differences.push_back(
                "Values [" + json1.dump() + "] and [" + json2.dump() + "] differ by ["
                + std::to_string(numDiff) + "] which is greater than tolerance ["
                + std::to_string(tolerance) + "] at [" + jsonPointer + "]");
        }

        break;
      }

      // Handle the rest
      case nlohmann::detail::value_t::null:
      case nlohmann::detail::value_t::string:
      case nlohmann::detail::value_t::boolean:
      case nlohmann::detail::value_t::binary:
      case nlohmann::detail::value_t::discarded:
      default: {
        // This "if" is redundant with the short-circuit check but is included for clarity/safety
        if (json1 != json2) {
          differences.push_back(
                "Values [" + json1.dump() + "] and [" + json2.dump() + "] differ at ["
                + jsonPointer + "]");
        }

        break;
      }
    }

    return differences;
  }

  /**
   * Asserts that two JSON objects are the same except for numerical values are within
   * a given tolerance.
   */
  ::testing::AssertionResult AssertJsonsNear(
      const char* json1_expr,
      const char* json2_expr,
      const char* tolerance_expr,
      const nlohmann::json &json1,
      const nlohmann::json &json2,
      double tolerance) {

    std::vector<std::string> differences = compareJsons(json1, json2, std::string(""), tolerance);

    if (differences.size() > 0) {
      ::testing::AssertionResult failure = ::testing::AssertionFailure()
          << "JSONs " << json1_expr << " and " << json2_expr << " are different within a tolerance of "
          << tolerance_expr << std::endl;
      for (size_t i = 0; i < differences.size(); i++) {
        failure << differences[i] << std::endl;
      }
      return failure;
    }

    return ::testing::AssertionSuccess();
  }

  // Check to see if a QString contains only numeric values.
  bool isNumeric(QString str){
    QRegExp re("^(?:[+-]?(?:\\d+|\\d*\\.(?=\\d)\\d*)(?:[eE][+-]?\\d+)?)$");
    return re.exactMatch(str);
  }

  // Compares CSV lines
  void compareCsvLine(CSVReader::CSVAxis csvLine, QString headerStr, int initialIndex) {
    QStringList compareMe = headerStr.split(",");
    for (int i=initialIndex; i<compareMe.size(); i++) {
      if (isNumeric(compareMe[i].trimmed()) && isNumeric(QString(csvLine[i].trimmed()))) {
        EXPECT_NEAR(csvLine[i].toDouble(), compareMe[i].toDouble(), 0.000001);
      }
      else{
        EXPECT_EQ(QString(csvLine[i].trimmed()).toStdString(), compareMe[i].trimmed().toStdString());
      }
    }
  };


  // Compares CSV lines allowing use of a custom delimiter (defaults to ",")
  void compareCsvLineCustomDelimiter(CSVReader::CSVAxis csvLine, QString headerStr,
                                     QString delimiter, int initialIndex) {
    QStringList compareMe = headerStr.split(delimiter);
    for (int i=initialIndex; i<compareMe.size(); i++) {
      if (isNumeric(compareMe[i].trimmed()) && isNumeric(QString(csvLine[i].trimmed()))) {
        EXPECT_NEAR(csvLine[i].toDouble(), compareMe[i].toDouble(), 0.000001);
      }
      else{
        EXPECT_EQ(QString(csvLine[i].trimmed()).toStdString(), compareMe[i].trimmed().toStdString());
      }
    }
  };

  // Compares CSV lines
  void compareCsvLine(CSVReader::CSVAxis csvLine, CSVReader::CSVAxis csvLine2, int initialIndex,
                      double tolerance) {
    for (int i=initialIndex; i < csvLine.dim(); i++) {
      if (isNumeric(QString(csvLine[i].trimmed())) && isNumeric(QString(csvLine2[i].trimmed()))) {
        EXPECT_NEAR(csvLine[i].toDouble(), csvLine2[i].toDouble(), tolerance);
      }
      else{
        EXPECT_EQ(QString(csvLine[i].trimmed()).toStdString(), csvLine2[i].trimmed().toStdString());
      }
    }
  };

  // Writes binary kernels to the data area. Unsure of the best way to handle
  // clean up. Didn't want to dive into the rabbit hole of C++ alternatives
  // to python yeild statements
  QVector<QString> generateBinaryKernels(QVector<QString> kernelList) {
    QVector<QString> binaryKernelList;

    for (QString kernel : kernelList) {
      FileName file(kernel);
      QString pathToBinaryKernel = file.path() + "/" + file.baseName() + "." + file.extension().replace('x', 'b');
      FileName binaryFile(pathToBinaryKernel);

      if (file.extension().contains("x") && !binaryFile.fileExists()) {
        QString path = file.expanded();
        QString command = "tobin " + path;
        command += " >nul 2>nul";
        int status = system(command.toLatin1().data());

        if (status != 0) {
          QString msg = "Executing command [" + command +
                        "] failed with return status [" + toString(status) + "]";
          throw IException(IException::Programmer, msg, _FILEINFO_);
        }
      }
      binaryKernelList.append(pathToBinaryKernel);
    }
    return binaryKernelList;
  }

  QString fileListToString(QVector<QString> fileList) {
    QString filesAsString("(");

    for (int i = 0; i < fileList.size(); i++) {
      FileName file(fileList[i]);

      filesAsString += file.expanded();
      if (i != fileList.size() - 1) {
        filesAsString += ", ";
      }
    }
    filesAsString += ")";
    return filesAsString;
  }

}
