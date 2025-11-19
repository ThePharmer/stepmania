/* ScreenEditNoteField - Note editing operations and menu handlers.
 *
 * This file contains note editing and menu handler methods extracted from 
 * ScreenEdit.cpp to improve maintainability and reduce file size. 
 * All methods in this file are part of the ScreenEdit class.
 *
 * This file contains:
 * - Note placement and editing operations
 * - All menu choice handlers (Main, Alter, Area, Steps, Song, Timing, BGChange)
 * - Note field scrolling and navigation
 * - Course attack setup
 * - Step attack and keysound menus
 * - Help system
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
// Note Field and Menu Handler Methods (extracted from ScreenEdit.cpp)
// ============================================================================

void ScreenEdit::ScrollTo( float fDestinationBeat )
{
	CLAMP( fDestinationBeat, 0, GetMaximumBeatForMoving() );

	// Don't play the sound and do the hold note logic below if our position didn't change.
	const float fOriginalBeat = GetAppropriatePosition().m_fSongBeat;
	if( fOriginalBeat == fDestinationBeat )
		return;

	SetBeat(fDestinationBeat);

	// check to see if they're holding a button
	for( int n=0; n<NUM_EDIT_BUTTON_COLUMNS; n++ )
	{
		int iCol = n;

		// Ctrl + number = input to right half
		if( EditIsBeingPressed(EDIT_BUTTON_RIGHT_SIDE) )
			ShiftToRightSide( iCol, m_NoteDataEdit.GetNumTracks() );

		if( iCol >= m_NoteDataEdit.GetNumTracks() )
			continue;	// skip

		EditButton b = EditButton(EDIT_BUTTON_COLUMN_0+n);
		if( !EditIsBeingPressed(b) )
			continue;

		// create a new hold note
		int iStartRow = BeatToNoteRow( min(fOriginalBeat, fDestinationBeat) );
		int iEndRow = BeatToNoteRow( max(fOriginalBeat, fDestinationBeat) );

		// Don't SaveUndo.  We want to undo the whole hold, not just the last segment
		// that the user made.  Dragging the hold bigger can only absorb and remove
		// other taps, so dragging won't cause us to exceed the note limit.
		TapNote tn = EditIsBeingPressed(EDIT_BUTTON_LAY_ROLL) ? TAP_ORIGINAL_ROLL_HEAD : TAP_ORIGINAL_HOLD_HEAD;

		tn.pn = m_InputPlayerNumber;
		m_NoteDataEdit.AddHoldNote( iCol, iStartRow, iEndRow, tn );
	}

	if( EditIsBeingPressed(EDIT_BUTTON_SCROLL_SELECT) )
	{
		/* Shift is being held.
		 * If this is the first time we've moved since shift was depressed,
		 * the old position (before this move) becomes the start pos: */
		int iDestinationRow = BeatToNoteRow( fDestinationBeat );
		if( m_iShiftAnchor == -1 )
			m_iShiftAnchor = BeatToNoteRow(fOriginalBeat);

		if( iDestinationRow == m_iShiftAnchor )
		{
			// We're back at the anchor, so we have nothing selected.
			m_NoteFieldEdit.m_iBeginMarker = m_NoteFieldEdit.m_iEndMarker = -1;
		}
		else
		{
			m_NoteFieldEdit.m_iBeginMarker = m_iShiftAnchor;
			m_NoteFieldEdit.m_iEndMarker = iDestinationRow;
			if( m_NoteFieldEdit.m_iBeginMarker > m_NoteFieldEdit.m_iEndMarker )
				swap( m_NoteFieldEdit.m_iBeginMarker, m_NoteFieldEdit.m_iEndMarker );
		}
	}

	m_soundChangeLine.Play(true);
}

static LocalizedString NEW_KEYSOUND_FILE("ScreenEdit", "Enter New Keysound File");

void ScreenEdit::HandleSongInformationChoice( SongInformationChoice c, const vector<int> &iAnswers )
{
	Song* pSong = GAMESTATE->m_pCurSong;
	pSong->m_DisplayBPMType = static_cast<DisplayBPM>(iAnswers[display_bpm]);

	switch( c )
	{
	case main_title:
		ScreenTextEntry::TextEntry( SM_None, ENTER_MAIN_TITLE, pSong->m_sMainTitle, 100, nullptr, ChangeMainTitle, nullptr );
		break;
	case sub_title:
		ScreenTextEntry::TextEntry( SM_None, ENTER_SUB_TITLE, pSong->m_sSubTitle, 100, nullptr, ChangeSubTitle, nullptr );
		break;
	case artist:
		ScreenTextEntry::TextEntry( SM_None, ENTER_ARTIST, pSong->m_sArtist, 100, nullptr, ChangeArtist, nullptr );
		break;
	case genre:
		ScreenTextEntry::TextEntry( SM_None, ENTER_GENRE, pSong->m_sGenre, 100, nullptr, ChangeGenre, nullptr );
		break;
	case credit:
		ScreenTextEntry::TextEntry( SM_None, ENTER_CREDIT, pSong->m_sCredit, 100, nullptr, ChangeCredit, nullptr );
		break;
	case preview:
		ScreenTextEntry::TextEntry(SM_None, ENTER_PREVIEW, pSong->m_PreviewFile, 100, SongUtil::ValidateCurrentSongPreview, ChangePreview, nullptr);
		break;
	case main_title_transliteration:
		ScreenTextEntry::TextEntry( SM_None, ENTER_MAIN_TITLE_TRANSLIT, pSong->m_sMainTitleTranslit, 100, nullptr, ChangeMainTitleTranslit, nullptr );
		break;
	case sub_title_transliteration:
		ScreenTextEntry::TextEntry( SM_None, ENTER_SUB_TITLE_TRANSLIT, pSong->m_sSubTitleTranslit, 100, nullptr, ChangeSubTitleTranslit, nullptr );
		break;
	case artist_transliteration:
		ScreenTextEntry::TextEntry( SM_None, ENTER_ARTIST_TRANSLIT, pSong->m_sArtistTranslit, 100, nullptr, ChangeArtistTranslit, nullptr );
		break;
	case last_second_hint:
		ScreenTextEntry::TextEntry( SM_None, ENTER_LAST_SECOND_HINT,
					   std::to_string(pSong->GetSpecifiedLastSecond()), 20,
					   ScreenTextEntry::FloatValidate, ChangeLastSecondHint, nullptr );
		break;
	case preview_start:
		ScreenTextEntry::TextEntry( SM_None, ENTER_PREVIEW_START,
					   std::to_string(pSong->m_fMusicSampleStartSeconds), 20,
					   ScreenTextEntry::FloatValidate, ChangePreviewStart, nullptr );
		break;
	case preview_length:
		ScreenTextEntry::TextEntry( SM_None, ENTER_PREVIEW_LENGTH,
					   std::to_string(pSong->m_fMusicSampleLengthSeconds), 20,
					   ScreenTextEntry::FloatValidate, ChangePreviewLength, nullptr );
		break;
	case min_bpm:
		ScreenTextEntry::TextEntry( SM_None, ENTER_MIN_BPM,
					   std::to_string(pSong->m_fSpecifiedBPMMin), 20,
					   ScreenTextEntry::FloatValidate, ChangeMinBPM, nullptr );
		break;
	case max_bpm:
		ScreenTextEntry::TextEntry( SM_None, ENTER_MAX_BPM,
					   std::to_string(pSong->m_fSpecifiedBPMMax), 20,
					   ScreenTextEntry::FloatValidate, ChangeMaxBPM, nullptr );
		break;
	default: break;
	};
	SetDirty(true);
}


