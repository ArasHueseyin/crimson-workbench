CXX = clang++
CXXFLAGS = -std=c++20 -O2 -Wall -Wextra -Werror
BUILD = ../../.local/macos-items

.PHONY: all test
all: $(BUILD)/libCrimsonItems.dylib

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/libCrimsonItems.dylib: src/probe.cpp src/mount.h src/context.h src/world_probe.h src/world_read.h src/weather_read.h src/inventory.h src/prepare.h src/execute.h src/transport.h src/protocol.h | $(BUILD)
	$(CXX) $(CXXFLAGS) -dynamiclib -arch arm64 src/probe.cpp -o $@

$(BUILD)/context-tests: tests/context_tests.cpp src/context.h | $(BUILD)
	$(CXX) $(CXXFLAGS) tests/context_tests.cpp -o $@

$(BUILD)/inventory-tests: tests/inventory_tests.cpp src/inventory.h | $(BUILD)
	$(CXX) $(CXXFLAGS) tests/inventory_tests.cpp -o $@

$(BUILD)/prepare-tests: tests/prepare_tests.cpp src/prepare.h | $(BUILD)
	$(CXX) $(CXXFLAGS) tests/prepare_tests.cpp -o $@

$(BUILD)/transport-tests: tests/transport_tests.cpp src/transport.h src/protocol.h | $(BUILD)
	$(CXX) $(CXXFLAGS) tests/transport_tests.cpp -o $@

$(BUILD)/execute-tests: tests/execute_tests.cpp src/execute.h src/prepare.h src/inventory.h | $(BUILD)
	$(CXX) $(CXXFLAGS) tests/execute_tests.cpp -o $@

$(BUILD)/mount-tests: tests/mount_tests.cpp src/mount.h src/execute.h src/protocol.h | $(BUILD)
	$(CXX) $(CXXFLAGS) tests/mount_tests.cpp -o $@

test: $(BUILD)/mount-tests $(BUILD)/context-tests $(BUILD)/inventory-tests $(BUILD)/prepare-tests $(BUILD)/transport-tests $(BUILD)/execute-tests $(BUILD)/world-probe-tests $(BUILD)/world-read-tests $(BUILD)/weather-read-tests
	$(BUILD)/mount-tests
	$(BUILD)/context-tests
	$(BUILD)/inventory-tests
	$(BUILD)/prepare-tests
	$(BUILD)/transport-tests
	$(BUILD)/execute-tests
	$(BUILD)/world-probe-tests
	$(BUILD)/world-read-tests
	$(BUILD)/weather-read-tests

$(BUILD)/world-probe-tests: tests/world_probe_tests.cpp src/world_probe.h | $(BUILD)
	$(CXX) $(CXXFLAGS) tests/world_probe_tests.cpp -o $@

world-probe-test: $(BUILD)/world-probe-tests
	$(BUILD)/world-probe-tests

$(BUILD)/world-read-tests: tests/world_read_tests.cpp src/world_read.h src/world_probe.h | $(BUILD)
	$(CXX) $(CXXFLAGS) tests/world_read_tests.cpp -o $@

$(BUILD)/weather-read-tests: tests/weather_read_tests.cpp src/weather_read.h src/world_probe.h | $(BUILD)
	$(CXX) $(CXXFLAGS) tests/weather_read_tests.cpp -o $@
