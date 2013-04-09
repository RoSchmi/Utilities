#include <cstring>
#include <gtest/gtest.h>

#include <Utilities/Array.h>

using namespace Utilities;

TEST(Array, Creation) {
	Array foo;
	EXPECT_EQ(0, foo.getSize());

	Array bar(100);
	EXPECT_EQ(100, bar.getSize());

//	Array baz = bar;
//	EXPECT_EQ(100, baz.getSize());
}

TEST(Array, Expands) {
	Array foo;
	uint8 data[] = {1, 2, 3, 4, 5};
	foo.write(data, 50, 5);
	foo.write(data, 500, 5);
	foo.write(data, 500000, 5);
}

TEST(Array, Read) {
	Array foo;
	uint8 data[] = {1, 2, 3, 4, 5};
	foo.write(data, 10, 5);
}