static LocalizedString ENTER_BEAT_0_OFFSET			( "ScreenEdit", "Enter the offset for the song.");
static LocalizedString ENTER_BPM_VALUE				( "ScreenEdit", "Enter a new BPM value." );
static LocalizedString ENTER_STOP_VALUE				( "ScreenEdit", "Enter a new Stop value." );
static LocalizedString ENTER_DELAY_VALUE			( "ScreenEdit", "Enter a new Delay value." );
static LocalizedString ENTER_TICKCOUNT_VALUE			( "ScreenEdit", "Enter a new Tickcount value." );
static LocalizedString ENTER_COMBO_VALUE			( "ScreenEdit", "Enter a new Combo value." );
static LocalizedString ENTER_LABEL_VALUE			( "ScreenEdit", "Enter a new Label value." );
static LocalizedString ENTER_WARP_VALUE				( "ScreenEdit", "Enter a new Warp value." );
static LocalizedString ENTER_SPEED_PERCENT_VALUE		( "ScreenEdit", "Enter a new Speed percent value." );
static LocalizedString ENTER_SPEED_WAIT_VALUE			( "ScreenEdit", "Enter a new Speed wait value." );
static LocalizedString ENTER_SPEED_MODE_VALUE			( "ScreenEdit", "Enter a new Speed mode value." );
static LocalizedString ENTER_SCROLL_VALUE			( "ScreenEdit", "Enter a new Scroll value." );
static LocalizedString ENTER_FAKE_VALUE				( "ScreenEdit", "Enter a new Fake value." );
static LocalizedString CONFIRM_TIMING_ERASE			( "ScreenEdit", "Are you sure you want to erase this chart's timing data?" );
void ScreenEdit::HandleTimingDataInformationChoice( TimingDataInformationChoice c, const vector<int> &iAnswers )
{
	switch( c )
	{
	DEFAULT_FAIL( c );
	case beat_0_offset:
		ScreenTextEntry::TextEntry(
			SM_BackFromBeat0Change,
			ENTER_BEAT_0_OFFSET,
			std::to_string(GetAppropriateTiming().m_fBeat0OffsetInSeconds),
			20
			);
		break;
	case bpm:
		ScreenTextEntry::TextEntry(
			SM_BackFromBPMChange,
			ENTER_BPM_VALUE,
			std::to_string( GetAppropriateTiming().GetBPMAtBeat( GetBeat() ) ),
			10
			);
		break;
	case stop:
		ScreenTextEntry::TextEntry(
			SM_BackFromStopChange,
			ENTER_STOP_VALUE,
			std::to_string( GetAppropriateTiming().GetStopAtBeat( GetBeat() ) ),
			10
			);
		break;
	case delay:
		ScreenTextEntry::TextEntry(
			SM_BackFromDelayChange,
			ENTER_DELAY_VALUE,
			std::to_string( GetAppropriateTiming().GetDelayAtBeat( GetBeat() ) ),
			10
		);
		break;
	case tickcount:
		ScreenTextEntry::TextEntry(
			SM_BackFromTickcountChange,
			ENTER_TICKCOUNT_VALUE,
			ssprintf( "%d", GetAppropriateTiming().GetTickcountAtBeat( GetBeat() ) ),
			2
			);
		break;
	case combo:
	{
		const ComboSegment *cs = GetAppropriateTiming().GetComboSegmentAtBeat(GetBeat());
		ScreenTextEntry::TextEntry(SM_BackFromComboChange,
								   ENTER_COMBO_VALUE,
								   ssprintf( "%d/%d",
											cs->GetCombo(),
											cs->GetMissCombo()),
								   7);
		break;
	}
	case label:
		ScreenTextEntry::TextEntry(
		   SM_BackFromLabelChange,
		   ENTER_LABEL_VALUE,
		   ssprintf( "%s", GetAppropriateTiming().GetLabelAtBeat( GetBeat() ).c_str() ),
		   64
		   );
		break;
	case warp:
		ScreenTextEntry::TextEntry(
		   SM_BackFromWarpChange,
		   ENTER_WARP_VALUE,
		   std::to_string( GetAppropriateTiming().GetWarpAtBeat( GetBeat() ) ),
		   10
		   );
		break;
	case speed_percent:
		ScreenTextEntry::TextEntry(
		   SM_BackFromSpeedPercentChange,
		   ENTER_SPEED_PERCENT_VALUE,
		   std::to_string( GetAppropriateTiming().GetSpeedSegmentAtBeat( GetBeat() )->GetRatio() ),
		   10
		   );
		break;
	case scroll:
		ScreenTextEntry::TextEntry(
		   SM_BackFromScrollChange,
		   ENTER_SCROLL_VALUE,
		   std::to_string( GetAppropriateTiming().GetScrollSegmentAtBeat( GetBeat() )->GetRatio() ),
		   10
		   );
		break;
	case speed_wait:
		ScreenTextEntry::TextEntry(
		   SM_BackFromSpeedWaitChange,
		   ENTER_SPEED_WAIT_VALUE,
		   std::to_string( GetAppropriateTiming().GetSpeedSegmentAtBeat( GetBeat() )->GetDelay() ),
		   10
		   );
		break;
	case speed_mode:
		{
			ScreenTextEntry::TextEntry(
						SM_BackFromSpeedModeChange,
						   ENTER_SPEED_MODE_VALUE,
						   "",
						   3
			);

			break;
		}
	case fake:
		{
			ScreenTextEntry::TextEntry(
				SM_BackFromFakeChange,
				ENTER_FAKE_VALUE,
			        std::to_string(GetAppropriateTiming().GetFakeAtBeat( GetBeat() ) ),
				10
			);
			break;
		}
		case shift_timing_in_region_down:
			m_timing_change_menu_purpose= menu_is_for_shifting;
			m_timing_rows_being_shitted= GetRowsFromAnswers(c, iAnswers);
			DisplayTimingChangeMenu();
			break;
		case shift_timing_in_region_up:
			m_timing_change_menu_purpose= menu_is_for_shifting;
			m_timing_rows_being_shitted= -GetRowsFromAnswers(c, iAnswers);
			DisplayTimingChangeMenu();
			break;
		case copy_timing_in_region:
			m_timing_change_menu_purpose= menu_is_for_copying;
			DisplayTimingChangeMenu();
			break;
		case clear_timing_in_region:
			m_timing_change_menu_purpose= menu_is_for_clearing;
			DisplayTimingChangeMenu();
			break;
		case paste_timing_from_clip:
			clipboardFullTiming.CopyRange(0, MAX_NOTE_ROW, TimingSegmentType_Invalid, GetRow(), GetAppropriateTimingForUpdate());
			break;
	case copy_full_timing:
	{
		clipboardFullTiming = GetAppropriateTiming();
		break;
	}
	case paste_full_timing:
	{
		if(GAMESTATE->m_bIsUsingStepTiming)
		{
			GAMESTATE->m_pCurSteps[PLAYER_1]->m_Timing = clipboardFullTiming;
		}
		else
		{
			GAMESTATE->m_pCurSong->m_SongTiming = clipboardFullTiming;
		}
		SetDirty(true);
		break;
	}
	case erase_step_timing:
		ScreenPrompt::Prompt( SM_DoEraseStepTiming, CONFIRM_TIMING_ERASE , PROMPT_YES_NO, ANSWER_NO );
	break;

	}
}

void ScreenEdit::HandleTimingDataChangeChoice(TimingDataChangeChoice choice,
	const vector<int>& answers)
{
	TimingSegmentType change_type= TimingSegmentType_Invalid;
	switch(choice)
	{
		case timing_all:
			change_type= TimingSegmentType_Invalid;
			break;
		case timing_bpm:
			change_type= SEGMENT_BPM;
			break;
		case timing_stop:
			change_type= SEGMENT_STOP;
			break;
		case timing_delay:
			change_type= SEGMENT_DELAY;
			break;
		case timing_time_sig:
			change_type= SEGMENT_TIME_SIG;
			break;
		case timing_warp:
			change_type= SEGMENT_WARP;
			break;
		case timing_label:
			change_type= SEGMENT_LABEL;
			break;
		case timing_tickcount:
			change_type= SEGMENT_TICKCOUNT;
			break;
		case timing_combo:
			change_type= SEGMENT_COMBO;
			break;
		case timing_speed:
			change_type= SEGMENT_SPEED;
			break;
		case timing_scroll:
			change_type= SEGMENT_SCROLL;
			break;
		case timing_fake:
			change_type= SEGMENT_FAKE;
			break;
		default: break;
	}
	int begin= m_NoteFieldEdit.m_iBeginMarker;
	int end= m_NoteFieldEdit.m_iEndMarker;
	if(begin < 0)
	{
		begin= GetRow();
	}
	if(end < 0)
	{
		end= MAX_NOTE_ROW;
	}
	switch(m_timing_change_menu_purpose)
	{
		case menu_is_for_copying:
			clipboardFullTiming.Clear();
			GetAppropriateTiming().CopyRange(begin, end, change_type, 0, clipboardFullTiming);
			break;
		case menu_is_for_shifting:
			GetAppropriateTimingForUpdate().ShiftRange(begin, end, change_type, m_timing_rows_being_shitted);
			break;
		case menu_is_for_clearing:
			GetAppropriateTimingForUpdate().ClearRange(begin, end, change_type);
			break;
		default: break;
	}
}

void ScreenEdit::HandleBGChangeChoice( BGChangeChoice c, const vector<int> &iAnswers )
{
	BackgroundChange newChange;

	auto &changes = m_pSong->GetBackgroundChanges(g_CurrentBGChangeLayer);
	for (auto iter = changes.begin(); iter != changes.end(); ++iter)
	{
		if( iter->m_fStartBeat == GAMESTATE->m_Position.m_fSongBeat )
		{
			newChange = *iter;
			// delete the old change.  We'll add a new one below.
			changes.erase( iter );
			break;
		}
	}

	newChange.m_fStartBeat    = GAMESTATE->m_Position.m_fSongBeat;
	newChange.m_fRate         = StringToFloat( g_BackgroundChange.rows[rate].choices[iAnswers[rate]] )/100.f;
	newChange.m_sTransition   = iAnswers[transition] ? g_BackgroundChange.rows[transition].choices[iAnswers[transition]] : RString();
	newChange.m_def.m_sEffect = iAnswers[effect]     ? g_BackgroundChange.rows[effect].choices[iAnswers[effect]]         : RString();
	newChange.m_def.m_sColor1 = iAnswers[color1]     ? g_BackgroundChange.rows[color1].choices[iAnswers[color1]]         : RString();
	newChange.m_def.m_sColor2 = iAnswers[color2]     ? g_BackgroundChange.rows[color2].choices[iAnswers[color2]]         : RString();
	switch( iAnswers[file1_type] )
	{
	DEFAULT_FAIL( iAnswers[file1_type] );
	case none:			newChange.m_def.m_sFile1 = "";					break;
	case dynamic_random:		newChange.m_def.m_sFile1 = RANDOM_BACKGROUND_FILE;		break;
	case baked_random:		newChange.m_def.m_sFile1 = GetOneBakedRandomFile( m_pSong );	break;
	case song_bganimation:
	case song_movie:
	case song_bitmap:
	case global_bganimation:
	case global_movie:
	case global_movie_song_group:
	case global_movie_song_group_and_genre:
		{
			BGChangeChoice row1 = (BGChangeChoice)(file1_song_bganimation + iAnswers[file1_type]);
			newChange.m_def.m_sFile1 = g_BackgroundChange.rows[row1].choices.empty() ? "" : g_BackgroundChange.rows[row1].choices[iAnswers[row1]];
		}
		break;
	}
	switch( iAnswers[file2_type] )
	{
	DEFAULT_FAIL(iAnswers[file2_type]);
	case none:				newChange.m_def.m_sFile2 = "";				break;
	case dynamic_random:		newChange.m_def.m_sFile2 = RANDOM_BACKGROUND_FILE;		break;
	case baked_random:		newChange.m_def.m_sFile2 = GetOneBakedRandomFile( m_pSong );	break;
	case song_bganimation:
	case song_movie:
	case song_bitmap:
	case global_bganimation:
	case global_movie:
	case global_movie_song_group:
	case global_movie_song_group_and_genre:
		{
			BGChangeChoice row2 = (BGChangeChoice)(file2_song_bganimation + iAnswers[file2_type]);
			newChange.m_def.m_sFile2 = g_BackgroundChange.rows[row2].choices.empty() ? "" : g_BackgroundChange.rows[row2].choices[iAnswers[row2]];
		}
		break;
	}


	if( c == delete_change || newChange.m_def.m_sFile1.empty() )
	{
		// don't add
	}
	else
	{
		m_pSong->AddBackgroundChange( g_CurrentBGChangeLayer, newChange );
	}
	g_CurrentBGChangeLayer = BACKGROUND_LAYER_Invalid;
}

void ScreenEdit::SetupCourseAttacks()
{
	/* This is the first beat that can be changed without it being visible.  Until
	 * we draw for the first time, any beat can be changed. */
	GAMESTATE->m_pPlayerState[PLAYER_1]->m_fLastDrawnBeat = -100;

	// Put course options into effect.
	GAMESTATE->m_pPlayerState[PLAYER_1]->m_ModsToApply.clear();
	GAMESTATE->m_pPlayerState[PLAYER_1]->RemoveActiveAttacks();


	if( GAMESTATE->m_pCurCourse )
	{
		AttackArray Attacks;

		if( EDIT_MODE == EditMode_CourseMods )
		{
			Attacks = GAMESTATE->m_pCurCourse->m_vEntries[GAMESTATE->m_iEditCourseEntryIndex].attacks;
		}
		else
		{
			GAMESTATE->m_pCurCourse->RevertFromDisk();	// Remove this and have a separate reload key?

			for( unsigned e = 0; e < GAMESTATE->m_pCurCourse->m_vEntries.size(); ++e )
			{
				if( GAMESTATE->m_pCurCourse->m_vEntries[e].songID.ToSong() != m_pSong )
					continue;

				Attacks = GAMESTATE->m_pCurCourse->m_vEntries[e].attacks;
				break;
			}
		}

		for (Attack &attack: Attacks)
			GAMESTATE->m_pPlayerState[PLAYER_1]->LaunchAttack( attack );
	}
	else
	{
		const PlayerOptions &p = GAMESTATE->m_pPlayerState[PLAYER_1]->m_PlayerOptions.GetCurrent();
		if (GAMESTATE->m_pCurSong && p.m_fNoAttack == 0 && p.m_fRandAttack == 0 )
		{
			AttackArray &attacks = GAMESTATE->m_bIsUsingStepTiming ?
				GAMESTATE->m_pCurSteps[PLAYER_1]->m_Attacks :
				GAMESTATE->m_pCurSong->m_Attacks;

			if (attacks.size() > 0)
			{
				for (Attack &attack : attacks)
				{
					// LaunchAttack is actually a misnomer.  The function actually adds
					// the attack to a list in the PlayerState which is checked and
					// updated every tick to see which ones to actually activate. -Kyz
					GAMESTATE->m_pPlayerState[PLAYER_1]->LaunchAttack( attack );
				}
			}
		}
	}

	GAMESTATE->m_pPlayerState[PLAYER_1]->RebuildPlayerOptionsFromActiveAttacks();
}

