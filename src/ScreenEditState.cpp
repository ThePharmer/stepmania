/* ScreenEditState - Edit mode state management, undo/redo, and save operations.
 *
 * This file contains state management methods extracted from ScreenEdit.cpp
 * to improve maintainability and reduce file size. All methods in this file
 * are part of the ScreenEdit class.
 *
 * This file contains:
 * - Undo/Redo functionality
 * - Save and autosave operations
 * - Edit state transitions
 * - Revert operations
 * - Dirty flag management
 */

#include "global.h"
#include <utility>
#include <float.h>
#include "ScreenEdit.h"
#include "ActorUtil.h"
#include "AdjustSync.h"
#include "ArrowEffects.h"
#include "BackgroundUtil.h"
#include "CommonMetrics.h"
#include "GameManager.h"
#include "GamePreferences.h"
#include "GameSoundManager.h"
#include "GameState.h"
#include "InputEventPlus.h"
#include "InputMapper.h"
#include "LocalizedString.h"
#include "NoteDataUtil.h"
#include "NoteSkinManager.h"
#include "NoteTypes.h"
#include "NotesWriterSM.h"
#include "PrefsManager.h"
#include "RageSoundManager.h"
#include "RageSoundReader_FileReader.h"
#include "RageInput.h"
#include "RageLog.h"
#include "ScreenDimensions.h"
#include "ScreenManager.h"
#include "ScreenMiniMenu.h"
#include "ScreenPrompt.h"
#include "ScreenSaveSync.h"
#include "ScreenTextEntry.h"
#include "SongManager.h"
#include "SongUtil.h"
#include "SpecialFiles.h"
#include "StepsUtil.h"
#include "Style.h"
#include "ThemeManager.h"
#include "ThemeMetric.h"
#include "TimingData.h"
#include "Game.h"
#include "RageSoundReader.h"

static Preference<float> g_iDefaultRecordLength( "DefaultRecordLength", 4 );
static Preference<bool> g_bEditorShowBGChangesPlay( "EditorShowBGChangesPlay", true );

/** @brief How long must the button be held to generate a hold in record mode? */
const float record_hold_default= 0.3f;
float record_hold_seconds = record_hold_default;
const float time_between_autosave= 300.0f; // 5 minutes. -Kyz

#define PLAYER_X		(SCREEN_CENTER_X)
#define PLAYER_Y		(SCREEN_CENTER_Y)
#define PLAYER_HEIGHT		(360)
#define PLAYER_Y_STANDARD	(PLAYER_Y-((SCREEN_HEIGHT/480)*(PLAYER_HEIGHT/2)))

#define EDIT_X			(SCREEN_CENTER_X)
#define EDIT_Y			(PLAYER_Y)

#define RECORD_X		(SCREEN_CENTER_X)
#define RECORD_Y		(SCREEN_CENTER_Y)

#define PLAY_RECORD_HELP_TEXT	THEME->GetString(m_sName,"PlayRecordHelpText")
#define EDIT_HELP_TEXT		THEME->GetString(m_sName,"EditHelpText")

#define SET_MOD_SCREEN THEME->GetMetric("ScreenEdit", "SetModScreen")
#define OPTIONS_SCREEN THEME->GetMetric("ScreenEdit", "OptionsScreen")

