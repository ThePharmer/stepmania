/* ScreenGameplayLogic - Core gameplay logic, scoring, and judgment.
 *
 * This file contains gameplay logic methods extracted from ScreenGameplay.cpp
 * to improve maintainability and reduce file size. All methods in this file
 * are part of the ScreenGameplay class.
 *
 * This file contains:
 * - Song position and timing updates
 * - Haste rate calculations  
 * - Lights updates
 * - Crossed message sending
 * - Statistics saving
 * - Song/Stage completion handling
 * - Replay saving/loading
 */

#include "global.h"
#include "ScreenGameplay.h"
#include "SongManager.h"
#include "ScreenManager.h"
#include "GameConstantsAndTypes.h"
#include "PrefsManager.h"
#include "GamePreferences.h"
#include "GameManager.h"
#include "RageFileManager.h"
#include "Steps.h"
#include "RageLog.h"
#include "LifeMeter.h"
#include "LifeMeterBar.h"
#include "GameState.h"
#include "ScoreDisplayNormal.h"
#include "ScoreDisplayPercentage.h"
#include "ScoreDisplayLifeTime.h"
#include "ScoreDisplayOni.h"
#include "ScoreDisplayRave.h"
#include "ThemeManager.h"
#include "RageTimer.h"
#include "ScoreKeeperNormal.h"
#include "ScoreKeeperRave.h"
#include "LyricsLoader.h"
#include "ActorUtil.h"
#include "ArrowEffects.h"
#include "RageSoundManager.h"
#include "RageSoundReader.h"
#include "RageTextureManager.h"
#include "GameSoundManager.h"
#include "CombinedLifeMeterTug.h"
#include "Inventory.h"
#include "Course.h"
#include "NoteDataUtil.h"
#include "UnlockManager.h"
#include "LightsManager.h"
#include "ProfileManager.h"
#include "StatsManager.h"
#include "PlayerAI.h" // for NUM_SKILL_LEVELS
#include "NetworkSyncManager.h"
#include "DancingCharacters.h"
#include "ScreenDimensions.h"
#include "ThemeMetric.h"
#include "PlayerState.h"
#include "Style.h"
#include "LuaManager.h"
#include "MemoryCardManager.h"
#include "CommonMetrics.h"
#include "InputMapper.h"
#include "Game.h"
#include "ActiveAttackList.h"
#include "Player.h"
#include "StepsDisplay.h"
#include "XmlFile.h"
#include "Background.h"
#include "Foreground.h"
#include "ScreenSaveSync.h"
#include "AdjustSync.h"
#include "SongUtil.h"
#include "Song.h"
#include "XmlFileUtil.h"
#include "Profile.h" // for replay data stuff
#include "RageDisplay.h"

// Defines
#define SHOW_LIFE_METER_FOR_DISABLED_PLAYERS	THEME->GetMetricB(m_sName,"ShowLifeMeterForDisabledPlayers")
#define SHOW_SCORE_IN_RAVE			THEME->GetMetricB(m_sName,"ShowScoreInRave")
#define SONG_POSITION_METER_WIDTH		THEME->GetMetricF(m_sName,"SongPositionMeterWidth")
#define STOP_COURSE_EARLY			THEME->GetMetricB(m_sName,"StopCourseEarly")	// evaluate this every time it's used

static ThemeMetric<float> INITIAL_BACKGROUND_BRIGHTNESS	("ScreenGameplay","InitialBackgroundBrightness");
static ThemeMetric<float> SECONDS_BETWEEN_COMMENTS	("ScreenGameplay","SecondsBetweenComments");
static ThemeMetric<RString> SCORE_KEEPER_CLASS		("ScreenGameplay","ScoreKeeperClass");
static ThemeMetric<bool> FORCE_IMMEDIATE_FAIL_FOR_BATTERY("ScreenGameplay", "ForceImmediateFailForBattery");

AutoScreenMessage( SM_PlayGo );

// received while STATE_DANCING
AutoScreenMessage( SM_LoadNextSong );
AutoScreenMessage( SM_StartLoadingNextSong );