float ScreenEdit::GetMaximumBeatForNewNote() const
{
	switch( EDIT_MODE.GetValue() )
	{
	DEFAULT_FAIL( EDIT_MODE.GetValue() );
	case EditMode_Practice:
	case EditMode_CourseMods:
	case EditMode_Home:
		{
			Song &s = *GAMESTATE->m_pCurSong;
			float fEndBeat = s.GetLastBeat();

			/* Round up to the next measure end.  Some songs end on weird beats
			 * mid-measure, and it's odd to have movement capped to these weird
			 * beats. */
			TimingData &timing = s.m_SongTiming;
			float playerBeat = GetAppropriatePosition().m_fSongBeat;
			int beatsPerMeasure = timing.GetTimeSignatureSegmentAtBeat( playerBeat )->GetNum();
			fEndBeat += beatsPerMeasure;
			fEndBeat = ftruncf( fEndBeat, (float)beatsPerMeasure );

			return fEndBeat;
		}
	case EditMode_Full:
		return NoteRowToBeat(MAX_NOTE_ROW);
	}
}

float ScreenEdit::GetMaximumBeatForMoving() const
{
	float fEndBeat = GetMaximumBeatForNewNote();

	/* Jump to GetLastBeat even if it's past the song's last beat
	 * so that users can delete garbage steps past then end that they have
	 * have inserted in a text editor.  Once they delete all steps on
	 * GetLastBeat() and move off of that beat, they won't be able to return. */
	fEndBeat = max( fEndBeat, m_NoteDataEdit.GetLastBeat() );

	return fEndBeat;
}

struct EditHelpLine
{
	const char *szEnglishDescription;
	vector<EditButton> veb;

	EditHelpLine(
		const char *_szEnglishDescription,
		EditButton eb0,
		EditButton eb1 = EditButton_Invalid,
		EditButton eb2 = EditButton_Invalid,
		EditButton eb3 = EditButton_Invalid,
		EditButton eb4 = EditButton_Invalid,
		EditButton eb5 = EditButton_Invalid,
		EditButton eb6 = EditButton_Invalid,
		EditButton eb7 = EditButton_Invalid,
		EditButton eb8 = EditButton_Invalid,
		EditButton eb9 = EditButton_Invalid )
	{
		szEnglishDescription = _szEnglishDescription;
#define PUSH_IF_VALID( x ) if( x != EditButton_Invalid ) veb.push_back( x );
		PUSH_IF_VALID( eb0 );
		PUSH_IF_VALID( eb1 );
		PUSH_IF_VALID( eb2 );
		PUSH_IF_VALID( eb3 );
		PUSH_IF_VALID( eb4 );
		PUSH_IF_VALID( eb5 );
		PUSH_IF_VALID( eb6 );
		PUSH_IF_VALID( eb7 );
		PUSH_IF_VALID( eb8 );
		PUSH_IF_VALID( eb9 );
#undef PUSH_IF_VALID
	}
};
// TODO: Identify which of these can be removed and sent to a readme.
static const EditHelpLine g_EditHelpLines[] =
{
	EditHelpLine( "Move cursor",					EDIT_BUTTON_SCROLL_UP_LINE,		EDIT_BUTTON_SCROLL_DOWN_LINE ),
	EditHelpLine( "Jump measure",					EDIT_BUTTON_SCROLL_UP_PAGE,		EDIT_BUTTON_SCROLL_DOWN_PAGE ),
	EditHelpLine( "Jump measure",					EDIT_BUTTON_SCROLL_PREV_MEASURE,	EDIT_BUTTON_SCROLL_NEXT_MEASURE ),
	EditHelpLine( "Select region",					EDIT_BUTTON_SCROLL_SELECT ),
	EditHelpLine( "Jump to first/last beat",			EDIT_BUTTON_SCROLL_HOME,		EDIT_BUTTON_SCROLL_END ),
	EditHelpLine( "Change zoom",					EDIT_BUTTON_SCROLL_SPEED_UP,		EDIT_BUTTON_SCROLL_SPEED_DOWN ),
	EditHelpLine( "Play",						EDIT_BUTTON_PLAY_SELECTION ),
	EditHelpLine( "Play current beat to end",			EDIT_BUTTON_PLAY_FROM_CURSOR ),
	EditHelpLine( "Play whole song",				EDIT_BUTTON_PLAY_FROM_START ),
	EditHelpLine( "Record",						EDIT_BUTTON_RECORD_SELECTION ),
	EditHelpLine( "Set selection",					EDIT_BUTTON_LAY_SELECT ),
	EditHelpLine( "Next/prev steps of same StepsType",		EDIT_BUTTON_OPEN_PREV_STEPS, 		EDIT_BUTTON_OPEN_NEXT_STEPS ),
	EditHelpLine( "Decrease/increase BPM at cur beat",		EDIT_BUTTON_BPM_DOWN,			EDIT_BUTTON_BPM_UP ),
	EditHelpLine( "Decrease/increase stop at cur beat",		EDIT_BUTTON_STOP_DOWN,			EDIT_BUTTON_STOP_UP ),
	EditHelpLine( "Decrease/increase delay at cur beat",		EDIT_BUTTON_DELAY_DOWN,			EDIT_BUTTON_DELAY_UP ),
	EditHelpLine( "Decrease/increase music offset",			EDIT_BUTTON_OFFSET_DOWN,		EDIT_BUTTON_OFFSET_UP ),
	EditHelpLine( "Decrease/increase sample music start",		EDIT_BUTTON_SAMPLE_START_DOWN,		EDIT_BUTTON_SAMPLE_START_UP ),
	EditHelpLine( "Decrease/increase sample music length",		EDIT_BUTTON_SAMPLE_LENGTH_DOWN,		EDIT_BUTTON_SAMPLE_LENGTH_UP ),
	EditHelpLine( "Play sample music",				EDIT_BUTTON_PLAY_SAMPLE_MUSIC ),
	EditHelpLine( "Add/Edit Background Change",			EDIT_BUTTON_OPEN_BGCHANGE_LAYER1_MENU ),
	EditHelpLine( "Insert beat and shift down",			EDIT_BUTTON_INSERT ),
	EditHelpLine( "Shift BPM changes and stops down one beat",	EDIT_BUTTON_INSERT_SHIFT_PAUSES ),
	EditHelpLine( "Delete beat and shift up",			EDIT_BUTTON_DELETE ),
	EditHelpLine( "Shift BPM changes and stops up one beat",	EDIT_BUTTON_DELETE_SHIFT_PAUSES ),
	EditHelpLine( "Cycle between tap notes",			EDIT_BUTTON_CYCLE_TAP_LEFT,		EDIT_BUTTON_CYCLE_TAP_RIGHT ),
	EditHelpLine( "Add to/remove from right half",			EDIT_BUTTON_RIGHT_SIDE ),
	EditHelpLine( "Switch Timing",					EDIT_BUTTON_SWITCH_TIMINGS ),
	EditHelpLine( "Switch player (Routine only)",			EDIT_BUTTON_SWITCH_PLAYERS ),
};

static bool IsMapped( EditButton eb, const MapEditToDI &editmap )
{
	for( int s=0; s<NUM_EDIT_TO_DEVICE_SLOTS; s++ )
	{
		DeviceInput diPress = editmap.button[eb][s];
		if( diPress.IsValid() )
			return true;
	}
	return false;
}

static void ProcessKeyName( RString &s )
{
	s.Replace( "Key_", "" );
}

static void ProcessKeyNames( vector<RString> &vs, bool doSort )
{
	for (RString &s : vs)
		ProcessKeyName( s );

	if (doSort)
		sort( vs.begin(), vs.end() );
	vector<RString>::iterator toDelete = unique( vs.begin(), vs.end() );
	vs.erase(toDelete, vs.end());
}

static RString GetDeviceButtonsLocalized( const vector<EditButton> &veb, const MapEditToDI &editmap )
{
	vector<RString> vsPress;
	vector<RString> vsHold;
	for (EditButton const &eb : veb)
	{
		if( !IsMapped( eb, editmap ) )
			continue;

		for( int s=0; s<NUM_EDIT_TO_DEVICE_SLOTS; s++ )
		{
			DeviceInput diPress = editmap.button[eb][s];
			DeviceInput diHold = editmap.hold[eb][s];
			if( diPress.IsValid() )
				vsPress.push_back( Capitalize(INPUTMAN->GetLocalizedInputString(diPress)) );
			if( diHold.IsValid() )
				vsHold.push_back( Capitalize(INPUTMAN->GetLocalizedInputString(diHold)) );
		}
	}

	ProcessKeyNames( vsPress, false );
	ProcessKeyNames( vsHold, true );

	RString s = join("/",vsPress);
	if( !vsHold.empty() )
		s = join("/",vsHold) + " + " + s;
	return s;
}

void ScreenEdit::DoStepAttackMenu()
{
	const TimingData &timing = GetAppropriateTiming();
	float startTime = timing.GetElapsedTimeFromBeat(GetBeat());
	AttackArray &attacks =
		(GAMESTATE->m_bIsUsingStepTiming ? m_pSteps->m_Attacks : m_pSong->m_Attacks);
	vector<int> points = FindAllAttacksAtTime(attacks, startTime);

	g_AttackAtTimeMenu.rows.clear();
	unsigned index = 0;

	for (int &i : points)
	{
		const Attack &attack = attacks[i];
		RString desc = ssprintf("%g -> %g (%d mod[s])",
			startTime, startTime + attack.fSecsRemaining,
			attack.GetNumAttacks());

		g_AttackAtTimeMenu.rows.push_back(MenuRowDef(index++,
			desc,
			true,
			EditMode_CourseMods,
			false,
			false,
			0,
			"Modify",
			"Delete"));
	}
	g_AttackAtTimeMenu.rows.push_back(MenuRowDef(index,
		"Add Attack",
		true,
		EditMode_CourseMods,
		true,
		true,
		0,
		nullptr));

	EditMiniMenu(&g_AttackAtTimeMenu, SM_BackFromAttackAtTime);
}