AutoScreenMessage( SM_UpdateTextInfo );
AutoScreenMessage( SM_BackFromMainMenu );
AutoScreenMessage( SM_BackFromAreaMenu );
AutoScreenMessage( SM_BackFromAlterMenu );
AutoScreenMessage( SM_BackFromArbitraryRemap );
AutoScreenMessage( SM_BackFromStepsInformation );
AutoScreenMessage( SM_BackFromStepsData );
AutoScreenMessage( SM_BackFromOptions );
AutoScreenMessage( SM_BackFromSongInformation );
AutoScreenMessage( SM_BackFromBGChange );
AutoScreenMessage( SM_BackFromInsertTapAttack );
AutoScreenMessage( SM_BackFromInsertTapAttackPlayerOptions );
AutoScreenMessage( SM_BackFromAttackAtTime );
AutoScreenMessage( SM_BackFromInsertStepAttack );
AutoScreenMessage( SM_BackFromAddingModToExistingAttack );
AutoScreenMessage( SM_BackFromEditingModToExistingAttack );
AutoScreenMessage( SM_BackFromEditingAttackStart );
AutoScreenMessage( SM_BackFromEditingAttackLength );
AutoScreenMessage( SM_BackFromAddingAttackToChart );
AutoScreenMessage( SM_BackFromInsertStepAttackPlayerOptions );
AutoScreenMessage( SM_BackFromInsertCourseAttack );
AutoScreenMessage( SM_BackFromInsertCourseAttackPlayerOptions );
AutoScreenMessage( SM_BackFromCourseModeMenu );
AutoScreenMessage( SM_BackFromKeysoundTrack );
AutoScreenMessage( SM_BackFromNewKeysound );
AutoScreenMessage( SM_DoRevertToLastSave );
AutoScreenMessage( SM_DoRevertFromDisk );
AutoScreenMessage( SM_ConfirmClearArea );
AutoScreenMessage( SM_BackFromTimingDataInformation );
AutoScreenMessage(SM_BackFromTimingDataChangeInformation);
AutoScreenMessage( SM_BackFromDifficultyMeterChange );
AutoScreenMessage( SM_BackFromBeat0Change );
AutoScreenMessage( SM_BackFromBPMChange );
AutoScreenMessage( SM_BackFromStopChange );
AutoScreenMessage( SM_BackFromDelayChange );
AutoScreenMessage( SM_BackFromTickcountChange );
AutoScreenMessage( SM_BackFromComboChange );
AutoScreenMessage( SM_BackFromLabelChange );
AutoScreenMessage( SM_BackFromWarpChange );
AutoScreenMessage( SM_BackFromSpeedPercentChange );
AutoScreenMessage( SM_BackFromSpeedWaitChange );
AutoScreenMessage( SM_BackFromSpeedModeChange );
AutoScreenMessage( SM_BackFromScrollChange );
AutoScreenMessage( SM_BackFromFakeChange );
AutoScreenMessage( SM_BackFromStepMusicChange );
AutoScreenMessage( SM_DoEraseStepTiming );
AutoScreenMessage( SM_DoSaveAndExit );
AutoScreenMessage( SM_DoExit );
AutoScreenMessage( SM_AutoSaveSuccessful );
AutoScreenMessage( SM_SaveSuccessful );
AutoScreenMessage( SM_SaveSuccessNoSM );
AutoScreenMessage( SM_SaveFailed );

static const char *EditStateNames[] = {
	"Edit",
	"Record",
	"RecordPaused",
	"Playing"
};
XToString( EditState );
LuaXType( EditState );

map<RString, EditButton> name_to_edit_button;

// ============================================================================
// State Management Methods (extracted from ScreenEdit.cpp)
// ============================================================================

