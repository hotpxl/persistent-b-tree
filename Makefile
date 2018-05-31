CXX := clang++-mp-5.0
CXXFLAGS += -g -Wall -Wextra -std=c++17 -fsanitize=address

b_tree: b_tree.cpp
	$(CXX) $< -o $@ $(CXXFLAGS)

clean:
	rm -rf b_tree b_tree.dSYM

.PHONY: clean
