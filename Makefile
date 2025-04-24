# Compiler and Flags
CXX = g++
CXXFLAGS = -Wall -O2 -std=c++17 -pthread

# Directories
ISR_DIR = ../isr
INDEX_DIR = ../index
UTILS_DIR = ../utils
QUERY_COMPILER_DIR = ../queryCompiler
UTF8PROC_DIR = ../index/stemmer/utf8proc


# Source Files
SRCS = ../dynamicRanker/driver.cpp \
       ../dynamicRanker/heuristics.cpp \
       $(ISR_DIR)/isr.cpp \
       $(ISR_DIR)/isrHandler.cpp \
       $(INDEX_DIR)/index.cpp \
       $(UTILS_DIR)/Utf8.cpp \
       $(UTILS_DIR)/IndexBlob.cpp \
       $(UTILS_DIR)/searchstring.cpp \
		 $(QUERY_COMPILER_DIR)/compiler.cpp \
		 $(QUERY_COMPILER_DIR)/tokenstream.cpp \
		 ../index/stemmer/stemmer.cpp \
		 LinuxTinyServer.cpp


# Object Files
OBJS = $(SRCS:.cpp=.o)

# Library Flags
LDFLAGS = -L$(UTF8PROC_DIR) -Wl,-rpath,$(UTF8PROC_DIR) -lutf8proc

# Target Executable
TARGET = server

# Default Target
all: $(TARGET)

# Build the target executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Compile source files to object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up build artifacts
clean:
	rm -f $(OBJS) $(TARGET)

# Phony targets
.PHONY: all clean