static LocalizedString TRACK_NUM("ScreenEdit", "Track %d");
static LocalizedString NO_KEYSND("ScreenEdit", "None");
static LocalizedString NEWKEYSND("ScreenEdit", "New Sound");

void ScreenEdit::DoKeyboardTrackMenu()
{
	g_KeysoundTrack.rows.clear();
	vector<RString> &kses = m_pSong->m_vsKeysoundFile;

	vector<RString> choices;
	for (RString const &ks : kses)
	{
		choices.push_back(ks);
	}
	choices.push_back(NEWKEYSND);
	choices.push_back(NO_KEYSND);
	int numKeysounds = kses.size();
	int foundKeysounds = 0;
	for (int i = 0; i < m_NoteDataEdit.GetNumTracks(); ++i)
	{
		const TapNote &tn = m_NoteDataEdit.GetTapNote(i, this->GetRow());
		int keyIndex = tn.iKeysoundIndex;
		if (keyIndex == -1)
		{
			keyIndex = numKeysounds;
		}
		else
		{
			++foundKeysounds;
		}

		g_KeysoundTrack.rows.push_back(MenuRowDef(i, ssprintf(TRACK_NUM.GetValue(), i + 1),
												  true, EditMode_Full, false, false, keyIndex, choices));
	}
	g_KeysoundTrack.rows.push_back(MenuRowDef(m_NoteDataEdit.GetNumTracks(), "Remove Keysound",
											  foundKeysounds > 0, EditMode_Full, false, false, 0, kses));

	EditMiniMenu(&g_KeysoundTrack, SM_BackFromKeysoundTrack);
}

void ScreenEdit::DoHelp()
{
	g_EditHelp.rows.clear();

	for( unsigned i=0; i<ARRAYLEN(g_EditHelpLines); ++i )
	{
		const EditHelpLine &hl = g_EditHelpLines[i];

		if( !IsMapped(hl.veb[0],m_EditMappingsDeviceInput) )
			continue;

		RString sButtons = GetDeviceButtonsLocalized( hl.veb, m_EditMappingsDeviceInput );
		RString sDescription = THEME->GetString( "EditHelpDescription", hl.szEnglishDescription );

		// TODO: Better way of hiding routine only key on non-routine.
		if( hl.veb[0] == EDIT_BUTTON_SWITCH_PLAYERS && m_InputPlayerNumber == PLAYER_INVALID )
		{
			continue;
		}

		g_EditHelp.rows.push_back( MenuRowDef( -1, sDescription, false, EditMode_Practice, false, false, 0, sButtons ) );
	}

	EditMiniMenu( &g_EditHelp );
}

// lua start
#include "LuaBinding.h"

/** @brief Allow Lua to have access to ScreenEdit. */
class LunaScreenEdit: public Luna<ScreenEdit>
{
public:
	DEFINE_METHOD( GetEditState, GetEditState() )
	LunaScreenEdit()
	{
		ADD_METHOD( GetEditState );
	}
};

LUA_REGISTER_DERIVED_CLASS( ScreenEdit, ScreenWithMenuElements )

