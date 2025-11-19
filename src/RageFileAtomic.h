/* RageFileAtomic - Atomic file writing utilities. */

#ifndef RAGE_FILE_ATOMIC_H
#define RAGE_FILE_ATOMIC_H

#include "RageFile.h"
#include "RageFileManager.h"

/**
 * @brief Atomic file writer that ensures data integrity during saves.
 *
 * This class implements the write-to-temp-then-rename pattern to prevent
 * data loss during file writes. If the application crashes during write,
 * the original file remains intact.
 *
 * Usage:
 * @code
 * AtomicFileWriter writer("path/to/file.xml");
 * RageFile f;
 * if (!writer.Open(f)) {
 *     // Handle error
 *     return false;
 * }
 *
 * // Write data to f
 * f.Write(data, size);
 *
 * // Commit atomically - data is now safely on disk
 * if (!writer.Commit(f)) {
 *     // Handle error - temp file will be cleaned up automatically
 *     return false;
 * }
 * @endcode
 */
class AtomicFileWriter
{
public:
	/**
	 * @brief Construct an atomic file writer.
	 * @param targetPath The final destination path for the file.
	 */
	AtomicFileWriter(const RString& targetPath);

	/**
	 * @brief Destructor - cleans up temp file if not committed.
	 */
	~AtomicFileWriter();

	/**
	 * @brief Open the temporary file for writing.
	 * @param f RageFile object to use for writing.
	 * @return true on success, false on error.
	 */
	bool Open(RageFile& f);

	/**
	 * @brief Commit the write atomically.
	 *
	 * This flushes the file to disk (fsync), closes it, and atomically
	 * renames it to the target path. After this call succeeds, the data
	 * is guaranteed to be on disk.
	 *
	 * @param f RageFile object that was used for writing.
	 * @return true on success, false on error.
	 */
	bool Commit(RageFile& f);

	/**
	 * @brief Get the temporary file path being used.
	 * @return The temporary file path.
	 */
	const RString& GetTempPath() const { return m_sTempPath; }

	/**
	 * @brief Get the target file path.
	 * @return The target file path.
	 */
	const RString& GetTargetPath() const { return m_sTargetPath; }

private:
	RString m_sTargetPath;  // Final destination path
	RString m_sTempPath;    // Temporary file path
	bool m_bCommitted;      // Whether commit has been called successfully

	/**
	 * @brief Generate a unique temporary file suffix.
	 * @return A unique suffix string based on time and randomness.
	 */
	static RString MakeTempSuffix();

	// Prevent copying
	AtomicFileWriter(const AtomicFileWriter&);
	AtomicFileWriter& operator=(const AtomicFileWriter&);
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