// received while STATE_OUTRO
AutoScreenMessage( SM_DoPrevScreen );
AutoScreenMessage( SM_DoNextScreen );

// received while STATE_INTRO
AutoScreenMessage( SM_StartHereWeGo );
AutoScreenMessage( SM_StopHereWeGo );

AutoScreenMessage( SM_BattleTrickLevel1 );
AutoScreenMessage( SM_BattleTrickLevel2 );
AutoScreenMessage( SM_BattleTrickLevel3 );

static Preference<bool> g_bCenter1Player( "Center1Player", false );
static Preference<bool> g_bShowLyrics( "ShowLyrics", true );
static Preference<float> g_fNetStartOffset( "NetworkStartOffset", -3.0 );
static Preference<bool> g_bEasterEggs( "EasterEggs", true );


PlayerInfo::PlayerInfo(): m_pn(PLAYER_INVALID), m_mp(MultiPlayer_Invalid),

// ============================================================================
// Gameplay Logic Methods (extracted from ScreenGameplay.cpp)
// ============================================================================

void ScreenGameplay::PlayTicks()
{
	/* TODO: Allow all players to have ticks. Not as simple as it looks.
	 * If a loop takes place, it could make one player's ticks come later
	 * than intended. Any help here would be appreciated. -Wolfman2000 */
	Player &player = *m_vPlayerInfo[GAMESTATE->GetMasterPlayerNumber()].m_pPlayer;
	const NoteData &nd = player.GetNoteData();
	m_GameplayAssist.PlayTicks( nd, player.GetPlayerState() );
}

/* Play announcer "type" if it's been at least fSeconds since the last announcer. */
void ScreenGameplay::SaveStats()
{
	float fMusicLen = GAMESTATE->m_pCurSong->m_fMusicLengthSeconds;

	FOREACH_EnabledPlayerInfo( m_vPlayerInfo, pi )
	{
		/* Note that adding stats is only meaningful for the counters (eg. RadarCategory_Jumps),
		 * not for the percentages (RadarCategory_Air). */
		RadarValues rv;
		PlayerStageStats &pss = *pi->GetPlayerStageStats();
		const NoteData &nd = pi->m_pPlayer->GetNoteData();
		PlayerNumber pn = pi->m_pn;

		GAMESTATE->SetProcessedTimingData(GAMESTATE->m_pCurSteps[pn]->GetTimingData());
		NoteDataUtil::CalculateRadarValues( nd, fMusicLen, rv );
		pss.m_radarPossible += rv;
		NoteDataWithScoring::GetActualRadarValues( nd, pss, fMusicLen, rv );
		pss.m_radarActual += rv;
		GAMESTATE->SetProcessedTimingData(nullptr);
	}
}

void ScreenGameplay::SongFinished()
{
	FOREACH_EnabledPlayer(pn)
	{
		if(GAMESTATE->m_pCurSteps[pn])
		{
			GAMESTATE->m_pCurSteps[pn]->GetTimingData()->ReleaseLookup();
		}
	}
	AdjustSync::HandleSongEnd();
	SaveStats(); // Let subclasses save the stats.
	/* Extremely important: if we don't remove attacks before moving on to the next
	 * screen, they'll still be turned on eventually. */
	GAMESTATE->RemoveAllActiveAttacks();
	FOREACH_VisiblePlayerInfo( m_vPlayerInfo, pi )
		pi->m_pActiveAttackList->Refresh();
}

