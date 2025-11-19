#include "global.h"
#include "StageProgressionManager.h"
#include "GameState.h"
#include "Song.h"
#include "Course.h"
#include "Steps.h"
#include "Trail.h"
#include "PrefsManager.h"
#include "StatsManager.h"
#include "ProfileManager.h"
#include "ThemeMetric.h"
#include "AdjustSync.h"
#include "CryptManager.h"
#include "PlayerState.h"
#include "RageLog.h"

static const ThemeMetric<bool> EDIT_ALLOWED_FOR_EXTRA("GameState", "EditAllowedForExtra");
static const ThemeMetric<Difficulty> MIN_DIFFICULTY_FOR_EXTRA("GameState", "MinDifficultyForExtra");
static const ThemeMetric<Grade> GRADE_TIER_FOR_EXTRA_1("GameState", "GradeTierForExtra1");
static const ThemeMetric<bool> ALLOW_EXTRA_2("GameState", "AllowExtra2");
static const ThemeMetric<Grade> GRADE_TIER_FOR_EXTRA_2("GameState", "GradeTierForExtra2");
static ThemeMetric<bool> ARE_STAGE_PLAYER_MODS_FORCED("GameState", "AreStagePlayerModsForced");
static ThemeMetric<bool> ARE_STAGE_SONG_MODS_FORCED("GameState", "AreStageSongModsForced");

StageProgressionManager::StageProgressionManager() :
	m_iCurrentStageIndex(0),
	m_iNumStagesOfThisSong(0),
	m_bDemonstrationOrJukebox(false),
	m_bJukeboxUsesModifiers(false),
	m_bEarnedExtraStage(false),
	m_bBackedOutOfFinalStage(false),
	m_AdjustTokensBySongCostForFinalStageCheck(true),
	m_iGameSeed(0),
	m_iStageSeed(0)
{
	FOREACH_PlayerNumber(pn)
	{
		m_iAwardedExtraStages[pn] = 0;
	}
	m_timeGameStarted.SetZero();
}

StageProgressionManager::~StageProgressionManager()
{
}

void StageProgressionManager::Reset()
{
	m_timeGameStarted.SetZero();
	m_iCurrentStageIndex = 0;
	m_iNumStagesOfThisSong = 0;
	m_bDemonstrationOrJukebox = false;
	m_bJukeboxUsesModifiers = false;
	m_bBackedOutOfFinalStage = false;
	m_bEarnedExtraStage = false;
	m_AdjustTokensBySongCostForFinalStageCheck = true;

	m_iGameSeed = rand();
	m_iStageSeed = rand();

	FOREACH_PlayerNumber(pn)
	{
		m_iAwardedExtraStages[pn] = 0;
	}
}

// Static helper methods
int StageProgressionManager::GetNumStagesMultiplierForSong(const Song* pSong)
{
	int iNumStages = 1;

	ASSERT(pSong != nullptr);
	if (pSong->IsMarathon())
		iNumStages *= 3;
	if (pSong->IsLong())
		iNumStages *= 2;

	return iNumStages;
}

int StageProgressionManager::GetNumStagesForSongAndStyleType(const Song* pSong, StyleType st)
{
	// Not implemented in original, placeholder for future use
	return GetNumStagesMultiplierForSong(pSong);
}

int StageProgressionManager::GetNumStagesForCurrentSongAndStepsOrCourse() const
{
	int iNumStagesOfThisSong = 1;
	if (GAMESTATE->m_pCurSong)
	{
		/* Extra stages need to only count as one stage in case a multi-stage
		 * song is chosen. */
		if (IsAnExtraStage())
			iNumStagesOfThisSong = 1;
		else
			iNumStagesOfThisSong = GetNumStagesMultiplierForSong(GAMESTATE->m_pCurSong);
	}
	else if (GAMESTATE->m_pCurCourse)
		iNumStagesOfThisSong = PREFSMAN->m_iSongsPerPlay;
	else
		return -1;

	iNumStagesOfThisSong = max(iNumStagesOfThisSong, 1);

	return iNumStagesOfThisSong;
}

