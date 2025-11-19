/* RageFileLock - Advisory file locking to prevent concurrent writes. */

#ifndef RAGE_FILE_LOCK_H
#define RAGE_FILE_LOCK_H

#include "RageString.hpp"

/**
 * @brief Advisory file lock to prevent concurrent file access.
 *
 * This class provides cross-platform advisory file locking using:
 * - flock() on POSIX systems (Linux, macOS, etc.)
 * - LockFileEx() on Windows
 *
 * Advisory locks prevent multiple instances of StepMania from corrupting
 * files by writing to them simultaneously. The lock is automatically
 * released when the RageFileLock object is destroyed.
 *
 * Usage:
 * @code
 * RageFileLock lock("path/to/file.xml");
 * if (!lock.Lock()) {
 *     LOG->Warn("Could not acquire lock, another instance may be writing");
 *     return false;
 * }
 *
 * // Perform file operations
 * // ...
 *
 * // Lock is automatically released when 'lock' goes out of scope
 * @endcode
 */
class RageFileLock
{
public:
	/**
	 * @brief Construct a file lock for the given path.
	 * @param sPath The path to lock (a .lock file will be created).
	 */
	explicit RageFileLock(const RString& sPath);

	/**
	 * @brief Destructor - automatically releases the lock.
	 */
	~RageFileLock();

	/**
	 * @brief Acquire an exclusive lock on the file.
	 *
	 * This will block briefly if another process holds the lock, then
	 * fail if the lock cannot be acquired within a timeout period.
	 *
	 * @param bWait If true, wait for lock. If false, fail immediately if locked.
	 * @return true if lock acquired, false otherwise.
	 */
	bool Lock(bool bWait = true);

	/**
	 * @brief Release the lock.
	 *
	 * The lock is automatically released in the destructor, but this
	 * can be called to release it earlier.
	 */
	void Unlock();

	/**
	 * @brief Check if this object currently holds the lock.
	 * @return true if locked, false otherwise.
	 */
	bool IsLocked() const { return m_bLocked; }

	/**
	 * @brief Get the lock file path.
	 * @return The lock file path.
	 */
	const RString& GetLockPath() const { return m_sLockPath; }

private:
	RString m_sLockPath;  // Path to the lock file
	bool m_bLocked;       // Whether we currently hold the lock
	int m_iFD;            // File descriptor (POSIX) or -1 if not locked

#if defined(_WIN32)
	void* m_hFile;        // HANDLE on Windows (stored as void* to avoid including windows.h)
#endif

	// Prevent copying
	RageFileLock(const RageFileLock&);
	RageFileLock& operator=(const RageFileLock&);
};

#endif

/*
 * Copyright (c) 2025 StepMania Contributors
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
