CXX = g++
LIBS = -lmysqlclient -lssl -lcrypto

MODE ?= debug

SRCS = $(wildcard *.cpp)
OBJS = $(SRCS:.cpp=.o)

BUILD_DIR = build/$(MODE)
TARGET = $(BUILD_DIR)/login
OBJS_DIR = $(BUILD_DIR)/obj
OBJS_FULL = $(addprefix $(OBJS_DIR)/, $(OBJS))

ifeq ($(MODE),debug)
CXXFLAGS = -Wall -std=c++17 -g -O0
else ifeq ($(MODE),release)
CXXFLAGS = -Wall -std=c++17 -O2
else ifeq ($(MODE),asan)
CXXFLAGS = -Wall -Wextra -std=c++17 -g -O1 -fsanitize=address -fno-omit-frame-pointer
LDFLAGS  = -fsanitize=address
else
$(error MODE must be debug, release  or asan)
endif

$(TARGET): $(OBJS_FULL)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $^ -o $@ $(LIBS) $(LDFLAGS)

$(OBJS_DIR)/%.o: %.cpp
	@mkdir -p $(OBJS_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -rf build