// Called by ScreenGameplay. Set the length of the current song.
void StageProgressionManager::BeginStage()
{
	if (m_bDemonstrationOrJukebox)
		return;

	// This should only be called once per stage.
	if (m_iNumStagesOfThisSong != 0)
		LOG->Warn("XXX: m_iNumStagesOfThisSong == %i?", m_iNumStagesOfThisSong);

	GAMESTATE->ResetStageStatistics();
	AdjustSync::ResetOriginalSyncData();

	if (!ARE_STAGE_PLAYER_MODS_FORCED)
	{
		FOREACH_PlayerNumber(p)
		{
			ModsGroup<PlayerOptions>& po = GAMESTATE->m_pPlayerState[p]->m_PlayerOptions;
			po.Assign(ModsLevel_Stage,
				GAMESTATE->m_pPlayerState[p]->m_PlayerOptions.GetPreferred());
		}
	}
	if (!ARE_STAGE_SONG_MODS_FORCED)
		GAMESTATE->m_SongOptions.Assign(ModsLevel_Stage, GAMESTATE->m_SongOptions.GetPreferred());

	STATSMAN->m_CurStageStats.m_fMusicRate = GAMESTATE->m_SongOptions.GetSong().m_fMusicRate;
	m_iNumStagesOfThisSong = GetNumStagesForCurrentSongAndStepsOrCourse();
	ASSERT(m_iNumStagesOfThisSong != -1);

	FOREACH_EnabledPlayer(p)
	{
		// only do this check with human players, assume CPU players (Rave)
		// always have tokens. -aj (this could probably be moved below, even.)
		if (!GAMESTATE->IsEventMode() && !GAMESTATE->IsCpuPlayer(p))
		{
			int player_tokens = GAMESTATE->m_iPlayerStageTokens[p];
			if (player_tokens < m_iNumStagesOfThisSong)
			{
				LuaHelpers::ReportScriptErrorFmt("Player %d only has %d stage tokens, but needs %d.", p, player_tokens, m_iNumStagesOfThisSong);
			}
		}
		GAMESTATE->m_iPlayerStageTokens[p] -= m_iNumStagesOfThisSong;
	}

	FOREACH_HumanPlayer(pn)
	{
		if (GAMESTATE->CurrentOptionsDisqualifyPlayer(pn))
			STATSMAN->m_CurStageStats.m_player[pn].m_bDisqualified = true;
	}

	m_bEarnedExtraStage = false;
	m_sStageGUID = CryptManager::GenerateRandomUUID();
}

void StageProgressionManager::CancelStage()
{
	FOREACH_CpuPlayer(p)
	{
		switch (GAMESTATE->m_PlayMode)
		{
		case PLAY_MODE_BATTLE:
		case PLAY_MODE_RAVE:
			GAMESTATE->m_iPlayerStageTokens[p] = PREFSMAN->m_iSongsPerPlay;
		default:
			break;
		}
	}

	FOREACH_EnabledPlayer(p)
		GAMESTATE->m_iPlayerStageTokens[p] += m_iNumStagesOfThisSong;

	m_iNumStagesOfThisSong = 0;
	GAMESTATE->ResetStageStatistics();
}

void StageProgressionManager::CommitStageStats()
{
	if (m_bDemonstrationOrJukebox)
		return;

	STATSMAN->CommitStatsToProfiles(&STATSMAN->m_CurStageStats);

	// Update TotalPlaySeconds.
	int iPlaySeconds = max(0, (int)m_timeGameStarted.GetDeltaTime());

	Profile* pMachineProfile = PROFILEMAN->GetMachineProfile();
	pMachineProfile->m_iTotalSessionSeconds += iPlaySeconds;

	FOREACH_HumanPlayer(p)
	{
		Profile* pPlayerProfile = PROFILEMAN->GetProfile(p);
		if (pPlayerProfile)
			pPlayerProfile->m_iTotalSessionSeconds += iPlaySeconds;
	}
}

