/**
 * @file test_stage_progression.cpp
 * @brief Unit tests for stage progression management.
 *
 * These tests verify stage progression logic including stage counting,
 * extra stages, and demonstration mode.
 */

#include <gtest/gtest.h>
#include "mocks/MockStageProgressionManager.h"

/**
 * @brief Test fixture for StageProgressionManager tests.
 */
class StageProgressionTest : public ::testing::Test
{
protected:
	MockStageProgressionManager manager;

	void SetUp() override
	{
		manager.SetTotalStages(3);
	}
};

/**
 * @brief Test initial stage state.
 */
TEST_F(StageProgressionTest, InitialState)
{
	EXPECT_EQ(0, manager.GetCurrentStageIndex());
	EXPECT_EQ(1, manager.GetNumStagesOfThisSong());
	EXPECT_FALSE(manager.IsDemonstrationOrJukebox());
	EXPECT_FALSE(manager.IsInStage());
}

/**
 * @brief Test beginning and finishing a stage.
 */
TEST_F(StageProgressionTest, BeginAndFinishStage)
{
	manager.BeginStage();
	EXPECT_TRUE(manager.IsInStage());
	EXPECT_EQ(0, manager.GetCurrentStageIndex());

	manager.FinishStage();
	EXPECT_FALSE(manager.IsInStage());
	EXPECT_EQ(1, manager.GetCurrentStageIndex());
}

/**
 * @brief Test progression through multiple stages.
 */
TEST_F(StageProgressionTest, MultipleStages)
{
	// Stage 0
	manager.BeginStage();
	manager.FinishStage();
	EXPECT_EQ(1, manager.GetCurrentStageIndex());

	// Stage 1
	manager.BeginStage();
	manager.FinishStage();
	EXPECT_EQ(2, manager.GetCurrentStageIndex());

	// Stage 2 (final)
	manager.BeginStage();
	manager.FinishStage();
	EXPECT_EQ(3, manager.GetCurrentStageIndex());
}

/**
 * @brief Test detecting final stage.
 */
TEST_F(StageProgressionTest, DetectFinalStage)
{
	manager.SetCurrentStageIndex(0);
	EXPECT_FALSE(manager.IsFinalStage());

	manager.SetCurrentStageIndex(1);
	EXPECT_FALSE(manager.IsFinalStage());

	manager.SetCurrentStageIndex(2); // Last stage in 3-stage game
	EXPECT_TRUE(manager.IsFinalStage());
}

/**
 * @brief Test detecting extra stage.
 */
TEST_F(StageProgressionTest, DetectExtraStage)
{
	manager.SetCurrentStageIndex(2);
	EXPECT_FALSE(manager.IsExtraStage());

	manager.SetCurrentStageIndex(3); // Beyond normal stages
	EXPECT_TRUE(manager.IsExtraStage());

	manager.SetCurrentStageIndex(4);
	EXPECT_TRUE(manager.IsExtraStage());
}

/**
 * @brief Test demonstration mode flag.
 */
TEST_F(StageProgressionTest, DemonstrationMode)
{
	EXPECT_FALSE(manager.IsDemonstrationOrJukebox());

	manager.SetDemonstrationOrJukebox(true);
	EXPECT_TRUE(manager.IsDemonstrationOrJukebox());

	manager.SetDemonstrationOrJukebox(false);
	EXPECT_FALSE(manager.IsDemonstrationOrJukebox());
}

/**
 * @brief Test setting number of stages for current song.
 */
TEST_F(StageProgressionTest, NumStagesOfSong)
{
	manager.SetNumStagesOfThisSong(2);
	EXPECT_EQ(2, manager.GetNumStagesOfThisSong());

	manager.SetNumStagesOfThisSong(1);
	EXPECT_EQ(1, manager.GetNumStagesOfThisSong());
}

/**
 * @brief Test reset functionality.
 */
TEST_F(StageProgressionTest, Reset)
{
	// Set up some state
	manager.BeginStage();
	manager.FinishStage();
	manager.SetDemonstrationOrJukebox(true);
	manager.SetNumStagesOfThisSong(5);

	// Reset
	manager.Reset();

	// Verify reset to initial state
	EXPECT_EQ(0, manager.GetCurrentStageIndex());
	EXPECT_EQ(1, manager.GetNumStagesOfThisSong());
	EXPECT_FALSE(manager.IsDemonstrationOrJukebox());
	EXPECT_FALSE(manager.IsInStage());
}

/**
 * @brief Test that finishing without beginning doesn't advance stage.
 */
TEST_F(StageProgressionTest, FinishWithoutBeginNoAdvance)
{
	EXPECT_EQ(0, manager.GetCurrentStageIndex());

	manager.FinishStage(); // Finish without begin
	EXPECT_EQ(0, manager.GetCurrentStageIndex()); // Should not advance
}

/**
 * @brief Test total stages configuration.
 */
TEST_F(StageProgressionTest, TotalStages)
{
	EXPECT_EQ(3, manager.GetTotalStages());

	manager.SetTotalStages(5);
	EXPECT_EQ(5, manager.GetTotalStages());
}

/**
 * @brief Test stage progression with different total stage counts.
 */
TEST_F(StageProgressionTest, DifferentTotalStages)
{
	manager.SetTotalStages(4);

	manager.SetCurrentStageIndex(2);
	EXPECT_FALSE(manager.IsFinalStage());
	EXPECT_FALSE(manager.IsExtraStage());

	manager.SetCurrentStageIndex(3); // Last stage
	EXPECT_TRUE(manager.IsFinalStage());
	EXPECT_FALSE(manager.IsExtraStage());

	manager.SetCurrentStageIndex(4); // Extra stage
	EXPECT_FALSE(manager.IsFinalStage());
	EXPECT_TRUE(manager.IsExtraStage());
}
