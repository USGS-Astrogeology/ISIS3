#include "gtest/gtest.h"

#include "Color.h"
#include "IException.h"
#include "TestUtilities.h"

#include <QDebug>
#include <QColor>


namespace Isis{


  class QStringQColorPair: public ::testing::TestWithParam<std::pair<QString, QColor> > {
    // Intentionally left empty
  };


  class InvalidColorString: public ::testing::TestWithParam<QString> {
  // Intentionally left empty
  };


  TEST_P (QStringQColorPair, ValidColorToString) {
    EXPECT_PRED_FORMAT2(AssertQStringsEqual, GetParam().first, Color::toRGBAString(GetParam().second));
  }


  TEST (ColorTests, InvalidColorToString) {
    std::string message = "Can not convert an invalid color to an RGBA string.  "
                      "There is no string representation of an invalid color";
    try {
      Color::toRGBAString(QColor());
      FAIL() << "Expected an IException";
    }
    catch(IException &e) {
      EXPECT_PRED_FORMAT2(
            AssertIExceptionMessage,
            e,
            message);
      EXPECT_PRED_FORMAT2(
            AssertIExceptionError,
            e,
            IException::ErrorType::Unknown);
    }
    catch(...) {
      FAIL() << "Expected error message: \"" << message.toStdString() << "\"";
    }
  }


  TEST_P (QStringQColorPair, ValidStringToColor) {
    EXPECT_EQ(GetParam().second, Color::fromRGBAString(GetParam().first));
  }


  TEST_P (InvalidColorString, InvalidStringToColor) {
    EXPECT_EQ(QColor(QColor::Invalid), Color::fromRGBAString(GetParam()));
  }


  TEST_P (QStringQColorPair, ValidColorRGBAFormat) {
    EXPECT_TRUE(Color::colorRGBAFormat().exactMatch(GetParam().first));
  }


  TEST_P (InvalidColorString, InvalidColorRGBAFormat) {
    EXPECT_FALSE(Color::colorRGBAFormat().exactMatch(GetParam()));
  }

  INSTANTIATE_TEST_SUITE_P (Color,
                           QStringQColorPair,
                           ::testing::Values(std::make_pair(QString("#000000ff"), QColor(0, 0, 0)),
                                             std::make_pair(QString("#00000000"), QColor(0, 0, 0, 0)),
                                             std::make_pair(QString("#ff000000"), QColor(255, 0, 0, 0)),
                                             std::make_pair(QString("#00ff0000"), QColor(0, 255, 0, 0)),
                                             std::make_pair(QString("#0000ff00"), QColor(0, 0, 255, 0)),
                                             std::make_pair(QString("#000000ff"), QColor(0, 0, 0, 255)),
                                             std::make_pair(QString("#ffffffff"), QColor(255, 255, 255, 255)),
                                             std::make_pair(QString("#0a141e28"), QColor(10, 20, 30, 40))));


  INSTANTIATE_TEST_SUITE_P (Color,
                           InvalidColorString,
                           ::testing::Values(QString("#rrggbbaa"),
                                            QString(" 00112233"),
                                            QString(""),
                                            QString("#001122")));

}