/* Called by ScreenSelectMusic (etc). Increment the stage counter if we just
 * played a song. Might be called more than once. */
void StageProgressionManager::FinishStage()
{
	// Increment the stage counter.
	const int iOldStageIndex = m_iCurrentStageIndex;
	++m_iCurrentStageIndex;

	m_iNumStagesOfThisSong = 0;

	EarnedExtraStage e = CalculateEarnedExtraStage();
	STATSMAN->m_CurStageStats.m_EarnedExtraStage = e;
	if (e != EarnedExtraStage_No)
	{
		LOG->Trace("awarded extra stage");
		FOREACH_HumanPlayer(p)
		{
			// todo: unhardcode the extra stage limit? -aj
			if (m_iAwardedExtraStages[p] < 2)
			{
				++m_iAwardedExtraStages[p];
				++GAMESTATE->m_iPlayerStageTokens[p];
				m_bEarnedExtraStage = true;
			}
		}
	}

	// Save the current combo to the profiles so it can be used for ComboContinuesBetweenSongs.
	FOREACH_HumanPlayer(p)
	{
		Profile* pProfile = PROFILEMAN->GetProfile(p);
		pProfile->m_iCurrentCombo = STATSMAN->m_CurStageStats.m_player[p].m_iCurCombo;
	}

	if (m_bDemonstrationOrJukebox)
		return;

	// todo: simplify. profile saving is accomplished in ScreenProfileSave
	// now; all this code does differently is save machine profile as well. -aj
	if (GAMESTATE->IsEventMode())
	{
		const int iSaveProfileEvery = 3;
		if (iOldStageIndex / iSaveProfileEvery < m_iCurrentStageIndex / iSaveProfileEvery)
		{
			LOG->Trace("Played %i stages; saving profiles ...", iSaveProfileEvery);
			PROFILEMAN->SaveMachineProfile();
			GAMESTATE->SavePlayerProfiles();
		}
	}
}

int StageProgressionManager::GetSmallestNumStagesLeftForAnyHumanPlayer() const
{
	if (GAMESTATE->IsEventMode())
		return 999;
	int iSmallest = INT_MAX;
	FOREACH_HumanPlayer(p)
		iSmallest = min(iSmallest, GAMESTATE->m_iPlayerStageTokens[p]);
	return iSmallest;
}

bool StageProgressionManager::IsFinalStageForAnyHumanPlayer() const
{
	return GetSmallestNumStagesLeftForAnyHumanPlayer() == 1;
}

bool StageProgressionManager::IsFinalStageForEveryHumanPlayer() const
{
	int song_cost = 1;
	if (GAMESTATE->m_pCurSong != nullptr)
	{
		if (GAMESTATE->m_pCurSong->IsLong())
		{
			song_cost = 2;
		}
		else if (GAMESTATE->m_pCurSong->IsMarathon())
		{
			song_cost = 3;
		}
	}
	// If we're on gameplay or evaluation, they set this to false because those
	// screens have already had the stage tokens subtracted.
	song_cost *= m_AdjustTokensBySongCostForFinalStageCheck;
	int num_on_final = 0;
	int num_humans = 0;
	FOREACH_HumanPlayer(p)
	{
		if (GAMESTATE->m_iPlayerStageTokens[p] - song_cost <= 0)
		{
			++num_on_final;
		}
		++num_humans;
	}
	return num_on_final >= num_humans;
}

bool StageProgressionManager::IsAnExtraStage() const
{
	if (GAMESTATE->GetMasterPlayerNumber() == PlayerNumber_Invalid)
		return false;
	return !GAMESTATE->IsEventMode() && !GAMESTATE->IsCourseMode() &&
		m_iAwardedExtraStages[GAMESTATE->GetMasterPlayerNumber()] > 0;
}

