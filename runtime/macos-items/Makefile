CXX = clang++
CXXFLAGS = -std=c++20 -O2 -Wall -Wextra -Werror
BUILD = ../../.local/macos-items

.PHONY: all test
all: $(BUILD)/libCrimsonItemsProbe.dylib

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/libCrimsonItemsProbe.dylib: src/probe.cpp src/context.h src/inventory.h src/experiment.h src/prepare.h | $(BUILD)
	$(CXX) $(CXXFLAGS) -dynamiclib -arch arm64 src/probe.cpp -o $@

$(BUILD)/context-tests: tests/context_tests.cpp src/context.h | $(BUILD)
	$(CXX) $(CXXFLAGS) tests/context_tests.cpp -o $@

$(BUILD)/inventory-tests: tests/inventory_tests.cpp src/inventory.h | $(BUILD)
	$(CXX) $(CXXFLAGS) tests/inventory_tests.cpp -o $@

$(BUILD)/experiment-tests: tests/experiment_tests.cpp src/experiment.h | $(BUILD)
	$(CXX) $(CXXFLAGS) tests/experiment_tests.cpp -o $@

$(BUILD)/prepare-tests: tests/prepare_tests.cpp src/prepare.h | $(BUILD)
	$(CXX) $(CXXFLAGS) tests/prepare_tests.cpp -o $@

test: $(BUILD)/context-tests $(BUILD)/inventory-tests $(BUILD)/experiment-tests $(BUILD)/prepare-tests
	$(BUILD)/context-tests
	$(BUILD)/inventory-tests
	$(BUILD)/experiment-tests
	$(BUILD)/prepare-tests
