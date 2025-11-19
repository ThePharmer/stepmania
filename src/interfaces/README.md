# StepMania Interfaces

## Purpose

This directory contains C++ interface definitions (pure virtual classes) that enable dependency injection and unit testing in StepMania.

## Why Interfaces?

StepMania historically used a singleton-based architecture with global manager objects:

```cpp
// Traditional approach - tight coupling to singletons
void MyFunction()
{
    if (GAMESTATE->IsPlayerJoined(PLAYER_1))
    {
        // Do something with GAMESTATE singleton
    }
}
```

**Problems with this approach:**
- **Not testable**: Can't unit test without initializing entire engine
- **High coupling**: Code directly depends on concrete singletons
- **Hard to mock**: Can't substitute test doubles
- **Initialization order**: Singletons must be initialized in specific order

## Solution: Interfaces

Interfaces decouple code from concrete implementations:

```cpp
// New approach - dependency injection via interface
void MyFunction(IPlayerStateManager* playerMgr)
{
    if (playerMgr->IsPlayerJoined(PLAYER_1))
    {
        // Works with any implementation of IPlayerStateManager
    }
}

// Production code uses real implementation
MyFunction(GAMESTATE);

// Test code uses mock implementation
MockPlayerStateManager mockMgr;
MyFunction(&mockMgr);
```

## Available Interfaces

### IPlayerStateManager

**Purpose**: Manage player join state, player numbers, and stage tokens.

**Key Methods:**
- `IsPlayerJoined(PlayerNumber)` - Check if player is joined
- `JoinPlayer(PlayerNumber)` - Join a player
- `GetPlayerState(PlayerNumber)` - Get player's state object
- `GetNumPlayersJoined()` - Count joined players

**Use Cases:**
- Screens that need to check player join status
- Systems that modify player state
- Testing player-related logic

### ISongSelectionState

**Purpose**: Manage currently selected songs, courses, steps, and trails.

**Key Methods:**
- `SetCurrentSong(Song*)` - Set the active song
- `GetCurrentSong()` - Get the active song
- `SetCurrentSteps(PlayerNumber, Steps*)` - Set player's steps
- `GetCurrentSteps(PlayerNumber)` - Get player's steps

**Use Cases:**
- Music selection screens
- Gameplay initialization
- Testing song selection logic

### IStageProgressionManager

**Purpose**: Track progression through game stages, including extra stages.

**Key Methods:**
- `BeginStage()` - Start a new stage
- `FinishStage()` - Complete current stage
- `GetCurrentStageIndex()` - Get current stage number
- `IsExtraStage()` - Check if in extra stage
- `IsFinalStage()` - Check if final stage

**Use Cases:**
- Stage transition logic
- Extra stage unlocking
- Testing stage progression

### IAtomicFileWriter

**Purpose**: Safely write files using atomic operations to prevent data corruption.

**Key Methods:**
- `Open(RageFile&)` - Open temporary file for writing
- `Commit(RageFile&)` - Atomically replace target file
- `IsCommitted()` - Check if write was committed

**Use Cases:**
- Profile saving (Stats.xml)
- Preferences saving (Preferences.ini)
- Any critical file write operation
- Testing file operations without I/O

## Creating New Interfaces

### 1. Identify Manager to Abstract

Look for:
- Classes with "Manager" suffix
- Global singleton objects
- Code that's hard to test

### 2. Extract Pure Virtual Interface

```cpp
// src/interfaces/IMyManager.h
#ifndef I_MY_MANAGER_H
#define I_MY_MANAGER_H

class IMyManager
{
public:
    virtual ~IMyManager() = default;

    // Pure virtual methods
    virtual void DoSomething() = 0;
    virtual int GetValue() const = 0;
};

#endif
```

### 3. Create Mock Implementation

```cpp
// src/tests/mocks/MockMyManager.h
#include "interfaces/IMyManager.h"

class MockMyManager : public IMyManager
{
private:
    int m_value;

public:
    MockMyManager() : m_value(0) {}

    void DoSomething() override
    {
        m_value++;
    }

    int GetValue() const override
    {
        return m_value;
    }
};
```

### 4. Adapt Existing Manager (Optional)

```cpp
// MyManager.h
#include "interfaces/IMyManager.h"

class MyManager : public IMyManager
{
public:
    // Implement interface methods
    void DoSomething() override;
    int GetValue() const override;

    // Can still have additional methods
    void ManagerSpecificMethod();
};
```

### 5. Use Dependency Injection

