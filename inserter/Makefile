CXX ?= g++

CFLAGS ?= -O2
CXXFLAGS ?= $(CFLAGS)

SRC_CXXFLAGS += -std=c++11

all: ta_insert.cpp text.cpp lz77.cpp tiles.cpp images.cpp
	$(CXX) -o ../a $(CXXFLAGS) $(SRC_CXXFLAGS) $^

.PHONY: all
