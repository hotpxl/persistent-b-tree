CXX := g++
CXXFLAGS += -g -Wall -Wextra -std=c++14

bp_tree: bp_tree.cpp
	$(CXX) $< -o $@ $(CXXFLAGS)