/*
 * (c) 2001-2004 Chris Danford
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
void ScreenEdit::OnSnapModeChange()
{
	m_soundChangeSnap.Play(true);

	NoteType nt = m_SnapDisplay.GetNoteType();
	int iStepIndex = BeatToNoteRow( GetBeat() );
	int iElementsPerNoteType = BeatToNoteRow( NoteTypeToBeat(nt) );
	int iStepIndexHangover = iStepIndex % iElementsPerNoteType;
	SetBeat( GetBeat() - NoteRowToBeat( iStepIndexHangover ) );
}



// Helper function for below

// Begin helper functions for InputEdit


static void ChangeDescription( const RString &sNew )
{
	Steps* pSteps = GAMESTATE->m_pCurSteps[PLAYER_1];

	// Don't erase edit descriptions.
	if( sNew.empty() && pSteps->GetDifficulty() == Difficulty_Edit )
		return;

	pSteps->SetDescription(sNew);
}

static void ChangeChartName( const RString &sNew )
{
	Steps *pSteps = GAMESTATE->m_pCurSteps[PLAYER_1];
	pSteps->SetChartName(sNew);
}

static void ChangeChartStyle( const RString &sNew )
{
	Steps* pSteps = GAMESTATE->m_pCurSteps[PLAYER_1];
	pSteps->SetChartStyle(sNew);
}

static void ChangeStepCredit( const RString &sNew )
{
	Steps* pSteps = GAMESTATE->m_pCurSteps[PLAYER_1];
	pSteps->SetCredit(sNew);
}

static void ChangeStepMeter( const RString &sNew )
{
	int diff = StringToInt(sNew);
	GAMESTATE->m_pCurSteps[PLAYER_1]->SetMeter(max(diff, 1));
}

static void ChangeStepMusic(const RString& sNew)
{
	Steps* pSteps = GAMESTATE->m_pCurSteps[PLAYER_1];
	pSteps->SetMusicFile(sNew);
}

static void ChangeMainTitle( const RString &sNew )
{
	Song* pSong = GAMESTATE->m_pCurSong;
	pSong->m_sMainTitle = sNew;
}

static void ChangeSubTitle( const RString &sNew )
{
	Song* pSong = GAMESTATE->m_pCurSong;
	pSong->m_sSubTitle = sNew;
}

static void ChangeArtist( const RString &sNew )
{
	Song* pSong = GAMESTATE->m_pCurSong;
	pSong->m_sArtist = sNew;
}

static void ChangeGenre( const RString &sNew )
{
	Song* pSong = GAMESTATE->m_pCurSong;
	pSong->m_sGenre = sNew;
}

static void ChangeCredit( const RString &sNew )
{
	Song* pSong = GAMESTATE->m_pCurSong;
	pSong->m_sCredit = sNew;
}

static void ChangePreview(const RString& sNew)
{
	Song* pSong = GAMESTATE->m_pCurSong;
	if(!sNew.empty())
	{
		RString error;
		RageSoundReader* sample= RageSoundReader_FileReader::OpenFile(pSong->GetPreviewMusicPath(), error);
		if(sample == nullptr)
		{
			LOG->UserLog( "Preview file", pSong->GetPreviewMusicPath(), "couldn't be opened: %s", error.c_str() );
		}
		else
		{
			pSong->m_fMusicSampleLengthSeconds= sample->GetLength() / 1000.0f;
			pSong->m_PreviewFile= sNew;
			delete sample;
		}
	}
	else
	{
		pSong->m_PreviewFile= sNew;
	}
}

static void ChangeMainTitleTranslit( const RString &sNew )
{
	Song* pSong = GAMESTATE->m_pCurSong;
	pSong->m_sMainTitleTranslit = sNew;
}

static void ChangeSubTitleTranslit( const RString &sNew )
{
	Song* pSong = GAMESTATE->m_pCurSong;
	pSong->m_sSubTitleTranslit = sNew;
}

static void ChangeArtistTranslit( const RString &sNew )
{
	Song* pSong = GAMESTATE->m_pCurSong;
	pSong->m_sArtistTranslit = sNew;
}

static void ChangeLastSecondHint( const RString &sNew )
{
	Song &s = *GAMESTATE->m_pCurSong;
	s.SetSpecifiedLastSecond(StringToFloat(sNew));
}

static void ChangePreviewStart( const RString &sNew )
{
	GAMESTATE->m_pCurSong->m_fMusicSampleStartSeconds = StringToFloat( sNew );
}

static void ChangePreviewLength( const RString &sNew )
{
	GAMESTATE->m_pCurSong->m_fMusicSampleLengthSeconds = StringToFloat( sNew );
}

static void ChangeMinBPM( const RString &sNew )
{
	GAMESTATE->m_pCurSong->m_fSpecifiedBPMMin = StringToFloat( sNew );
}

static void ChangeStepsMinBPM(const RString &sNew)
{
	Steps *step = GAMESTATE->m_pCurSteps[PLAYER_1];
	step->SetMinBPM(StringToFloat(sNew));
}

static void ChangeMaxBPM( const RString &sNew )
{
	GAMESTATE->m_pCurSong->m_fSpecifiedBPMMax = StringToFloat( sNew );
}

static void ChangeStepsMaxBPM(const RString &sNew)
{
	Steps *step = GAMESTATE->m_pCurSteps[PLAYER_1];
	step->SetMaxBPM(StringToFloat(sNew));
}

int ScreenEdit::GetSongOrNotesEnd()
{
	return max(m_iStartPlayingAt, max(m_NoteDataEdit.GetLastRow(),
			BeatToNoteRow(m_pSteps->GetTimingData()->GetBeatFromElapsedTime(
					GAMESTATE->m_pCurSong->m_fMusicLengthSeconds))));
}

void ScreenEdit::HandleMainMenuChoice( MainMenuChoice c, const vector<int> &iAnswers )
{
	GAMESTATE->SetProcessedTimingData(m_pSteps->GetTimingData());
	switch( c )
	{
		DEFAULT_FAIL( c );
		case play_selection:
			if( m_NoteFieldEdit.m_iBeginMarker!=-1 && m_NoteFieldEdit.m_iEndMarker!=-1 )
				HandleAlterMenuChoice( play );
			else if( m_NoteFieldEdit.m_iBeginMarker!=-1 )
				HandleMainMenuChoice( play_selection_start_to_end );
			else
				HandleMainMenuChoice( play_current_beat_to_end );
			break;
		case play_whole_song:
			{
				m_iStartPlayingAt = 0;
				m_iStopPlayingAt= GetSongOrNotesEnd();
				TransitionEditState( STATE_PLAYING );
			}
			break;
		case play_selection_start_to_end:
			{
				m_iStartPlayingAt = m_NoteFieldEdit.m_iBeginMarker;
				m_iStopPlayingAt = max( m_iStartPlayingAt, m_NoteDataEdit.GetLastRow() );
				TransitionEditState( STATE_PLAYING );
			}
			break;
		case play_current_beat_to_end:
			{
				m_iStartPlayingAt = BeatToNoteRow(GAMESTATE->m_pPlayerState[PLAYER_1]->m_Position.m_fSongBeat);
				m_iStopPlayingAt= GetSongOrNotesEnd();
				TransitionEditState( STATE_PLAYING );
			}
			break;
		case set_selection_start:
			{
				const int iCurrentRow = BeatToNoteRow(GAMESTATE->m_pPlayerState[PLAYER_1]->m_Position.m_fSongBeat);
				if( m_NoteFieldEdit.m_iEndMarker!=-1 && iCurrentRow >= m_NoteFieldEdit.m_iEndMarker )
				{
					SCREENMAN->PlayInvalidSound();
				}
				else
				{
					m_NoteFieldEdit.m_iBeginMarker = iCurrentRow;
					m_soundMarker.Play(true);
				}
			}
			break;
		case set_selection_end:
			{
				const int iCurrentRow = BeatToNoteRow(GAMESTATE->m_pPlayerState[PLAYER_1]->m_Position.m_fSongBeat);
				if( m_NoteFieldEdit.m_iBeginMarker!=-1 && iCurrentRow <= m_NoteFieldEdit.m_iBeginMarker )
				{
					SCREENMAN->PlayInvalidSound();
				}
				else
				{
					m_NoteFieldEdit.m_iEndMarker = iCurrentRow;
					m_soundMarker.Play(true);
				}
			}
			break;
		case edit_steps_information:
			{
				/* XXX: If the difficulty is changed from EDIT, and pSteps->WasLoadedFromProfile()
				 * is true, we should warn that the steps will no longer be saved to the profile. */
				Steps* pSteps = GAMESTATE->m_pCurSteps[PLAYER_1];

				g_StepsInformation.rows[difficulty].choices.clear();
				FOREACH_ENUM( Difficulty, dc )
				{
					g_StepsInformation.rows[difficulty].choices.push_back( "|" + CustomDifficultyToLocalizedString( GetCustomDifficulty(pSteps->m_StepsType, dc, CourseType_Invalid) ) );
				}
				g_StepsInformation.rows[difficulty].iDefaultChoice = pSteps->GetDifficulty();
				g_StepsInformation.rows[difficulty].bEnabled = (EDIT_MODE.GetValue() >= EditMode_Full);
				g_StepsInformation.rows[meter].SetOneUnthemedChoice( ssprintf("%d", pSteps->GetMeter()) );
				g_StepsInformation.rows[meter].bEnabled = (EDIT_MODE.GetValue() >= EditMode_Home);
				g_StepsInformation.rows[predict_meter].SetOneUnthemedChoice( ssprintf("%.2f",pSteps->PredictMeter()) );
				g_StepsInformation.rows[chartname].bEnabled = (EDIT_MODE.GetValue() >= EditMode_Full);
				g_StepsInformation.rows[chartname].SetOneUnthemedChoice(pSteps->GetChartName());
				g_StepsInformation.rows[description].bEnabled = (EDIT_MODE.GetValue() >= EditMode_Full);
				g_StepsInformation.rows[description].SetOneUnthemedChoice( pSteps->GetDescription() );
				g_StepsInformation.rows[chartstyle].bEnabled = (EDIT_MODE.GetValue() >= EditMode_Full);
				g_StepsInformation.rows[chartstyle].SetOneUnthemedChoice( pSteps->GetChartStyle() );
				g_StepsInformation.rows[step_credit].bEnabled = (EDIT_MODE.GetValue() >= EditMode_Full);
				g_StepsInformation.rows[step_credit].SetOneUnthemedChoice( pSteps->GetCredit() );
				g_StepsInformation.rows[step_display_bpm].iDefaultChoice = pSteps->GetDisplayBPM();
				g_StepsInformation.rows[step_min_bpm].SetOneUnthemedChoice( std::to_string(pSteps->GetMinBPM()));
				g_StepsInformation.rows[step_max_bpm].SetOneUnthemedChoice( std::to_string(pSteps->GetMaxBPM()));
				g_StepsInformation.rows[step_music].bEnabled = (EDIT_MODE.GetValue() >= EditMode_Full);
				g_StepsInformation.rows[step_music].SetOneUnthemedChoice( pSteps->GetMusicFile() );
				EditMiniMenu( &g_StepsInformation, SM_BackFromStepsInformation, SM_None );
			}
			break;
		case view_steps_data:
		{
			float fMusicSeconds = m_pSoundMusic->GetLengthSeconds();
			Steps* pSteps = GAMESTATE->m_pCurSteps[PLAYER_1];
			const StepsTypeCategory &cat = GAMEMAN->GetStepsTypeInfo(pSteps->m_StepsType).m_StepsTypeCategory;
			if (cat == StepsTypeCategory_Couple || cat == StepsTypeCategory_Routine)
			{
				pair<int, int> tmp = m_NoteDataEdit.GetNumTapNotesTwoPlayer();
				g_StepsData.rows[tap_notes].SetOneUnthemedChoice( ssprintf("%d / %d", tmp.first, tmp.second) );
				tmp = m_NoteDataEdit.GetNumJumpsTwoPlayer();
				g_StepsData.rows[jumps].SetOneUnthemedChoice( ssprintf("%d / %d", tmp.first, tmp.second) );
				tmp = m_NoteDataEdit.GetNumHandsTwoPlayer();
				g_StepsData.rows[hands].SetOneUnthemedChoice( ssprintf("%d / %d", tmp.first, tmp.second) );
				tmp = m_NoteDataEdit.GetNumQuadsTwoPlayer();
				g_StepsData.rows[quads].SetOneUnthemedChoice( ssprintf("%d / %d", tmp.first, tmp.second) );
				tmp = m_NoteDataEdit.GetNumHoldNotesTwoPlayer();
				g_StepsData.rows[holds].SetOneUnthemedChoice( ssprintf("%d / %d", tmp.first, tmp.second) );
				tmp = m_NoteDataEdit.GetNumMinesTwoPlayer();
				g_StepsData.rows[mines].SetOneUnthemedChoice( ssprintf("%d / %d", tmp.first, tmp.second) );
				tmp = m_NoteDataEdit.GetNumRollsTwoPlayer();
				g_StepsData.rows[rolls].SetOneUnthemedChoice( ssprintf("%d / %d", tmp.first, tmp.second) );
				tmp = m_NoteDataEdit.GetNumLiftsTwoPlayer();
				g_StepsData.rows[lifts].SetOneUnthemedChoice( ssprintf("%d / %d", tmp.first, tmp.second) );
				tmp = m_NoteDataEdit.GetNumFakesTwoPlayer();
				g_StepsData.rows[fakes].SetOneUnthemedChoice( ssprintf("%d / %d", tmp.first, tmp.second) );
			}
			else
			{
				g_StepsData.rows[tap_notes].SetOneUnthemedChoice( ssprintf("%d", m_NoteDataEdit.GetNumTapNotes()) );
				g_StepsData.rows[jumps].SetOneUnthemedChoice( ssprintf("%d", m_NoteDataEdit.GetNumJumps()) );
				g_StepsData.rows[hands].SetOneUnthemedChoice( ssprintf("%d", m_NoteDataEdit.GetNumHands()) );
				g_StepsData.rows[quads].SetOneUnthemedChoice( ssprintf("%d", m_NoteDataEdit.GetNumQuads()) );
				g_StepsData.rows[holds].SetOneUnthemedChoice( ssprintf("%d", m_NoteDataEdit.GetNumHoldNotes()) );
				g_StepsData.rows[mines].SetOneUnthemedChoice( ssprintf("%d", m_NoteDataEdit.GetNumMines()) );
				g_StepsData.rows[rolls].SetOneUnthemedChoice( ssprintf("%d", m_NoteDataEdit.GetNumRolls()) );
				g_StepsData.rows[lifts].SetOneUnthemedChoice( ssprintf("%d", m_NoteDataEdit.GetNumLifts()) );
				g_StepsData.rows[fakes].SetOneUnthemedChoice( ssprintf("%d", m_NoteDataEdit.GetNumFakes()) );
			}
			RadarValues radar;
			radar.Zero();
			NoteDataUtil::CalculateRadarValues(m_NoteDataEdit, fMusicSeconds, radar);
			g_StepsData.rows[stream].SetOneUnthemedChoice(ssprintf("%.2f", radar[RadarCategory_Stream]));
			g_StepsData.rows[voltage].SetOneUnthemedChoice(ssprintf("%.2f", radar[RadarCategory_Voltage]));
			g_StepsData.rows[air].SetOneUnthemedChoice(ssprintf("%.2f", radar[RadarCategory_Air]));
			g_StepsData.rows[freeze].SetOneUnthemedChoice(ssprintf("%.2f", radar[RadarCategory_Freeze]));
			g_StepsData.rows[chaos].SetOneUnthemedChoice(ssprintf("%.2f", radar[RadarCategory_Chaos]));
			EditMiniMenu( &g_StepsData, SM_BackFromStepsData, SM_None );
			break;
		}
		case save:
		case save_on_exit:
			m_CurrentAction = c;
			PerformSave(false);
			break;
		case revert_to_last_save:
			ScreenPrompt::Prompt( SM_DoRevertToLastSave, REVERT_LAST_SAVE.GetValue() + "\n\n" + DESTROY_ALL_UNSAVED_CHANGES.GetValue(), PROMPT_YES_NO, ANSWER_NO );
			break;
		case revert_from_disk:
			ScreenPrompt::Prompt( SM_DoRevertFromDisk, REVERT_FROM_DISK.GetValue() + "\n\n" + DESTROY_ALL_UNSAVED_CHANGES.GetValue(), PROMPT_YES_NO, ANSWER_NO );
			break;
		case options:
			SCREENMAN->AddNewScreenToTop( OPTIONS_SCREEN, SM_BackFromOptions );
			break;
		case edit_song_info:
			{
				const Song* pSong = GAMESTATE->m_pCurSong;
				g_SongInformation.rows[main_title].SetOneUnthemedChoice( pSong->m_sMainTitle );
				g_SongInformation.rows[sub_title].SetOneUnthemedChoice( pSong->m_sSubTitle );
				g_SongInformation.rows[artist].SetOneUnthemedChoice( pSong->m_sArtist );
				g_SongInformation.rows[genre].SetOneUnthemedChoice( pSong->m_sGenre );
				g_SongInformation.rows[credit].SetOneUnthemedChoice( pSong->m_sCredit );
				g_SongInformation.rows[preview].SetOneUnthemedChoice(pSong->m_PreviewFile);
				g_SongInformation.rows[main_title_transliteration].SetOneUnthemedChoice( pSong->m_sMainTitleTranslit );
				g_SongInformation.rows[sub_title_transliteration].SetOneUnthemedChoice( pSong->m_sSubTitleTranslit );
				g_SongInformation.rows[artist_transliteration].SetOneUnthemedChoice( pSong->m_sArtistTranslit );
				g_SongInformation.rows[last_second_hint].SetOneUnthemedChoice( std::to_string(pSong->GetSpecifiedLastSecond()) );
				g_SongInformation.rows[preview_start].SetOneUnthemedChoice( std::to_string(pSong->m_fMusicSampleStartSeconds) );
				g_SongInformation.rows[preview_length].SetOneUnthemedChoice( std::to_string(pSong->m_fMusicSampleLengthSeconds) );
				g_SongInformation.rows[display_bpm].iDefaultChoice = pSong->m_DisplayBPMType;
				g_SongInformation.rows[min_bpm].SetOneUnthemedChoice( std::to_string(pSong->m_fSpecifiedBPMMin) );
				g_SongInformation.rows[max_bpm].SetOneUnthemedChoice( std::to_string(pSong->m_fSpecifiedBPMMax) );

				EditMiniMenu( &g_SongInformation, SM_BackFromSongInformation );
			}
			break;
		case edit_timing_data:
			{
				DisplayTimingMenu();
			}
			break;

		case play_preview_music:
			PlayPreviewMusic();
			break;
		case exit:
			switch( EDIT_MODE.GetValue() )
			{
			DEFAULT_FAIL( EDIT_MODE.GetValue() );
			case EditMode_Full:
			case EditMode_Home:
				if( IsDirty() )
					ScreenPrompt::Prompt( SM_DoSaveAndExit, SAVE_CHANGES_BEFORE_EXITING, PROMPT_YES_NO_CANCEL, ANSWER_CANCEL );
				else
					SCREENMAN->SendMessageToTopScreen( SM_DoExit );
				break;
			case EditMode_Practice:
			case EditMode_CourseMods:
				SCREENMAN->SendMessageToTopScreen( SM_DoExit );
				break;
			}
			break;
	};
	GAMESTATE->SetProcessedTimingData(nullptr);
}

