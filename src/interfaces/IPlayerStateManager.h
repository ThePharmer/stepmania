#ifndef I_PLAYER_STATE_MANAGER_H
#define I_PLAYER_STATE_MANAGER_H

#include "PlayerNumber.h"

class PlayerState;
class Profile;

/**
 * @brief Interface for managing player state across the game.
 *
 * This interface provides a clean abstraction for player state management,
 * enabling dependency injection and unit testing without requiring full
 * engine initialization.
 */
class IPlayerStateManager
{
public:
	virtual ~IPlayerStateManager() = default;

	/**
	 * @brief Check if a player has joined the game.
	 * @param pn The player number to check.
	 * @return true if the player is joined, false otherwise.
	 */
	virtual bool IsPlayerJoined(PlayerNumber pn) const = 0;

	/**
	 * @brief Join a player to the game.
	 * @param pn The player number to join.
	 */
	virtual void JoinPlayer(PlayerNumber pn) = 0;

	/**
	 * @brief Unjoin a player from the game.
	 * @param pn The player number to unjoin.
	 */
	virtual void UnjoinPlayer(PlayerNumber pn) = 0;

	/**
	 * @brief Get the player state for a specific player.
	 * @param pn The player number.
	 * @return Pointer to the PlayerState, or nullptr if not available.
	 */
	virtual PlayerState* GetPlayerState(PlayerNumber pn) = 0;
	virtual const PlayerState* GetPlayerState(PlayerNumber pn) const = 0;

	/**
	 * @brief Reset a player's state to initial values.
	 * @param pn The player number to reset.
	 */
	virtual void ResetPlayer(PlayerNumber pn) = 0;

	/**
	 * @brief Get the number of players currently joined.
	 * @return The count of joined players.
	 */
	virtual int GetNumPlayersJoined() const = 0;

	/**
	 * @brief Get the stage tokens for a player.
	 * @param pn The player number.
	 * @return The number of stage tokens.
	 */
	virtual int GetStageTokens(PlayerNumber pn) const = 0;

	/**
	 * @brief Add a stage token to a player.
	 * @param pn The player number.
	 */
	virtual void AddStageToPlayer(PlayerNumber pn) = 0;

	/**
	 * @brief Check if any players are joined.
	 * @return true if at least one player is joined.
	 */
	virtual bool AnyPlayersJoined() const = 0;
};

#endif // I_PLAYER_STATE_MANAGER_H
