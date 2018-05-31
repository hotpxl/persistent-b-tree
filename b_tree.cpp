#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

template <class KeyType, class MappedType, int kDegree>
struct Node {
  static_assert(1 < kDegree, "Invalid degree.");
  using ValueType = std::pair<KeyType, MappedType>;

  std::array<ValueType, 2 * kDegree - 1> values{};
  std::array<std::shared_ptr<Node>, 2 * kDegree> children{};
  int n = 0;

  void InsertNoSplit(int index, ValueType value, std::shared_ptr<Node> child,
                     bool insert_left_child);
  void RemoveNoMerge(int index, bool remove_left_child, bool no_check = false);
  void Merge(std::shared_ptr<Node> other, ValueType value);
  std::pair<std::shared_ptr<Node>, ValueType> Split();
};

template <class KeyType, class MappedType, int kDegree>
void Node<KeyType, MappedType, kDegree>::InsertNoSplit(
    int index, ValueType value, std::shared_ptr<Node> child,
    bool insert_left_child) {
  if (2 * kDegree - 1 <= n) {
    throw std::invalid_argument{"Node overflow."};
  }
  for (int i = n; index < i; --i) {
    values[i] = std::move(values[i - 1]);
  }
  for (int i = n + 1; (insert_left_child ? index : index + 1) < i; --i) {
    children[i] = std::move(children[i - 1]);
  }
  values[index] = std::move(value);
  children[insert_left_child ? index : index + 1] = std::move(child);
  ++n;
}

template <class KeyType, class MappedType, int kDegree>
void Node<KeyType, MappedType, kDegree>::RemoveNoMerge(int index,
                                                       bool remove_left_child,
                                                       bool no_check) {
  if (n <= (no_check ? 0 : kDegree - 1)) {
    throw std::invalid_argument{"Node underflow."};
  }
  for (int i = index; i + 1 < n; ++i) {
    values[i] = std::move(values[i + 1]);
  }
  for (int i = (remove_left_child ? index : index + 1); i < n; ++i) {
    children[i] = std::move(children[i + 1]);
  }
  --n;
}

template <class KeyType, class MappedType, int kDegree>
void Node<KeyType, MappedType, kDegree>::Merge(std::shared_ptr<Node> right,
                                               ValueType value) {
  if (2 * kDegree - 1 < n + right->n + 1) {
    throw std::invalid_argument{"Node overflow."};
  }
  values[n] = std::move(value);
  for (int i = 0; i < right->n; ++i) {
    values[n + 1 + i] = std::move(right->values[i]);
    children[n + 1 + i] = std::move(right->children[i]);
  }
  children[n + right->n + 1] = std::move(right->children[right->n]);
  n = n + right->n + 1;
}

template <class KeyType, class MappedType, int kDegree>
auto Node<KeyType, MappedType, kDegree>::Split()
    -> std::pair<std::shared_ptr<Node>, ValueType> {
  int left_n = n / 2;
  int right_n = n - left_n - 1;
  if (left_n < kDegree - 1 || right_n < kDegree - 1) {
    throw std::invalid_argument{"Node underflow."};
  }
  ValueType middle = std::move(values[left_n]);
  auto right = std::make_shared<Node>();
  for (int i = 0; i < right_n; ++i) {
    right->values[i] = std::move(values[i + left_n + 1]);
    right->children[i] = std::move(children[i + left_n + 1]);
  }
  right->children[right_n] = std::move(children[n]);
  n = left_n;
  right->n = right_n;
  return {std::move(right), std::move(middle)};
}

template <class KeyType, class MappedType, int kDegree>
std::vector<std::pair<std::shared_ptr<Node<KeyType, MappedType, kDegree>>, int>>
CopyPath(
    std::vector<std::pair<std::shared_ptr<Node<KeyType, MappedType, kDegree>>,
                          int>> const& path) {
  std::vector<
      std::pair<std::shared_ptr<Node<KeyType, MappedType, kDegree>>, int>>
      ret;
  std::shared_ptr<Node<KeyType, MappedType, kDegree>> last;
  for (auto i = path.rbegin(); i != path.rend(); ++i) {
    auto && [ node, index ] = *i;
    auto new_node = std::make_shared<Node<KeyType, MappedType, kDegree>>(*node);
    new_node->children[index] = std::move(last);
    last = std::move(new_node);
    ret.push_back({last, index});
  }
  std::reverse(ret.begin(), ret.end());
  return ret;
}