static LocalizedString ENTER_ARBITRARY_MAPPING( "ScreenEdit", "Enter the new track mapping." );
static LocalizedString TOO_MANY_TRACKS("ScreenEdit", "Too many tracks specified.");
static LocalizedString NOT_A_TRACK("ScreenEdit", "'%s' is not a track id.");
static LocalizedString OUT_OF_RANGE_ID("ScreenEdit", "Entry %d, '%d', is out of range 1 to %d.");
static LocalizedString CONFIRM_CLEAR("ScreenEdit", "Are you sure you want to clear %d notes?");

static bool ConvertMappingInputToMapping(RString const& mapstr, int* mapping, RString& error)
{
	vector<RString> mapping_input;
	split(mapstr, ",", mapping_input);
	size_t tracks_for_type= GAMEMAN->GetStepsTypeInfo(GAMESTATE->m_pCurSteps[0]->m_StepsType).iNumTracks;
	if(mapping_input.size() > tracks_for_type)
	{
		error= TOO_MANY_TRACKS;
		return false;
	}
	// mapping_input.size() < tracks_for_type is not checked because
	// unspecified tracks are mapped directly. -Kyz
	size_t track= 0;
	// track will be used for filling in the unspecified part of the mapping.
	for(; track < mapping_input.size(); ++track)
	{
		if(mapping_input[track].empty() || mapping_input[track] == " ")
		{
			// This allows blank entries to mean "pass through".
			mapping[track]= track+1;
		}
		else if(!(mapping_input[track] >> mapping[track]))
		{
			error= ssprintf(NOT_A_TRACK.GetValue(), mapping_input[track].c_str());
			return false;
		}
		if(mapping[track] < 1 || mapping[track] > static_cast<int>(tracks_for_type))
		{
			error= ssprintf(OUT_OF_RANGE_ID.GetValue(), track+1, mapping[track], tracks_for_type);
			return false;
		}
		// Simpler for the user if they input track ids starting at 1.
		--mapping[track];
	}
	for(; track < tracks_for_type; ++track)
	{
		mapping[track]= track;
	}
	return true;
}

static bool ArbitraryRemapValidate(const RString& answer, RString& error_out)
{
	int mapping[MAX_NOTE_TRACKS];
	return ConvertMappingInputToMapping(answer, mapping, error_out);
}

void ScreenEdit::HandleArbitraryRemapping(RString const& mapstr)
{
	const NoteData OldClipboard( m_Clipboard );
	HandleAlterMenuChoice( cut, false );
	int mapping[MAX_NOTE_TRACKS];
	RString error;
	// error is actually reported by the validate function, and unused here.
	if(ConvertMappingInputToMapping(mapstr, mapping, error))
	{
		NoteDataUtil::ArbitraryRemap(m_Clipboard, mapping);
	}
	HandleAreaMenuChoice( paste_at_begin_marker, false );
	m_Clipboard = OldClipboard;
}

