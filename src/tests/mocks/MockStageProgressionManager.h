#ifndef MOCK_STAGE_PROGRESSION_MANAGER_H
#define MOCK_STAGE_PROGRESSION_MANAGER_H

#include "interfaces/IStageProgressionManager.h"

/**
 * @brief Mock implementation of IStageProgressionManager for unit testing.
 *
 * This mock provides simple stage progression tracking without requiring
 * the full GameState singleton or engine state.
 */
class MockStageProgressionManager : public IStageProgressionManager
{
private:
	int m_iCurrentStageIndex;
	int m_iNumStagesOfThisSong;
	int m_iTotalStages;
	bool m_bDemonstrationOrJukebox;
	bool m_bInStage;

public:
	MockStageProgressionManager()
		: m_iCurrentStageIndex(0)
		, m_iNumStagesOfThisSong(1)
		, m_iTotalStages(3)
		, m_bDemonstrationOrJukebox(false)
		, m_bInStage(false)
	{
	}

	virtual ~MockStageProgressionManager() = default;

	void BeginStage() override
	{
		m_bInStage = true;
	}

	void FinishStage() override
	{
		if (m_bInStage)
		{
			m_iCurrentStageIndex++;
			m_bInStage = false;
		}
	}

	int GetCurrentStageIndex() const override
	{
		return m_iCurrentStageIndex;
	}

	void SetCurrentStageIndex(int index) override
	{
		m_iCurrentStageIndex = index;
	}

	int GetNumStagesOfThisSong() const override
	{
		return m_iNumStagesOfThisSong;
	}

	void SetNumStagesOfThisSong(int num) override
	{
		m_iNumStagesOfThisSong = num;
	}

	bool IsExtraStage() const override
	{
		return m_iCurrentStageIndex >= m_iTotalStages;
	}

	bool IsFinalStage() const override
	{
		return m_iCurrentStageIndex == (m_iTotalStages - 1);
	}

	bool IsDemonstrationOrJukebox() const override
	{
		return m_bDemonstrationOrJukebox;
	}

	void SetDemonstrationOrJukebox(bool bDemo) override
	{
		m_bDemonstrationOrJukebox = bDemo;
	}

	void Reset() override
	{
		m_iCurrentStageIndex = 0;
		m_iNumStagesOfThisSong = 1;
		m_bDemonstrationOrJukebox = false;
		m_bInStage = false;
	}

	int GetTotalStages() const override
	{
		return m_iTotalStages;
	}

	// Test helper methods
	void SetTotalStages(int total) { m_iTotalStages = total; }
	bool IsInStage() const { return m_bInStage; }
};

#endif // MOCK_STAGE_PROGRESSION_MANAGER_H
