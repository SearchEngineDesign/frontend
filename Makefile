# Compiler and Flags
CXX = g++
INCPATH = ../include
CXXFLAGS = -Wall -O2 -std=c++17 -g -I$(INCPATH) -pthread

# Source Files
SRC_FILES := $(shell find .. ! -name "runner.cpp" ! -name "test.cpp" ! -name "rank.cpp" ! -name "testQueryCompiler.cpp" -name "*.cpp")


# Library Flags
LDFLAGS = -L$(UTF8PROC_DIR) -Wl,-rpath,$(UTF8PROC_DIR) -lutf8proc -lssl -lcrypto

ifeq ($(OS),Windows_NT)
	OPENSSL_DIR = /usr/include/openssl
	INCLUDES = -I$(OPENSSL_DIR)
	LDFLAGS += -L/usr/lib -L"$(shell pwd)/../index/stemmer/utf8proc"
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Darwin)
		# macOS specific settings
		OPENSSL_DIR = /opt/homebrew/opt/openssl@3
		INCLUDES = -I$(OPENSSL_DIR)/include 
		LDFLAGS += -L$(OPENSSL_DIR)/lib -L"$(shell pwd)/../index/stemmer/utf8proc"
		RPATH_FLAG = -Wl,-rpath,"@executable_path/../index/stemmer/utf8proc"
	else
		# Linux specific settings
		OPENSSL_DIR = /usr/include/openssl
		INCLUDES = -I$(OPENSSL_DIR)
		LDFLAGS += -L/usr/lib -L"$(shell pwd)/../index/stemmer/utf8proc"
		RPATH_FLAG = -Wl,-rpath,"$(shell pwd)/../index/stemmer/utf8proc"
	endif
endif

# Object Files
OBJS = $(SRC_FILES:.cpp=.o)

# Target Executable
TARGET = server

# Default Target
all: $(TARGET)

# Build the target executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(RPATH_FLAG)

# Compile source files to object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up build artifacts
clean:
	rm -f $(OBJS) $(TARGET)

# Phony targets
.PHONY: all clean