void ScreenEdit::TransitionEditState( EditState em )
{
	EditState old = m_EditState;

	// If we're going from recording to paused, come back when we're done.
	if( old == STATE_RECORDING_PAUSED && em == STATE_PLAYING )
		m_bReturnToRecordMenuAfterPlay = true;

	const bool bStateChanging = em != old;

#if 0
	// If switching out of record, open the menu.
	{
		bool bGoToRecordMenu = (old == STATE_RECORDING);
		if( m_bReturnToRecordMenuAfterPlay && old == STATE_PLAYING )
		{
			bGoToRecordMenu = true;
			m_bReturnToRecordMenuAfterPlay = false;
		}

		if( bGoToRecordMenu )
			em = STATE_RECORDING_PAUSED;
	}
#endif

	// If we're playing music or assist ticks when changing modes, stop.
	SOUND->StopMusic();
	if( m_pSoundMusic )
		m_pSoundMusic->StopPlaying();
	m_GameplayAssist.StopPlaying();
	GAMESTATE->m_bGameplayLeadIn.Set( true );

	if( bStateChanging )
	{
		switch( old )
		{
		case STATE_EDITING:
			// If exiting EDIT mode, save the cursor position.
			m_fBeatToReturnTo = GetAppropriatePosition().m_fSongBeat;
			break;

		case STATE_PLAYING:
			AdjustSync::HandleSongEnd();
			if (!GAMESTATE->m_bIsUsingStepTiming)
				GAMESTATE->m_pCurSteps[PLAYER_1]->m_Timing = backupStepTiming;
			if( AdjustSync::IsSyncDataChanged() )
				ScreenSaveSync::PromptSaveSync();
			break;

		case STATE_RECORDING:
			SetDirty( true );
			if (!GAMESTATE->m_bIsUsingStepTiming)
				GAMESTATE->m_pCurSteps[PLAYER_1]->m_Timing = backupStepTiming;
			SaveUndo();

			// delete old TapNotes in the range
			m_NoteDataEdit.ClearRange( m_iStartPlayingAt, m_iStopPlayingAt );
			m_NoteDataEdit.CopyRange( m_NoteDataRecord, m_iStartPlayingAt, m_iStopPlayingAt, m_iStartPlayingAt );
			m_NoteDataRecord.ClearAll();

			CheckNumberOfNotesAndUndo();
			break;
		default: break;
		}
	}

	// Set up player options for this mode. (EDITING uses m_PlayerStateEdit,
	// which we don't need to change.)
	if( em != STATE_EDITING )
	{
		// Stop displaying course attacks, if any.
		GAMESTATE->m_pPlayerState[PLAYER_1]->RemoveActiveAttacks();
		// Load the player's default PlayerOptions.
		GAMESTATE->m_pPlayerState[PLAYER_1]->RebuildPlayerOptionsFromActiveAttacks();

		// Snap to current options.
		GAMESTATE->m_pPlayerState[PLAYER_1]->m_PlayerOptions.SetCurrentToLevel( ModsLevel_Stage );
	}

	switch( em )
	{
	DEFAULT_FAIL( em );
	case STATE_EDITING:
		// Important: people will stop playing, change the BG and start again; make sure we reload
		m_Background.Unload();
		m_Foreground.Unload();

		// Restore the cursor position + Quantize + Clamp
		SetBeat( max( 0, Quantize( m_fBeatToReturnTo, NoteTypeToBeat(m_SnapDisplay.GetNoteType()) ) ) );
		GAMESTATE->m_bInStepEditor = true;
		break;

	case STATE_PLAYING:
	case STATE_RECORDING:
	{
		m_NoteDataEdit.RevalidateATIs(vector<int>(), false);
		if( bStateChanging )
			AdjustSync::ResetOriginalSyncData();

		/* Give a lead-in.  If we're loading Player, this must be done first.
		 * Also be sure to get the right timing. */
		float fSeconds = GetAppropriateTiming().GetElapsedTimeFromBeat( NoteRowToBeat(m_iStartPlayingAt) ) - PREFSMAN->m_EditRecordModeLeadIn;
		GAMESTATE->UpdateSongPosition( fSeconds, GetAppropriateTiming(), RageZeroTimer );

		GAMESTATE->m_bGameplayLeadIn.Set( false );

		if (!GAMESTATE->m_bIsUsingStepTiming)
		{
			// Substitute the song timing for the step timing during
			// preview if we're in song mode
			backupStepTiming = GAMESTATE->m_pCurSteps[PLAYER_1]->m_Timing;
			GAMESTATE->m_pCurSteps[PLAYER_1]->m_Timing.Clear();
		}

		/* Reset the note skin, in case preferences have changed. */
		// XXX
		// GAMESTATE->ResetNoteSkins();
		//GAMESTATE->res
		GAMESTATE->m_bInStepEditor = false;
		break;
	}
	case STATE_RECORDING_PAUSED:
		GAMESTATE->m_bInStepEditor = false;
		break;
	}

	switch( em )
	{
	case STATE_PLAYING:
		// If we're in course display mode, set that up.
		SetupCourseAttacks();

		m_Player.Load( m_NoteDataEdit );

		if( GAMESTATE->m_pPlayerState[PLAYER_1]->m_PlayerOptions.GetCurrent().m_fPlayerAutoPlay != 0 )
			GAMESTATE->m_pPlayerState[PLAYER_1]->m_PlayerController = PC_AUTOPLAY;
		else
			GAMESTATE->m_pPlayerState[PLAYER_1]->m_PlayerController = GamePreferences::m_AutoPlay;

		if( g_bEditorShowBGChangesPlay )
		{
			/* FirstBeat affects backgrounds, so commit changes to memory (not to disk)
			 * and recalc it. */
			Steps* pSteps = GAMESTATE->m_pCurSteps[PLAYER_1];
			ASSERT( pSteps != nullptr );
			pSteps->SetNoteData( m_NoteDataEdit );
			m_pSong->ReCalculateRadarValuesAndLastSecond();

			m_Background.Unload();
			m_Background.LoadFromSong( m_pSong );

			m_Foreground.Unload();
			m_Foreground.LoadFromSong( m_pSong );
		}

		break;
	case STATE_RECORDING:
	case STATE_RECORDING_PAUSED:
		// initialize m_NoteFieldRecord
		m_NoteDataRecord.CopyAll( m_NoteDataEdit );

		// highlight the section being recorded
		m_NoteFieldRecord.m_iBeginMarker = m_iStartPlayingAt;
		m_NoteFieldRecord.m_iEndMarker = m_iStopPlayingAt;

		break;
	default: break;
	}

	// Show/hide depending on edit state (em)
	m_sprOverlay->PlayCommand( EditStateToString(em) );
	m_sprUnderlay->PlayCommand( EditStateToString(em) );

	m_Background.SetVisible( g_bEditorShowBGChangesPlay  &&  em != STATE_EDITING );
	m_textInputTips.SetVisible( em == STATE_EDITING );
	m_textInfo.SetVisible( em == STATE_EDITING );
	// Play the OnCommands again so that these will be re-hidden if the OnCommand hides them.
	if( em == STATE_EDITING )
	{
		m_textInputTips.PlayCommand( "On" );
		m_textInfo.PlayCommand( "On" );
	}
	m_textPlayRecordHelp.SetVisible( em != STATE_EDITING );
	m_SnapDisplay.SetVisible( em == STATE_EDITING );
	m_NoteFieldEdit.SetVisible( em == STATE_EDITING );
	m_NoteFieldRecord.SetVisible( em == STATE_RECORDING  ||  em == STATE_RECORDING_PAUSED );
	m_Player->SetVisible( em == STATE_PLAYING );
	m_Foreground.SetVisible( g_bEditorShowBGChangesPlay  &&  em != STATE_EDITING );

	switch( em )
	{
	case STATE_PLAYING:
	case STATE_RECORDING:
		{
		const float fStartSeconds = GetAppropriateTiming().GetElapsedTimeFromBeat( GetBeat() );
		LOG->Trace( "Starting playback at %f", fStartSeconds );

		RageSoundParams p;
		p.m_fSpeed = GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate;
		p.m_StartSecond = fStartSeconds;
		p.StopMode = RageSoundParams::M_CONTINUE;
		m_pSoundMusic->SetProperty( "AccurateSync", true );
		m_pSoundMusic->Play(false, &p);
		break;
		}
	default: break;
	}

	m_EditState = em;
}

