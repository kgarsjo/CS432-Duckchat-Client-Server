#include <gtest/gtest.h>

TEST(SanityCheckGtest, T1) {
	EXPECT_EQ(1, 1);
}

TEST(SanityCheckGtest, T2) {
	EXPECT_EQ(2, 2);
}


class ClientTests : public ::testing::Test {

};

TEST_F(ClientTests, Login) {

}

TEST_F(ClientTests, LoginNullString) {

}

TEST_F(ClientTests, LoginEmptyString) {

}

int main(int argc, char **argv) {
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
