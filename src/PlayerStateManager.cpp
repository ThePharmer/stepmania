#include "global.h"
#include "PlayerStateManager.h"
#include "PlayerState.h"
#include "GameState.h"
#include "GameManager.h"
#include "PrefsManager.h"
#include "MessageManager.h"
#include "ProfileManager.h"
#include "MemoryCardManager.h"
#include "Bookkeeper.h"
#include "StatsManager.h"

PlayerStateManager::PlayerStateManager() :
	m_masterPlayerNumber(PLAYER_INVALID),
	m_bMultiplayer(false),
	m_iNumMultiplayerNoteFields(1)
{
	FOREACH_PlayerNumber(pn)
	{
		m_bSideIsJoined[pn] = false;
		m_iPlayerStageTokens[pn] = 0;
		m_pPlayerState[pn] = new PlayerState;
		m_pPlayerState[pn]->SetPlayerNumber(pn);
	}

	FOREACH_MultiPlayer(mp)
	{
		m_pMultiPlayerState[mp] = new PlayerState;
		m_pMultiPlayerState[mp]->SetPlayerNumber(PLAYER_1);
		m_pMultiPlayerState[mp]->m_mp = mp;
		m_MultiPlayerStatus[mp] = MultiPlayerStatus_NotJoined;
	}
}

PlayerStateManager::~PlayerStateManager()
{
	FOREACH_PlayerNumber(pn)
		SAFE_DELETE(m_pPlayerState[pn]);
	FOREACH_MultiPlayer(mp)
		SAFE_DELETE(m_pMultiPlayerState[mp]);
}

void PlayerStateManager::Reset()
{
	m_masterPlayerNumber = PLAYER_INVALID;

	FOREACH_PlayerNumber(pn)
	{
		m_bSideIsJoined[pn] = false;
		m_iPlayerStageTokens[pn] = 0;
	}

	FOREACH_MultiPlayer(mp)
		m_MultiPlayerStatus[mp] = MultiPlayerStatus_NotJoined;

	m_bMultiplayer = false;
	m_iNumMultiplayerNoteFields = 1;
}

void PlayerStateManager::ResetPlayer(PlayerNumber pn)
{
	m_iPlayerStageTokens[pn] = 0;
	m_pPlayerState[pn]->Reset();
	PROFILEMAN->UnloadProfile(pn);
}