```cpp
// Before
void MyFunction()
{
    MYMANAGER->DoSomething();
}

// After
void MyFunction(IMyManager* mgr)
{
    mgr->DoSomething();
}
```

## Interface Design Guidelines

### Keep Interfaces Focused

Each interface should have a single, well-defined responsibility:

```cpp
// GOOD: Focused interface
class IPlayerStateManager
{
    virtual bool IsPlayerJoined(PlayerNumber pn) const = 0;
    virtual void JoinPlayer(PlayerNumber pn) = 0;
    // ... related methods
};

// BAD: Mixed responsibilities
class IGameState
{
    virtual bool IsPlayerJoined(PlayerNumber pn) const = 0;
    virtual Song* GetCurrentSong() const = 0;
    virtual void RenderScreenEffects() = 0;  // Different concern!
};
```

### Use Const Correctness

```cpp
class IMyManager
{
public:
    // Query methods should be const
    virtual int GetValue() const = 0;

    // Mutation methods are non-const
    virtual void SetValue(int value) = 0;
};
```

### Avoid Dependencies

Interfaces should minimize dependencies on other types:

```cpp
// GOOD: Minimal dependencies
class IPlayerStateManager
{
    virtual bool IsPlayerJoined(PlayerNumber pn) const = 0;
    virtual PlayerState* GetPlayerState(PlayerNumber pn) = 0;
};

// BAD: Too many dependencies
class IPlayerStateManager
{
    virtual ComplexStruct GetComplexData(
        OtherManager* mgr,
        ExternalType* ext,
        YetAnotherClass* yac) = 0;
};
```

### Document Interface Contracts

```cpp
class IPlayerStateManager
{
public:
    /**
     * @brief Check if a player has joined the game.
     * @param pn The player number to check.
     * @return true if the player is joined, false otherwise.
     * @note Returns false for invalid player numbers.
     */
    virtual bool IsPlayerJoined(PlayerNumber pn) const = 0;
};
```

## Benefits

### Testability

```cpp
// Easy to test without game engine
TEST_F(MyTest, TestPlayerLogic)
{
    MockPlayerStateManager mockMgr;
    mockMgr.JoinPlayer(PLAYER_1);

    MyFunction(&mockMgr);

    EXPECT_TRUE(mockMgr.IsPlayerJoined(PLAYER_1));
}
```

### Flexibility

```cpp
// Can swap implementations
void UseManager(IPlayerStateManager* mgr)
{
    // Works with GameState, mock, or any other implementation
}

// Production
UseManager(GAMESTATE);

// Testing
MockPlayerStateManager testMgr;
UseManager(&testMgr);

// Demo mode
DemoPlayerStateManager demoMgr;
UseManager(&demoMgr);
```

### Decoupling

```cpp
// File A depends on interface
#include "interfaces/IPlayerStateManager.h"

void FunctionInFileA(IPlayerStateManager* mgr)
{
    // No dependency on GameState.h or implementation details
}

// File B provides implementation
#include "GameState.h"

void FunctionInFileB()
{
    FunctionInFileA(GAMESTATE);
}
```

## Migration Strategy

### Phase 1: Create Interfaces (Current Phase)

- Define interfaces for key managers
- Create mock implementations
- Write initial unit tests

### Phase 2: Gradual Adoption

- New code uses interfaces from the start
- Existing code updated as files are modified
- Focus on high-value, frequently-changed code

### Phase 3: Refactor Core Systems

- Make existing managers implement interfaces
- Update singletons to expose interface accessors
- Increase test coverage

### Phase 4: Full Dependency Injection

- Constructor injection for major subsystems
- Eliminate most direct singleton access
- Comprehensive test coverage

## Backward Compatibility

Interfaces don't break existing code:

```cpp
// Existing code still works
GAMESTATE->IsPlayerJoined(PLAYER_1);

// New code can use interface
IPlayerStateManager* mgr = GAMESTATE->GetPlayerStateManager();
mgr->IsPlayerJoined(PLAYER_1);

// Or dependency injection
void NewFunction(IPlayerStateManager* mgr)
{
    mgr->IsPlayerJoined(PLAYER_1);
}
```

## See Also

- [TESTING.md](../../Docs/TESTING.md) - Testing infrastructure documentation
- [CODE_REVIEW_2025.md](../../Docs/CODE_REVIEW_2025.md) - Architectural analysis
- `src/tests/mocks/` - Mock implementations for testing
- `src/tests/unit/` - Example unit tests
