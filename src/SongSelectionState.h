#ifndef SONG_SELECTION_STATE_H
#define SONG_SELECTION_STATE_H

#include "GameConstantsAndTypes.h"
#include "Difficulty.h"
#include "MessageManager.h"
#include "RageUtil.h"

class Song;
class Course;
class Steps;
class Trail;

/**
 * @brief Manages current song/course/steps selection.
 *
 * This class was extracted from GameState as part of the architectural
 * refactoring to decompose the god object. It handles:
 * - Current song and preferred song
 * - Current course and preferred course
 * - Current steps/trail for each player
 * - Preferred groups and sort orders
 * - Preferred difficulty and steps type
 */
class SongSelectionState
{
public:
	SongSelectionState();
	~SongSelectionState();

	// Initialization and Reset
	void Reset();

	// Current Song
	Song* GetCurrentSong() const { return m_pCurSong; }
	void SetCurrentSong(Song* pSong);
	Song* GetPreferredSong() const { return m_pPreferredSong; }
	void SetPreferredSong(Song* pSong) { m_pPreferredSong = pSong; }

	// Current Course
	Course* GetCurrentCourse() const { return m_pCurCourse; }
	void SetCurrentCourse(Course* pCourse);
	Course* GetPreferredCourse() const { return m_pPreferredCourse; }
	void SetPreferredCourse(Course* pCourse) { m_pPreferredCourse = pCourse; }

	// Current Steps
	Steps* GetCurrentSteps(PlayerNumber pn) const { return m_pCurSteps[pn]; }
	void SetCurrentSteps(PlayerNumber pn, Steps* pSteps);

	// Current Trail
	Trail* GetCurrentTrail(PlayerNumber pn) const { return m_pCurTrail[pn]; }
	void SetCurrentTrail(PlayerNumber pn, Trail* pTrail);

	// Preferred Groups
	const RString& GetPreferredSongGroup() const { return m_sPreferredSongGroup; }
	void SetPreferredSongGroup(const RString& sGroup);
	const RString& GetPreferredCourseGroup() const { return m_sPreferredCourseGroup; }
	void SetPreferredCourseGroup(const RString& sGroup);

	// Sort Order
	SortOrder GetSortOrder() const { return m_SortOrder; }
	void SetSortOrder(SortOrder so);
	SortOrder GetPreferredSortOrder() const { return m_PreferredSortOrder; }
	void SetPreferredSortOrder(SortOrder so) { m_PreferredSortOrder = so; }

	// Preferred StepsType
	StepsType GetPreferredStepsType() const { return m_PreferredStepsType; }
	void SetPreferredStepsType(StepsType st);

	// Preferred Difficulty
	Difficulty GetPreferredDifficulty(PlayerNumber pn) const { return m_PreferredDifficulty[pn]; }
	void SetPreferredDifficulty(PlayerNumber pn, Difficulty dc);
	CourseDifficulty GetPreferredCourseDifficulty(PlayerNumber pn) const { return m_PreferredCourseDifficulty[pn]; }
	void SetPreferredCourseDifficulty(PlayerNumber pn, CourseDifficulty cd);

	// Difficulty Changing
	bool ChangePreferredDifficultyAndStepsType(PlayerNumber pn, Difficulty dc, StepsType st);
	bool ChangePreferredDifficulty(PlayerNumber pn, int dir);
	bool ChangePreferredCourseDifficultyAndStepsType(PlayerNumber pn, CourseDifficulty cd, StepsType st);
	bool ChangePreferredCourseDifficulty(PlayerNumber pn, int dir);
	bool IsCourseDifficultyShown(CourseDifficulty cd);
	Difficulty GetClosestShownDifficulty(PlayerNumber pn) const;
	Difficulty GetEasiestStepsDifficulty() const;
	Difficulty GetHardestStepsDifficulty() const;

	// BroadcastOnChange wrappers for external access
	BroadcastOnChangePtr<Song>& GetCurSongBroadcast() { return m_pCurSong; }
	BroadcastOnChangePtr<Course>& GetCurCourseBroadcast() { return m_pCurCourse; }
	BroadcastOnChangePtr1D<Steps, NUM_PLAYERS>& GetCurStepsBroadcast() { return m_pCurSteps; }
	BroadcastOnChangePtr1D<Trail, NUM_PLAYERS>& GetCurTrailBroadcast() { return m_pCurTrail; }
	BroadcastOnChange<RString>& GetPreferredSongGroupBroadcast() { return m_sPreferredSongGroup; }
	BroadcastOnChange<RString>& GetPreferredCourseGroupBroadcast() { return m_sPreferredCourseGroup; }
	BroadcastOnChange<StepsType>& GetPreferredStepsTypeBroadcast() { return m_PreferredStepsType; }
	BroadcastOnChange1D<Difficulty, NUM_PLAYERS>& GetPreferredDifficultyBroadcast() { return m_PreferredDifficulty; }
	BroadcastOnChange1D<CourseDifficulty, NUM_PLAYERS>& GetPreferredCourseDifficultyBroadcast() { return m_PreferredCourseDifficulty; }
	BroadcastOnChange<SortOrder>& GetSortOrderBroadcast() { return m_SortOrder; }

private:
	// Current song/course selection
	BroadcastOnChangePtr<Song> m_pCurSong;
	Song* m_pPreferredSong;
	BroadcastOnChangePtr<Course> m_pCurCourse;
	Course* m_pPreferredCourse;

	// Current steps/trail for each player
	BroadcastOnChangePtr1D<Steps, NUM_PLAYERS> m_pCurSteps;
	BroadcastOnChangePtr1D<Trail, NUM_PLAYERS> m_pCurTrail;

	// Preferred groups
	BroadcastOnChange<RString> m_sPreferredSongGroup;
	BroadcastOnChange<RString> m_sPreferredCourseGroup;

	// Sort order
	BroadcastOnChange<SortOrder> m_SortOrder;
	SortOrder m_PreferredSortOrder;

	// Preferred difficulty and steps type
	BroadcastOnChange<StepsType> m_PreferredStepsType;
	BroadcastOnChange1D<Difficulty, NUM_PLAYERS> m_PreferredDifficulty;
	BroadcastOnChange1D<CourseDifficulty, NUM_PLAYERS> m_PreferredCourseDifficulty;

	SongSelectionState(const SongSelectionState& rhs);
	SongSelectionState& operator=(const SongSelectionState& rhs);
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