template <class KeyType, class MappedType, int kDegree>
std::shared_ptr<Node<KeyType, MappedType, kDegree>> FixUnderflow(
    std::vector<
        std::pair<std::shared_ptr<Node<KeyType, MappedType, kDegree>>, int>>
        path) {
  if (path.empty()) {
    return nullptr;
  }
  auto old_root = path[0].first;
  while (!path.empty()) {
    auto[top, _] = std::move(path.back());
    path.pop_back();
    if (kDegree - 1 <= top->n) {
      return old_root;
    }
    if (path.empty()) {
      if (top->n != 0) {
        return top;
      } else {
        return top->children[0];
      }
    }
    auto[parent, parent_index] = path.back();
    if (0 < parent_index &&
        kDegree - 1 < parent->children[parent_index - 1]->n) {
      auto sibling = std::make_shared<Node<KeyType, MappedType, kDegree>>(
          *parent->children[parent_index - 1]);
      parent->children[parent_index - 1] = sibling;
      top->InsertNoSplit(0, std::move(parent->values[parent_index - 1]),
                         std::move(sibling->children[sibling->n]), true);
      parent->values[parent_index - 1] =
          std::move(sibling->values[sibling->n - 1]);
      sibling->RemoveNoMerge(sibling->n - 1, false);
      return old_root;
    }
    if (parent_index < parent->n &&
        kDegree - 1 < parent->children[parent_index + 1]->n) {
      auto sibling = std::make_shared<Node<KeyType, MappedType, kDegree>>(
          *parent->children[parent_index + 1]);
      parent->children[parent_index + 1] = sibling;
      top->InsertNoSplit(top->n, std::move(parent->values[parent_index]),
                         std::move(sibling->children[0]), false);
      parent->values[parent_index] = std::move(sibling->values[0]);
      sibling->RemoveNoMerge(0, true);
      return old_root;
    }
    if (0 < parent_index) {
      auto sibling = std::make_shared<Node<KeyType, MappedType, kDegree>>(
          *parent->children[parent_index - 1]);
      parent->children[parent_index - 1] = sibling;
      sibling->Merge(top, std::move(parent->values[parent_index - 1]));
      parent->RemoveNoMerge(parent_index - 1, false, true);
    } else if (parent_index < parent->n) {
      auto sibling = std::make_shared<Node<KeyType, MappedType, kDegree>>(
          *parent->children[parent_index + 1]);
      parent->children[parent_index + 1] = sibling;
      top->Merge(sibling, std::move(parent->values[parent_index]));
      parent->RemoveNoMerge(parent_index, false, true);
    } else {
      throw std::invalid_argument{"Unable to merge nodes."};
    }
  }
  assert(false);
}

template <class KeyType, class MappedType, int kDegree>
std::shared_ptr<Node<KeyType, MappedType, kDegree>> FixOverflow(
    std::vector<
        std::pair<std::shared_ptr<Node<KeyType, MappedType, kDegree>>, int>>
        path,
    typename Node<KeyType, MappedType, kDegree>::ValueType value) {
  auto old_root = path.empty() ? nullptr : path[0].first;
  std::shared_ptr<Node<KeyType, MappedType, kDegree>> right_child;
  while (!path.empty()) {
    auto[top, index] = std::move(path.back());
    path.pop_back();
    if (top->n < 2 * kDegree - 1) {
      top->InsertNoSplit(index, std::move(value), std::move(right_child),
                         false);
      return old_root;
    }
    auto[right, middle] = top->Split();
    if (index <= top->n) {
      top->InsertNoSplit(index, std::move(value), std::move(right_child),
                         false);
    } else {
      right->InsertNoSplit(index - top->n - 1, std::move(value),
                           std::move(right_child), false);
    }
    value = std::move(middle);
    right_child = std::move(right);
  }
  auto new_root = std::make_shared<Node<KeyType, MappedType, kDegree>>();
  new_root->values[0] = std::move(value);
  new_root->children[0] = std::move(old_root);
  new_root->children[1] = std::move(right_child);
  new_root->n = 1;
  return new_root;
}

