# Compiler and Flags
CXX = g++
CXXFLAGS = -Wall -O2 -std=c++17 -pthread

# Directories
ISR_DIR = ../isr
INDEX_DIR = ../index
UTILS_DIR = ../utils
RANK_DIR = ../ranking

# Source Files
SRCS = $(RANK_DIR)/driver.cpp \
       $(RANK_DIR)/heuristics.cpp \
       $(ISR_DIR)/isr.cpp \
       $(ISR_DIR)/isrHandler.cpp \
       $(INDEX_DIR)/index.cpp \
       $(UTILS_DIR)/Utf8.cpp \
       $(UTILS_DIR)/IndexBlob.cpp \
       $(UTILS_DIR)/searchstring.cpp \
		 LinuxTinyServer.cpp

# Object Files
OBJS = $(SRCS:.cpp=.o)

# Target Executable
TARGET = server

# Default Target
all: $(TARGET)

# Build the target executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Compile source files to object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up build artifacts
clean:
	rm -f $(OBJS) $(TARGET)

# Phony targets
.PHONY: all clean
