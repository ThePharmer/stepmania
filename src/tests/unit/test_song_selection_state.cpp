/**
 * @file test_song_selection_state.cpp
 * @brief Unit tests for song selection state management.
 *
 * These tests verify song and course selection logic without requiring
 * full engine initialization or the GameState singleton.
 */

#include <gtest/gtest.h>
#include "mocks/MockSongSelectionState.h"

/**
 * @brief Test fixture for SongSelectionState tests.
 */
class SongSelectionStateTest : public ::testing::Test
{
protected:
	MockSongSelectionState state;

	void SetUp() override
	{
		// Fresh state for each test
	}
};

/**
 * @brief Test that selection state starts empty.
 */
TEST_F(SongSelectionStateTest, StartsEmpty)
{
	EXPECT_EQ(nullptr, state.GetCurrentSong());
	EXPECT_EQ(nullptr, state.GetCurrentCourse());
	EXPECT_FALSE(state.HasCurrentSong());
	EXPECT_FALSE(state.HasCurrentCourse());
}

/**
 * @brief Test setting and getting current song.
 */
TEST_F(SongSelectionStateTest, SetCurrentSong)
{
	// Use a dummy pointer for testing (not dereferenced)
	Song* pDummySong = reinterpret_cast<Song*>(0x1234);

	state.SetCurrentSong(pDummySong);

	EXPECT_EQ(pDummySong, state.GetCurrentSong());
	EXPECT_TRUE(state.HasCurrentSong());
}

/**
 * @brief Test setting and getting current course.
 */
TEST_F(SongSelectionStateTest, SetCurrentCourse)
{
	// Use a dummy pointer for testing
	Course* pDummyCourse = reinterpret_cast<Course*>(0x5678);

	state.SetCurrentCourse(pDummyCourse);

	EXPECT_EQ(pDummyCourse, state.GetCurrentCourse());
	EXPECT_TRUE(state.HasCurrentCourse());
}

/**
 * @brief Test setting steps for a specific player.
 */
TEST_F(SongSelectionStateTest, SetCurrentSteps)
{
	Steps* pDummySteps = reinterpret_cast<Steps*>(0xABCD);

	state.SetCurrentSteps(PLAYER_1, pDummySteps);

	EXPECT_EQ(pDummySteps, state.GetCurrentSteps(PLAYER_1));
	EXPECT_EQ(nullptr, state.GetCurrentSteps(PLAYER_2));
}

/**
 * @brief Test setting steps for multiple players.
 */
TEST_F(SongSelectionStateTest, SetStepsMultiplePlayers)
{
	Steps* pSteps1 = reinterpret_cast<Steps*>(0x1111);
	Steps* pSteps2 = reinterpret_cast<Steps*>(0x2222);

	state.SetCurrentSteps(PLAYER_1, pSteps1);
	state.SetCurrentSteps(PLAYER_2, pSteps2);

	EXPECT_EQ(pSteps1, state.GetCurrentSteps(PLAYER_1));
	EXPECT_EQ(pSteps2, state.GetCurrentSteps(PLAYER_2));
}

/**
 * @brief Test setting trail for a specific player.
 */
TEST_F(SongSelectionStateTest, SetCurrentTrail)
{
	Trail* pDummyTrail = reinterpret_cast<Trail*>(0xDEAD);

	state.SetCurrentTrail(PLAYER_1, pDummyTrail);

	EXPECT_EQ(pDummyTrail, state.GetCurrentTrail(PLAYER_1));
	EXPECT_EQ(nullptr, state.GetCurrentTrail(PLAYER_2));
}

/**
 * @brief Test resetting clears all selection state.
 */
TEST_F(SongSelectionStateTest, ResetClearsAll)
{
	// Set up some state
	Song* pDummySong = reinterpret_cast<Song*>(0x1234);
	Course* pDummyCourse = reinterpret_cast<Course*>(0x5678);
	Steps* pDummySteps = reinterpret_cast<Steps*>(0xABCD);

	state.SetCurrentSong(pDummySong);
	state.SetCurrentCourse(pDummyCourse);
	state.SetCurrentSteps(PLAYER_1, pDummySteps);

	// Reset
	state.Reset();

	// Verify everything is cleared
	EXPECT_EQ(nullptr, state.GetCurrentSong());
	EXPECT_EQ(nullptr, state.GetCurrentCourse());
	EXPECT_EQ(nullptr, state.GetCurrentSteps(PLAYER_1));
	EXPECT_FALSE(state.HasCurrentSong());
	EXPECT_FALSE(state.HasCurrentCourse());
}

/**
 * @brief Test boundary conditions with invalid player numbers.
 */
TEST_F(SongSelectionStateTest, InvalidPlayerNumbers)
{
	Steps* pDummySteps = reinterpret_cast<Steps*>(0xABCD);

	// Should handle invalid player numbers gracefully
	state.SetCurrentSteps(static_cast<PlayerNumber>(-1), pDummySteps);
	EXPECT_EQ(nullptr, state.GetCurrentSteps(static_cast<PlayerNumber>(-1)));

	state.SetCurrentSteps(static_cast<PlayerNumber>(999), pDummySteps);
	EXPECT_EQ(nullptr, state.GetCurrentSteps(static_cast<PlayerNumber>(999)));
}

/**
 * @brief Test that setting nullptr is valid.
 */
TEST_F(SongSelectionStateTest, SetNullIsValid)
{
	Song* pDummySong = reinterpret_cast<Song*>(0x1234);

	state.SetCurrentSong(pDummySong);
	EXPECT_TRUE(state.HasCurrentSong());

	state.SetCurrentSong(nullptr);
	EXPECT_FALSE(state.HasCurrentSong());
	EXPECT_EQ(nullptr, state.GetCurrentSong());
}
