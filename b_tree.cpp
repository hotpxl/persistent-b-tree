#include "b_tree.h"
#include "gumbo.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <set>

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

// from http://thispointer.com/find-and-replace-all-occurrences-of-a-sub-string-in-c/
void replace_all(std::string & data, std::string toSearch, std::string replaceStr)
{
  // Get the first occurrence
  size_t pos = data.find(toSearch);
 
  // Repeat till end is reached
  while( pos != std::string::npos)
  {
    // Replace this occurrence of Sub String
    data.replace(pos, toSearch.size(), replaceStr);
    // Get the next occurrence from the current position
    pos =data.find(toSearch, pos + toSearch.size());
  }
}


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
  std::string info();
  std::vector<DOMNode *> children;
  std::string tag;
  std::vector<std::pair<std::string, std::string> > attrs;
  std::string text; // only if node is GumboText

private:
  size_t hash_value;
  bool hash_computed;
  // size_t type;
};


DOMNode::DOMNode() {
  hash_computed = false;
  parent = NULL;
}

DOMNode::~DOMNode() {
}

std::string DOMNode::info() {
  std::string info_blob = "<tag: " + tag + ", " + std::to_string(hash_value);
  info_blob.append(", #children: " + std::to_string(children.size()));
  info_blob.append(", text: " + text);
  info_blob.append(", attrs: [");
  for (size_t i = 0; i < attrs.size(); i++) {
    info_blob.append("(" + attrs[i].first + ":" + attrs[i].second + "),");
  }
  info_blob.append("]");
  info_blob.append(">");
  return info_blob;
}

void DOMNode::add_child(DOMNode *child) {
  children.push_back(child);
}

void DOMNode::add_attr(std::string name, std::string value) {
  replace_all(value, "\"", "&quot;");
  replace_all(value, "<", "&lt;");
  replace_all(value, ">", "&gt;");
  replace_all(value, "&", "&amp;");
  replace_all(value, "'", "&#39;");
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

// .................
// start DOMTree class
// .................
class DOMTree
{
public:
  DOMTree(DOMNode *root);
  ~DOMTree();

  DOMNode *root;
  std::set<size_t> hashes;
private:
  void init_hashes(DOMNode *node);
};


DOMTree::DOMTree(DOMNode *root) {
  this->root = root;
  init_hashes(root);
}

void DOMTree::init_hashes(DOMNode *node) {
  hashes.insert(node->get_hash());

  for (size_t i = 0; i < node->children.size(); i++) {
    init_hashes(node->children[i]);
  }
}

DOMTree::~DOMTree() {
}
// .................
// end DOMTree class
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

static DOMNode *load_dom(const GumboNode* root) {
  if (root->type != GUMBO_NODE_ELEMENT) {
    std::cout << (root->type == GUMBO_NODE_WHITESPACE) << std::endl;
    exit(0);
  }

  // init node
  DOMNode *node = new DOMNode();
  node->set_tag(gumbo_normalized_tagname(root->v.element.tag));

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
      child_node = load_dom(child);
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

static std::shared_ptr<Node<size_t, DOMNode *, 2>>
_init_persistent_tree(DOMNode *root,
                      std::shared_ptr<Node<size_t, DOMNode *, 2>> persistent_root) {

  persistent_root = Insert(persistent_root, std::make_pair(root->get_hash(), root));

  for (size_t i = 0; i < root->children.size(); i++) {
    persistent_root = _init_persistent_tree(root->children[i], persistent_root);
  }

  return persistent_root;
}

static std::shared_ptr<Node<size_t, DOMNode *, 2>>
init_persistent_tree(DOMNode *root) {
  std::shared_ptr<Node<size_t, DOMNode *, 2>> persistent_root;
  persistent_root = _init_persistent_tree(root, persistent_root);

  return persistent_root;
}

// may not be exact
static std::string create_html_str(DOMNode *root) {
  if (root->tag == "") {
    return root->text;
  } else {
    std::string html_str;
    html_str.append("<" + root->tag + " "); // open
    // attrs
    for (size_t i = 0; i < root->attrs.size(); i++) {
      html_str.append(root->attrs[i].first + "=\"" + root->attrs[i].second + "\" ");
    }
    html_str.append(">");
    for (size_t i = 0; i < root->children.size(); i++) {
      html_str.append(create_html_str(root->children[i]));
    }
    html_str.append("</" + root->tag + ">");
    return html_str;
  }
}

static std::shared_ptr<Node<size_t, DOMNode *, 2>>
merge_persistent_tree(DOMTree *tree, std::shared_ptr<Node<size_t, DOMNode *, 2>> persistent_root) {
  // generate current version of the tree from persistent root
  DOMTree *originalTree = new DOMTree(get_root(persistent_root->values[0].second));
  // remove all elements that are no longer present
  for (std::set<size_t>::iterator it=originalTree->hashes.begin(); it!=originalTree->hashes.end(); ++it) {
    if (tree->hashes.find(*it) == tree->hashes.end()) {
      std::cout << ((DOMNode *) *Find(persistent_root, *it))->children.size() << std::endl;
      // persistent_root = Remove(persistent_root, *it);
    }
  }
  // add all elements that 
}

int main(int argc, const char** argv) {
  if (argc != 3) {
    printf("Usage: get_title <html filename> <html filename>.\n");
    exit(EXIT_FAILURE);
  }
  const char* filename1 = argv[1];
  const char* filename2 = argv[2];

  FILE* fp1 = fopen(filename1, "r");
  if (!fp1) {
    printf("File %s not found!\n", filename1);
    exit(EXIT_FAILURE);
  }
  char* input1;
  int input_length1;
  read_file(fp1, &input1, &input_length1);

  FILE* fp2 = fopen(filename2, "r");
  if (!fp2) {
    printf("File %s not found!\n", filename2);
    exit(EXIT_FAILURE);
  }
  char* input2;
  int input_length2;
  read_file(fp2, &input2, &input_length2);

  GumboOutput* output1 = gumbo_parse_with_options(
      &kGumboDefaultOptions, input1, input_length1);
  GumboOutput* output2 = gumbo_parse_with_options(
      &kGumboDefaultOptions, input2, input_length2);

  DOMTree *tree1 = new DOMTree(load_dom(output1->root));
  std::shared_ptr<Node<size_t, DOMNode *, 2>> persistent_root= init_persistent_tree(tree1->root);

  DOMTree *tree2 = new DOMTree(load_dom(output2->root));
  persistent_root = merge_persistent_tree(tree2, persistent_root);

  gumbo_destroy_output(&kGumboDefaultOptions, output1);
  gumbo_destroy_output(&kGumboDefaultOptions, output2);
  free(input1);
  free(input2);
  return 0;
}