void PlayerStateManager::JoinPlayer(PlayerNumber pn)
{
	// Make sure the join will be successful before doing it. -Kyz
	{
		int players_joined = 0;
		for (int i = 0; i < NUM_PLAYERS; ++i)
		{
			players_joined += m_bSideIsJoined[i];
		}
		if (players_joined > 0)
		{
			const Style* cur_style = GAMESTATE->GetCurrentStyle(PLAYER_INVALID);
			if (cur_style)
			{
				const Style* new_style = GAMEMAN->GetFirstCompatibleStyle(GAMESTATE->m_pCurGame,
					players_joined + 1, cur_style->m_StepsType);
				if (new_style == nullptr)
				{
					return;
				}
			}
		}
	}

	/* If joint premium and we're not taking away a credit for the 2nd join,
	 * give the new player the same number of stage tokens that the old player
	 * has. */
	if (GAMESTATE->GetCoinMode() == CoinMode_Pay &&
		GAMESTATE->GetPremium() == Premium_2PlayersFor1Credit &&
		GetNumSidesJoined() == 1)
	{
		m_iPlayerStageTokens[pn] = m_iPlayerStageTokens[m_masterPlayerNumber];
	}
	else
	{
		m_iPlayerStageTokens[pn] = PREFSMAN->m_iSongsPerPlay;
	}

	m_bSideIsJoined[pn] = true;

	if (m_masterPlayerNumber == PLAYER_INVALID)
		m_masterPlayerNumber = pn;

	// if first player to join, trigger BeginGame in GameState
	if (GetNumSidesJoined() == 1)
		GAMESTATE->BeginGame();

	// Count each player join as a play.
	{
		Profile* pMachineProfile = PROFILEMAN->GetMachineProfile();
		pMachineProfile->m_iTotalSessions++;
	}

	// Set the current style to something appropriate for the new number of joined players.
	// beat gametype's versus styles use a different stepstype from its single
	// styles, so when GameCommand tries to join both players for a versus
	// style, it hits the assert when joining the first player.  So if the first
	// player is being joined and the current styletype is for two players,
	// assume that the second player will be joined immediately afterwards and
	// don't try to change the style. -Kyz
	const Style* cur_style = GAMESTATE->GetCurrentStyle(PLAYER_INVALID);
	if (cur_style != nullptr && !(pn == PLAYER_1 &&
		(cur_style->m_StyleType == StyleType_TwoPlayersTwoSides ||
			cur_style->m_StyleType == StyleType_TwoPlayersSharedSides)))
	{
		const Style* pStyle;
		// Only use one player for StyleType_OnePlayerTwoSides and StepsTypes
		// that can only be played by one player (e.g. dance-solo,
		// dance-threepanel, popn-nine). -aj
		// XXX?: still shows joined player as "Insert Card". May not be an issue? -aj
		if (cur_style->m_StyleType == StyleType_OnePlayerTwoSides ||
			cur_style->m_StepsType == StepsType_dance_solo ||
			cur_style->m_StepsType == StepsType_dance_threepanel ||
			cur_style->m_StepsType == StepsType_popn_nine)
			pStyle = GAMEMAN->GetFirstCompatibleStyle(GAMESTATE->m_pCurGame, 1, cur_style->m_StepsType);
		else
			pStyle = GAMEMAN->GetFirstCompatibleStyle(GAMESTATE->m_pCurGame, GetNumSidesJoined(), cur_style->m_StepsType);

		// use SetCurrentStyle in case of StyleType_OnePlayerTwoSides
		GAMESTATE->SetCurrentStyle(pStyle, pn);
	}

	Message msg(MessageIDToString(Message_PlayerJoined));
	msg.SetParam("Player", pn);
	MESSAGEMAN->Broadcast(msg);
}

void PlayerStateManager::UnjoinPlayer(PlayerNumber pn)
{
	/* Unjoin STATSMAN first, so steps used by this player are released
	 * and can be released by PROFILEMAN. */
	STATSMAN->UnjoinPlayer(pn);
	m_bSideIsJoined[pn] = false;
	m_iPlayerStageTokens[pn] = 0;

	ResetPlayer(pn);

	if (m_masterPlayerNumber == pn)
	{
		// We can't use GetFirstHumanPlayer() because if both players were joined, GetFirstHumanPlayer() will always return PLAYER_1, even when PLAYER_1 is the player we're unjoining.
		FOREACH_HumanPlayer(hp)
		{
			if (pn != hp)
			{
				m_masterPlayerNumber = hp;
			}
		}
		if (m_masterPlayerNumber == pn)
		{
			m_masterPlayerNumber = PLAYER_INVALID;
		}
	}

	Message msg(MessageIDToString(Message_PlayerUnjoined));
	msg.SetParam("Player", pn);
	MESSAGEMAN->Broadcast(msg);
}

bool PlayerStateManager::JoinInputInternal(PlayerNumber pn)
{
	if (!GAMESTATE->PlayersCanJoin())
		return false;

	// If this side is already in, don't re-join.
	if (m_bSideIsJoined[pn])
		return false;

	// subtract coins
	int iCoinsNeededToJoin = GAMESTATE->GetCoinsNeededToJoin();

	if (GAMESTATE->m_iCoins < iCoinsNeededToJoin)
		return false;	// not enough coins

	GAMESTATE->m_iCoins.Set(GAMESTATE->m_iCoins - iCoinsNeededToJoin);

	JoinPlayer(pn);

	// On Join, make sure to update Coins File
	BOOKKEEPER->WriteCoinsFile(GAMESTATE->m_iCoins.Get());

	return true;
}

// Handle an input that can join a player. Return true if the player joined.
bool PlayerStateManager::JoinInput(PlayerNumber pn)
{
	// When AutoJoin is enabled, join all players on a single start press.
	if (GAMESTATE->m_bAutoJoin.Get())
		return JoinPlayers();
	else
		return JoinInputInternal(pn);
}

