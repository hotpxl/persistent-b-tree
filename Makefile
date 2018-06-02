CXX := clang++
CXXFLAGS += -g -Wall -Wextra -std=c++17 -fsanitize=address -I/usr/local/Cellar/gumbo-parser/0.10.1/include -L/usr/local/Cellar/gumbo-parser/0.10.1/lib -lgumbo

b_tree: b_tree.cpp b_tree.h 
	$(CXX) $< -o $@ $(CXXFLAGS)

clean:
	rm -rf b_tree b_tree.dSYM

.PHONY: clean