void ScreenEdit::CheckNumberOfNotesAndUndo()
{
	if( EDIT_MODE.GetValue() != EditMode_Home )
		return;

	const float fBeat = GAMESTATE->m_pPlayerState[PLAYER_1]->m_Position.m_fSongBeat;
	const TimeSignatureSegment * curTime = GAMESTATE->m_pCurSong->m_SongTiming.GetTimeSignatureSegmentAtBeat( fBeat );
	int rowsPerMeasure = curTime->GetDen() * curTime->GetNum();

	for( int row=0; row<=m_NoteDataEdit.GetLastRow(); row+=rowsPerMeasure )
	{
		int iNumNotesThisMeasure = 0;
		FOREACH_NONEMPTY_ROW_ALL_TRACKS_RANGE( m_NoteDataEdit, r, row, row+rowsPerMeasure )
			iNumNotesThisMeasure += m_NoteDataEdit.GetNumTapNonEmptyTracks( r );
	}

	if( GAMESTATE->m_pCurSteps[0]->IsAnEdit() )
	{
		/* Check that the action didn't push notes any farther past the last
		 * measure. This blocks Insert Beat from pushing past the end, but allows
		 * Delete Beat to pull back the notes that are already past the end.
		 */
		float fNewLastBeat = m_NoteDataEdit.GetLastBeat();
		bool bLastBeatIncreased = fNewLastBeat > m_Undo.GetLastBeat();
		bool bPassedTheEnd = fNewLastBeat > GetMaximumBeatForNewNote();
		if( bLastBeatIncreased && bPassedTheEnd )
		{
			Undo();
			m_bHasUndo = false;
			RString sError = CREATES_NOTES_PAST_END.GetValue() + "\n\n" + CHANGE_REVERTED.GetValue();
			ScreenPrompt::Prompt( SM_None, sError );
			return;
		}
	}
}