void ScreenEdit::HandleAlterMenuChoice(AlterMenuChoice c, const vector<int> &answers, bool allow_undo, bool prompt_clear)
{
	ASSERT_M(m_NoteFieldEdit.m_iBeginMarker!=-1 && m_NoteFieldEdit.m_iEndMarker!=-1,
			 "You can only alter a selection of notes with a selection to begin with!");

	bool bSaveUndo = true;
	switch (c)
	{
		case play:
		case record:
		case cut:
		case copy:
		{
			bSaveUndo = false;
		}
		default:
			break;
	}

	if( bSaveUndo )
		SetDirty( true );

	/* We call HandleAreaMenuChoice recursively. Only the outermost
	 * HandleAreaMenuChoice should allow Undo so that the inner calls don't
	 * also save Undo and mess up the outermost */
	if(!allow_undo)
		bSaveUndo = false;

	if( bSaveUndo )
		SaveUndo();

	switch(c)
	{
		case cut:
		{
			HandleAlterMenuChoice(copy, false, false);
			HandleAlterMenuChoice(clear, false, false);
		}
			break;
		case copy:
		{

			m_Clipboard.ClearAll();
			m_Clipboard.CopyRange( m_NoteDataEdit, m_NoteFieldEdit.m_iBeginMarker, m_NoteFieldEdit.m_iEndMarker );
		}
			break;
		case clear:
		{
			int note_count= m_NoteDataEdit.GetNumTapNotesNoTiming(
				m_NoteFieldEdit.m_iBeginMarker, m_NoteFieldEdit.m_iEndMarker);
			if(note_count >= PREFSMAN->m_EditClearPromptThreshold && prompt_clear)
			{
				ScreenPrompt::Prompt(SM_ConfirmClearArea, ssprintf(CONFIRM_CLEAR.GetValue(), note_count), PROMPT_YES_NO);
			}
			else
			{
				m_NoteDataEdit.ClearRange(
					m_NoteFieldEdit.m_iBeginMarker, m_NoteFieldEdit.m_iEndMarker);
			}
		}
			break;
		case quantize:
		{
			NoteType nt = (NoteType)answers[c];
			NoteDataUtil::SnapToNearestNoteType(m_NoteDataEdit, nt, nt,
							    m_NoteFieldEdit.m_iBeginMarker,
							    m_NoteFieldEdit.m_iEndMarker );
			break;
		}
		case turn:
		{
			const NoteData OldClipboard( m_Clipboard );
			HandleAlterMenuChoice( cut, false );

			StepsType st = GAMESTATE->GetCurrentStyle(GAMESTATE->GetMasterPlayerNumber())->m_StepsType;
			TurnType tt = (TurnType)answers[c];
			switch( tt )
			{
				DEFAULT_FAIL( tt );
				case left:		NoteDataUtil::Turn( m_Clipboard, st, NoteDataUtil::left );		break;
				case right:		NoteDataUtil::Turn( m_Clipboard, st, NoteDataUtil::right );		break;
				case mirror:		NoteDataUtil::Turn( m_Clipboard, st, NoteDataUtil::mirror );		break;
				case turn_backwards:		NoteDataUtil::Turn( m_Clipboard, st, NoteDataUtil::backwards );		break;
				case shuffle:		NoteDataUtil::Turn( m_Clipboard, st, NoteDataUtil::shuffle );		break;
				case super_shuffle:	NoteDataUtil::Turn( m_Clipboard, st, NoteDataUtil::super_shuffle );	break;
			}

			HandleAreaMenuChoice( paste_at_begin_marker, false );
			m_Clipboard = OldClipboard;
		}
			break;
		case transform:
		{
			int iBeginRow = m_NoteFieldEdit.m_iBeginMarker;
			int iEndRow = m_NoteFieldEdit.m_iEndMarker;
			TransformType tt = (TransformType)answers[c];
			StepsType st = GAMESTATE->GetCurrentStyle(GAMESTATE->GetMasterPlayerNumber())->m_StepsType;

			switch( tt )
			{
					DEFAULT_FAIL( tt );
				case noholds:		NoteDataUtil::RemoveHoldNotes( m_NoteDataEdit, iBeginRow, iEndRow );	break;
				case nomines:		NoteDataUtil::RemoveMines( m_NoteDataEdit, iBeginRow, iEndRow );	break;
				case little:		NoteDataUtil::Little( m_NoteDataEdit, iBeginRow, iEndRow );		break;
				case wide:		NoteDataUtil::Wide( m_NoteDataEdit, iBeginRow, iEndRow );		break;
				case big:		NoteDataUtil::Big( m_NoteDataEdit, iBeginRow, iEndRow );		break;
				case quick:		NoteDataUtil::Quick( m_NoteDataEdit, iBeginRow, iEndRow );		break;
				case skippy:		NoteDataUtil::Skippy( m_NoteDataEdit, iBeginRow, iEndRow );		break;
				case add_mines:		NoteDataUtil::AddMines( m_NoteDataEdit, iBeginRow, iEndRow );		break;
				case echo:		NoteDataUtil::Echo( m_NoteDataEdit, iBeginRow, iEndRow );		break;
				case stomp:		NoteDataUtil::Stomp( m_NoteDataEdit, st, iBeginRow, iEndRow );		break;
				case planted:		NoteDataUtil::Planted( m_NoteDataEdit, iBeginRow, iEndRow );		break;
				case floored:		NoteDataUtil::Floored( m_NoteDataEdit, iBeginRow, iEndRow );		break;
				case twister:		NoteDataUtil::Twister( m_NoteDataEdit, iBeginRow, iEndRow );		break;
				case nojumps:		NoteDataUtil::RemoveJumps( m_NoteDataEdit, iBeginRow, iEndRow );	break;
				case nohands:		NoteDataUtil::RemoveHands( m_NoteDataEdit, iBeginRow, iEndRow );	break;
				case noquads:		NoteDataUtil::RemoveQuads( m_NoteDataEdit, iBeginRow, iEndRow );	break;
				case nostretch:		NoteDataUtil::RemoveStretch( m_NoteDataEdit, st, iBeginRow, iBeginRow );break;
			}

			// bake in the additions
			NoteDataUtil::ConvertAdditionsToRegular( m_NoteDataEdit );
			break;
		}
		case alter:
		{
			const NoteData OldClipboard( m_Clipboard );
			HandleAlterMenuChoice( cut, false );

			AlterType at = (AlterType)answers[c];
			switch( at )
			{
				DEFAULT_FAIL( at );
				case autogen_to_fill_width:
				{
					NoteData temp( m_Clipboard );
					int iMaxNonEmptyTrack = NoteDataUtil::GetMaxNonEmptyTrack( temp );
					if( iMaxNonEmptyTrack == -1 )
						break;
					temp.SetNumTracks( iMaxNonEmptyTrack+1 );
					NoteDataUtil::LoadTransformedSlidingWindow( temp, m_Clipboard, m_Clipboard.GetNumTracks() );
					NoteDataUtil::RemoveStretch( m_Clipboard, GAMESTATE->m_pCurSteps[0]->m_StepsType );
				}
					break;
				case backwards:			NoteDataUtil::Backwards( m_Clipboard );			break;
				case swap_sides:		NoteDataUtil::SwapSides( m_Clipboard );			break;
				case copy_left_to_right:	NoteDataUtil::CopyLeftToRight( m_Clipboard );		break;
				case copy_right_to_left:	NoteDataUtil::CopyRightToLeft( m_Clipboard );		break;
				case clear_left:		NoteDataUtil::ClearLeft( m_Clipboard );			break;
				case clear_right:		NoteDataUtil::ClearRight( m_Clipboard );		break;
				case collapse_to_one:		NoteDataUtil::CollapseToOne( m_Clipboard );		break;
				case collapse_left:		NoteDataUtil::CollapseLeft( m_Clipboard );		break;
				case shift_left:		NoteDataUtil::ShiftLeft( m_Clipboard );			break;
				case shift_right:		NoteDataUtil::ShiftRight( m_Clipboard );		break;
				case swap_up_down: NoteDataUtil::SwapUpDown(m_Clipboard, GAMESTATE->m_pCurSteps[0]->m_StepsType); break;
				case arbitrary_remap:
					ScreenTextEntry::TextEntry(
						SM_BackFromArbitraryRemap, ENTER_ARBITRARY_MAPPING,
						"1, 2, 3, 4", MAX_NOTE_TRACKS * 4,
						// 2 chars for digit, one for comma, one for space.
						ArbitraryRemapValidate
				);
					break;
			}

			HandleAreaMenuChoice( paste_at_begin_marker, false );
			m_Clipboard = OldClipboard;
			break;
		}
		case tempo:
		{
			// This affects all steps.
			AlterType at = (AlterType)answers[c];
			float fScale = -1;

			switch( at )
			{
					DEFAULT_FAIL( at );
				case compress_2x:	fScale = 0.5f;		break;
				case compress_3_2:	fScale = 2.0f/3;	break;
				case compress_4_3:	fScale = 0.75f;		break;
				case expand_4_3:	fScale = 4.0f/3;	break;
				case expand_3_2:	fScale = 1.5f;		break;
				case expand_2x:		fScale = 2;		break;
			}

			int iStartIndex  = m_NoteFieldEdit.m_iBeginMarker;
			int iEndIndex    = m_NoteFieldEdit.m_iEndMarker;
			int iNewEndIndex = iEndIndex + lrintf( (iEndIndex - iStartIndex) * (fScale - 1) );

			// scale currently editing notes
			NoteDataUtil::ScaleRegion( m_NoteDataEdit, fScale, iStartIndex, iEndIndex );

			// scale timing data
			GetAppropriateTimingForUpdate().ScaleRegion(fScale,
							   m_NoteFieldEdit.m_iBeginMarker,
							   m_NoteFieldEdit.m_iEndMarker, true );

			m_NoteFieldEdit.m_iEndMarker = iNewEndIndex;
			break;

		}

		case play:
			m_iStartPlayingAt = m_NoteFieldEdit.m_iBeginMarker;
			m_iStopPlayingAt = m_NoteFieldEdit.m_iEndMarker;
			TransitionEditState( STATE_PLAYING );
			break;
		case record:
			m_iStartPlayingAt = m_NoteFieldEdit.m_iBeginMarker;
			m_iStopPlayingAt = m_NoteFieldEdit.m_iEndMarker;
			TransitionEditState( STATE_RECORDING );
			break;
		case preview_designation:
		{
			float fMarkerStart = GetAppropriateTiming().GetElapsedTimeFromBeat( NoteRowToBeat(m_NoteFieldEdit.m_iBeginMarker) );
			float fMarkerEnd = GetAppropriateTiming().GetElapsedTimeFromBeat( NoteRowToBeat(m_NoteFieldEdit.m_iEndMarker) );
			GAMESTATE->m_pCurSong->m_fMusicSampleStartSeconds = fMarkerStart;
			GAMESTATE->m_pCurSong->m_fMusicSampleLengthSeconds = fMarkerEnd - fMarkerStart;
			break;
		}
		case convert_to_pause:
		{
			float fMarkerStart = GetAppropriateTiming().GetElapsedTimeFromBeat( NoteRowToBeat(m_NoteFieldEdit.m_iBeginMarker) );
			float fMarkerEnd = GetAppropriateTiming().GetElapsedTimeFromBeat( NoteRowToBeat(m_NoteFieldEdit.m_iEndMarker) );

			// The length of the stop segment we're going to create.  This includes time spent in any
			// stops in the selection, which will be deleted and subsumed into the new stop.
			float fStopLength = fMarkerEnd - fMarkerStart;

			// be sure not to clobber the row at the start - a row at the end
			// can be dropped safely, though
			NoteDataUtil::DeleteRows( m_NoteDataEdit,
						 m_NoteFieldEdit.m_iBeginMarker + 1,
						 m_NoteFieldEdit.m_iEndMarker-m_NoteFieldEdit.m_iBeginMarker
						 );
			// For TimingData, it makes more sense not to offset by a row
			GetAppropriateTimingForUpdate().DeleteRows( m_NoteFieldEdit.m_iBeginMarker,
							  m_NoteFieldEdit.m_iEndMarker-m_NoteFieldEdit.m_iBeginMarker );
			GetAppropriateTimingForUpdate().SetStopAtRow( m_NoteFieldEdit.m_iBeginMarker, fStopLength );
			m_NoteFieldEdit.m_iBeginMarker = -1;
			m_NoteFieldEdit.m_iEndMarker = -1;
			break;
		}
		case convert_to_delay:
		{
			float fMarkerStart = GetAppropriateTiming().GetElapsedTimeFromBeat( NoteRowToBeat(m_NoteFieldEdit.m_iBeginMarker) );
			float fMarkerEnd = GetAppropriateTiming().GetElapsedTimeFromBeat( NoteRowToBeat(m_NoteFieldEdit.m_iEndMarker) );

			// The length of the delay segment we're going to create.  This includes time spent in any
			// stops in the selection, which will be deleted and subsumed into the new stop.
			float fStopLength = fMarkerEnd - fMarkerStart;

			NoteDataUtil::DeleteRows( m_NoteDataEdit,
						 m_NoteFieldEdit.m_iBeginMarker,
						 m_NoteFieldEdit.m_iEndMarker-m_NoteFieldEdit.m_iBeginMarker
						 );
			GetAppropriateTimingForUpdate().DeleteRows( m_NoteFieldEdit.m_iBeginMarker,
							  m_NoteFieldEdit.m_iEndMarker-m_NoteFieldEdit.m_iBeginMarker );
			GetAppropriateTimingForUpdate().SetDelayAtRow( m_NoteFieldEdit.m_iBeginMarker, fStopLength );
			m_NoteFieldEdit.m_iBeginMarker = -1;
			m_NoteFieldEdit.m_iEndMarker = -1;
			break;
		}
		case convert_to_warp:
		{
			float startBeat = NoteRowToBeat(m_NoteFieldEdit.m_iBeginMarker);
			float lengthBeat = NoteRowToBeat(m_NoteFieldEdit.m_iEndMarker) - startBeat;
			GetAppropriateTimingForUpdate().SetWarpAtBeat(startBeat,lengthBeat);
			SetDirty(true);
			break;
		}
		case convert_to_attack:
		{
			float startBeat = NoteRowToBeat(m_NoteFieldEdit.m_iBeginMarker);
			float endBeat = NoteRowToBeat(m_NoteFieldEdit.m_iEndMarker);
			const TimingData &timing = GetAppropriateTiming();
			float &start = g_fLastInsertAttackPositionSeconds;
			float &length = g_fLastInsertAttackDurationSeconds;
			start = timing.GetElapsedTimeFromBeat(startBeat);
			length = timing.GetElapsedTimeFromBeat(endBeat) - start;

			AttackArray &attacks = GAMESTATE->m_bIsUsingStepTiming ?
				m_pSteps->m_Attacks : m_pSong->m_Attacks;
			int iAttack = FindAttackAtTime(attacks, start);

			ModsGroup<PlayerOptions> &toEdit = GAMESTATE->m_pPlayerState[PLAYER_1]->m_PlayerOptions;
			this->originalPlayerOptions.Assign(ModsLevel_Preferred, toEdit.GetPreferred());
			PlayerOptions po;
			if (iAttack >= 0)
				po.FromString(attacks[iAttack].sModifiers);

			toEdit.Assign( ModsLevel_Preferred, po );
			SCREENMAN->AddNewScreenToTop( SET_MOD_SCREEN, SM_BackFromInsertStepAttackPlayerOptions );
			SetDirty(true);
			break;
		}
		case convert_to_fake:
		{
			int startRow = m_NoteFieldEdit.m_iBeginMarker;
			float lengthBeat = NoteRowToBeat(m_NoteFieldEdit.m_iEndMarker) - NoteRowToBeat(startRow);
			GetAppropriateTimingForUpdate().AddSegment( FakeSegment(startRow,lengthBeat) );
			SetDirty(true);
			break;
		}
		case routine_invert_notes:
		{
			NoteData &nd = this->m_NoteDataEdit;
			NoteField &nf = this->m_NoteFieldEdit;
			FOREACH_NONEMPTY_ROW_ALL_TRACKS_RANGE(nd, r,
							      nf.m_iBeginMarker,
							      nf.m_iEndMarker)
			{
				for (int t = 0; t < nd.GetNumTracks(); t++)
				{
					const TapNote &tn = nd.GetTapNote(t, r);
					if (tn.type != TapNoteType_Empty)
					{
						TapNote nTap = tn;
						nTap.pn = (tn.pn == PLAYER_1 ?
							   PLAYER_2 : PLAYER_1);
						m_NoteDataEdit.SetTapNote(t, r, nTap);
					}
				}
			}
			break;
		}
		case routine_mirror_1_to_2:
		case routine_mirror_2_to_1:
		{
			PlayerNumber oPN = (c == routine_mirror_1_to_2 ?
					    PLAYER_1 : PLAYER_2);
			PlayerNumber nPN = (c == routine_mirror_1_to_2 ?
					    PLAYER_2 : PLAYER_1);
			int nTrack = -1;
			NoteData &nd = this->m_NoteDataEdit;
			NoteField &nf = this->m_NoteFieldEdit;
			int tracks = nd.GetNumTracks();
			FOREACH_NONEMPTY_ROW_ALL_TRACKS_RANGE(nd, r,
							      nf.m_iBeginMarker,
							      nf.m_iEndMarker)
			{
				for (int t = 0; t < tracks; t++)
				{
					const TapNote &tn = nd.GetTapNote(t, r);
					if (tn.type != TapNoteType_Empty && tn.pn == oPN)
					{
						TapNote nTap = tn;
						nTap.pn = nPN;
						StepsType curType = GAMESTATE->m_pCurSteps[PLAYER_1]->m_StepsType;
						// TODO: Find a better way to do this.
						if (curType == StepsType_dance_routine)
						{
							nTrack = tracks - t - 1;
						}
						else if (curType == StepsType_pump_routine)
						{
							switch (t)
							{
								case 0: nTrack = 8; break;
								case 1: nTrack = 9; break;
								case 2: nTrack = 7; break;
								case 3: nTrack = 5; break;
								case 4: nTrack = 6; break;
								case 5: nTrack = 3; break;
								case 6: nTrack = 4; break;
								case 7: nTrack = 2; break;
								case 8: nTrack = 0; break;
								case 9: nTrack = 1; break;
								default: FAIL_M(ssprintf("Invalid column %d for pump-routine", t)); break;
							}
						}
						m_NoteDataEdit.SetTapNote(nTrack, r, nTap);
					}
				}
			}
			break;
		}
		default: break;
	}

}

