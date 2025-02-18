#include "IEndian.h"

#include <gtest/gtest.h>
#include <QPair>

using namespace Isis;

class ConversionTest : public ::testing::TestWithParam<QPair <QString, Isis::ByteOrder> > {
	// Intentionally empty
};

TEST_P(ConversionTest, ByteOrder) {
	ASSERT_EQ(ByteOrderEnumeration(GetParam().first), GetParam().second);
	ASSERT_EQ(ByteOrderName(GetParam().second), GetParam().first);
}

QPair<QString, Isis::ByteOrder> noOrder("None", Isis::ByteOrder::NoByteOrder);
QPair<QString, Isis::ByteOrder> lsb("Lsb", Isis::ByteOrder::Lsb);
QPair<QString, Isis::ByteOrder> msb("Msb", Isis::ByteOrder::Msb);

INSTANTIATE_TEST_SUITE_P(Endian, ConversionTest, ::testing::Values(noOrder, lsb, msb));
