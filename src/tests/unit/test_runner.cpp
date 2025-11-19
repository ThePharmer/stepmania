/**
 * @file test_runner.cpp
 * @brief Main entry point for StepMania unit tests.
 *
 * This file provides the main() function for running all unit tests.
 * Google Test automatically discovers and runs all tests.
 */

#include <gtest/gtest.h>

int main(int argc, char** argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
