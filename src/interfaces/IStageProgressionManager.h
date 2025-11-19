#ifndef I_STAGE_PROGRESSION_MANAGER_H
#define I_STAGE_PROGRESSION_MANAGER_H

/**
 * @brief Interface for managing stage progression through the game.
 *
 * This interface handles the progression through stages, including
 * tracking current stage, extra stages, and demonstration mode.
 */
class IStageProgressionManager
{
public:
	virtual ~IStageProgressionManager() = default;

	/**
	 * @brief Begin a new stage.
	 */
	virtual void BeginStage() = 0;

	/**
	 * @brief Finish the current stage.
	 */
	virtual void FinishStage() = 0;

	/**
	 * @brief Get the current stage index.
	 * @return The current stage index (0-based).
	 */
	virtual int GetCurrentStageIndex() const = 0;

	/**
	 * @brief Set the current stage index.
	 * @param index The stage index to set.
	 */
	virtual void SetCurrentStageIndex(int index) = 0;

	/**
	 * @brief Get the number of stages for this song.
	 * @return The number of stages.
	 */
	virtual int GetNumStagesOfThisSong() const = 0;

	/**
	 * @brief Set the number of stages for this song.
	 * @param num The number of stages.
	 */
	virtual void SetNumStagesOfThisSong(int num) = 0;

	/**
	 * @brief Check if the current stage is an extra stage.
	 * @return true if in extra stage.
	 */
	virtual bool IsExtraStage() const = 0;

	/**
	 * @brief Check if the current stage is the final stage.
	 * @return true if this is the final stage.
	 */
	virtual bool IsFinalStage() const = 0;

	/**
	 * @brief Check if in demonstration or jukebox mode.
	 * @return true if in demonstration/jukebox mode.
	 */
	virtual bool IsDemonstrationOrJukebox() const = 0;

	/**
	 * @brief Set demonstration or jukebox mode.
	 * @param bDemo true to enable demonstration mode.
	 */
	virtual void SetDemonstrationOrJukebox(bool bDemo) = 0;

	/**
	 * @brief Reset stage progression to initial state.
	 */
	virtual void Reset() = 0;

	/**
	 * @brief Get the total number of stages allowed.
	 * @return The total number of stages.
	 */
	virtual int GetTotalStages() const = 0;
};

#endif // I_STAGE_PROGRESSION_MANAGER_H
