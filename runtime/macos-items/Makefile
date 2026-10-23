CXX = clang++
CXXFLAGS = -std=c++20 -O2 -Wall -Wextra -Werror
BUILD = ../../.local/macos-items

.PHONY: all test
all: $(BUILD)/libCrimsonItems.dylib

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/libCrimsonItems.dylib: src/probe.cpp src/context.h src/inventory.h src/prepare.h src/execute.h src/transport.h ../../runtime/live-items/src/protocol.h | $(BUILD)
	$(CXX) $(CXXFLAGS) -dynamiclib -arch arm64 src/probe.cpp -o $@

$(BUILD)/context-tests: tests/context_tests.cpp src/context.h | $(BUILD)
	$(CXX) $(CXXFLAGS) tests/context_tests.cpp -o $@

$(BUILD)/inventory-tests: tests/inventory_tests.cpp src/inventory.h | $(BUILD)
	$(CXX) $(CXXFLAGS) tests/inventory_tests.cpp -o $@

$(BUILD)/prepare-tests: tests/prepare_tests.cpp src/prepare.h | $(BUILD)
	$(CXX) $(CXXFLAGS) tests/prepare_tests.cpp -o $@

$(BUILD)/transport-tests: tests/transport_tests.cpp src/transport.h ../../runtime/live-items/src/protocol.h | $(BUILD)
	$(CXX) $(CXXFLAGS) tests/transport_tests.cpp -o $@

$(BUILD)/execute-tests: tests/execute_tests.cpp src/execute.h src/prepare.h src/inventory.h | $(BUILD)
	$(CXX) $(CXXFLAGS) tests/execute_tests.cpp -o $@

test: $(BUILD)/context-tests $(BUILD)/inventory-tests $(BUILD)/prepare-tests $(BUILD)/transport-tests $(BUILD)/execute-tests
	$(BUILD)/context-tests
	$(BUILD)/inventory-tests
	$(BUILD)/prepare-tests
	$(BUILD)/transport-tests
	$(BUILD)/execute-tests
