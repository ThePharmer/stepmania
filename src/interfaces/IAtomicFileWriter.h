#ifndef I_ATOMIC_FILE_WRITER_H
#define I_ATOMIC_FILE_WRITER_H

#include "RageUtil.h"

class RageFile;

/**
 * @brief Interface for atomic file writing operations.
 *
 * This interface provides safe file writing that ensures data integrity
 * by using write-to-temp-then-rename pattern. This prevents data loss
 * if a crash occurs during write operations.
 */
class IAtomicFileWriter
{
public:
	virtual ~IAtomicFileWriter() = default;

	/**
	 * @brief Open a temporary file for writing.
	 * @param f The RageFile to open.
	 * @return true if opened successfully.
	 */
	virtual bool Open(RageFile& f) = 0;

	/**
	 * @brief Commit the temporary file to the target location.
	 *
	 * This atomically replaces the target file with the temporary file.
	 * Includes fsync to ensure data is written to disk.
	 *
	 * @param f The RageFile to commit.
	 * @return true if committed successfully.
	 */
	virtual bool Commit(RageFile& f) = 0;

	/**
	 * @brief Get the target file path.
	 * @return The target path.
	 */
	virtual RString GetTargetPath() const = 0;

	/**
	 * @brief Get the temporary file path.
	 * @return The temporary path.
	 */
	virtual RString GetTempPath() const = 0;

	/**
	 * @brief Check if the file has been committed.
	 * @return true if committed.
	 */
	virtual bool IsCommitted() const = 0;
};

#endif // I_ATOMIC_FILE_WRITER_H
