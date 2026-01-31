CXX = g++
CXXFLAGS = -std=c++20 -Wall -I./src -MMD -MP
TARGET = ./out/compiler

SRC_DIR = src
SRC = $(wildcard $(SRC_DIR)/*.cpp)
OBJ = $(patsubst $(SRC_DIR)/%.cpp, out/build/%.o, $(SRC))

all: $(TARGET)

$(TARGET): $(OBJ)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $^ -o $@

out/build/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# To wczyta automatycznie wygenerowane zależności
-include $(OBJ:.o=.d)

clean:
	rm -rf out/build $(TARGET) 

.PHONY: all clean run