void ScreenGameplay::StageFinished( bool bBackedOut )
{
	if( GAMESTATE->IsCourseMode() && GAMESTATE->m_PlayMode != PLAY_MODE_ENDLESS )
	{
		LOG->Trace("Stage finished at index %i/%i", GAMESTATE->GetCourseSongIndex(), (int) m_apSongsQueue.size() );
		// +1 to skip the current song; that song has already passed.
		for( unsigned i = GAMESTATE->GetCourseSongIndex()+1; i < m_apSongsQueue.size(); ++i )
		{
			LOG->Trace("Running stats for %i", i );
			SetupSong( i );
			FOREACH_EnabledPlayerInfo( m_vPlayerInfo, pi )
				pi->m_pPlayer->ApplyWaitingTransforms();
			SongFinished();
		}
	}

	if( bBackedOut )
	{
		GAMESTATE->CancelStage();
		return;
	}

	// If all players failed, kill.
	if( STATSMAN->m_CurStageStats.AllFailed() )
	{
		FOREACH_HumanPlayer( p )
			GAMESTATE->m_iPlayerStageTokens[p] = 0;
	}

	FOREACH_HumanPlayer( pn )
		STATSMAN->m_CurStageStats.m_player[pn].CalcAwards( pn, STATSMAN->m_CurStageStats.m_bGaveUp, STATSMAN->m_CurStageStats.m_bUsedAutoplay );
	STATSMAN->m_CurStageStats.FinalizeScores( false );

	GAMESTATE->CommitStageStats();

	// save current stage stats
	STATSMAN->m_vPlayedStageStats.push_back( STATSMAN->m_CurStageStats );

	STATSMAN->CalcAccumPlayedStageStats();
	GAMESTATE->FinishStage();
}

void ScreenGameplay::SaveReplay()
{
	/* Replay data TODO:
	 * Add more player information (?)
	 * Add AutoGen flag if steps were autogen?
	 * Add proper steps hash?
	 * Add modifiers used
	 * Add date played, machine played on, etc.
	 * Hash of some stuff to validate data (see Profile)
	 */
	FOREACH_HumanPlayer( pn )
	{
		FOREACH_EnabledPlayerInfo( m_vPlayerInfo, pi )
		{
			Profile *pTempProfile = PROFILEMAN->GetProfile(pn);

			XNode *p = new XNode("ReplayData");
			// append version number (in case the format changes)
			p->AppendAttr("Version",0);

			// song information node
			SongID songID;
			songID.FromSong(GAMESTATE->m_pCurSong);
			XNode *pSongInfoNode = songID.CreateNode();
			pSongInfoNode->AppendChild("Title", GAMESTATE->m_pCurSong->GetDisplayFullTitle());
			pSongInfoNode->AppendChild("Artist", GAMESTATE->m_pCurSong->GetDisplayArtist());
			p->AppendChild(pSongInfoNode);

			// steps information
			StepsID stepsID;
			stepsID.FromSteps( GAMESTATE->m_pCurSteps[pn] );
			XNode *pStepsInfoNode = stepsID.CreateNode();
			// hashing = argh
			//pStepsInfoNode->AppendChild("StepsHash", stepsID.ToSteps(GAMESTATE->m_pCurSong,false)->GetHash());
			p->AppendChild(pStepsInfoNode);

			// player information node (rival data sup)
			XNode *pPlayerInfoNode = new XNode("Player");
			pPlayerInfoNode->AppendChild("DisplayName", pTempProfile->m_sDisplayName);
			pPlayerInfoNode->AppendChild("Guid", pTempProfile->m_sGuid);
			p->AppendChild(pPlayerInfoNode);

			// the timings.
			p->AppendChild( pi->m_pPlayer->GetNoteData().CreateNode() );

			// Find a file name for the replay
			vector<RString> files;
			GetDirListing( "Save/Replays/replay*", files, false, false );
			sort( files.begin(), files.end() );

			// Files should be of the form "replay#####.xml".
			int iIndex = 0;

			for( int i = files.size()-1; i >= 0; --i )
			{
				static Regex re( "^replay([0-9]{5})\\....$" );
				vector<RString> matches;
				if( !re.Compare( files[i], matches ) )
					continue;

				ASSERT( matches.size() == 1 );
				iIndex = StringToInt( matches[0] )+1;
				break;
			}

			RString sFileName = ssprintf( "replay%05d.xml", iIndex );

			XmlFileUtil::SaveToFile( p, "Save/Replays/"+sFileName );
			SAFE_DELETE( p );
			return;
		}
	}
}

