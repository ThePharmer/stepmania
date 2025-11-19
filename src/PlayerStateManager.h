#ifndef PLAYER_STATE_MANAGER_H
#define PLAYER_STATE_MANAGER_H

#include "GameConstantsAndTypes.h"
#include "MessageManager.h"

class PlayerState;

/**
 * @brief Manages player state including joining, tokens, and player data.
 *
 * This class was extracted from GameState as part of the architectural
 * refactoring to decompose the god object. It handles all player-related
 * state management including:
 * - Player joining/unjoining
 * - Stage tokens (credits)
 * - Player enabled/human state queries
 * - Multi-player status
 */
class PlayerStateManager
{
public:
	PlayerStateManager();
	~PlayerStateManager();

	// Initialization and Reset
	void Reset();
	void ResetPlayer(PlayerNumber pn);

	// Player Joining
	void JoinPlayer(PlayerNumber pn);
	void UnjoinPlayer(PlayerNumber pn);
	bool JoinInput(PlayerNumber pn);
	bool JoinPlayers();

	// Player State Queries
	bool IsPlayerJoined(PlayerNumber pn) const { return m_bSideIsJoined[pn]; }
	int GetNumSidesJoined() const;
	bool IsPlayerEnabled(PlayerNumber pn) const;
	bool IsMultiPlayerEnabled(MultiPlayer mp) const;
	bool IsPlayerEnabled(const PlayerState* pPlayerState) const;
	int GetNumPlayersEnabled() const;

	// Human vs CPU
	bool IsHumanPlayer(PlayerNumber pn) const;
	int GetNumHumanPlayers() const;
	PlayerNumber GetFirstHumanPlayer() const;
	PlayerNumber GetFirstDisabledPlayer() const;
	bool IsCpuPlayer(PlayerNumber pn) const;
	bool AnyPlayersAreCpu() const;

	// Master Player
	PlayerNumber GetMasterPlayerNumber() const { return m_masterPlayerNumber; }
	void SetMasterPlayerNumber(PlayerNumber pn);

	// Stage Tokens
	int GetPlayerStageTokens(PlayerNumber pn) const { return m_iPlayerStageTokens[pn]; }
	void SetPlayerStageTokens(PlayerNumber pn, int tokens) { m_iPlayerStageTokens[pn] = tokens; }
	void AddStageToPlayer(PlayerNumber pn);

	// Player State Access
	PlayerState* GetPlayerState(PlayerNumber pn) const { return m_pPlayerState[pn]; }
	PlayerState* GetMultiPlayerState(MultiPlayer mp) const { return m_pMultiPlayerState[mp]; }

	// Multi-player Mode
	bool IsMultiplayer() const { return m_bMultiplayer; }
	void SetMultiplayer(bool b) { m_bMultiplayer = b; }
	int GetNumMultiplayerNoteFields() const { return m_iNumMultiplayerNoteFields; }
	void SetNumMultiplayerNoteFields(int n) { m_iNumMultiplayerNoteFields = n; }

	MultiPlayerStatus GetMultiPlayerStatus(MultiPlayer mp) const { return m_MultiPlayerStatus[mp]; }
	void SetMultiPlayerStatus(MultiPlayer mp, MultiPlayerStatus status) { m_MultiPlayerStatus = status; }

	// Update
	void Update(float fDelta);

private:
	// Player joining state
	bool m_bSideIsJoined[NUM_PLAYERS];
	int m_iPlayerStageTokens[NUM_PLAYERS];
	PlayerNumber m_masterPlayerNumber;

	// Player state objects
	PlayerState* m_pPlayerState[NUM_PLAYERS];
	PlayerState* m_pMultiPlayerState[NUM_MultiPlayer];

	// Multi-player
	MultiPlayerStatus m_MultiPlayerStatus[NUM_MultiPlayer];
	bool m_bMultiplayer;
	int m_iNumMultiplayerNoteFields;

	// Helper methods
	bool JoinInputInternal(PlayerNumber pn);

	PlayerStateManager(const PlayerStateManager& rhs);
	PlayerStateManager& operator=(const PlayerStateManager& rhs);
};

#endif

/**
 * @file
 * @author StepMania Team (Refactored 2025)
 * @section LICENSE
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
