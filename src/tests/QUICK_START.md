# StepMania Testing Infrastructure - Quick Start

## 5-Minute Setup

### 1. Build and Run Tests

```bash
cd /home/user/stepmania/src/tests
./build_and_run_tests.sh
```

That's it! Tests will build and run automatically.

## What You Need to Know

### Test Files Are Here

```
src/
├── interfaces/          # Interface definitions
│   ├── IPlayerStateManager.h
│   ├── ISongSelectionState.h
│   ├── IStageProgressionManager.h
│   └── IAtomicFileWriter.h
│
├── tests/
│   ├── mocks/          # Mock implementations
│   │   ├── MockPlayerStateManager.h
│   │   ├── MockSongSelectionState.h
│   │   ├── MockStageProgressionManager.h
│   │   └── MockAtomicFileWriter.h
│   │
│   └── unit/           # Unit tests
│       ├── test_player_state_manager.cpp
│       ├── test_song_selection_state.cpp
│       ├── test_stage_progression.cpp
│       └── test_atomic_file_writer.cpp
```

### Write Your First Test (30 seconds)

```cpp
#include <gtest/gtest.h>
#include "mocks/MockPlayerStateManager.h"

TEST(MyNewTest, PlayerCanJoin)
{
    MockPlayerStateManager manager;
    manager.JoinPlayer(PLAYER_1);
    EXPECT_TRUE(manager.IsPlayerJoined(PLAYER_1));
}
```

Add to `src/tests/unit/CMakeLists.txt`:
```cmake
add_executable(
  stepmania_unit_tests
  test_player_state_manager.cpp
  test_my_new_feature.cpp     # Add this line
  test_runner.cpp
)
```

Rebuild and run:
```bash
./build_and_run_tests.sh
```

## Common Commands

```bash
# Run all tests
./build_and_run_tests.sh

# Clean rebuild
./build_and_run_tests.sh --clean

# Build without running
./build_and_run_tests.sh --build-only

# Run specific test
./stepmania_unit_tests --gtest_filter="PlayerStateManagerTest.*"

# List all tests
./stepmania_unit_tests --gtest_list_tests
```

## Need More Help?

- **Full Documentation**: See [TESTING.md](../../Docs/TESTING.md)
- **Interface Guide**: See [interfaces/README.md](../interfaces/README.md)
- **Implementation Summary**: See [TESTING_INFRASTRUCTURE_SUMMARY.md](../../Docs/TESTING_INFRASTRUCTURE_SUMMARY.md)
- **Google Test Docs**: https://google.github.io/googletest/

## Testing Patterns

### Pattern 1: Test State Changes

```cpp
TEST_F(MyTest, StateChanges)
{
    // Arrange
    MockPlayerStateManager mgr;
    
    // Act
    mgr.JoinPlayer(PLAYER_1);
    
    // Assert
    EXPECT_TRUE(mgr.IsPlayerJoined(PLAYER_1));
}
```

### Pattern 2: Test Multiple Conditions

```cpp
TEST_F(MyTest, MultipleConditions)
{
    MockPlayerStateManager mgr;
    mgr.JoinPlayer(PLAYER_1);
    mgr.JoinPlayer(PLAYER_2);
    
    EXPECT_TRUE(mgr.IsPlayerJoined(PLAYER_1));
    EXPECT_TRUE(mgr.IsPlayerJoined(PLAYER_2));
    EXPECT_EQ(2, mgr.GetNumPlayersJoined());
}
```

### Pattern 3: Test Error Conditions

```cpp
TEST_F(MyTest, HandlesInvalidInput)
{
    MockPlayerStateManager mgr;
    
    // Should handle gracefully
    EXPECT_FALSE(mgr.IsPlayerJoined(static_cast<PlayerNumber>(-1)));
}
```

## Key Concepts

**Interfaces**: Pure virtual classes that define behavior
```cpp
class IPlayerStateManager {
    virtual bool IsPlayerJoined(PlayerNumber pn) const = 0;
};
```

**Mocks**: Simple implementations for testing
```cpp
class MockPlayerStateManager : public IPlayerStateManager {
    // Simple in-memory implementation
};
```

**Tests**: Verify behavior
```cpp
TEST_F(MyTest, BehaviorIsCorrect) {
    // Arrange, Act, Assert
}
```

## Testing Checklist

- [ ] Interfaces in `src/interfaces/`
- [ ] Mocks in `src/tests/mocks/`
- [ ] Tests in `src/tests/unit/`
- [ ] Added to CMakeLists.txt
- [ ] Tests pass
- [ ] Documented behavior

## Example: Full Test Development

### 1. Create Interface
```cpp
// src/interfaces/IMyManager.h
class IMyManager {
public:
    virtual ~IMyManager() = default;
    virtual int DoSomething(int value) = 0;
};
```

### 2. Create Mock
```cpp
// src/tests/mocks/MockMyManager.h
class MockMyManager : public IMyManager {
    int m_value = 0;
public:
    int DoSomething(int value) override {
        return m_value += value;
    }
};
```

### 3. Write Tests
```cpp
// src/tests/unit/test_my_manager.cpp
#include <gtest/gtest.h>
#include "mocks/MockMyManager.h"

TEST(MyManagerTest, DoSomethingAddsValue) {
    MockMyManager mgr;
    EXPECT_EQ(5, mgr.DoSomething(5));
    EXPECT_EQ(10, mgr.DoSomething(5));
}
```

### 4. Build and Run
```bash
./build_and_run_tests.sh
```

## Success!

You now have:
- 44 working test cases
- Complete testing infrastructure
- Examples to follow
- Documentation to reference

Happy testing!