/*
void ScreenGameplay::PlayAnnouncer( const RString &type, float fSeconds, float *fDeltaSeconds )
{
	if( GAMESTATE->m_fOpponentHealthPercent == 0 )
		return; // Shut the announcer up

	/* Don't play in demonstration. */
	if( GAMESTATE->m_bDemonstrationOrJukebox )
		return;

	/* Don't play before the first beat, or after we're finished. */
	if( m_DancingState != STATE_DANCING )
		return;
	if(GAMESTATE->m_pCurSong == nullptr  ||	// this will be true on ScreenDemonstration sometimes
	   GAMESTATE->m_Position.m_fSongBeat < GAMESTATE->m_pCurSong->GetFirstBeat())
		return;

	if( *fDeltaSeconds < fSeconds )
		return;
	*fDeltaSeconds = 0;

	SOUND->PlayOnceFromAnnouncer( type );
}

void ScreenGameplay::UpdateSongPosition( float fDeltaTime )
{
	if( !m_pSoundMusic->IsPlaying() )
		return;

	RageTimer tm;
	const float fSeconds = m_pSoundMusic->GetPositionSeconds( nullptr, &tm );
	const float fAdjust = SOUND->GetFrameTimingAdjustment( fDeltaTime );
	GAMESTATE->UpdateSongPosition( fSeconds+fAdjust, GAMESTATE->m_pCurSong->m_SongTiming, tm+fAdjust );
}

bool ScreenGameplay::AllAreFailing()
{
	FOREACH_EnabledPlayerInfo( m_vPlayerInfo, pi )
	{
		if( pi->m_pLifeMeter && !pi->m_pLifeMeter->IsFailing() )
			return false;
	}
	return true;
}

void ScreenGameplay::GetMusicEndTiming( float &fSecondsToStartFadingOutMusic, float &fSecondsToStartTransitioningOut )
{
	float fLastStepSeconds = GAMESTATE->m_pCurSong->GetLastSecond();
	fLastStepSeconds += Player::GetMaxStepDistanceSeconds();

	float fTransitionLength;
	if( !GAMESTATE->IsCourseMode() || IsLastSong() )
		fTransitionLength = OUT_TRANSITION_LENGTH;
	else
		fTransitionLength = COURSE_TRANSITION_LENGTH;

	fSecondsToStartTransitioningOut = fLastStepSeconds;

	// Align the end of the music fade to the end of the transition.
	float fSecondsToFinishFadingOutMusic = fSecondsToStartTransitioningOut + fTransitionLength;
	if( fSecondsToFinishFadingOutMusic < GAMESTATE->m_pCurSong->m_fMusicLengthSeconds )
		fSecondsToStartFadingOutMusic = fSecondsToFinishFadingOutMusic - MUSIC_FADE_OUT_SECONDS;
	else
		fSecondsToStartFadingOutMusic = GAMESTATE->m_pCurSong->m_fMusicLengthSeconds; // don't fade

	/* Make sure we keep going long enough to register a miss for the last note, and
	 * never start fading before the last note. */
	fSecondsToStartFadingOutMusic = max( fSecondsToStartFadingOutMusic, fLastStepSeconds );
	fSecondsToStartTransitioningOut = max( fSecondsToStartTransitioningOut, fLastStepSeconds );

	/* Make sure the fade finishes before the transition finishes. */
	fSecondsToStartTransitioningOut = max( fSecondsToStartTransitioningOut, fSecondsToStartFadingOutMusic + MUSIC_FADE_OUT_SECONDS - fTransitionLength );
}

float ScreenGameplay::GetHasteRate()
{
	return m_fCurrHasteRate;
}