void ScreenEdit::SetDirty(bool dirty)
{
	if(EDIT_MODE.GetValue() != EditMode_Full)
	{
		m_dirty= false;
		m_next_autosave_time= -1.0f;
		return;
	}
	if(dirty)
	{
		if(!m_dirty)
		{
			m_next_autosave_time= RageTimer::GetTimeSinceStartFast() + time_between_autosave;
		}
	}
	else
	{
		m_next_autosave_time= -1.0f;
	}
	m_dirty= dirty;
}

void ScreenEdit::PerformSave(bool autosave)
{
	// copy edit into current Steps
	m_pSteps->SetNoteData( m_NoteDataEdit );

	// don't forget the attacks.
	m_pSong->m_Attacks = GAMESTATE->m_pCurSong->m_Attacks;
	m_pSong->m_sAttackString = GAMESTATE->m_pCurSong->m_Attacks.ToVectorString();
	m_pSteps->m_Attacks = GAMESTATE->m_pCurSteps[PLAYER_1]->m_Attacks;
	m_pSteps->m_sAttackString = GAMESTATE->m_pCurSteps[PLAYER_1]->m_Attacks.ToVectorString();

	// If one of the charts uses split timing, then it cannot be accurately
	// saved in the .sm format.  So saving the .sm is disabled.
	bool uses_split= m_pSong->AnyChartUsesSplitTiming();
	const ScreenMessage save_message= autosave ? SM_AutoSaveSuccessful
		: (uses_split ? SM_SaveSuccessNoSM : SM_SaveSuccessful);

	switch( EDIT_MODE.GetValue() )
	{
		DEFAULT_FAIL( EDIT_MODE.GetValue() );
		case EditMode_Home:
			{
				ASSERT( m_pSteps->IsAnEdit() );

				RString sError;
				m_pSteps->CalculateRadarValues( m_pSong->m_fMusicLengthSeconds );
				if( !NotesWriterSM::WriteEditFileToMachine(m_pSong, m_pSteps, sError) )
				{
					ScreenPrompt::Prompt( SM_None, sError );
					break;
				}

				m_pSteps->SetSavedToDisk( true );

				// HACK: clear undo, so "exit" below knows we don't need to save.
				// This only works because important non-steps data can't be changed in
				// home mode (BPMs, stops).
				ClearUndo();

				SCREENMAN->ZeroNextUpdate();

				HandleScreenMessage(save_message);

				/* FIXME
					 RString s;
					 switch( c )
					 {
					 case save:			s = "ScreenMemcardSaveEditsAfterSave";	break;
					 case save_on_exit:	s = "ScreenMemcardSaveEditsAfterExit";	break;
					 default:		FAIL_M(ssprintf("Invalid menu choice: %i", c));
					 }
					 SCREENMAN->AddNewScreenToTop( s );
				*/
			}
			break;
		case EditMode_Full:
			{
				// This will recalculate radar values.
				m_pSong->Save(autosave);
				SCREENMAN->ZeroNextUpdate();

				HandleScreenMessage(save_message);
			}
			break;
		case EditMode_CourseMods:
		case EditMode_Practice:
			break;
	}
	m_soundSave.Play(true);
}