// Attempt to join all players, as if each player pressed Start.
bool PlayerStateManager::JoinPlayers()
{
	bool bJoined = false;
	FOREACH_PlayerNumber(pn)
	{
		if (JoinInputInternal(pn))
			bJoined = true;
	}
	return bJoined;
}

int PlayerStateManager::GetNumSidesJoined() const
{
	int iNumSidesJoined = 0;
	FOREACH_PlayerNumber(pn)
	{
		if (m_bSideIsJoined[pn])
			iNumSidesJoined++;
	}
	return iNumSidesJoined;
}

bool PlayerStateManager::IsPlayerEnabled(PlayerNumber pn) const
{
	// This must be called after GAMESTATE->m_PlayMode is set
	ASSERT_M(GAMESTATE->m_PlayMode != PlayMode_Invalid, "GAMESTATE->m_PlayMode is not set yet");
	return IsPlayerEnabled(m_pPlayerState[pn]);
}

bool PlayerStateManager::IsMultiPlayerEnabled(MultiPlayer mp) const
{
	// This must be called after GAMESTATE->m_PlayMode is set
	ASSERT_M(GAMESTATE->m_PlayMode != PlayMode_Invalid, "GAMESTATE->m_PlayMode is not set yet");
	return IsPlayerEnabled(m_pMultiPlayerState[mp]);
}

bool PlayerStateManager::IsPlayerEnabled(const PlayerState* pPlayerState) const
{
	if (GAMESTATE->m_bMultiplayer)
		return m_MultiPlayerStatus[pPlayerState->m_mp] != MultiPlayerStatus_NotJoined;

	switch (GAMESTATE->m_PlayMode)
	{
	case PLAY_MODE_BATTLE:
	case PLAY_MODE_RAVE:
		return true;
	default:
		return m_bSideIsJoined[pPlayerState->GetPlayerNumber()];
	}
}

int PlayerStateManager::GetNumPlayersEnabled() const
{
	if (GAMESTATE->m_bMultiplayer)
	{
		int iNum = 0;
		FOREACH_MultiPlayer(mp)
		{
			if (IsMultiPlayerEnabled(mp))
				iNum++;
		}
		return iNum;
	}
	else
	{
		int iNum = 0;
		FOREACH_PlayerNumber(pn)
		{
			if (IsPlayerEnabled(pn))
				iNum++;
		}
		return iNum;
	}
}

bool PlayerStateManager::IsHumanPlayer(PlayerNumber pn) const
{
	if (!IsPlayerEnabled(pn))
		return false;
	return m_pPlayerState[pn]->m_PlayerController == PC_HUMAN;
}

int PlayerStateManager::GetNumHumanPlayers() const
{
	int count = 0;
	FOREACH_PlayerNumber(pn)
	{
		if (IsHumanPlayer(pn))
			count++;
	}
	return count;
}

PlayerNumber PlayerStateManager::GetFirstHumanPlayer() const
{
	FOREACH_PlayerNumber(pn)
	{
		if (IsHumanPlayer(pn))
			return pn;
	}
	return PLAYER_INVALID;
}

PlayerNumber PlayerStateManager::GetFirstDisabledPlayer() const
{
	FOREACH_PlayerNumber(pn)
	{
		if (!IsPlayerEnabled(pn))
			return pn;
	}
	return PLAYER_INVALID;
}

bool PlayerStateManager::IsCpuPlayer(PlayerNumber pn) const
{
	if (!IsPlayerEnabled(pn))
		return false;
	return m_pPlayerState[pn]->m_PlayerController == PC_CPU ||
		m_pPlayerState[pn]->m_PlayerController == PC_AUTOPLAY;
}

bool PlayerStateManager::AnyPlayersAreCpu() const
{
	FOREACH_PlayerNumber(pn)
	{
		if (IsCpuPlayer(pn))
			return true;
	}
	return false;
}

void PlayerStateManager::SetMasterPlayerNumber(PlayerNumber pn)
{
	m_masterPlayerNumber = pn;
}

void PlayerStateManager::AddStageToPlayer(PlayerNumber pn)
{
	m_iPlayerStageTokens[pn]++;
}

void PlayerStateManager::Update(float fDelta)
{
	FOREACH_PlayerNumber(p)
	{
		m_pPlayerState[p]->Update(fDelta);
	}
}