void ScreenGameplay::UpdateHasteRate()
{
	if( GAMESTATE->m_Position.m_fMusicSeconds < GAMESTATE->m_fLastHasteUpdateMusicSeconds || // new song
		GAMESTATE->m_Position.m_fMusicSeconds > GAMESTATE->m_fLastHasteUpdateMusicSeconds + m_fHasteTimeBetweenUpdates )
	{
		bool bAnyPlayerHitAllNotes = false;
		FOREACH_EnabledPlayerInfo( m_vPlayerInfo, pi )
		{
			if( !GAMESTATE->IsHumanPlayer(pi->m_pn) )
				continue;

			PlayerState *pPS = pi->GetPlayerState();
			if( pPS->m_iTapsHitSinceLastHasteUpdate > 0 &&
				pPS->m_iTapsMissedSinceLastHasteUpdate == 0 )
				bAnyPlayerHitAllNotes = true;

			pPS->m_iTapsHitSinceLastHasteUpdate = 0;
			pPS->m_iTapsMissedSinceLastHasteUpdate = 0;
		}

		if( bAnyPlayerHitAllNotes )
			GAMESTATE->m_fHasteRate += 0.1f;
		CLAMP( GAMESTATE->m_fHasteRate, -1.0f, +1.0f );

		GAMESTATE->m_fLastHasteUpdateMusicSeconds = GAMESTATE->m_Position.m_fMusicSeconds;
	}

	/* If the life meter is less than half full, push the haste rate down to let
	 * the player use his accumulated haste time. */
	float fMaxLife = 0;
	FOREACH_EnabledPlayerInfo( m_vPlayerInfo, pi )
	{
		if( !GAMESTATE->IsHumanPlayer(pi->m_pn) )
			continue;
		// In Battle/Rave mode, the players don't have life meters.
		if(pi->m_pLifeMeter)
		{
			fMaxLife= max(fMaxLife, pi->m_pLifeMeter->GetLife());
		}
		else
		{
			fMaxLife= 1;
		}
	}
	if( fMaxLife <= m_fHasteLifeSwitchPoint )
		GAMESTATE->m_fHasteRate = SCALE( fMaxLife, 0.0f, m_fHasteLifeSwitchPoint, -1.0f, 0.0f );
	CLAMP( GAMESTATE->m_fHasteRate, -1.0f, +1.0f );

	float fSpeed = 1.0f;
	// If there are no turning points or no add amounts, the bad themer probably thinks that's a way to disable haste.
	// Since we're outside a lua function, crashing (asserting) won't point back to the source of the problem.
	if(m_HasteTurningPoints.size() < 2 || m_HasteAddAmounts.size() < 2 ||
		m_HasteTurningPoints.size() != m_HasteAddAmounts.size())
	{
		m_fCurrHasteRate= fSpeed;
		return;
	}
	float options_haste= GAMESTATE->m_SongOptions.GetCurrent().m_fHaste;
	float scale_from_low= -1;
	float scale_from_high= 1;
	float scale_to_low= 0;
	float scale_to_high=0;
	for(size_t turning_point= 0; turning_point < m_HasteTurningPoints.size();
			++turning_point)
	{
		float curr_turning_point= m_HasteTurningPoints[turning_point];
		scale_from_high= curr_turning_point;
		scale_to_high= m_HasteAddAmounts[turning_point];
		if(GAMESTATE->m_fHasteRate < curr_turning_point)
		{
			break;
		}
		scale_from_low= curr_turning_point;
		scale_to_low= m_HasteAddAmounts[turning_point];
	}
	// If negative haste is being used, the game instead slows down when the player does well.
	float speed_add= SCALE(GAMESTATE->m_fHasteRate, scale_from_low, scale_from_high, scale_to_low, scale_to_high) * options_haste;
	if(scale_from_low == scale_from_high)
	{
		speed_add= scale_to_high * options_haste;
	}
	CLAMP(speed_add, -1.0f, 1.0f);

	// Only adjust speed_add by AccumulatedHasteSeconds when the player is losing seconds.  Otherwise, gaining the first second is interfered with.
	bool losing_seconds= false;
	if(options_haste > 0)
	{
		losing_seconds= speed_add < 0;
	}
	else
	{
		losing_seconds= speed_add > 0;
	}
	if( losing_seconds && GAMESTATE->m_fAccumulatedHasteSeconds <= 1 )
	{
		/* Only allow slowing down the song while the players have accumulated
		 * haste. This prevents dragging on the song by keeping the life meter
		 * nearly empty. */
		/* In positive haste mode, the player accumulates seconds while the song
		 * is sped up, and loses them while the song is slowed down.  "<= 1"
		 * means that the player is only eligible to slow the song down when
		 * they are down to their last accumulated second. -Kyz */
		// 1 second left is full speed_add, 0 seconds left is no speed_add.
		float clamp_secs= max(0, GAMESTATE->m_fAccumulatedHasteSeconds);
		speed_add = speed_add * clamp_secs;
	}
	fSpeed += speed_add;
	m_fCurrHasteRate= fSpeed;
}

