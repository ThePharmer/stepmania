#include "global.h"
#include "RageFileAtomic.h"
#include "RageUtil.h"
#include "RageLog.h"

#include <ctime>
#include <cstdlib>

// Platform-specific includes for fsync
#if defined(_WIN32)
	#include <windows.h>
	#include <io.h>
#else
	#include <unistd.h>
#endif

AtomicFileWriter::AtomicFileWriter(const RString& targetPath)
	: m_sTargetPath(targetPath)
	, m_sTempPath(targetPath + ".tmp." + MakeTempSuffix())
	, m_bCommitted(false)
{
}

AtomicFileWriter::~AtomicFileWriter()
{
	// Cleanup temp file if not committed
	if (!m_bCommitted && FILEMAN->IsAFile(m_sTempPath))
	{
		if (!FILEMAN->Remove(m_sTempPath))
		{
			LOG->Warn("AtomicFileWriter: Failed to remove temp file '%s'",
				m_sTempPath.c_str());
		}
	}
}

bool AtomicFileWriter::Open(RageFile& f)
{
	// Open the temporary file for writing
	if (!f.Open(m_sTempPath, RageFile::WRITE))
	{
		LOG->Warn("AtomicFileWriter: Failed to open temp file '%s': %s",
			m_sTempPath.c_str(), f.GetError().c_str());
		return false;
	}

	return true;
}

bool AtomicFileWriter::Commit(RageFile& f)
{
	if (m_bCommitted)
	{
		LOG->Warn("AtomicFileWriter: Commit called twice for '%s'",
			m_sTargetPath.c_str());
		return false;
	}

	// Step 1: Flush to OS buffers
	if (f.Flush() == -1)
	{
		LOG->Warn("AtomicFileWriter: Flush failed for '%s': %s",
			m_sTempPath.c_str(), f.GetError().c_str());
		return false;
	}

	// Step 2: Sync to disk (platform-specific fsync)
	int fd = f.GetFD();
	if (fd != -1)
	{
#if defined(_WIN32)
		// Windows: Use FlushFileBuffers
		HANDLE h = (HANDLE)_get_osfhandle(fd);
		if (h == INVALID_HANDLE_VALUE)
		{
			LOG->Warn("AtomicFileWriter: Invalid file handle for '%s'",
				m_sTempPath.c_str());
			return false;
		}

		if (!FlushFileBuffers(h))
		{
			LOG->Warn("AtomicFileWriter: FlushFileBuffers failed for '%s' (error %d)",
				m_sTempPath.c_str(), GetLastError());
			return false;
		}
#else
		// POSIX: Use fsync
		if (fsync(fd) == -1)
		{
			LOG->Warn("AtomicFileWriter: fsync failed for '%s': %s",
				m_sTempPath.c_str(), strerror(errno));
			return false;
		}
#endif
	}
	else
	{
		// File descriptor not available - this may be normal for some file types
		// (e.g., files in memory or archives), so we'll log a trace but not fail
		LOG->Trace("AtomicFileWriter: No file descriptor available for '%s', "
			"skipping fsync", m_sTempPath.c_str());
	}

	// Step 3: Close the file
	f.Close();

	// Step 4: Atomic rename (POSIX rename() is atomic, Windows MoveFile is atomic)
	// Note: FILEMAN->Move handles cross-platform differences
	if (!FILEMAN->Move(m_sTempPath, m_sTargetPath))
	{
		LOG->Warn("AtomicFileWriter: Failed to rename '%s' to '%s'",
			m_sTempPath.c_str(), m_sTargetPath.c_str());
		return false;
	}

	m_bCommitted = true;
	LOG->Trace("AtomicFileWriter: Successfully committed '%s'", m_sTargetPath.c_str());
	return true;
}

RString AtomicFileWriter::MakeTempSuffix()
{
	// Create a unique suffix using timestamp and random number
	// This helps prevent collisions if multiple writes happen quickly
	unsigned int timestamp = static_cast<unsigned int>(time(nullptr));
	int random = rand() % 10000;
	return ssprintf("%u%04d", timestamp, random);
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
