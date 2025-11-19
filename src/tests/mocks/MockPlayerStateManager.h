#ifndef MOCK_PLAYER_STATE_MANAGER_H
#define MOCK_PLAYER_STATE_MANAGER_H

#include "interfaces/IPlayerStateManager.h"
#include <map>

/**
 * @brief Mock implementation of IPlayerStateManager for unit testing.
 *
 * This mock allows testing code that depends on player state management
 * without requiring the full GameState singleton or engine initialization.
 */
class MockPlayerStateManager : public IPlayerStateManager
{
private:
	bool m_bPlayersJoined[NUM_PLAYERS];
	int m_iStageTokens[NUM_PLAYERS];
	std::map<PlayerNumber, PlayerState*> m_PlayerStates;

public:
	MockPlayerStateManager()
	{
		for (int i = 0; i < NUM_PLAYERS; ++i)
		{
			m_bPlayersJoined[i] = false;
			m_iStageTokens[i] = 0;
		}
	}

	virtual ~MockPlayerStateManager()
	{
		// Clean up any PlayerState objects we created
		for (auto& pair : m_PlayerStates)
		{
			delete pair.second;
		}
	}

	bool IsPlayerJoined(PlayerNumber pn) const override
	{
		if (pn < 0 || pn >= NUM_PLAYERS)
			return false;
		return m_bPlayersJoined[pn];
	}

	void JoinPlayer(PlayerNumber pn) override
	{
		if (pn >= 0 && pn < NUM_PLAYERS)
		{
			m_bPlayersJoined[pn] = true;
			if (m_PlayerStates.find(pn) == m_PlayerStates.end())
			{
				m_PlayerStates[pn] = new PlayerState();
				m_PlayerStates[pn]->SetPlayerNumber(pn);
			}
		}
	}

	void UnjoinPlayer(PlayerNumber pn) override
	{
		if (pn >= 0 && pn < NUM_PLAYERS)
		{
			m_bPlayersJoined[pn] = false;
		}
	}

	PlayerState* GetPlayerState(PlayerNumber pn) override
	{
		auto it = m_PlayerStates.find(pn);
		return (it != m_PlayerStates.end()) ? it->second : nullptr;
	}

	const PlayerState* GetPlayerState(PlayerNumber pn) const override
	{
		auto it = m_PlayerStates.find(pn);
		return (it != m_PlayerStates.end()) ? it->second : nullptr;
	}

	void ResetPlayer(PlayerNumber pn) override
	{
		if (pn >= 0 && pn < NUM_PLAYERS)
		{
			m_bPlayersJoined[pn] = false;
			m_iStageTokens[pn] = 0;
			PlayerState* pState = GetPlayerState(pn);
			if (pState)
			{
				pState->Reset();
			}
		}
	}

	int GetNumPlayersJoined() const override
	{
		int count = 0;
		for (int i = 0; i < NUM_PLAYERS; ++i)
		{
			if (m_bPlayersJoined[i])
				++count;
		}
		return count;
	}

	int GetStageTokens(PlayerNumber pn) const override
	{
		if (pn >= 0 && pn < NUM_PLAYERS)
			return m_iStageTokens[pn];
		return 0;
	}

	void AddStageToPlayer(PlayerNumber pn) override
	{
		if (pn >= 0 && pn < NUM_PLAYERS)
		{
			m_iStageTokens[pn]++;
		}
	}

	bool AnyPlayersJoined() const override
	{
		return GetNumPlayersJoined() > 0;
	}
};

#endif // MOCK_PLAYER_STATE_MANAGER_H
