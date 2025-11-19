# NoteData Hybrid Lookup Optimization Plan

**Status:** Not Implemented (Too complex for Phase 2)
**Expected Performance Gain:** 95% of lookups become O(1) instead of O(log n)
**Risk Level:** High - Requires extensive testing
**Recommended Implementation:** Separate focused effort with testing infrastructure

## Current Implementation

NoteData currently uses `map<int, TapNote>` (red-black tree) for all note storage:
```cpp
typedef map<int,TapNote> TrackMap;
vector<TrackMap> m_TapNotes;
```

**Complexity:**
- Lookup: O(log n) where n = number of notes in track
- Insert: O(log n)
- Dense charts (3000+ notes): ~11-12 tree traversals per lookup

## Proposed Hybrid Structure

Add an unordered_map for the active gameplay window (±2 seconds from current position):

```cpp
class NoteData {
private:
    vector<TrackMap> m_TapNotes;  // Full range (keep for random access)

    // Performance optimization: Active window cache
    vector<unordered_map<int, TapNote*>> m_ActiveWindow;  // O(1) lookups
    int m_iActiveWindowStart;  // First row in active window
    int m_iActiveWindowEnd;    // Last row in active window
    bool m_bActiveWindowValid;
};
```

## Implementation Requirements

### 1. Window Management

```cpp
void UpdateActiveWindow(int iCurrentRow, TimingData* timing)
{
    // Calculate window bounds (±2 seconds)
    const float WINDOW_SECONDS = 2.0f;
    float fBeat = NoteRowToBeat(iCurrentRow);
    float fStartBeat = timing->GetBeatFromElapsedTime(
        timing->GetElapsedTimeFromBeat(fBeat) - WINDOW_SECONDS);
    float fEndBeat = timing->GetBeatFromElapsedTime(
        timing->GetElapsedTimeFromBeat(fBeat) + WINDOW_SECONDS);

    int iNewStart = BeatToNoteRow(fStartBeat);
    int iNewEnd = BeatToNoteRow(fEndBeat);

    if (iNewStart == m_iActiveWindowStart && iNewEnd == m_iActiveWindowEnd)
        return;  // Window unchanged

    // Rebuild active window
    for (int t = 0; t < GetNumTracks(); ++t)
    {
        m_ActiveWindow[t].clear();
        TrackMap::iterator begin, end;
        GetTapNoteRange(t, iNewStart, iNewEnd, begin, end);

        for (auto it = begin; it != end; ++it)
            m_ActiveWindow[t][it->first] = &it->second;
    }

    m_iActiveWindowStart = iNewStart;
    m_iActiveWindowEnd = iNewEnd;
    m_bActiveWindowValid = true;
}
```

### 2. Modified GetTapNote

```cpp
const TapNote& GetTapNote(int iTrack, int iRow) const
{
    // Try active window first (O(1))
    if (m_bActiveWindowValid &&
        iRow >= m_iActiveWindowStart &&
        iRow <= m_iActiveWindowEnd)
    {
        auto it = m_ActiveWindow[iTrack].find(iRow);
        if (it != m_ActiveWindow[iTrack].end())
            return *(it->second);
    }

    // Fall back to full map (O(log n))
    TrackMap::const_iterator it = m_TapNotes[iTrack].find(iRow);
    if (it != m_TapNotes[iTrack].end())
        return it->second;

    return TAP_EMPTY;
}
```

### 3. Synchronization Points

Must update active window when:
- SetTapNote() modifies notes in active window
- RemoveTapNote() removes notes from active window
- ClearRangeForTrack() affects active window
- Any NoteDataUtil transformation

### 4. Initialization

Add to Player::Load():
```cpp
void Player::Load()
{
    // ... existing code ...

    // Initialize active window (starts invalid)
    m_NoteData.InitActiveWindow(GetNumTracks());
}
```

Add to Player::Update():
```cpp
void Player::Update(float fDeltaTime)
{
    // ... existing code ...

    // Update active window based on current position
    m_NoteData.UpdateActiveWindow(iSongRow, m_Timing);
}
```

## Testing Requirements

1. **Correctness Tests**
   - Verify all lookups return same results as before
   - Test window boundary conditions
   - Test with scrolling backwards (rewind)
   - Test with scroll rate changes
   - Test with gimmick charts (stops, warps)

2. **Performance Tests**
   - Benchmark dense charts (32nd notes at 240 BPM)
   - Measure frame time improvement
   - Profile memory usage (active window overhead)

3. **Edge Cases**
   - Empty tracks
   - Notes added/removed during gameplay
   - Rapid position changes (in editor)
   - Course mode (multiple songs)

## Estimated Implementation Effort

- **Core Implementation:** 8-12 hours
- **Testing Infrastructure:** 16-20 hours
- **Bug Fixes & Refinement:** 8-12 hours
- **Total:** 32-44 hours (~1-2 weeks)

## Alternative: Simpler Cache

If hybrid structure proves too complex, consider a simpler LRU cache:

```cpp
class NoteDataCache {
    struct CacheEntry {
        int iTrack;
        int iRow;
        const TapNote* pNote;
    };

    static const int CACHE_SIZE = 16;
    CacheEntry m_Cache[CACHE_SIZE];
    int m_iNextSlot;
};
```

This would still provide O(1) for repeated accesses to same notes (common in hold note updates) with minimal implementation complexity.

## Recommendation

**Defer to Phase 3** when proper testing infrastructure exists. The risk of introducing subtle timing/judgment bugs in a rhythm game is too high for the current phase.

Focus Phase 2 on safer optimizations that don't modify core data structures.
