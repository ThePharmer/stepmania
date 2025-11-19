#ifndef MSD_LOADER_HELPERS_H
#define MSD_LOADER_HELPERS_H

#include "MsdFile.h"
#include "RageUtil.h"

class Song;
class SMLoader;
class TimingData;

/**
 * @brief Base structure for MSD tag parsing information
 *
 * This provides common functionality for both SM and SSC loaders,
 * reducing code duplication across different MSD-based file formats.
 */
template<typename LoaderType>
struct MsdSongTagInfo
{
	LoaderType* loader;
	Song* song;
	const MsdFile::value_t* params;
	const RString& path;
	bool from_cache;

	MsdSongTagInfo(LoaderType* l, Song* s, const RString& p, bool fc = false)
		: loader(l), song(s), path(p), from_cache(fc)
	{}
};

/**
 * @brief Common tag handler functions for MSD-based loaders
 *
 * These template functions work with any loader type (SMLoader, SSCLoader, etc.)
 * and eliminate duplicate code between different MSD file format parsers.
 */
namespace MsdTagHandlers
{
	// Basic song metadata handlers
	template<typename TagInfo>
	void SetTitle(TagInfo& info)
	{
		info.song->m_sMainTitle = (*info.params)[1];
		info.loader->SetSongTitle((*info.params)[1]);
	}

	template<typename TagInfo>
	void SetSubtitle(TagInfo& info)
	{
		info.song->m_sSubTitle = (*info.params)[1];
	}

	template<typename TagInfo>
	void SetArtist(TagInfo& info)
	{
		info.song->m_sArtist = (*info.params)[1];
	}

	template<typename TagInfo>
	void SetTitleTranslit(TagInfo& info)
	{
		info.song->m_sMainTitleTranslit = (*info.params)[1];
	}

	template<typename TagInfo>
	void SetSubtitleTranslit(TagInfo& info)
	{
		info.song->m_sSubTitleTranslit = (*info.params)[1];
	}

	template<typename TagInfo>
	void SetArtistTranslit(TagInfo& info)
	{
		info.song->m_sArtistTranslit = (*info.params)[1];
	}

	template<typename TagInfo>
	void SetGenre(TagInfo& info)
	{
		info.song->m_sGenre = (*info.params)[1];
	}

	template<typename TagInfo>
	void SetCredit(TagInfo& info)
	{
		info.song->m_sCredit = (*info.params)[1];
		Trim(info.song->m_sCredit);
	}

	// File path handlers
	template<typename TagInfo>
	void SetBanner(TagInfo& info)
	{
		info.song->m_sBannerFile = (*info.params)[1];
	}

	template<typename TagInfo>
	void SetBackground(TagInfo& info)
	{
		info.song->m_sBackgroundFile = (*info.params)[1];
	}

	template<typename TagInfo>
	void SetLyricsPath(TagInfo& info)
	{
		info.song->m_sLyricsFile = (*info.params)[1];
	}

	template<typename TagInfo>
	void SetCDTitle(TagInfo& info)
	{
		info.song->m_sCDTitleFile = (*info.params)[1];
	}

	template<typename TagInfo>
	void SetMusic(TagInfo& info)
	{
		info.song->m_sMusicFile = (*info.params)[1];
	}

	// Timing and audio handlers
	template<typename TagInfo>
	void SetSampleStart(TagInfo& info)
	{
		info.song->m_fMusicSampleStartSeconds = HHMMSSToSeconds((*info.params)[1]);
	}

	template<typename TagInfo>
	void SetSampleLength(TagInfo& info)
	{
		info.song->m_fMusicSampleLengthSeconds = HHMMSSToSeconds((*info.params)[1]);
	}

	template<typename TagInfo>
	void SetDisplayBPM(TagInfo& info)
	{
		// #DISPLAYBPM:[xxx][xxx:xxx]|[*];
		if((*info.params)[1] == "*")
		{
			info.song->m_DisplayBPMType = DISPLAY_BPM_RANDOM;
		}
		else
		{
			info.song->m_DisplayBPMType = DISPLAY_BPM_SPECIFIED;
			info.song->m_fSpecifiedBPMMin = StringToFloat((*info.params)[1]);
			if((*info.params)[2].empty())
			{
				info.song->m_fSpecifiedBPMMax = info.song->m_fSpecifiedBPMMin;
			}
			else
			{
				info.song->m_fSpecifiedBPMMax = StringToFloat((*info.params)[2]);
			}
		}
	}

	template<typename TagInfo>
	void SetSelectable(TagInfo& info)
	{
		if((*info.params)[1].EqualsNoCase("YES"))
		{
			info.song->m_SelectionDisplay = info.song->SHOW_ALWAYS;
		}
		else if((*info.params)[1].EqualsNoCase("NO"))
		{
			info.song->m_SelectionDisplay = info.song->SHOW_NEVER;
		}
		// ROULETTE from 3.9. It was removed since UnlockManager can serve
		// the same purpose somehow. This, of course, assumes you're using
		// unlocks. -aj
		else if((*info.params)[1].EqualsNoCase("ROULETTE"))
		{
			info.song->m_SelectionDisplay = info.song->SHOW_ALWAYS;
		}
		/* The following two cases are just fixes to make sure simfiles that
		 * used 3.9+ features are not excluded here */
		else if((*info.params)[1].EqualsNoCase("ES") || (*info.params)[1].EqualsNoCase("OMES"))
		{
			info.song->m_SelectionDisplay = info.song->SHOW_ALWAYS;
		}
		else if(StringToInt((*info.params)[1]) > 0)
		{
			info.song->m_SelectionDisplay = info.song->SHOW_ALWAYS;
		}
		else
		{
			LOG->UserLog("Song file", info.path, "has an unknown #SELECTABLE value, \"%s\"; ignored.",
				(*info.params)[1].c_str());
		}
	}

	template<typename TagInfo>
	void SetKeysounds(TagInfo& info)
	{
		RString keysounds = (*info.params)[1];
		// Handle SSC format escaped hash
		if(keysounds.length() >= 2 && keysounds.substr(0, 2) == "\\#")
		{
			keysounds = keysounds.substr(1);
		}
		split(keysounds, ",", info.song->m_vsKeysoundFile);
	}
}

/**
 * @brief Helper functions for common MSD parsing operations
 */
namespace MsdParsingHelpers
{
	/**
	 * @brief Parse a beat=value pair from an MSD string
	 * @param expression The expression string (e.g., "4.0=120.0")
	 * @param outBeat The parsed beat value
	 * @param outValue The parsed value
	 * @param tagName The tag name for error reporting
	 * @param songTitle The song title for error reporting
	 * @return true if parsing succeeded, false otherwise
	 */
	bool ParseBeatValuePair(const RString& expression, float& outBeat, float& outValue,
		const RString& tagName, const RString& songTitle);

	/**
	 * @brief Validate a beat value is non-negative
	 * @param beat The beat value to validate
	 * @param tagName The tag name for error reporting
	 * @param songTitle The song title for error reporting
	 * @return true if valid, false otherwise
	 */
	bool ValidateBeat(float beat, const RString& tagName, const RString& songTitle);
}

#endif

/**
 * @file
 * @author StepMania Development Team (c) 2025
 * @section LICENSE
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
