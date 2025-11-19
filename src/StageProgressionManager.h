#ifndef STAGE_PROGRESSION_MANAGER_H
#define STAGE_PROGRESSION_MANAGER_H

#include "GameConstantsAndTypes.h"
#include "RageTimer.h"
#include "RageUtil.h"

class Song;

/**
 * @brief Manages stage progression, extra stages, and game flow.
 *
 * This class was extracted from GameState as part of the architectural
 * refactoring to decompose the god object. It handles:
 * - Stage counting and progression
 * - Extra stage awards
 * - Demo/jukebox mode
 * - Stage lifecycle (BeginStage, FinishStage, etc.)
 */
class StageProgressionManager
{
public:
	StageProgressionManager();
	~StageProgressionManager();

	// Initialization and Reset
	void Reset();

	// Stage Lifecycle
	void BeginStage();
	void CancelStage();
	void CommitStageStats();
	void FinishStage();

	// Stage Index
	int GetCurrentStageIndex() const { return m_iCurrentStageIndex; }
	void SetCurrentStageIndex(int i) { m_iCurrentStageIndex = i; }
	void IncrementStageIndex() { ++m_iCurrentStageIndex; }

	// Stage Counting
	int GetNumStagesOfThisSong() const { return m_iNumStagesOfThisSong; }
	void SetNumStagesOfThisSong(int n) { m_iNumStagesOfThisSong = n; }

	// Demo/Jukebox Mode
	bool IsDemonstrationOrJukebox() const { return m_bDemonstrationOrJukebox; }
	void SetDemonstrationOrJukebox(bool b) { m_bDemonstrationOrJukebox = b; }
	bool JukeboxUsesModifiers() const { return m_bJukeboxUsesModifiers; }
	void SetJukeboxUsesModifiers(bool b) { m_bJukeboxUsesModifiers = b; }

	// Extra Stages
	bool IsAnExtraStage() const;
	bool IsExtraStage() const;
	bool IsExtraStage2() const;
	bool HasEarnedExtraStage() const { return m_bEarnedExtraStage; }
	void SetEarnedExtraStage(bool b) { m_bEarnedExtraStage = b; }
	int GetAwardedExtraStages(PlayerNumber pn) const { return m_iAwardedExtraStages[pn]; }
	void SetAwardedExtraStages(PlayerNumber pn, int n) { m_iAwardedExtraStages[pn] = n; }
	void IncrementAwardedExtraStages(PlayerNumber pn) { ++m_iAwardedExtraStages[pn]; }

	// Stage Tokens/Final Stage
	bool IsFinalStageForAnyHumanPlayer() const;
	bool IsFinalStageForEveryHumanPlayer() const;
	int GetSmallestNumStagesLeftForAnyHumanPlayer() const;
	bool GetAdjustTokensBySongCostForFinalStageCheck() const { return m_AdjustTokensBySongCostForFinalStageCheck; }
	void SetAdjustTokensBySongCostForFinalStageCheck(bool b) { m_AdjustTokensBySongCostForFinalStageCheck = b; }

	// Current Stage
	Stage GetCurrentStage() const;

	// Backed Out
	bool GetBackedOutOfFinalStage() const { return m_bBackedOutOfFinalStage; }
	void SetBackedOutOfFinalStage(bool b) { m_bBackedOutOfFinalStage = b; }

	// Game Start Time
	const RageTimer& GetTimeGameStarted() const { return m_timeGameStarted; }
	void TouchTimeGameStarted() { m_timeGameStarted.Touch(); }
	void SetTimeGameStartedToZero() { m_timeGameStarted.SetZero(); }

	// Seeds
	int GetGameSeed() const { return m_iGameSeed; }
	void SetGameSeed(int seed) { m_iGameSeed = seed; }
	int GetStageSeed() const { return m_iStageSeed; }
	void SetStageSeed(int seed) { m_iStageSeed = seed; }
	void SetNewStageSeed();

	// Stage GUID
	const RString& GetStageGUID() const { return m_sStageGUID; }
	void SetStageGUID(const RString& guid) { m_sStageGUID = guid; }

	// Static helpers
	static int GetNumStagesMultiplierForSong(const Song* pSong);
	static int GetNumStagesForSongAndStyleType(const Song* pSong, StyleType st);
	int GetNumStagesForCurrentSongAndStepsOrCourse() const;

private:
	// Stage progression
	int m_iCurrentStageIndex;
	int m_iNumStagesOfThisSong;

	// Demo/Jukebox
	bool m_bDemonstrationOrJukebox;
	bool m_bJukeboxUsesModifiers;

	// Extra stages
	int m_iAwardedExtraStages[NUM_PLAYERS];
	bool m_bEarnedExtraStage;
	bool m_bBackedOutOfFinalStage;

	// Final stage checking
	bool m_AdjustTokensBySongCostForFinalStageCheck;

	// Game timing
	RageTimer m_timeGameStarted;

	// Seeds for randomization
	int m_iGameSeed;
	int m_iStageSeed;

	// Stage identifier
	RString m_sStageGUID;

	// Helper for extra stage calculation
	EarnedExtraStage CalculateEarnedExtraStage() const;

	StageProgressionManager(const StageProgressionManager& rhs);
	StageProgressionManager& operator=(const StageProgressionManager& rhs);
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
