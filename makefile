CXX = g++
CXXFLAGS = -std=c++20 -Wall -I./src -MMD -MP -pipe -O2
TARGET = ./out/compiler

SRC_DIR = src
BUILD_DIR = out/build

SRC = $(shell find $(SRC_DIR) -name "*.cpp")
OBJ = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SRC))

all: $(TARGET)

$(TARGET): $(OBJ)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

-include $(OBJ:.o=.d)

clean:
	rm -rf out/build $(TARGET)

.PHONY: all clean run