#include <gtest/gtest.h>

TEST(SanityCheckGtest, T1) {
	EXPECT_EQ(1, 1);
}

TEST(SanityCheckGtest, T2) {
	EXPECT_EQ(2, 2);
}


class ClientTests : public ::testing::Test {
public:
	ClientTests() {
		
	}
};

TEST_F(ClientTests, Login) {

}

TEST_F(ClientTests, LoginNullString) {

}

TEST_F(ClientTests, LoginEmptyString) {

}

TEST_F(ClientTests, Logout) {

}

TEST_F(ClientTests, Say) {

}

TEST_F(ClientTests, Join) {

}

TEST_F(ClientTests, Leave) {

}

TEST_F(ClientTests, Switch) {

}

TEST_F(ClientTests, List) {

}

TEST_F (ClientTests, Who) {

}

int main(int argc, char **argv) {
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
