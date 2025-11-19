#ifndef MOCK_SONG_SELECTION_STATE_H
#define MOCK_SONG_SELECTION_STATE_H

#include "interfaces/ISongSelectionState.h"
#include "PlayerNumber.h"

/**
 * @brief Mock implementation of ISongSelectionState for unit testing.
 *
 * This mock provides a simple in-memory implementation of song selection
 * state without requiring the full GameState singleton.
 */
class MockSongSelectionState : public ISongSelectionState
{
private:
	Song* m_pCurrentSong;
	Course* m_pCurrentCourse;
	Steps* m_pCurrentSteps[NUM_PLAYERS];
	Trail* m_pCurrentTrail[NUM_PLAYERS];

public:
	MockSongSelectionState()
		: m_pCurrentSong(nullptr)
		, m_pCurrentCourse(nullptr)
	{
		for (int i = 0; i < NUM_PLAYERS; ++i)
		{
			m_pCurrentSteps[i] = nullptr;
			m_pCurrentTrail[i] = nullptr;
		}
	}

	virtual ~MockSongSelectionState() = default;

	void SetCurrentSong(Song* pSong) override
	{
		m_pCurrentSong = pSong;
	}

	Song* GetCurrentSong() const override
	{
		return m_pCurrentSong;
	}

	void SetCurrentCourse(Course* pCourse) override
	{
		m_pCurrentCourse = pCourse;
	}

	Course* GetCurrentCourse() const override
	{
		return m_pCurrentCourse;
	}

	void SetCurrentSteps(PlayerNumber pn, Steps* pSteps) override
	{
		if (pn >= 0 && pn < NUM_PLAYERS)
		{
			m_pCurrentSteps[pn] = pSteps;
		}
	}

	Steps* GetCurrentSteps(PlayerNumber pn) const override
	{
		if (pn >= 0 && pn < NUM_PLAYERS)
			return m_pCurrentSteps[pn];
		return nullptr;
	}

	void SetCurrentTrail(PlayerNumber pn, Trail* pTrail) override
	{
		if (pn >= 0 && pn < NUM_PLAYERS)
		{
			m_pCurrentTrail[pn] = pTrail;
		}
	}

	Trail* GetCurrentTrail(PlayerNumber pn) const override
	{
		if (pn >= 0 && pn < NUM_PLAYERS)
			return m_pCurrentTrail[pn];
		return nullptr;
	}

	bool HasCurrentSong() const override
	{
		return m_pCurrentSong != nullptr;
	}

	bool HasCurrentCourse() const override
	{
		return m_pCurrentCourse != nullptr;
	}

	void Reset() override
	{
		m_pCurrentSong = nullptr;
		m_pCurrentCourse = nullptr;
		for (int i = 0; i < NUM_PLAYERS; ++i)
		{
			m_pCurrentSteps[i] = nullptr;
			m_pCurrentTrail[i] = nullptr;
		}
	}
};

#endif // MOCK_SONG_SELECTION_STATE_H
