#ifndef Fixtures_h
#define Fixtures_h

#include "gtest/gtest.h"

#include <QString>

#include "PvlKeyword.h"

namespace Isis {

class RawPvlKeywords : public ::testing::Test {
  protected:
    // You can define per-test set-up logic as usual.
    void SetUp() override;

    // You can define per-test tear-down logic as usual.
    void TearDown() override;

    
    std::vector<QString> keywordsToTry;
    std::vector<PvlKeyword> results;
    std::vector<bool> valid;
  };
}

#endif