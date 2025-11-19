# StepMania Testing Infrastructure

## Overview

This document describes the unit testing infrastructure for StepMania, implemented as part of Phase 3 Architectural Refactoring from the November 2025 code review.

**Key Goals:**
- Enable unit testing without full engine initialization
- Reduce coupling through dependency injection
- Provide fast, isolated tests
- Make it easy for developers to add new tests
- Improve code quality and prevent regressions

## Architecture

### Directory Structure

```
src/
├── interfaces/                 # Interface definitions for dependency injection
│   ├── IPlayerStateManager.h
│   ├── ISongSelectionState.h
│   ├── IStageProgressionManager.h
│   └── IAtomicFileWriter.h
├── tests/
│   ├── mocks/                 # Mock implementations for testing
│   │   ├── MockPlayerStateManager.h
│   │   ├── MockSongSelectionState.h
│   │   ├── MockStageProgressionManager.h
│   │   └── MockAtomicFileWriter.h
│   └── unit/                  # Unit test files
│       ├── CMakeLists.txt
│       ├── test_runner.cpp
│       ├── test_player_state_manager.cpp
│       ├── test_song_selection_state.cpp
│       ├── test_stage_progression.cpp
│       └── test_atomic_file_writer.cpp
```

### Design Patterns

#### Dependency Injection via Interfaces

The testing infrastructure introduces C++ interfaces (pure virtual classes) for key managers. This enables:

1. **Testability**: Code can depend on interfaces instead of concrete singletons
2. **Isolation**: Tests use mock implementations, no engine initialization needed
3. **Flexibility**: Multiple implementations can coexist (real, mock, test doubles)

**Example Interface:**

```cpp
class IPlayerStateManager
{
public:
    virtual ~IPlayerStateManager() = default;
    virtual bool IsPlayerJoined(PlayerNumber pn) const = 0;
    virtual void JoinPlayer(PlayerNumber pn) = 0;
    // ... more methods
};
```

#### Mock Objects

Mock implementations provide controlled behavior for testing:

```cpp
class MockPlayerStateManager : public IPlayerStateManager
{
    // Simple in-memory implementation
    // No singletons, no file I/O, no complex dependencies
};
```

## Building and Running Tests

### Prerequisites

- CMake 3.14 or higher
- C++14 compatible compiler
- Internet connection (for downloading Google Test)

### Building Tests

From the project root:

```bash
# Create build directory
mkdir -p build && cd build

# Configure with CMake
cmake ..

# Build tests
make stepmania_unit_tests

# Or build everything including tests
make
```

### Running Tests

```bash
# Run all tests
./src/tests/unit/stepmania_unit_tests

# Or use CMake's test runner
make test

# Or use the custom target
make run_tests
```

### Running Specific Tests

```bash
# Run tests matching a pattern
./stepmania_unit_tests --gtest_filter="PlayerStateManagerTest.*"

# Run a specific test
./stepmania_unit_tests --gtest_filter="PlayerStateManagerTest.JoinSinglePlayer"

# List all tests
./stepmania_unit_tests --gtest_list_tests
```

## Writing New Tests

### Step 1: Identify What to Test

Good candidates for unit tests:
- Business logic that doesn't require rendering
- State management (player state, song selection, etc.)
- Data transformations
- File operations (using mocks)
- Algorithms (scoring, timing calculations)

Avoid testing in unit tests:
- OpenGL rendering code
- Full gameplay loops
- Code that requires sound/input devices

### Step 2: Create or Use Interfaces

If testing code that depends on singletons, create an interface:

```cpp
// src/interfaces/IMyManager.h
class IMyManager
{
public:
    virtual ~IMyManager() = default;
    virtual int DoSomething(int value) = 0;
};
```

### Step 3: Create Mock Implementation

```cpp
// src/tests/mocks/MockMyManager.h
#include "interfaces/IMyManager.h"

class MockMyManager : public IMyManager
{
private:
    int m_value;

public:
    MockMyManager() : m_value(0) {}

    int DoSomething(int value) override
    {
        m_value += value;
        return m_value;
    }

    // Test helpers
    int GetValue() const { return m_value; }
};
```

### Step 4: Write Tests

```cpp
// src/tests/unit/test_my_feature.cpp
#include <gtest/gtest.h>
#include "mocks/MockMyManager.h"

class MyFeatureTest : public ::testing::Test
{
protected:
    MockMyManager manager;

    void SetUp() override
    {
        // Setup code runs before each test
    }

    void TearDown() override
    {
        // Cleanup code runs after each test
    }
};

TEST_F(MyFeatureTest, DescriptiveTestName)
{
    // Arrange
    int input = 5;

    // Act
    int result = manager.DoSomething(input);

    // Assert
    EXPECT_EQ(5, result);
    EXPECT_EQ(5, manager.GetValue());
}
```

### Step 5: Add to CMakeLists.txt

Edit `src/tests/unit/CMakeLists.txt`:

```cmake
add_executable(
  stepmania_unit_tests
  test_player_state_manager.cpp
  test_song_selection_state.cpp
  test_my_feature.cpp           # Add your test file
  test_runner.cpp
)
```

### Step 6: Build and Run

```bash
cd build
make stepmania_unit_tests
./src/tests/unit/stepmania_unit_tests
```

## Google Test Assertions

### Common Assertions

```cpp
// Boolean conditions
EXPECT_TRUE(condition);
EXPECT_FALSE(condition);

// Equality
EXPECT_EQ(expected, actual);
EXPECT_NE(value1, value2);

// Comparisons
EXPECT_LT(val1, val2);  // Less than
EXPECT_LE(val1, val2);  // Less than or equal
EXPECT_GT(val1, val2);  // Greater than
EXPECT_GE(val1, val2);  // Greater than or equal

// Pointers
EXPECT_EQ(nullptr, ptr);
EXPECT_NE(nullptr, ptr);

// Floating point
EXPECT_FLOAT_EQ(expected, actual);
EXPECT_DOUBLE_EQ(expected, actual);
EXPECT_NEAR(val1, val2, abs_error);

// String comparison
EXPECT_STREQ("expected", actual);
EXPECT_STRNE(str1, str2);
```

### EXPECT vs ASSERT

