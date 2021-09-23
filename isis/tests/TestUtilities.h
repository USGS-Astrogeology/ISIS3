#ifndef TestUtilities_h
#define TestUtilities_h

#include "gmock/gmock.h"

#include <string>
#include <vector>

#include <QString>

#include "csm.h"

#include "IException.h"
#include "PvlGroup.h"

#include "Pvl.h"
#include "PvlObject.h"

#include "CSVReader.h"

namespace Isis {

  ::testing::AssertionResult AssertIExceptionMessage(
          const char* e_expr,
          const char* contents_expr,
          IException &e,
          QString contents);

  ::testing::AssertionResult AssertIExceptionError(
          const char* e_expr,
          const char* errorType_expr,
          IException &e,
          IException::ErrorType errorType);

  ::testing::AssertionResult AssertQStringsEqual(
        const char* string1_expr,
        const char* string2_expr,
        QString string1,
        QString string2);

  ::testing::AssertionResult AssertPvlGroupEqual(
      const char* group1_expr,
      const char* group2_expr,
      PvlGroup group1,
      PvlGroup group2);

  ::testing::AssertionResult AssertPvlGroupKeywordsEqual(
      const char* group1_expr,
      const char* group2_expr,
      PvlGroup group1,
      PvlGroup group2);

  ::testing::AssertionResult AssertVectorsNear(
      const char* vec1_expr,
      const char* vec2_expr,
      const char* tolerance_expr,
      const std::vector<double> &vec1,
      const std::vector<double> &vec2,
      double tolerance);

  bool isNumeric(QString str);
  void compareCsvLine(CSVReader::CSVAxis csvLine, QString headerStr, int initialIndex=0);
  void compareCsvLine(CSVReader::CSVAxis csvLine, CSVReader::CSVAxis csvLine2, int initialIndex=0,
                      double tolerance = 0.000001);

  ::testing::Matcher<const csm::ImageCoord&> MatchImageCoord(const csm::ImageCoord &expected);
  ::testing::Matcher<const csm::EcefCoord&> MatchEcefCoord(const csm::EcefCoord &expected);
}

#endif
