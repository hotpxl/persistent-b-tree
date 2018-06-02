#include "b_tree.h"
#include "gumbo.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <iostream>
#include <functional>
#include <string>

// static void test() {
//   std::shared_ptr<Node<std::string, std::string, 20>> current_root;
//   std::vector<std::shared_ptr<Node<std::string, std::string, 20>>> tree_history;
//   std::vector<std::string> values;

//   for (int i = 0; i < 500; i++) {
//     values.push_back("v" + std::to_string(rand() % 100));
//     std::string key = "k" + std::to_string(i);
//     std::cout << "key: " << key << " value: " << values[i] << std::endl;
//     current_root = Insert(current_root, std::make_pair(key, values[i]));
//     tree_history.push_back(current_root);
//   }

//   for (int i = 0; i < 500; i++) {
//     std::string key = "k" + std::to_string(i);
//     // std::cout << (std::string) *Find(tree_history[i], key) << std::endl;
//     assert((std::string) *Find(tree_history[i], key) == values[i]);
//   }
//   std::cout << "found all at relative root " << std::endl;

//   for (int i = 0; i < 500; i++) {
//     std::string key = "k" + std::to_string(i);
//     assert((std::string)*Find(tree_history[499], key) == values[i]);
//   }
//   std::cout << "found all at current root " << std::endl;

//   for (int i = 500; i < 600; i++) {
//     std::string key = "k" + std::to_string(i);
//     assert(Find(tree_history[499], key) == OPTIONAL_NS::nullopt);
//   }
//   std::cout << "no false positives" << std::endl;

//   // remove 50 - 199 from latest version of the tree
//   for (int i = 50; i < 200; i++) {
//     std::string key = "k" + std::to_string(i);
//     current_root = Remove(current_root, key);
//   }

//   // ensure that 50 - 199 aren't in latest version of tree
//   for (int i = 50; i < 200; i++) {
//     std::string key = "k" + std::to_string(i);
//     assert(Find(current_root, key) == OPTIONAL_NS::nullopt);
//   }
//   std::cout << "no remove false positives" << std::endl;

//   for (int i = 50; i < 200; i++) {
//     std::string key = "k" + std::to_string(i);
//     assert((std::string)*Find(tree_history[499], key) == values[i]);
//   }
//   std::cout << "no remove false negatives" << std::endl;
// }

// .................
// start DOMNode class
// .................
class DOMNode
{
public:
  DOMNode();
  ~DOMNode();
  
  size_t get_hash();
  void add_child(DOMNode *child);
  void add_attr(std::string name, std::string value);
  void set_tag(std::string tag);
  void set_text(std::string text);
  void hash();

private:
  size_t hash_value;
  bool hash_computed;
  std::string tag;
  // size_t type;
  std::string text; // only if node is GumboText
  std::vector<DOMNode *> children;
  std::vector<std::pair<std::string, std::string> > attrs;
};


DOMNode::DOMNode() {
  hash_computed = false;
}

DOMNode::~DOMNode() {
}

void DOMNode::add_child(DOMNode *child) {
  children.push_back(child);
}

void DOMNode::add_attr(std::string name, std::string value) {
  attrs.push_back(std::make_pair(name, value));
}

void DOMNode::set_tag(std::string tag) {
  this->tag = tag;
}

void DOMNode::set_text(std::string text) {
  this->text = text;
}

void DOMNode::hash() {
  std::string text_blob;
  text_blob.append(tag);
  text_blob.append(text);
  for (size_t i = 0; i < children.size(); i++) {
    text_blob.append(std::to_string(children[i]->get_hash()));
  }
  for (size_t i = 0; i < attrs.size(); i++) {
    text_blob.append(attrs[i].first + ":" + attrs[i].second);
  }
  hash_value = std::hash<std::string>{}(text_blob);
  hash_computed = true;
}

size_t DOMNode::get_hash() {
  assert(hash_computed);
  return hash_value;
}
// .................
// end DOMNode class
// .................

static void read_file(FILE* fp, char** output, int* length) {
  struct stat filestats;
  int fd = fileno(fp);
  fstat(fd, &filestats);
  *length = filestats.st_size;
  *output = (char *)malloc(*length + 1);
  int start = 0;
  int bytes_read;
  while ((bytes_read = fread(*output + start, 1, *length - start, fp))) {
    start += bytes_read;
  }
}

static DOMNode *traverse(const GumboNode* root) {
  if (root->type != GUMBO_NODE_ELEMENT) {
    std::cout << (root->type == GUMBO_NODE_WHITESPACE) << std::endl;
    exit(0);
  }

  // init node
  DOMNode *node = new DOMNode();

  const GumboVector* attrs = &root->v.element.attributes;
  for (size_t i = 0; i < attrs->length; i++) {
    GumboAttribute* attr = (GumboAttribute *)attrs->data[i];
    node->add_attr(attr->name, attr->value);
  }

  const GumboVector* root_children = &root->v.element.children;
  for (size_t i = 0; i < root_children->length; ++i) {
    GumboNode* child = (GumboNode*)root_children->data[i];
    DOMNode *child_node;
    if (child->type == GUMBO_NODE_ELEMENT) {
      // recurse
      child_node = traverse(child);
    } else {
      // no recurse
      child_node = new DOMNode();
      child_node->set_text(child->v.text.text);
      child_node->hash();
    }
    node->add_child(child_node);
  }

  node->hash();
  return node;
}

int main(int argc, const char** argv) {
  if (argc != 2) {
    printf("Usage: get_title <html filename>.\n");
    exit(EXIT_FAILURE);
  }
  const char* filename = argv[1];

  FILE* fp = fopen(filename, "r");
  if (!fp) {
    printf("File %s not found!\n", filename);
    exit(EXIT_FAILURE);
  }

  char* input;
  int input_length;
  read_file(fp, &input, &input_length);

  GumboOutput* output = gumbo_parse_with_options(
      &kGumboDefaultOptions, input, input_length);

  traverse(output->root);

  gumbo_destroy_output(&kGumboDefaultOptions, output);
  free(input);
  return 0;
}
