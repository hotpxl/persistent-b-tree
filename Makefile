CXX := clang++
CXXFLAGS += -g -Wall -Wextra -std=c++17 -fsanitize=address

b_tree: b_tree.cpp b_tree.h
	$(CXX) $< -o $@ $(CXXFLAGS)

clean:
	rm -rf b_tree b_tree.dSYM

.PHONY: clean