// TODO use comparator?
template <class KeyType, class MappedType, int kDegree>
std::shared_ptr<Node<KeyType, MappedType, kDegree>> Insert(
    std::shared_ptr<Node<KeyType, MappedType, kDegree>> root,
    typename Node<KeyType, MappedType, kDegree>::ValueType value) {
  std::vector<
      std::pair<std::shared_ptr<Node<KeyType, MappedType, kDegree>>, int>>
      path;
  std::shared_ptr<Node<KeyType, MappedType, kDegree>> current = root;
  while (current) {
    int l = 0;
    int r = current->n;
    while (l < r) {
      int m = (l + r) / 2;
      if (current->values[m].first < value.first) {
        l = m + 1;
      } else {
        r = m;
      }
    }
    if (l < current->n && current->values[l].first == value.first) {
      return root;
    }
    path.push_back({current, l});
    current = current->children[l];
  }
  return FixOverflow(CopyPath(path), std::move(value));
}

template <class KeyType, class MappedType, int kDegree>
std::optional<std::reference_wrapper<MappedType>> Find(
    std::shared_ptr<Node<KeyType, MappedType, kDegree>> root,
    KeyType const& k) {
  std::shared_ptr<Node<KeyType, MappedType, kDegree>> current = root;
  while (current) {
    int l = 0;
    int r = current->n;
    while (l < r) {
      int m = (l + r) / 2;
      if (current->values[m].first < k) {
        l = m + 1;
      } else {
        r = m;
      }
    }
    if (l < current->n && current->values[l].first == k) {
      return std::make_optional(
          std::reference_wrapper<MappedType>{current->values[l].second});
    }
    current = current->children[l];
  }
  return std::nullopt;
}

template <class KeyType, class MappedType, int kDegree>
std::shared_ptr<Node<KeyType, MappedType, kDegree>> Remove(
    std::shared_ptr<Node<KeyType, MappedType, kDegree>> root,
    KeyType const& k) {
  std::vector<
      std::pair<std::shared_ptr<Node<KeyType, MappedType, kDegree>>, int>>
      path;
  std::shared_ptr<Node<KeyType, MappedType, kDegree>> current = root;
  while (current) {
    int l = 0;
    int r = current->n;
    while (l < r) {
      int m = (l + r) / 2;
      if (current->values[m].first < k) {
        l = m + 1;
      } else {
        r = m;
      }
    }
    if (l < current->n && current->values[l].first == k) {
      int current_i = path.size();
      path.push_back({current, l + 1});
      auto next = current->children[l + 1];
      bool swap = false;
      while (next != nullptr) {
        path.push_back({next, 0});
        swap = true;
        next = next->children[0];
      }
      path = CopyPath(path);
      if (swap) {
        path[current_i].first->values[l] =
            std::move(path.back().first->values[0]);
        path.back().first->RemoveNoMerge(0, true, true);
      } else {
        path[current_i].first->RemoveNoMerge(l, true, true);
      }
      return FixUnderflow(std::move(path));
    }
    path.push_back({current, l});
    current = current->children[l];
  }
  return root;
}

template <class KeyType, class MappedType, int kDegree>
std::string ToDot(
    std::shared_ptr<Node<KeyType, MappedType, kDegree>> const& root) {
  std::stringstream ss;
  ss << "digraph G {node [shape=record, height=.1];";
  int node_counter = 1;
  std::function<void(std::shared_ptr<Node<KeyType, MappedType, kDegree>> const&,
                     int)>
      print_node =
          [&ss, &node_counter, &print_node](
              std::shared_ptr<Node<KeyType, MappedType, kDegree>> const& node,
              int id) {
            int start = node_counter;
            node_counter += node->n + 1;
            std::stringstream label;
            for (int i = 0; i < node->n; ++i) {
              label << "<f" << i << ">|" << node->values[i].first << "|";
            }
            label << "<f" << node->n << ">";
            ss << "node" << id << " [label=\"" << label.str() << "\"];";
            for (int i = 0; i < node->n + 1; ++i) {
              auto&& child = node->children[i];
              if (child) {
                print_node(child, start + i);
                ss << "node" << id << ":f" << i << " -> "
                   << "node" << start + i << ";";
              }
            }
          };
  if (root) {
    print_node(root, 0);
  }
  ss << "}";
  return ss.str();
}

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
