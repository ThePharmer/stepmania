#ifndef I_SONG_SELECTION_STATE_H
#define I_SONG_SELECTION_STATE_H

#include "PlayerNumber.h"

class Song;
class Steps;
class Course;
class Trail;

/**
 * @brief Interface for managing song and course selection state.
 *
 * This interface provides abstraction for song selection logic,
 * enabling testing without full engine dependencies.
 */
class ISongSelectionState
{
public:
	virtual ~ISongSelectionState() = default;

	/**
	 * @brief Set the currently selected song.
	 * @param pSong Pointer to the song to select.
	 */
	virtual void SetCurrentSong(Song* pSong) = 0;

	/**
	 * @brief Get the currently selected song.
	 * @return Pointer to the current song, or nullptr if none selected.
	 */
	virtual Song* GetCurrentSong() const = 0;

	/**
	 * @brief Set the currently selected course.
	 * @param pCourse Pointer to the course to select.
	 */
	virtual void SetCurrentCourse(Course* pCourse) = 0;

	/**
	 * @brief Get the currently selected course.
	 * @return Pointer to the current course, or nullptr if none selected.
	 */
	virtual Course* GetCurrentCourse() const = 0;

	/**
	 * @brief Set the steps for a specific player.
	 * @param pn The player number.
	 * @param pSteps Pointer to the steps to set.
	 */
	virtual void SetCurrentSteps(PlayerNumber pn, Steps* pSteps) = 0;

	/**
	 * @brief Get the steps for a specific player.
	 * @param pn The player number.
	 * @return Pointer to the player's steps, or nullptr if not set.
	 */
	virtual Steps* GetCurrentSteps(PlayerNumber pn) const = 0;

	/**
	 * @brief Set the trail for a specific player.
	 * @param pn The player number.
	 * @param pTrail Pointer to the trail to set.
	 */
	virtual void SetCurrentTrail(PlayerNumber pn, Trail* pTrail) = 0;

	/**
	 * @brief Get the trail for a specific player.
	 * @param pn The player number.
	 * @return Pointer to the player's trail, or nullptr if not set.
	 */
	virtual Trail* GetCurrentTrail(PlayerNumber pn) const = 0;

	/**
	 * @brief Check if any song is currently selected.
	 * @return true if a song is selected.
	 */
	virtual bool HasCurrentSong() const = 0;

	/**
	 * @brief Check if any course is currently selected.
	 * @return true if a course is selected.
	 */
	virtual bool HasCurrentCourse() const = 0;

	/**
	 * @brief Reset all selection state to initial values.
	 */
	virtual void Reset() = 0;
};

#endif // I_SONG_SELECTION_STATE_H