void ScreenGameplay::UpdateLights()
{
	if( !LIGHTSMAN->IsEnabled() )
		return;
	if( m_CabinetLightsNoteData.GetNumTracks() == 0 )	// light data wasn't loaded
		return;

	bool bBlinkCabinetLight[NUM_CabinetLight];
	bool bBlinkGameButton[NUM_GameController][NUM_GameButton];
	ZERO( bBlinkCabinetLight );
	ZERO( bBlinkGameButton );
	{
		const float fSongBeat = GAMESTATE->m_Position.m_fLightSongBeat;
		const int iSongRow = BeatToNoteRowNotRounded( fSongBeat );

		static int iRowLastCrossed = 0;

		FOREACH_CabinetLight( cl )
		{
			// for each index we crossed since the last update:
			FOREACH_NONEMPTY_ROW_IN_TRACK_RANGE( m_CabinetLightsNoteData, cl, r, iRowLastCrossed+1, iSongRow+1 )
			{
				if( m_CabinetLightsNoteData.GetTapNote( cl, r ).type != TapNoteType_Empty )
					bBlinkCabinetLight[cl] = true;
			}

			if( m_CabinetLightsNoteData.IsHoldNoteAtRow( cl, iSongRow ) )
				bBlinkCabinetLight[cl] = true;
		}

		FOREACH_EnabledPlayerNumberInfo( m_vPlayerInfo, pi )
		{
			const Style* pStyle = GAMESTATE->GetCurrentStyle(pi->m_pn);
			const NoteData &nd = pi->m_pPlayer->GetNoteData();
			for( int t=0; t<nd.GetNumTracks(); t++ )
			{
				bool bBlink = false;

				// for each index we crossed since the last update:
				FOREACH_NONEMPTY_ROW_IN_TRACK_RANGE( nd, t, r, iRowLastCrossed+1, iSongRow+1 )
				{
					const TapNote &tn = nd.GetTapNote( t, r );
					if( tn.type != TapNoteType_Mine )
						bBlink = true;
				}

				// check if a hold should be active
				if( nd.IsHoldNoteAtRow( t, iSongRow ) )
					bBlink = true;

				if( bBlink )
				{
					vector<GameInput> gi;
					pStyle->StyleInputToGameInput( t, pi->m_pn, gi );
					for(size_t i= 0; i < gi.size(); ++i)
					{
						bBlinkGameButton[gi[i].controller][gi[i].button] = true;
					}
				}
			}
		}

		iRowLastCrossed = iSongRow;
	}

	// Before the first beat of the song, all cabinet lights solid on (except for menu buttons).
	Song &s = *GAMESTATE->m_pCurSong;
	bool bOverrideCabinetBlink = (GAMESTATE->m_Position.m_fSongBeat < s.GetFirstBeat());
	FOREACH_CabinetLight( cl )
		bBlinkCabinetLight[cl] |= bOverrideCabinetBlink;

	// Send blink data.
	FOREACH_CabinetLight( cl )
	{
		if( bBlinkCabinetLight[cl] )
			LIGHTSMAN->BlinkCabinetLight( cl );
	}

	FOREACH_ENUM( GameController,  gc )
	{
		FOREACH_ENUM( GameButton,  gb )
		{
			if( bBlinkGameButton[gc][gb] )
				LIGHTSMAN->BlinkGameButton( GameInput(gc,gb) );
		}
	}
}