void ScreenEdit::HandleAreaMenuChoice( AreaMenuChoice c, const vector<int> &iAnswers, bool bAllowUndo )
{
	bool bSaveUndo = true;
	switch( c )
	{
		case clear_clipboard:
		case undo:
			bSaveUndo = false;
			break;
		default:
			break;
	}

	if( bSaveUndo )
		SetDirty( true );

	/* We call HandleAreaMenuChoice recursively. Only the outermost
	 * HandleAreaMenuChoice should allow Undo so that the inner calls don't
	 * also save Undo and mess up the outermost */
	if( !bAllowUndo )
		bSaveUndo = false;

	if( bSaveUndo )
		SaveUndo();

	switch( c )
	{
		DEFAULT_FAIL( c );

		case paste_at_current_beat:
		case paste_at_begin_marker:
			{
				int iDestFirstRow = -1;
				switch( c )
				{
					DEFAULT_FAIL( c );
					case paste_at_current_beat:
						iDestFirstRow = BeatToNoteRow( GetAppropriatePosition().m_fSongBeat );
						break;
					case paste_at_begin_marker:
						ASSERT( m_NoteFieldEdit.m_iBeginMarker!=-1 );
						iDestFirstRow = m_NoteFieldEdit.m_iBeginMarker;
						break;
				}

				int iRowsToCopy = m_Clipboard.GetLastRow()+1;
				m_NoteDataEdit.CopyRange( m_Clipboard, 0, iRowsToCopy, iDestFirstRow );
			}
			break;

		case insert_and_shift:
			NoteDataUtil::InsertRows( m_NoteDataEdit, GetRow(),
				GetRowsFromAnswers(c, iAnswers));
			break;
		case delete_and_shift:
			NoteDataUtil::DeleteRows( m_NoteDataEdit, GetRow(),
				GetRowsFromAnswers(c, iAnswers));
			break;
		case shift_pauses_forward:
			GetAppropriateTimingForUpdate().InsertRows( GetRow(),
				GetRowsFromAnswers(c, iAnswers));
			break;
		case shift_pauses_backward:
			GetAppropriateTimingForUpdate().DeleteRows( GetRow(),
				GetRowsFromAnswers(c, iAnswers));
			break;

		case convert_pause_to_beat:
		{
			float fStopSeconds = GetAppropriateTiming().GetStopAtRow(GetRow());
			GetAppropriateTimingForUpdate().SetStopAtBeat( GetBeat() , 0 );

			float fStopBeats = fStopSeconds * GetAppropriateTiming().GetBPMAtBeat( GetBeat() ) / 60;

			// don't move the step from where it is, just move everything later
			NoteDataUtil::InsertRows( m_NoteDataEdit, GetRow() + 1, BeatToNoteRow(fStopBeats) );
			GetAppropriateTimingForUpdate().InsertRows( GetRow() + 1, BeatToNoteRow(fStopBeats) );
		}
		break;
		case convert_delay_to_beat:
		{
			TimingData &timing = GetAppropriateTimingForUpdate();
			float pause = timing.GetDelayAtRow(GetRow());
			timing.SetDelayAtRow(GetRow(), 0);

			float pauseBeats = pause * timing.GetBPMAtBeat(GetBeat()) / 60;

			NoteDataUtil::InsertRows(m_NoteDataEdit, GetRow(), BeatToNoteRow(pauseBeats));
			timing.InsertRows(GetRow(), BeatToNoteRow(pauseBeats));
			break;
		}
		case last_second_at_beat:
		{
			const TimingData &timing = GetAppropriateTiming();
			Song &s = *GAMESTATE->m_pCurSong;
			s.SetSpecifiedLastSecond(timing.GetElapsedTimeFromBeat(GetBeat()));
			break;
		}
		case undo:
			Undo();
			break;
		case clear_clipboard:
		{
			m_Clipboard.ClearAll();
			break;
		}
		case modify_attacks_at_row:
		{
			this->DoStepAttackMenu();
			break;
		}
		case modify_keysounds_at_row:
		{
			this->DoKeyboardTrackMenu();
			break;
		}
	};

	if( bSaveUndo )
		CheckNumberOfNotesAndUndo();
}

void ScreenEdit::HandleStepsDataChoice( StepsDataChoice c, const vector<int> &iAnswers )
{
	return; // nothing is done with the choices. Yet.
}

static LocalizedString ENTER_NEW_DESCRIPTION( "ScreenEdit", "Enter a new description." );
static LocalizedString ENTER_NEW_CHART_NAME("ScreenEdit", "Enter a new chart name.");
static LocalizedString ENTER_NEW_CHART_STYLE( "ScreenEdit", "Enter a new chart style." );
static LocalizedString ENTER_NEW_STEP_AUTHOR( "ScreenEdit", "Enter the author who made this step pattern." );
static LocalizedString ENTER_NEW_METER( "ScreenEdit", "Enter a new meter." );
static LocalizedString ENTER_MIN_BPM			("ScreenEdit","Enter a new min BPM.");
static LocalizedString ENTER_MAX_BPM			("ScreenEdit","Enter a new max BPM.");
static LocalizedString ENTER_NEW_STEP_MUSIC("ScreenEdit", "Enter the music file for this chart.");
void ScreenEdit::HandleStepsInformationChoice( StepsInformationChoice c, const vector<int> &iAnswers )
{
	Steps* pSteps = GAMESTATE->m_pCurSteps[PLAYER_1];
	Difficulty dc = (Difficulty)iAnswers[difficulty];
	pSteps->SetDifficulty( dc );
	pSteps->SetDisplayBPM(static_cast<DisplayBPM>(iAnswers[step_display_bpm]));

	switch( c )
	{
		case chartname:
		{
			ScreenTextEntry::TextEntry(SM_None,
									   ENTER_NEW_CHART_NAME,
									   m_pSteps->GetChartName(),
									   MAX_STEPS_DESCRIPTION_LENGTH,
									   SongUtil::ValidateCurrentStepsChartName,
									   ChangeChartName,
									   nullptr);
			break;
		}
		case description:
		{
			ScreenTextEntry::TextEntry(SM_None,
									   ENTER_NEW_DESCRIPTION,
									   m_pSteps->GetDescription(),
									   MAX_STEPS_DESCRIPTION_LENGTH,
									   SongUtil::ValidateCurrentStepsDescription,
									   ChangeDescription,
									   nullptr);
			break;
		}
		case chartstyle:
		{
			ScreenTextEntry::TextEntry(SM_None,
									   ENTER_NEW_CHART_STYLE,
									   m_pSteps->GetChartStyle(),
									   255,
									   nullptr,
									   ChangeChartStyle,
									   nullptr);
			break;
		}
		case step_credit:
		{
			ScreenTextEntry::TextEntry(SM_None,
									   ENTER_NEW_STEP_AUTHOR,
									   m_pSteps->GetCredit(),
									   255,
									   SongUtil::ValidateCurrentStepsCredit,
									   ChangeStepCredit,
									   nullptr);
			break;
		}
		case meter:
		{
			ScreenTextEntry::TextEntry(SM_BackFromDifficultyMeterChange,
									   ENTER_NEW_METER,
									   ssprintf("%d", m_pSteps->GetMeter()),
									   4,
									   ScreenTextEntry::IntValidate,
									   ChangeStepMeter,
									   nullptr);
			break;
		}
		case step_min_bpm:
		{
			ScreenTextEntry::TextEntry(SM_None, ENTER_MIN_BPM,
									   std::to_string(pSteps->GetMinBPM()), 20,
									   ScreenTextEntry::FloatValidate,
									   ChangeStepsMinBPM, nullptr);
			break;
		}
		case step_max_bpm:
		{
			ScreenTextEntry::TextEntry(SM_None, ENTER_MAX_BPM,
									   std::to_string(pSteps->GetMaxBPM()), 20,
									   ScreenTextEntry::FloatValidate,
									   ChangeStepsMaxBPM, nullptr);
			break;
		}
		case step_music:
		{
			ScreenTextEntry::TextEntry(SM_BackFromStepMusicChange,
									   ENTER_NEW_STEP_MUSIC,
									   m_pSteps->GetMusicFile(),
									   255,
									   SongUtil::ValidateCurrentStepsMusic,
									   ChangeStepMusic,
									   nullptr);
			break;
		}
	default:
		break;
	}
	SetDirty(true);
}

static LocalizedString ENTER_MAIN_TITLE			("ScreenEdit","Enter a new main title.");
static LocalizedString ENTER_SUB_TITLE			("ScreenEdit","Enter a new sub title.");
static LocalizedString ENTER_ARTIST			("ScreenEdit","Enter a new artist.");
static LocalizedString ENTER_GENRE			("ScreenEdit","Enter a new genre.");
static LocalizedString ENTER_CREDIT			("ScreenEdit","Enter a new credit.");
static LocalizedString ENTER_PREVIEW			("ScreenEdit","Enter a preview file.");
static LocalizedString ENTER_MAIN_TITLE_TRANSLIT	("ScreenEdit","Enter a new main title transliteration.");
static LocalizedString ENTER_SUB_TITLE_TRANSLIT		("ScreenEdit","Enter a new sub title transliteration.");
static LocalizedString ENTER_ARTIST_TRANSLIT		("ScreenEdit","Enter a new artist transliteration.");
static LocalizedString ENTER_LAST_SECOND_HINT		("ScreenEdit","Enter a new last second hint.");
static LocalizedString ENTER_PREVIEW_START		("ScreenEdit","Enter a new preview start.");
static LocalizedString ENTER_PREVIEW_LENGTH		("ScreenEdit","Enter a new preview length.");
