CXX ?= c++
BUILD_DIR ?= build
CPPFLAGS += -Icpp/include -Icpp/third_party -Icpp/engine/include
CXXFLAGS ?= -O3 -DNDEBUG -std=c++20 -Wall -Wextra -Wpedantic
LDLIBS += -pthread

COMMON_SOURCES := \
  cpp/src/simulator.cpp \
  cpp/src/native_events.cpp
ENGINE_SOURCES := \
  cpp/engine/src/core.cpp \
  cpp/engine/src/expression.cpp \
  cpp/engine/src/events.cpp \
  cpp/engine/src/lower.cpp \
  cpp/engine/src/sweep.cpp \
  cpp/engine/src/graph.cpp \
  cpp/engine/src/collect.cpp \
  cpp/engine/src/transitions.cpp \
  cpp/engine/src/transaction.cpp \
  cpp/engine/src/identity.cpp \
  cpp/engine/src/path.cpp \
  cpp/engine/src/aot.cpp \
  cpp/engine/src/compiler.cpp \
  cpp/engine/src/bridge.cpp

COMMON_OBJECTS := $(patsubst cpp/%.cpp,$(BUILD_DIR)/%.o,$(COMMON_SOURCES))
ENGINE_OBJECTS := $(patsubst cpp/%.cpp,$(BUILD_DIR)/%.o,$(ENGINE_SOURCES))
REPLAY_OBJECT := $(BUILD_DIR)/engine/search/replay.o
GOAL_ROUTE_OBJECT := $(BUILD_DIR)/experiments/goal_route.o

.PHONY: all build replay search verify clean
all: build
build: $(BUILD_DIR)/mota-native-replay $(BUILD_DIR)/mota-goal-route
replay: $(BUILD_DIR)/mota-native-replay
search: $(BUILD_DIR)/mota-goal-route
verify: replay
	MOTA_BUILD_DIR="$(abspath $(BUILD_DIR))" python3 tools/verify_solution.py

$(BUILD_DIR)/mota-native-replay: $(COMMON_OBJECTS) $(ENGINE_OBJECTS) $(REPLAY_OBJECT)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $^ $(LDLIBS) -o $@

$(BUILD_DIR)/mota-goal-route: $(COMMON_OBJECTS) $(ENGINE_OBJECTS) $(GOAL_ROUTE_OBJECT)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $^ $(LDLIBS) -o $@

$(BUILD_DIR)/%.o: cpp/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

-include $(COMMON_OBJECTS:.o=.d) $(ENGINE_OBJECTS:.o=.d) $(REPLAY_OBJECT:.o=.d) $(GOAL_ROUTE_OBJECT:.o=.d)

clean:
	@echo 'Remove the build directory manually when desired.'
