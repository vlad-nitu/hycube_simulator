# Makefile

# Paths
# BASE_PATH = /home/vlnitu/virtuoso_artifact/morpher-v2/cppsimulator/src
BASE_PATH = /home/morpher-v2/cppsimulator/src
INCLUDE_PATH = $(BASE_PATH)
LIBRARY_PATH = $(BASE_PATH)/build
LIBRARY_NAME = hycube_simulator

# Compiler
CXX = g++

# Source file
SOURCE = 4radix.cpp

# Output binary
BINARY = main

# Flags
CXXFLAGS = -I$(INCLUDE_PATH)
LDFLAGS = -L$(LIBRARY_PATH) -l$(LIBRARY_NAME)

$(BINARY): $(SOURCE)
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm -f $(BINARY)

