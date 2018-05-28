CXX := clang++-mp-5.0
CXXFLAGS += -g -Wall -Wextra -std=c++17

b_tree: b_tree.cpp
	$(CXX) $< -o $@ $(CXXFLAGS)

clean:
	rm -f b_tree

.PHONY: clean