bool StageProgressionManager::IsExtraStage() const
{
	if (GAMESTATE->GetMasterPlayerNumber() == PlayerNumber_Invalid)
		return false;
	return !GAMESTATE->IsEventMode() && !GAMESTATE->IsCourseMode() &&
		m_iAwardedExtraStages[GAMESTATE->GetMasterPlayerNumber()] == 1;
}

bool StageProgressionManager::IsExtraStage2() const
{
	if (GAMESTATE->GetMasterPlayerNumber() == PlayerNumber_Invalid)
		return false;
	return !GAMESTATE->IsEventMode() && !GAMESTATE->IsCourseMode() &&
		m_iAwardedExtraStages[GAMESTATE->GetMasterPlayerNumber()] == 2;
}

Stage StageProgressionManager::GetCurrentStage() const
{
	if (m_bDemonstrationOrJukebox)
		return Stage_Demo;
	// "event" has precedence
	else if (GAMESTATE->IsEventMode())
		return Stage_Event;
	else if (GAMESTATE->m_PlayMode == PLAY_MODE_ONI)
		return Stage_Oni;
	else if (GAMESTATE->m_PlayMode == PLAY_MODE_NONSTOP)
		return Stage_Nonstop;
	else if (GAMESTATE->m_PlayMode == PLAY_MODE_ENDLESS)
		return Stage_Endless;
	else if (IsExtraStage())
		return Stage_Extra1;
	else if (IsExtraStage2())
		return Stage_Extra2;
	// Previous logic did not factor in current song length, or the fact that
	// players aren't allowed to start a song with 0 tokens.  This new
	// function also has logic for handling the Gameplay and Evaluation cases
	// which used to require workarounds on the theme side. -Kyz
	else if (IsFinalStageForEveryHumanPlayer())
		return Stage_Final;
	else
	{
		switch (m_iCurrentStageIndex)
		{
		case 0:	return Stage_1st;
		case 1:	return Stage_2nd;
		case 2:	return Stage_3rd;
		case 3:	return Stage_4th;
		case 4:	return Stage_5th;
		case 5:	return Stage_6th;
		default:	return Stage_Next;
		}
	}
}

void StageProgressionManager::SetNewStageSeed()
{
	m_iStageSeed = rand();
}

EarnedExtraStage StageProgressionManager::CalculateEarnedExtraStage() const
{
	if (GAMESTATE->IsEventMode())
		return EarnedExtraStage_No;

	if (!PREFSMAN->m_bAllowExtraStage)
		return EarnedExtraStage_No;

	if (GAMESTATE->m_PlayMode != PLAY_MODE_REGULAR)
		return EarnedExtraStage_No;

	if (m_bBackedOutOfFinalStage)
		return EarnedExtraStage_No;

	if (GetSmallestNumStagesLeftForAnyHumanPlayer() > 0)
		return EarnedExtraStage_No;

	if (m_iAwardedExtraStages[GAMESTATE->GetMasterPlayerNumber()] >= 2)
		return EarnedExtraStage_No;

	FOREACH_EnabledPlayer(pn)
	{
		Difficulty dc = GAMESTATE->m_pCurSteps[pn]->GetDifficulty();
		switch (dc)
		{
		case Difficulty_Edit:
			if (!EDIT_ALLOWED_FOR_EXTRA)
				continue; // can't use edit steps
			break;
		default:
			if (dc < MIN_DIFFICULTY_FOR_EXTRA)
				continue; // not hard enough!
			break;
		}

		if (IsExtraStage())
		{
			if (ALLOW_EXTRA_2 && STATSMAN->m_CurStageStats.m_player[pn].GetGrade() <= GRADE_TIER_FOR_EXTRA_2)
				return EarnedExtraStage_Extra2;
		}
		else if (STATSMAN->m_CurStageStats.m_player[pn].GetGrade() <= GRADE_TIER_FOR_EXTRA_1)
		{
			return EarnedExtraStage_Extra1;
		}
	}

	return EarnedExtraStage_No;
}
