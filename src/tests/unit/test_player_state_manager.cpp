/**
 * @file test_player_state_manager.cpp
 * @brief Unit tests for player state management.
 *
 * These tests demonstrate the testing infrastructure and verify
 * player state management logic without requiring full engine initialization.
 */

#include <gtest/gtest.h>
#include "mocks/MockPlayerStateManager.h"

/**
 * @brief Test fixture for PlayerStateManager tests.
 *
 * Provides a clean MockPlayerStateManager instance for each test.
 */
class PlayerStateManagerTest : public ::testing::Test
{
protected:
	MockPlayerStateManager manager;

	void SetUp() override
	{
		// Fresh manager for each test
	}

	void TearDown() override
	{
		// Cleanup happens automatically
	}
};

/**
 * @brief Test that players start unjoined.
 */
TEST_F(PlayerStateManagerTest, PlayersStartUnjoined)
{
	EXPECT_FALSE(manager.IsPlayerJoined(PLAYER_1));
	EXPECT_FALSE(manager.IsPlayerJoined(PLAYER_2));
	EXPECT_EQ(0, manager.GetNumPlayersJoined());
	EXPECT_FALSE(manager.AnyPlayersJoined());
}

/**
 * @brief Test joining a single player.
 */
TEST_F(PlayerStateManagerTest, JoinSinglePlayer)
{
	manager.JoinPlayer(PLAYER_1);

	EXPECT_TRUE(manager.IsPlayerJoined(PLAYER_1));
	EXPECT_FALSE(manager.IsPlayerJoined(PLAYER_2));
	EXPECT_EQ(1, manager.GetNumPlayersJoined());
	EXPECT_TRUE(manager.AnyPlayersJoined());
}

/**
 * @brief Test joining multiple players.
 */
TEST_F(PlayerStateManagerTest, JoinMultiplePlayers)
{
	manager.JoinPlayer(PLAYER_1);
	manager.JoinPlayer(PLAYER_2);

	EXPECT_TRUE(manager.IsPlayerJoined(PLAYER_1));
	EXPECT_TRUE(manager.IsPlayerJoined(PLAYER_2));
	EXPECT_EQ(2, manager.GetNumPlayersJoined());
	EXPECT_TRUE(manager.AnyPlayersJoined());
}

/**
 * @brief Test unjoining a player.
 */
TEST_F(PlayerStateManagerTest, UnjoinPlayer)
{
	manager.JoinPlayer(PLAYER_1);
	manager.JoinPlayer(PLAYER_2);

	manager.UnjoinPlayer(PLAYER_1);

	EXPECT_FALSE(manager.IsPlayerJoined(PLAYER_1));
	EXPECT_TRUE(manager.IsPlayerJoined(PLAYER_2));
	EXPECT_EQ(1, manager.GetNumPlayersJoined());
}

/**
 * @brief Test getting player state after joining.
 */
TEST_F(PlayerStateManagerTest, GetPlayerStateAfterJoin)
{
	manager.JoinPlayer(PLAYER_1);

	PlayerState* pState = manager.GetPlayerState(PLAYER_1);
	ASSERT_NE(nullptr, pState);
	EXPECT_EQ(PLAYER_1, pState->m_PlayerNumber);
}

/**
 * @brief Test getting player state before joining returns nullptr.
 */
TEST_F(PlayerStateManagerTest, GetPlayerStateBeforeJoinReturnsNull)
{
	PlayerState* pState = manager.GetPlayerState(PLAYER_1);
	EXPECT_EQ(nullptr, pState);
}

/**
 * @brief Test stage token management.
 */
TEST_F(PlayerStateManagerTest, StageTokens)
{
	manager.JoinPlayer(PLAYER_1);

	EXPECT_EQ(0, manager.GetStageTokens(PLAYER_1));

	manager.AddStageToPlayer(PLAYER_1);
	EXPECT_EQ(1, manager.GetStageTokens(PLAYER_1));

	manager.AddStageToPlayer(PLAYER_1);
	EXPECT_EQ(2, manager.GetStageTokens(PLAYER_1));
}

/**
 * @brief Test resetting a player.
 */
TEST_F(PlayerStateManagerTest, ResetPlayer)
{
	manager.JoinPlayer(PLAYER_1);
	manager.AddStageToPlayer(PLAYER_1);

	manager.ResetPlayer(PLAYER_1);

	EXPECT_FALSE(manager.IsPlayerJoined(PLAYER_1));
	EXPECT_EQ(0, manager.GetStageTokens(PLAYER_1));
}

/**
 * @brief Test boundary conditions with invalid player numbers.
 */
TEST_F(PlayerStateManagerTest, InvalidPlayerNumbers)
{
	// Should not crash with invalid player numbers
	EXPECT_FALSE(manager.IsPlayerJoined(static_cast<PlayerNumber>(-1)));
	EXPECT_FALSE(manager.IsPlayerJoined(static_cast<PlayerNumber>(999)));

	manager.JoinPlayer(static_cast<PlayerNumber>(-1));
	EXPECT_EQ(0, manager.GetNumPlayersJoined());
}
