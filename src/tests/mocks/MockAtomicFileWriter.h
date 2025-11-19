#ifndef MOCK_ATOMIC_FILE_WRITER_H
#define MOCK_ATOMIC_FILE_WRITER_H

#include "interfaces/IAtomicFileWriter.h"
#include <string>

/**
 * @brief Mock implementation of IAtomicFileWriter for unit testing.
 *
 * This mock simulates atomic file writing without actually writing to disk,
 * allowing tests to verify the logic without file I/O.
 */
class MockAtomicFileWriter : public IAtomicFileWriter
{
private:
	RString m_sTargetPath;
	RString m_sTempPath;
	bool m_bCommitted;
	bool m_bOpened;
	bool m_bShouldFailOpen;
	bool m_bShouldFailCommit;

public:
	MockAtomicFileWriter(const RString& targetPath)
		: m_sTargetPath(targetPath)
		, m_sTempPath(targetPath + ".tmp.mock")
		, m_bCommitted(false)
		, m_bOpened(false)
		, m_bShouldFailOpen(false)
		, m_bShouldFailCommit(false)
	{
	}

	virtual ~MockAtomicFileWriter() = default;

	bool Open(RageFile& f) override
	{
		if (m_bShouldFailOpen)
			return false;

		m_bOpened = true;
		return true;
	}

	bool Commit(RageFile& f) override
	{
		if (!m_bOpened)
			return false;

		if (m_bShouldFailCommit)
			return false;

		if (m_bCommitted)
			return false;

		m_bCommitted = true;
		return true;
	}

	RString GetTargetPath() const override
	{
		return m_sTargetPath;
	}

	RString GetTempPath() const override
	{
		return m_sTempPath;
	}

	bool IsCommitted() const override
	{
		return m_bCommitted;
	}

	// Test helper methods to simulate failures
	void SetShouldFailOpen(bool bFail) { m_bShouldFailOpen = bFail; }
	void SetShouldFailCommit(bool bFail) { m_bShouldFailCommit = bFail; }
	bool IsOpened() const { return m_bOpened; }
};

#endif // MOCK_ATOMIC_FILE_WRITER_H