void ScreenEdit::CopyToLastSave()
{
	ASSERT( GAMESTATE->m_pCurSong != nullptr );
	ASSERT( GAMESTATE->m_pCurSteps[PLAYER_1] != nullptr );
	m_SongLastSave = *GAMESTATE->m_pCurSong;
	m_vStepsLastSave.clear();
	const vector<Steps*> &vSteps = GAMESTATE->m_pCurSong->GetStepsByStepsType( GAMESTATE->m_pCurSteps[PLAYER_1]->m_StepsType );
	for (Steps *it : vSteps)
		m_vStepsLastSave.push_back( *it );
}

void ScreenEdit::CopyFromLastSave()
{
	// We are assuming two things here:
	// 1) No steps can be created by ScreenEdit
	// 2) No steps can be deleted by ScreenEdit (except possibly when we exit)
	*GAMESTATE->m_pCurSong = m_SongLastSave;
	const vector<Steps*> &vSteps = GAMESTATE->m_pCurSong->GetStepsByStepsType( GAMESTATE->m_pCurSteps[PLAYER_1]->m_StepsType );
	ASSERT_M( vSteps.size() == m_vStepsLastSave.size(), ssprintf("Step sizes don't match: %d, %d", int(vSteps.size()), int(m_vStepsLastSave.size())) );
	for( unsigned i = 0; i < vSteps.size(); ++i )
		*vSteps[i] = m_vStepsLastSave[i];
}

void ScreenEdit::RevertFromDisk()
{
	ASSERT( GAMESTATE->m_pCurSteps[PLAYER_1] != nullptr );
	StepsID id;
	id.FromSteps( GAMESTATE->m_pCurSteps[PLAYER_1] );
	ASSERT( id.IsValid() );

	// If m_bInStepEditor is true while the song is reloaded, it screws up
	// loading and results in the steps being cleared.  -Kyz
	GAMESTATE->m_bInStepEditor= false;
	GAMESTATE->m_pCurSong->ReloadFromSongDir();
	GAMESTATE->m_bInStepEditor= true;

	Steps *pNewSteps = id.ToSteps( GAMESTATE->m_pCurSong, true );
	if( !pNewSteps )
	{
		// If the Steps we were currently editing vanished when we did the revert,
		// put a blank Steps in its place.  Note that this does not have to be the
		// work of someone maliciously changing the simfile; it could happen to
		// someone editing a new stepchart and reverting from disk, for example.
		pNewSteps = GAMESTATE->m_pCurSong->CreateSteps();
		pNewSteps->CreateBlank( id.GetStepsType() );
		pNewSteps->SetDifficulty( id.GetDifficulty() );
		GAMESTATE->m_pCurSong->AddSteps( pNewSteps );
	}
	GAMESTATE->m_pCurSteps[PLAYER_1].Set( pNewSteps );
	m_pSteps = pNewSteps;

	CopyToLastSave();
	SetDirty( false );
	SONGMAN->Invalidate( GAMESTATE->m_pCurSong );
}

void ScreenEdit::SaveUndo()
{
	m_bHasUndo = true;
	m_Undo.CopyAll( m_NoteDataEdit );
}

static LocalizedString UNDO			("ScreenEdit", "Undo");
static LocalizedString CANT_UNDO		("ScreenEdit", "Can't undo - no undo data.");
void ScreenEdit::Undo()
{
	if( m_bHasUndo )
	{
		swap( m_Undo, m_NoteDataEdit );
		SCREENMAN->SystemMessage( UNDO );
	}
	else
	{
		SCREENMAN->SystemMessage( CANT_UNDO );
		SCREENMAN->PlayInvalidSound();
	}
}

void ScreenEdit::ClearUndo()
{
	m_bHasUndo = false;
	m_Undo.ClearAll();
}

static LocalizedString CREATES_MORE_THAN_NOTES	( "ScreenEdit", "This change creates more than %d notes in a measure." );
static LocalizedString CREATES_NOTES_PAST_END	( "ScreenEdit", "This change creates notes past the end of the music and is not allowed." );
static LocalizedString CHANGE_REVERTED		( "ScreenEdit", "The change has been reverted." );
