#include <cstdio>
#include <iostream>
#include <functional>
#include <string>
#include <sstream>
#include <algorithm>
#include <utility>
#include <tuple>
#include <array>
#include <vector>

/**
 * TODO:
 * 1. make degree a template parameter
 * 2. k-v types
 */

template <int kDegree>
struct Node {
  static_assert(1 < kDegree, "Invalid degree.");
  std::array<int, 2*kDegree-1> keys;
  std::array<Node*, 2*kDegree> children;
  int n;
};

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
  Node<kDegree>* l = nullptr;
  Node<kDegree>* r = nullptr;
  while (!path.empty()) {
    Node<kDegree>* top;
    int index;
    std::tie(top, index) = path.back();
    path.pop_back();
    if (top->n + 1 < 2 * kDegree - 1) {
      std::copy_backward(top->keys.begin() + index, top->keys.begin() + top->n, top->keys.begin() + top->n + 1);
      std::copy_backward(top->children.begin() + index + 1, top->children.begin() + top->n + 1, top->children.begin() + top->n + 2);
      top->keys[index] = k;
      top->children[index] = l;
      top->children[index+1] = r;
      ++top->n;
      return root;
    }
    l = top;
    r = new Node<kDegree>{};
    k = top->keys[kDegree - 1];
    std::copy(l->keys.begin() + kDegree, l->keys.end(), r->keys.begin());
    std::copy(l->children.begin() + kDegree, l->children.end(), r->children.begin());
    l->n = kDegree - 1;
    r->n = kDegree - 1;
  }
  Node<kDegree>* new_root = new Node<kDegree>{};
  new_root->keys[0] = k;
  new_root->children[0] = l;
  new_root->children[1] = r;
  new_root->n = 1;
  return new_root;
}


template <int kDegree>
std::string ToDot(Node<kDegree>* root) {
  std::stringstream ss;
  ss << "digraph G {node [shape=record, height=.1];";
  int node_counter = 1;
  std::function<void(Node<kDegree>*, int)> print_node = [&ss, &node_counter, &print_node](Node<kDegree>* node, int id) {
    int start = node_counter;
    node_counter += node->n;
    std::stringstream label;
    for (int i = 0; i < node->n; ++i) {
      label << "<f" << i << ">|" << node->keys[i] << "|";
    }
    label << "<f" << node->n << ">";
    ss << "node" << id << " [label=\"" << label.str() << "\"];";
    for (int i = 0; i < node->n + 1; ++i) {
      auto child = node->children[i];
      print_node(child, start + i);
      ss << "node:" << id << " -> " << "node" << start + i << ";";
    }
  };
  if (root != nullptr) {
    print_node(root, 0);
  }
  ss << "}";
  return ss.str();
}


int main() {
  Node<2>* root = new Node<2>{};;
  root->n=0;
  std::cout << ToDot(root) << std::endl;
  root = Insert(root, 3);
  return 0;
}
