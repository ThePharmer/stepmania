#include "global.h"
#include "SongSelectionState.h"
#include "GameState.h"
#include "Song.h"
#include "Course.h"
#include "Steps.h"
#include "Trail.h"
#include "Style.h"
#include "PrefsManager.h"
#include "CommonMetrics.h"
#include "LuaManager.h"

SongSelectionState::SongSelectionState() :
	m_pCurSong(Message_CurrentSongChanged),
	m_pPreferredSong(nullptr),
	m_pCurCourse(Message_CurrentCourseChanged),
	m_pPreferredCourse(nullptr),
	m_pCurSteps(Message_CurrentStepsP1Changed),
	m_pCurTrail(Message_CurrentTrailP1Changed),
	m_sPreferredSongGroup(Message_PreferredSongGroupChanged),
	m_sPreferredCourseGroup(Message_PreferredCourseGroupChanged),
	m_SortOrder(Message_SortOrderChanged),
	m_PreferredSortOrder(SortOrder_Invalid),
	m_PreferredStepsType(Message_PreferredStepsTypeChanged),
	m_PreferredDifficulty(Message_PreferredDifficultyP1Changed),
	m_PreferredCourseDifficulty(Message_PreferredCourseDifficultyP1Changed)
{
	m_pCurSong.Set(nullptr);
	m_pCurCourse.Set(nullptr);
	FOREACH_PlayerNumber(pn)
	{
		m_pCurSteps[pn].Set(nullptr);
		m_pCurTrail[pn].Set(nullptr);
	}
}

SongSelectionState::~SongSelectionState()
{
}

void SongSelectionState::Reset()
{
	m_pCurSong.Set(GAMESTATE->GetDefaultSong());
	m_pPreferredSong = nullptr;
	m_pCurCourse.Set(nullptr);
	m_pPreferredCourse = nullptr;

	FOREACH_PlayerNumber(pn)
	{
		m_pCurSteps[pn].Set(nullptr);
		m_pCurTrail[pn].Set(nullptr);
		m_PreferredDifficulty[pn].Set(Difficulty_Invalid);
		m_PreferredCourseDifficulty[pn].Set(Difficulty_Medium);
	}

	m_sPreferredSongGroup.Set(GROUP_ALL);
	m_sPreferredCourseGroup.Set(GROUP_ALL);
	m_SortOrder.Set(SortOrder_Invalid);
	m_PreferredSortOrder = GetDefaultSort();
	m_PreferredStepsType.Set(StepsType_Invalid);
}

void SongSelectionState::SetCurrentSong(Song* pSong)
{
	m_pCurSong.Set(pSong);
}

void SongSelectionState::SetCurrentCourse(Course* pCourse)
{
	m_pCurCourse.Set(pCourse);
}

void SongSelectionState::SetCurrentSteps(PlayerNumber pn, Steps* pSteps)
{
	m_pCurSteps[pn].Set(pSteps);
}

void SongSelectionState::SetCurrentTrail(PlayerNumber pn, Trail* pTrail)
{
	m_pCurTrail[pn].Set(pTrail);
}

void SongSelectionState::SetPreferredSongGroup(const RString& sGroup)
{
	m_sPreferredSongGroup.Set(sGroup);
}

void SongSelectionState::SetPreferredCourseGroup(const RString& sGroup)
{
	m_sPreferredCourseGroup.Set(sGroup);
}

void SongSelectionState::SetSortOrder(SortOrder so)
{
	m_SortOrder.Set(so);
}

void SongSelectionState::SetPreferredStepsType(StepsType st)
{
	m_PreferredStepsType.Set(st);
}

void SongSelectionState::SetPreferredDifficulty(PlayerNumber pn, Difficulty dc)
{
	m_PreferredDifficulty[pn].Set(dc);
}

void SongSelectionState::SetPreferredCourseDifficulty(PlayerNumber pn, CourseDifficulty cd)
{
	m_PreferredCourseDifficulty[pn].Set(cd);
}

bool SongSelectionState::ChangePreferredDifficultyAndStepsType(PlayerNumber pn, Difficulty dc, StepsType st)
{
	m_PreferredDifficulty[pn].Set(dc);
	m_PreferredStepsType.Set(st);
	if (GAMESTATE->DifficultiesLocked())
		FOREACH_PlayerNumber(p)
			if (p != pn)
				m_PreferredDifficulty[p].Set(m_PreferredDifficulty[pn]);

	return true;
}

