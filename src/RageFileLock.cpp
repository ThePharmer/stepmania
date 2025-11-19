#include "global.h"
#include "RageFileLock.h"
#include "RageLog.h"
#include "RageUtil.h"
#include "RageFileManager.h"

#include <cerrno>
#include <cstring>

// Platform-specific includes
#if defined(_WIN32)
	#include <windows.h>
	#include <io.h>
	#define INVALID_FD -1
#else
	#include <unistd.h>
	#include <fcntl.h>
	#include <sys/file.h>
	#define INVALID_FD -1
#endif

RageFileLock::RageFileLock(const RString& sPath)
	: m_sLockPath(sPath + ".lock")
	, m_bLocked(false)
	, m_iFD(INVALID_FD)
#if defined(_WIN32)
	, m_hFile(INVALID_HANDLE_VALUE)
#endif
{
}

RageFileLock::~RageFileLock()
{
	Unlock();
}

bool RageFileLock::Lock(bool bWait)
{
	if (m_bLocked)
	{
		LOG->Warn("RageFileLock: Lock already held for '%s'", m_sLockPath.c_str());
		return true;  // Already locked by us
	}

#if defined(_WIN32)
	// Windows implementation using LockFileEx

	// Get the full path for Windows API
	RString sFullPath = FILEMAN->ResolvePath(m_sLockPath);

	// Open/create the lock file
	m_hFile = CreateFileA(
		sFullPath.c_str(),
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,  // Allow others to open, but not lock
		nullptr,
		OPEN_ALWAYS,  // Create if doesn't exist
		FILE_ATTRIBUTE_NORMAL,
		nullptr
	);

	if (m_hFile == INVALID_HANDLE_VALUE)
	{
		LOG->Warn("RageFileLock: Failed to open lock file '%s' (error %d)",
			m_sLockPath.c_str(), GetLastError());
		return false;
	}

	// Try to lock the file
	OVERLAPPED overlapped = {};
	DWORD dwFlags = LOCKFILE_EXCLUSIVE_LOCK;
	if (!bWait)
		dwFlags |= LOCKFILE_FAIL_IMMEDIATELY;

	if (!LockFileEx(m_hFile, dwFlags, 0, 1, 0, &overlapped))
	{
		DWORD dwError = GetLastError();
		if (dwError == ERROR_LOCK_VIOLATION || dwError == ERROR_IO_PENDING)
		{
			LOG->Trace("RageFileLock: File '%s' is locked by another process",
				m_sLockPath.c_str());
		}
		else
		{
			LOG->Warn("RageFileLock: LockFileEx failed for '%s' (error %d)",
				m_sLockPath.c_str(), dwError);
		}

		CloseHandle(m_hFile);
		m_hFile = INVALID_HANDLE_VALUE;
		return false;
	}

	m_bLocked = true;
	LOG->Trace("RageFileLock: Acquired lock for '%s'", m_sLockPath.c_str());
	return true;

#else
	// POSIX implementation using flock

	// Get the full path
	RString sFullPath = FILEMAN->ResolvePath(m_sLockPath);

	// Open/create the lock file
	m_iFD = open(sFullPath.c_str(), O_RDWR | O_CREAT, 0644);
	if (m_iFD == INVALID_FD)
	{
		LOG->Warn("RageFileLock: Failed to open lock file '%s': %s",
			m_sLockPath.c_str(), strerror(errno));
		return false;
	}

	// Try to acquire the lock
	int iFlags = LOCK_EX;  // Exclusive lock
	if (!bWait)
		iFlags |= LOCK_NB;  // Non-blocking

	if (flock(m_iFD, iFlags) == -1)
	{
		if (errno == EWOULDBLOCK)
		{
			LOG->Trace("RageFileLock: File '%s' is locked by another process",
				m_sLockPath.c_str());
		}
		else
		{
			LOG->Warn("RageFileLock: flock failed for '%s': %s",
				m_sLockPath.c_str(), strerror(errno));
		}

		close(m_iFD);
		m_iFD = INVALID_FD;
		return false;
	}

	m_bLocked = true;
	LOG->Trace("RageFileLock: Acquired lock for '%s'", m_sLockPath.c_str());
	return true;
#endif
}

void RageFileLock::Unlock()
{
	if (!m_bLocked)
		return;

#if defined(_WIN32)
	if (m_hFile != INVALID_HANDLE_VALUE)
	{
		// Unlock the file
		OVERLAPPED overlapped = {};
		UnlockFileEx(m_hFile, 0, 1, 0, &overlapped);

		// Close the handle
		CloseHandle(m_hFile);
		m_hFile = INVALID_HANDLE_VALUE;
	}
#else
	if (m_iFD != INVALID_FD)
	{
		// Release the lock
		flock(m_iFD, LOCK_UN);

		// Close the file descriptor
		close(m_iFD);
		m_iFD = INVALID_FD;
	}
#endif

	m_bLocked = false;
	LOG->Trace("RageFileLock: Released lock for '%s'", m_sLockPath.c_str());
}

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
