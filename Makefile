CXX := clang++
FORMATTER := clang-format
CXXFLAGS += -g -Wall -Wextra -std=c++17 -fsanitize=address

b_tree: b_tree.cpp b_tree.h
	$(CXX) $< -o $@ $(CXXFLAGS)

clean:
	rm -rf b_tree b_tree.dSYM

format:
	$(FORMATTER) -i -style="Google" $(wildcard *.cpp) $(wildcard *.h)

.PHONY: clean format
