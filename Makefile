# Makefile for problem 025

CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall

all: code cheat-submit.cpp anticheat-submit.cpp

# Create single-file versions for submission
cheat-submit.cpp: common.h cheat.cpp
	@echo "// Auto-generated file - do not edit" > cheat-submit.cpp
	@echo "" >> cheat-submit.cpp
	@sed '/^#include "common.h"/d' common.h >> cheat-submit.cpp
	@echo "" >> cheat-submit.cpp
	@sed '/^#include "common.h"/d' cheat.cpp >> cheat-submit.cpp

anticheat-submit.cpp: common.h anticheat.cpp
	@echo "// Auto-generated file - do not edit" > anticheat-submit.cpp
	@echo "" >> anticheat-submit.cpp
	@sed '/^#include "common.h"/d' common.h >> anticheat-submit.cpp
	@echo "" >> anticheat-submit.cpp
	@sed '/^#include "common.h"/d' anticheat.cpp >> anticheat-submit.cpp

# Build the main executable that can run both modes
code: main.cpp common.h
	$(CXX) $(CXXFLAGS) main.cpp -o code

clean:
	rm -f code cheat anticheat cheat-submit.cpp anticheat-submit.cpp *.o

.PHONY: all clean
