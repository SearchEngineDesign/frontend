# Compiler and Flags
CXX = g++
INCPATH = ../include
CXXFLAGS = -Wall -O2 -std=c++17 -g -I$(INCPATH) -pthread 

# Source Files
SRC_FILES1 := $(shell find .. ! -name "runner.cpp" ! -name "indexserver.cpp" ! -name "constructDataset.cpp" ! -name "driver.cpp" ! -name "test.cpp" ! -name "rank.cpp" ! -name "testQueryCompiler.cpp" -name "*.cpp")
SRC_FILES2 := $(shell find .. ! -name "runner.cpp" ! -name "server.cpp" ! -name "test.cpp" ! -name "rank.cpp" ! -name "testQueryCompiler.cpp" -name "*.cpp")

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

# Target Executable
TARGET1 = server
TARGET2 = indexserver


# Default Target
all: $(TARGET1)

# Build the target executable
$(TARGET1): $(SRC_FILES1:.cpp=.o) 
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(RPATH_FLAG)

# Build the target executable (index)
$(TARGET2): $(SRC_FILES2:.cpp=.o) 
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(RPATH_FLAG)

# Compile source files to object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: clean

# Clean up build artifacts
clean:
	rm -f  $(TARGET1) $(TARGET2)
	rm $(SRC_FILES1:.cpp=.o) $(SRC_FILES2:.cpp=.o) 

# Phony targets
.PHONY: all clean