#include <gtest/gtest.h>

#include "Preference.h"

using namespace Isis;

int main(int argc, char **argv) {
   Isis::Preference::Preferences(true);

   ::testing::InitGoogleTest(&argc, argv);
   return RUN_ALL_TESTS();
}
