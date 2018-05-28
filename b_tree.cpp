#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <functional>
#include <iostream>
#include <memory>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

/**
 * TODO:
 * 2. k-v types
 * 3. use shared_ptr
 */

template <int kDegree>
struct Node {
  static_assert(1 < kDegree, "Invalid degree.");
  std::array<int, 2 * kDegree - 1> keys;
  std::array<Node*, 2 * kDegree> children;
  int n;
};

template <int kDegree>
void InsertNoSplit(Node<kDegree>* node, int index, int key,
                   Node<kDegree>* child, bool insert_left_child) {
  if (2 * kDegree - 1 <= node->n) {
    throw std::invalid_argument{"Node overflow."};
  }
  for (int i = node->n; index < i; --i) {
    node->keys[i] = std::move(node->keys[i - 1]);
  }
  for (int i = node->n + 1; (insert_left_child ? index : index + 1) < i; --i) {
    node->children[i] = std::move(node->children[i - 1]);
  }
  node->keys[index] = std::move(key);
  node->children[insert_left_child ? index : index + 1] = std::move(child);
  ++node->n;
}

template <int kDegree>
void RemoveNoMerge(Node<kDegree>* node, int index, bool remove_left_child,
                   bool no_check = false) {
  if (!no_check && node->n <= kDegree - 1) {
    throw std::invalid_argument{"Node underflow."};
  }
  if (no_check && node->n <= 0) {
    throw std::invalid_argument{"Node underflow."};
  }
  for (int i = index; i + 1 < node->n; ++i) {
    node->keys[i] = std::move(node->keys[i + 1]);
  }
  for (int i = (remove_left_child ? index : index + 1); i < node->n; ++i) {
    node->children[i] = std::move(node->children[i + 1]);
  }
  --node->n;
}

template <int kDegree>
Node<kDegree>* Merge(Node<kDegree>* left, Node<kDegree>* right, int key) {
  if (2 * kDegree - 1 < left->n + right->n + 1) {
    throw std::invalid_argument{"Node overflow."};
  }
  left->keys[left->n] = std::move(key);
  for (int i = 0; i < right->n; ++i) {
    left->keys[left->n + 1 + i] = std::move(right->keys[i]);
    left->children[left->n + 1 + i] = std::move(right->children[i]);
  }
  left->children[left->n + right->n + 1] = std::move(right->children[right->n]);
  left->n = left->n + right->n + 1;
  delete right;
  return left;
}

template <int kDegree>
std::tuple<Node<kDegree>*, Node<kDegree>*, int> Split(Node<kDegree>* node) {
  int left_n = node->n / 2;
  int right_n = node->n - left_n - 1;
  if (left_n < kDegree - 1 || right_n < kDegree - 1) {
    throw std::invalid_argument{"Node underflow."};
  }
  int middle = std::move(node->keys[left_n]);
  Node<kDegree>* right = new Node<kDegree>{};
  for (int i = 0; i < right_n; ++i) {
    right->keys[i] = std::move(node->keys[i + left_n + 1]);
    right->children[i] = std::move(node->children[i + left_n + 1]);
  }
  right->children[right_n] = std::move(node->children[node->n]);
  node->n = left_n;
  right->n = right_n;
  return {node, right, std::move(middle)};
}

template <int kDegree>
Node<kDegree>* FixUnderflow(std::vector<std::pair<Node<kDegree>*, int>> path) {
  if (path.empty()) {
    return nullptr;
  }
  auto old_root = path[0].first;
  while (!path.empty()) {
    auto[top, _] = path.back();
    path.pop_back();
    if (kDegree - 1 <= top->n) {
      return old_root;
    }
    if (path.empty()) {
      if (top->n != 0) {
        return top;
      } else {
        auto ret = top->children[0];
        delete top;
        return ret;
      }
    }
    auto[parent, parent_index] = path.back();
    if (0 < parent_index &&
        kDegree - 1 < parent->children[parent_index - 1]->n) {
      auto sibling = parent->children[parent_index - 1];
      InsertNoSplit(top, 0, std::move(parent->keys[parent_index - 1]),
                    std::move(sibling->children[sibling->n]), true);
      parent->keys[parent_index - 1] = std::move(sibling->keys[sibling->n - 1]);
      RemoveNoMerge(sibling, sibling->n - 1, false);
      return old_root;
    }
    if (parent_index < parent->n &&
        kDegree - 1 < parent->children[parent_index + 1]->n) {
      auto sibling = parent->children[parent_index + 1];
      InsertNoSplit(top, top->n, std::move(parent->keys[parent_index]),
                    sibling->children[0], false);
      parent->keys[parent_index] = std::move(sibling->keys[0]);
      RemoveNoMerge(sibling, 0, true);
      return old_root;
    }
    if (0 < parent_index) {
      auto sibling = parent->children[parent_index - 1];
      Merge(sibling, top, std::move(parent->keys[parent_index - 1]));
      RemoveNoMerge(parent, parent_index - 1, false, true);
    } else if (parent_index < parent->n) {
      auto sibling = parent->children[parent_index + 1];
      Merge(top, sibling, std::move(parent->keys[parent_index]));
      RemoveNoMerge(parent, parent_index, false, true);
    } else {
      throw std::invalid_argument{"Unable to merge nodes."};
    }
  }
  assert(false);
}

