#!/bin/bash
# Build and run StepMania unit tests
# This script provides a convenient way to build and execute tests

set -e  # Exit on error

echo "=========================================="
echo "StepMania Unit Testing Infrastructure"
echo "=========================================="
echo ""

# Get script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$SCRIPT_DIR/../.."
BUILD_DIR="$PROJECT_ROOT/build_tests"

# Parse command line arguments
CLEAN_BUILD=false
RUN_TESTS=true
VERBOSE=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --clean)
            CLEAN_BUILD=true
            shift
            ;;
        --build-only)
            RUN_TESTS=false
            shift
            ;;
        --verbose)
            VERBOSE=true
            shift
            ;;
        --help)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --clean        Clean build directory before building"
            echo "  --build-only   Build tests but don't run them"
            echo "  --verbose      Show detailed build output"
            echo "  --help         Show this help message"
            echo ""
            echo "Examples:"
            echo "  $0                    # Build and run tests"
            echo "  $0 --clean            # Clean build and run tests"
            echo "  $0 --build-only       # Just build, don't run"
            echo ""
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# Clean build directory if requested
if [ "$CLEAN_BUILD" = true ]; then
    echo "Cleaning build directory..."
    rm -rf "$BUILD_DIR"
fi

# Create and enter build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with CMake
echo "Configuring build..."
if [ "$VERBOSE" = true ]; then
    cmake "$PROJECT_ROOT/src/tests/unit"
else
    cmake "$PROJECT_ROOT/src/tests/unit" > /dev/null 2>&1
fi

# Build tests
echo "Building tests..."
if [ "$VERBOSE" = true ]; then
    make stepmania_unit_tests
else
    make stepmania_unit_tests > /dev/null 2>&1
fi

if [ $? -eq 0 ]; then
    echo "✓ Build successful"
else
    echo "✗ Build failed"
    exit 1
fi

# Run tests if requested
if [ "$RUN_TESTS" = true ]; then
    echo ""
    echo "Running tests..."
    echo "=========================================="
    ./stepmania_unit_tests

    if [ $? -eq 0 ]; then
        echo ""
        echo "=========================================="
        echo "✓ All tests passed"
    else
        echo ""
        echo "=========================================="
        echo "✗ Some tests failed"
        exit 1
    fi
fi

echo ""
echo "Test executable: $BUILD_DIR/stepmania_unit_tests"
echo ""
echo "Run specific tests with:"
echo "  $BUILD_DIR/stepmania_unit_tests --gtest_filter=TestName.*"
echo ""
echo "List all tests with:"
echo "  $BUILD_DIR/stepmania_unit_tests --gtest_list_tests"
echo ""
