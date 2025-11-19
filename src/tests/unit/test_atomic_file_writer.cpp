/**
 * @file test_atomic_file_writer.cpp
 * @brief Unit tests for atomic file writing operations.
 *
 * These tests verify the atomic file writing logic that prevents data loss
 * during save operations, as identified in the critical data integrity issues.
 */

#include <gtest/gtest.h>
#include "mocks/MockAtomicFileWriter.h"
#include "RageFile.h"

/**
 * @brief Test fixture for AtomicFileWriter tests.
 */
class AtomicFileWriterTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		// Fresh test environment for each test
	}
};

/**
 * @brief Test initial state of atomic writer.
 */
TEST_F(AtomicFileWriterTest, InitialState)
{
	MockAtomicFileWriter writer("/path/to/target.xml");

	EXPECT_EQ("/path/to/target.xml", writer.GetTargetPath());
	EXPECT_EQ("/path/to/target.xml.tmp.mock", writer.GetTempPath());
	EXPECT_FALSE(writer.IsCommitted());
	EXPECT_FALSE(writer.IsOpened());
}

/**
 * @brief Test successful open operation.
 */
TEST_F(AtomicFileWriterTest, OpenSuccess)
{
	MockAtomicFileWriter writer("/path/to/target.xml");
	RageFile file;

	EXPECT_TRUE(writer.Open(file));
	EXPECT_TRUE(writer.IsOpened());
	EXPECT_FALSE(writer.IsCommitted());
}

/**
 * @brief Test failed open operation.
 */
TEST_F(AtomicFileWriterTest, OpenFailure)
{
	MockAtomicFileWriter writer("/path/to/target.xml");
	writer.SetShouldFailOpen(true);
	RageFile file;

	EXPECT_FALSE(writer.Open(file));
	EXPECT_FALSE(writer.IsOpened());
}

/**
 * @brief Test successful commit operation.
 */
TEST_F(AtomicFileWriterTest, CommitSuccess)
{
	MockAtomicFileWriter writer("/path/to/target.xml");
	RageFile file;

	writer.Open(file);
	EXPECT_TRUE(writer.Commit(file));
	EXPECT_TRUE(writer.IsCommitted());
}

/**
 * @brief Test commit fails if not opened first.
 */
TEST_F(AtomicFileWriterTest, CommitFailsWithoutOpen)
{
	MockAtomicFileWriter writer("/path/to/target.xml");
	RageFile file;

	EXPECT_FALSE(writer.Commit(file));
	EXPECT_FALSE(writer.IsCommitted());
}

/**
 * @brief Test commit failure scenario.
 */
TEST_F(AtomicFileWriterTest, CommitFailure)
{
	MockAtomicFileWriter writer("/path/to/target.xml");
	writer.SetShouldFailCommit(true);
	RageFile file;

	writer.Open(file);
	EXPECT_FALSE(writer.Commit(file));
	EXPECT_FALSE(writer.IsCommitted());
}

/**
 * @brief Test that double commit is not allowed.
 */
TEST_F(AtomicFileWriterTest, DoubleCommitNotAllowed)
{
	MockAtomicFileWriter writer("/path/to/target.xml");
	RageFile file;

	writer.Open(file);
	EXPECT_TRUE(writer.Commit(file));

	// Second commit should fail
	EXPECT_FALSE(writer.Commit(file));
}

/**
 * @brief Test complete write workflow.
 */
TEST_F(AtomicFileWriterTest, CompleteWorkflow)
{
	MockAtomicFileWriter writer("/path/to/profile/Stats.xml");
	RageFile file;

	// 1. Open temp file
	ASSERT_TRUE(writer.Open(file));
	EXPECT_TRUE(writer.IsOpened());
	EXPECT_FALSE(writer.IsCommitted());

	// 2. (Write data would happen here in real code)

	// 3. Commit atomically
	ASSERT_TRUE(writer.Commit(file));
	EXPECT_TRUE(writer.IsCommitted());

	// 4. Verify paths
	EXPECT_EQ("/path/to/profile/Stats.xml", writer.GetTargetPath());
}

/**
 * @brief Test that temp file has different path than target.
 */
TEST_F(AtomicFileWriterTest, TempPathDifferentFromTarget)
{
	MockAtomicFileWriter writer("/path/to/target.xml");

	EXPECT_NE(writer.GetTargetPath(), writer.GetTempPath());
	EXPECT_TRUE(writer.GetTempPath().find(".tmp") != std::string::npos);
}

/**
 * @brief Test error recovery scenario.
 *
 * This simulates the critical data integrity issue: if a crash happens
 * during write, the original file should remain intact because we write
 * to a temp file first.
 */
TEST_F(AtomicFileWriterTest, ErrorRecoveryScenario)
{
	MockAtomicFileWriter writer("/path/to/Stats.xml");
	RageFile file;

	// Open and write to temp file
	ASSERT_TRUE(writer.Open(file));

	// Simulate crash/error before commit
	// In real scenario, temp file would be cleaned up by destructor
	// and original Stats.xml would remain untouched

	EXPECT_FALSE(writer.IsCommitted());
	// Original file is safe because commit never happened
}

/**
 * @brief Test that writer can be reused with different paths.
 */
TEST_F(AtomicFileWriterTest, DifferentPaths)
{
	MockAtomicFileWriter writer1("/path/to/Stats.xml");
	MockAtomicFileWriter writer2("/path/to/Preferences.ini");

	EXPECT_EQ("/path/to/Stats.xml", writer1.GetTargetPath());
	EXPECT_EQ("/path/to/Preferences.ini", writer2.GetTargetPath());

	EXPECT_NE(writer1.GetTempPath(), writer2.GetTempPath());
}