void ScreenGameplay::SendCrossedMessages()
{
	{
		static int iRowLastCrossed = 0;

		float fPositionSeconds = GAMESTATE->m_Position.m_fMusicSeconds;
		float fSongBeat = GAMESTATE->m_pCurSong->m_SongTiming.GetBeatFromElapsedTime( fPositionSeconds );

		int iRowNow = BeatToNoteRowNotRounded( fSongBeat );
		iRowNow = max( 0, iRowNow );

		for( int r=iRowLastCrossed+1; r<=iRowNow; r++ )
		{
			if( GetNoteType( r ) == NOTE_TYPE_4TH )
				MESSAGEMAN->Broadcast( Message_BeatCrossed );
		}

		iRowLastCrossed = iRowNow;
	}

	{
		const int NUM_MESSAGES_TO_SEND = 4;
		const float MESSAGE_SPACING_SECONDS = 0.4f;

		PlayerNumber pn = PLAYER_INVALID;
		FOREACH_EnabledPlayerNumberInfo( m_vPlayerInfo, pi )
		{
			if( GAMESTATE->m_pCurSteps[ pi->m_pn ]->GetDifficulty() == Difficulty_Beginner )
			{
				pn = pi->m_pn;
				break;
			}
		}
		if( pn == PLAYER_INVALID )
			return;

		const NoteData &nd = m_vPlayerInfo[pn].m_pPlayer->GetNoteData();

		static int iRowLastCrossedAll[NUM_MESSAGES_TO_SEND] = { 0, 0, 0, 0 };
		for( int i=0; i<NUM_MESSAGES_TO_SEND; i++ )
		{
			float fNoteWillCrossInSeconds = MESSAGE_SPACING_SECONDS * i;

			float fPositionSeconds = GAMESTATE->m_Position.m_fMusicSeconds + fNoteWillCrossInSeconds;
			float fSongBeat = GAMESTATE->m_pCurSong->m_SongTiming.GetBeatFromElapsedTime( fPositionSeconds );

			int iRowNow = BeatToNoteRowNotRounded( fSongBeat );
			iRowNow = max( 0, iRowNow );
			int &iRowLastCrossed = iRowLastCrossedAll[i];

			FOREACH_NONEMPTY_ROW_ALL_TRACKS_RANGE( nd, r, iRowLastCrossed+1, iRowNow+1 )
			{
				int iNumTracksWithTapOrHoldHead = 0;
				for( int t=0; t<nd.GetNumTracks(); t++ )
				{
					if( nd.GetTapNote(t,r).type == TapNoteType_Empty )
						continue;

					iNumTracksWithTapOrHoldHead++;

					// send crossed message
					if(GAMESTATE->GetCurrentGame()->m_PlayersHaveSeparateStyles)
					{
						FOREACH_EnabledPlayerNumberInfo(m_vPlayerInfo, pi)
						{
							const Style *pStyle = GAMESTATE->GetCurrentStyle(pi->m_pn);
							RString sButton = pStyle->ColToButtonName( t );
							Message msg( i == 0 ? "NoteCrossed" : "NoteWillCross" );
							msg.SetParam( "ButtonName", sButton );
							msg.SetParam( "NumMessagesFromCrossed", i );
							msg.SetParam("PlayerNumber", pi->m_pn);
							MESSAGEMAN->Broadcast( msg );
						}
					}
					else
					{
						const Style *pStyle = GAMESTATE->GetCurrentStyle(PLAYER_INVALID);
						RString sButton = pStyle->ColToButtonName( t );
						Message msg( i == 0 ? "NoteCrossed" : "NoteWillCross" );
						msg.SetParam( "ButtonName", sButton );
						msg.SetParam( "NumMessagesFromCrossed", i );
						MESSAGEMAN->Broadcast( msg );
					}
				}

				if( iNumTracksWithTapOrHoldHead > 0 )
					MESSAGEMAN->Broadcast( (MessageID)(Message_NoteCrossed + i) );
				if( i == 0  &&  iNumTracksWithTapOrHoldHead >= 2 )
				{
					RString sMessageName = "NoteCrossedJump";
					MESSAGEMAN->Broadcast( sMessageName );
				}
			}

			iRowLastCrossed = iRowNow;
		}
	}
}