- `EXPECT_*`: Test continues after failure (check multiple things)
- `ASSERT_*`: Test stops immediately after failure (use when can't continue)

```cpp
TEST_F(MyTest, Example)
{
    // If this fails, the test can still continue
    EXPECT_NE(nullptr, ptr);

    // If this fails, stop immediately (avoid segfault)
    ASSERT_NE(nullptr, ptr);
    ptr->DoSomething();  // Safe to call after ASSERT
}
```

## Testing Best Practices

### 1. Test Isolation

Each test should be independent:

```cpp
// GOOD: Fresh state for each test
TEST_F(MyTest, Test1)
{
    manager.JoinPlayer(PLAYER_1);
    EXPECT_EQ(1, manager.GetNumPlayersJoined());
}

TEST_F(MyTest, Test2)
{
    // Starts fresh, not affected by Test1
    EXPECT_EQ(0, manager.GetNumPlayersJoined());
}
```

### 2. Arrange-Act-Assert Pattern

```cpp
TEST_F(MyTest, Example)
{
    // Arrange: Set up test conditions
    manager.JoinPlayer(PLAYER_1);
    manager.JoinPlayer(PLAYER_2);

    // Act: Perform the operation being tested
    manager.UnjoinPlayer(PLAYER_1);

    // Assert: Verify the result
    EXPECT_FALSE(manager.IsPlayerJoined(PLAYER_1));
    EXPECT_TRUE(manager.IsPlayerJoined(PLAYER_2));
    EXPECT_EQ(1, manager.GetNumPlayersJoined());
}
```

### 3. Descriptive Test Names

```cpp
// GOOD: Test name describes what is being tested
TEST_F(PlayerStateManagerTest, JoinPlayerMakesPlayerJoined)

// GOOD: Test name describes the scenario
TEST_F(PlayerStateManagerTest, GetPlayerStateReturnsNullBeforeJoin)

// BAD: Vague test name
TEST_F(PlayerStateManagerTest, Test1)
```

### 4. Test One Thing

```cpp
// GOOD: Test focuses on one behavior
TEST_F(MyTest, JoinPlayerIncreasesCount)
{
    manager.JoinPlayer(PLAYER_1);
    EXPECT_EQ(1, manager.GetNumPlayersJoined());
}

// BAD: Test tries to verify too much
TEST_F(MyTest, EverythingAboutPlayers)
{
    // Tests joining, unjoining, state, tokens, etc.
    // Split into multiple focused tests instead
}
```

### 5. Test Boundary Conditions

```cpp
TEST_F(MyTest, HandlesInvalidPlayerNumbers)
{
    EXPECT_FALSE(manager.IsPlayerJoined(static_cast<PlayerNumber>(-1)));
    EXPECT_FALSE(manager.IsPlayerJoined(static_cast<PlayerNumber>(999)));
}

TEST_F(MyTest, HandlesZeroCount)
{
    EXPECT_EQ(0, manager.GetNumPlayersJoined());
}

TEST_F(MyTest, HandlesMaxPlayers)
{
    for (int i = 0; i < NUM_PLAYERS; ++i)
        manager.JoinPlayer(static_cast<PlayerNumber>(i));

    EXPECT_EQ(NUM_PLAYERS, manager.GetNumPlayersJoined());
}
```

## Example Tests

### Testing Player State Management

See: `src/tests/unit/test_player_state_manager.cpp`

Key tests:
- Players start unjoined
- Joining/unjoining players
- Stage token management
- Invalid player number handling

### Testing Song Selection

See: `src/tests/unit/test_song_selection_state.cpp`

Key tests:
- Song/course selection
- Per-player steps/trails
- Reset functionality
- Null handling

### Testing Stage Progression

See: `src/tests/unit/test_stage_progression.cpp`

Key tests:
- Stage advancement
- Extra stage detection
- Final stage detection
- Demonstration mode

### Testing Atomic File Operations

See: `src/tests/unit/test_atomic_file_writer.cpp`

Key tests:
- Open/commit workflow
- Error handling
- Double-commit prevention
- Temp file isolation

## Integration with Existing Code

### Gradual Migration Strategy

You don't need to refactor everything at once:

1. **New code**: Use interfaces from the start
2. **Existing code**: Add interfaces gradually as you modify files
3. **Critical paths**: Prioritize testing high-risk areas

### Adapting Existing Managers

To make an existing manager testable:

```cpp
// Before: Direct singleton usage
void MyFunction()
{
    if (GAMESTATE->IsPlayerJoined(PLAYER_1))
    {
        // Do something
    }
}

// After: Dependency injection
void MyFunction(IPlayerStateManager* playerMgr)
{
    if (playerMgr->IsPlayerJoined(PLAYER_1))
    {
        // Do something
    }
}

// In production code
MyFunction(GAMESTATE->GetPlayerStateManager());

// In test code
MockPlayerStateManager mockMgr;
MyFunction(&mockMgr);
```

### Maintaining Backward Compatibility

The existing singleton architecture remains:

```cpp
// GameState.h
class GameState
{
    // Existing singleton interface stays the same
    bool IsPlayerJoined(PlayerNumber pn);

    // Add new interface accessor
    IPlayerStateManager* GetPlayerStateManager();
};
```

## Continuous Integration

### Running Tests in CI

Add to your CI pipeline (e.g., GitHub Actions):

```yaml
- name: Build Tests
  run: |
    mkdir build && cd build
    cmake ..
    make stepmania_unit_tests

- name: Run Tests
  run: |
    cd build
    ./src/tests/unit/stepmania_unit_tests --gtest_output=xml:test_results.xml
```

### Test Coverage

While not implemented yet, consider adding coverage tracking:

```bash
# Build with coverage flags
cmake -DCMAKE_CXX_FLAGS="--coverage" ..
make stepmania_unit_tests
./src/tests/unit/stepmania_unit_tests

# Generate coverage report
gcov src/tests/unit/*.cpp
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage_html
```

## Future Improvements

### Short Term (1-2 months)

- [ ] Add more manager interfaces (ProfileManager, SongManager, etc.)
- [ ] Increase test coverage to 30%+ of core logic
- [ ] Add integration tests for critical paths
- [ ] Set up automated CI testing

### Medium Term (3-6 months)

- [ ] Refactor GameState to use composition of managers
- [ ] Add performance benchmarks using Google Benchmark
- [ ] Add property-based testing for complex algorithms
- [ ] Test coverage > 50%

### Long Term (6+ months)

- [ ] Full dependency injection throughout codebase
- [ ] Eliminate most singleton dependencies
- [ ] Add end-to-end gameplay tests
- [ ] Achieve 70%+ test coverage

## Troubleshooting

### CMake Can't Find Google Test

Google Test is downloaded automatically via FetchContent. If this fails:

1. Check internet connection
2. Update CMake to 3.14+
3. Manually clone Google Test to `extern/googletest/`

### Compilation Errors

Common issues:

- **Missing includes**: Add to CMakeLists.txt `include_directories()`
- **Undefined references**: Check that mocks are header-only or linked properly
- **PlayerNumber undefined**: Include `PlayerNumber.h` or `GameConstantsAndTypes.h`

### Tests Crash

- Use `ASSERT_*` instead of `EXPECT_*` for pointer checks before dereferencing
- Ensure mocks don't depend on singletons
- Check that test fixtures properly initialize mock objects

### Tests Are Slow

Unit tests should run in < 1 second. If slow:

- Remove file I/O (use mocks)
- Remove sleep/delays
- Don't initialize the game engine
- Use mock objects instead of real implementations

## Additional Resources

- [Google Test Documentation](https://google.github.io/googletest/)
- [Google Test Primer](https://google.github.io/googletest/primer.html)
- [Google Test Advanced Guide](https://google.github.io/googletest/advanced.html)
- [Google Mock Documentation](https://google.github.io/googletest/gmock_for_dummies.html)

## Questions and Support

For questions about the testing infrastructure:

1. Review this document and existing test examples
2. Check the Google Test documentation
3. Ask in the StepMania development channels
4. File an issue on the StepMania GitHub repository

## Summary

The StepMania testing infrastructure provides:

- **Interfaces** for dependency injection
- **Mock implementations** for isolated testing
- **Google Test framework** for writing tests
- **CMake configuration** for building tests
- **Example tests** demonstrating best practices

This infrastructure enables fast, isolated unit tests without requiring full engine initialization, making it easy to add tests and improve code quality.
