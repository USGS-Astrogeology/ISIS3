#include "TestUtilities.h"

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

    return AssertPvlGroupKeywordsEqual(group1_expr, group2_expr, group1, group2);
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

  // Check to see if a QString contains only numeric values.
  bool isNumeric(QString str){
    QRegExp re("-*\\d*.*\\d*");
    return re.exactMatch(str);
  }


  // Compares CSV lines
  void compareCsvLine(CSVReader::CSVAxis csvLine, QString headerStr, int initialIndex) {
    QStringList compareMe = headerStr.split(",");
    for (int i=initialIndex; i<compareMe.size(); i++) {
      if (isNumeric(compareMe[i].trimmed())) {
        EXPECT_NEAR(csvLine[i].toDouble(), compareMe[i].toDouble(), 0.000001);
      }
      else{
        EXPECT_EQ(QString(csvLine[i]).toStdString(), compareMe[i].toStdString());
      }
    }
  };


  // Compares CSV lines
  void compareCsvLine(CSVReader::CSVAxis csvLine, CSVReader::CSVAxis csvLine2, int initialIndex,
                      double tolerance) {
    for (int i=initialIndex; i < csvLine.dim(); i++) {
      if (isNumeric(QString(csvLine[i].trimmed()))) {
        EXPECT_NEAR(csvLine[i].toDouble(), csvLine2[i].toDouble(), tolerance);
      }
      else{
        EXPECT_EQ(QString(csvLine[i]).toStdString(), csvLine2[i].toStdString());
      }
    }
  };


  // Matches a CSM Image Coord for gMock
  ::testing::Matcher<const csm::ImageCoord&> MatchImageCoord(const csm::ImageCoord &expected) {
    return ::testing::AllOf(
        ::testing::Field(&csm::ImageCoord::line, ::testing::DoubleNear(expected.line, 0.0001)),
        ::testing::Field(&csm::ImageCoord::samp, ::testing::DoubleNear(expected.samp, 0.0001))
    );
}


  // Matches a CSM ECEF Coord for gMock
  ::testing::Matcher<const csm::EcefCoord&> MatchEcefCoord(const csm::EcefCoord &expected) {
    return ::testing::AllOf(
        ::testing::Field(&csm::EcefCoord::x, ::testing::DoubleNear(expected.x, 0.0001)),
        ::testing::Field(&csm::EcefCoord::y, ::testing::DoubleNear(expected.y, 0.0001)),
        ::testing::Field(&csm::EcefCoord::z, ::testing::DoubleNear(expected.z, 0.0001))
    );
  }
}