template <int kDegree>
Node<kDegree>* FixOverflow(std::vector<std::pair<Node<kDegree>*, int>> path,
                           int key) {
  auto old_root = path.empty() ? nullptr : path[0].first;
  Node<kDegree>* right_child = nullptr;
  while (!path.empty()) {
    auto[top, index] = path.back();
    path.pop_back();
    if (top->n < 2 * kDegree - 1) {
      InsertNoSplit(top, index, std::move(key), right_child, false);
      return old_root;
    }
    auto[_, right, middle] = Split(top);
    if (index <= top->n) {
      InsertNoSplit(top, index, std::move(key), std::move(right_child), false);
    } else {
      InsertNoSplit(right, index - top->n - 1, std::move(key),
                    std::move(right_child), false);
    }
    key = std::move(middle);
    right_child = std::move(right);
  }
  Node<kDegree>* new_root = new Node<kDegree>{};
  new_root->keys[0] = std::move(key);
  new_root->children[0] = std::move(old_root);
  new_root->children[1] = std::move(right_child);
  new_root->n = 1;
  return new_root;
}

template <int kDegree>
Node<kDegree>* Insert(Node<kDegree>* root, int k) {
  std::vector<std::pair<Node<kDegree>*, int>> path;
  Node<kDegree>* current = root;
  while (current != nullptr) {
    int l = 0;
    int r = current->n;
    while (l < r) {
      int m = (l + r) / 2;
      if (current->keys[m] < k) {
        l = m + 1;
      } else {
        r = m;
      }
    }
    if (l < current->n && current->keys[l] == k) {
      return root;
    }
    path.push_back({current, l});
    current = current->children[l];
  }
  return FixOverflow(std::move(path), k);
}

template <int kDegree>
bool Find(Node<kDegree>* root, int k) {
  Node<kDegree>* current = root;
  while (current != nullptr) {
    int l = 0;
    int r = current->n;
    while (l < r) {
      int m = (l + r) / 2;
      if (current->keys[m] < k) {
        l = m + 1;
      } else {
        r = m;
      }
    }
    if (l < current->n && current->keys[l] == k) {
      return true;
    }
    current = current->children[l];
  }
  return false;
}

template <int kDegree>
Node<kDegree>* Remove(Node<kDegree>* root, int k) {
  std::vector<std::pair<Node<kDegree>*, int>> path;
  Node<kDegree>* current = root;
  while (current != nullptr) {
    int l = 0;
    int r = current->n;
    while (l < r) {
      int m = (l + r) / 2;
      if (current->keys[m] < k) {
        l = m + 1;
      } else {
        r = m;
      }
    }
    if (l < current->n && current->keys[l] == k) {
      path.push_back({current, l + 1});
      auto next = current->children[l + 1];
      Node<kDegree>* last = nullptr;
      while (next != nullptr) {
        path.push_back({next, 0});
        last = next;
        next = next->children[0];
      }
      if (last != nullptr) {
        current->keys[l] = std::move(last->keys[0]);
        RemoveNoMerge(last, 0, true, true);
      } else {
        RemoveNoMerge(current, l, true, true);
      }
      return FixUnderflow(path);
    }
    path.push_back({current, l});
    current = current->children[l];
  }
  return root;
}

template <int kDegree>
std::string ToDot(Node<kDegree>* root) {
  std::stringstream ss;
  ss << "digraph G {node [shape=record, height=.1];";
  int node_counter = 1;
  std::function<void(Node<kDegree>*, int)> print_node =
      [&ss, &node_counter, &print_node](Node<kDegree>* node, int id) {
        int start = node_counter;
        node_counter += node->n + 1;
        std::stringstream label;
        for (int i = 0; i < node->n; ++i) {
          label << "<f" << i << ">|" << node->keys[i] << "|";
        }
        label << "<f" << node->n << ">";
        ss << "node" << id << " [label=\"" << label.str() << "\"];";
        for (int i = 0; i < node->n + 1; ++i) {
          auto child = node->children[i];
          if (child != nullptr) {
            print_node(child, start + i);
            ss << "node" << id << ":f" << i << " -> "
               << "node" << start + i << ";";
          }
        }
      };
  if (root != nullptr) {
    print_node(root, 0);
  }
  ss << "}";
  return ss.str();
}

int main() {
  std::default_random_engine engine{std::random_device{}()};
  std::uniform_int_distribution<> distribution{0, 127};
  Node<2>* root = nullptr;
  /*
  for (int i = 0; i < 3; ++i) {
    root = Insert(root, distribution(engine));
  }
  root = Insert(root, 3);
  root = Insert(root, 2);
  root = Insert(root, 4);
  root = Insert(root, 5);
  */
  for (int i = 0; i < 90; ++i) {
    root = Insert(root, i);
  }
  root = Remove(root, 88);
  root = Remove(root, 89);
  root = Remove(root, 84);
  root = Remove(root, 87);
  root = Remove(root, 83);
  root = Remove(root, 79);
  root = Remove(root, 86);
  root = Remove(root, 85);
  for (int i = 0; i < 80; ++i) {
    root = Remove(root, i);
  }
  root = Remove(root, 80);
  root = Remove(root, 81);
  root = Remove(root, 82);

  std::cout << ToDot(root) << std::endl;
  return 0;
}
