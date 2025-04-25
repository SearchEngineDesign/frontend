# Compiler and Flags
CXX = g++
CXXFLAGS = -Wall -O2 -std=c++17 -pthread

# Directories
ISR_DIR = ../isr
INDEX_DIR = ../index
UTILS_DIR = ../utils
QUERY_COMPILER_DIR = ../queryCompiler
UTF8PROC_DIR = ../index/stemmer/utf8proc
PARSER_DIR = ../parser
CRAWL_DIR = ../Crawler

# Source Files
SRCS = ../dynamicRanker/driver.cpp \
       ../dynamicRanker/heuristics.cpp \
       $(ISR_DIR)/isr.cpp \
       $(ISR_DIR)/isrHandler.cpp \
       $(INDEX_DIR)/index.cpp \
	   $(PARSER_DIR)/HtmlParser.cpp \
	   $(PARSER_DIR)/HtmlTags.cpp \
	   $(CRAWL_DIR)/crawler.cpp \
       $(UTILS_DIR)/Utf8.cpp \
       $(UTILS_DIR)/IndexBlob.cpp \
       $(UTILS_DIR)/searchstring.cpp \
		 $(QUERY_COMPILER_DIR)/compiler.cpp \
		 $(QUERY_COMPILER_DIR)/tokenstream.cpp \
		 ../index/stemmer/stemmer.cpp \
		 server.cpp

# Library Flags
LDFLAGS = -L$(UTF8PROC_DIR) -Wl,-rpath,$(UTF8PROC_DIR) -lutf8proc -lssl

ifeq ($(OS),Windows_NT)
	OPENSSL_DIR = /usr/include/openssl
	INCLUDES = -I$(OPENSSL_DIR)
	LDFLAGS += -L/usr/lib -L"$(shell pwd)/index/stemmer/utf8proc"
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Darwin)
		# macOS specific settings
		OPENSSL_DIR = /opt/homebrew/opt/openssl@3
		INCLUDES = -I$(OPENSSL_DIR)/include 
		LDFLAGS += -L$(OPENSSL_DIR)/lib 
	else
		# Linux specific settings
		OPENSSL_DIR = /usr/include/openssl
		INCLUDES = -I$(OPENSSL_DIR)
	endif
endif

# Object Files
OBJS = $(SRCS:.cpp=.o)

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