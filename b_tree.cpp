#include "b_tree.h"

int main() {
  std::default_random_engine engine{std::random_device{}()};
  std::uniform_int_distribution<> distribution{0, 127};
  std::shared_ptr<Node<int, int, 2>> root;
  std::vector<std::shared_ptr<Node<int, int, 2>>> roots;
  for (int i = 0; i < 90; ++i) {
    root = Insert(root, std::make_pair(i, i * 100));
  }
  root = Remove(root, 88);
  root = Remove(root, 89);
  root = Remove(root, 84);
  root = Remove(root, 87);
  root = Remove(root, 83);
  root = Remove(root, 86);
  root = Remove(root, 85);
  for (int i = 0; i < 80; ++i) {
    assert(Find(root, i).value() == i * 100);
    root = Remove(root, i);
    roots.push_back(root);
  }
  root = Remove(root, 79);
  root = Remove(root, 80);
  root = Remove(root, 81);
  root = Remove(root, 82);

  for (auto&& i : roots) {
    std::cout << ToDot(i) << std::endl;
  }
  return 0;
}