/* When only displaying difficulties in DIFFICULTIES_TO_SHOW, use GetClosestShownDifficulty
 * to find which difficulty to show, and ChangePreferredDifficulty(pn, dir) to change
 * difficulty. */
bool SongSelectionState::ChangePreferredDifficulty(PlayerNumber pn, int dir)
{
	const vector<Difficulty>& v = CommonMetrics::DIFFICULTIES_TO_SHOW.GetValue();

	Difficulty d = GetClosestShownDifficulty(pn);
	for (;;)
	{
		d = enum_add2(d, dir);
		if (d < 0 || d >= NUM_Difficulty)
		{
			return false;
		}
		if (find(v.begin(), v.end(), d) != v.end())
		{
			break; // found
		}
	}
	m_PreferredDifficulty[pn].Set(d);
	return true;
}

/* The user may be set to prefer a difficulty that isn't always shown; typically,
 * Difficulty_Edit. Return the closest shown difficulty <= m_PreferredDifficulty. */
Difficulty SongSelectionState::GetClosestShownDifficulty(PlayerNumber pn) const
{
	const vector<Difficulty>& v = CommonMetrics::DIFFICULTIES_TO_SHOW.GetValue();

	Difficulty iClosest = (Difficulty)0;
	int iClosestDist = -1;
	for (Difficulty const& dc : v)
	{
		int iDist = m_PreferredDifficulty[pn] - dc;
		if (iDist < 0)
			continue;
		if (iClosestDist != -1 && iDist > iClosestDist)
			continue;
		iClosestDist = iDist;
		iClosest = dc;
	}

	return iClosest;
}

bool SongSelectionState::ChangePreferredCourseDifficultyAndStepsType(PlayerNumber pn, CourseDifficulty cd, StepsType st)
{
	m_PreferredCourseDifficulty[pn].Set(cd);
	m_PreferredStepsType.Set(st);
	if (PREFSMAN->m_bLockCourseDifficulties)
		FOREACH_PlayerNumber(p)
			if (p != pn)
				m_PreferredCourseDifficulty[p].Set(m_PreferredCourseDifficulty[pn]);

	return true;
}

bool SongSelectionState::ChangePreferredCourseDifficulty(PlayerNumber pn, int dir)
{
	/* If we have a course selected, only choose among difficulties available in the course. */
	const Course* pCourse = m_pCurCourse;

	const vector<CourseDifficulty>& v = CommonMetrics::COURSE_DIFFICULTIES_TO_SHOW.GetValue();

	CourseDifficulty cd = m_PreferredCourseDifficulty[pn];
	for (;;)
	{
		cd = enum_add2(cd, dir);
		if (cd < 0 || cd >= NUM_Difficulty)
		{
			return false;
		}
		if (find(v.begin(), v.end(), cd) == v.end())
		{
			continue; /* not available */
		}
		if (!pCourse || pCourse->GetTrail(GAMESTATE->GetCurrentStyle(pn)->m_StepsType, cd))
		{
			break;
		}
	}
	m_PreferredCourseDifficulty[pn].Set(cd);
	return true;
}

bool SongSelectionState::IsCourseDifficultyShown(CourseDifficulty cd)
{
	const vector<CourseDifficulty>& v = CommonMetrics::COURSE_DIFFICULTIES_TO_SHOW.GetValue();
	return find(v.begin(), v.end(), cd) != v.end();
}

Difficulty SongSelectionState::GetEasiestStepsDifficulty() const
{
	Difficulty dc = Difficulty_Invalid;
	FOREACH_HumanPlayer(p)
	{
		if (m_pCurSteps[p] == nullptr)
		{
			LuaHelpers::ReportScriptErrorFmt("GetEasiestStepsDifficulty called but p%i hasn't chosen notes", p + 1);
			continue;
		}
		dc = min(dc, m_pCurSteps[p]->GetDifficulty());
	}
	return dc;
}

Difficulty SongSelectionState::GetHardestStepsDifficulty() const
{
	Difficulty dc = Difficulty_Beginner;
	FOREACH_HumanPlayer(p)
	{
		if (m_pCurSteps[p] == nullptr)
		{
			LuaHelpers::ReportScriptErrorFmt("GetHardestStepsDifficulty called but p%i hasn't chosen notes", p + 1);
			continue;
		}
		dc = max(dc, m_pCurSteps[p]->GetDifficulty());
	}
	return dc;
}
