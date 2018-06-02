#include "b_tree.h"
#include "gumbo.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <set>
#include <unordered_map>

#include <iostream>
#include <functional>
#include <string>

#define B_ELEMS 2

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
  DOMTree(DOMNode *dom_root, std::shared_ptr<Node<size_t, DOMNode *, B_ELEMS>> persistent_root);
  ~DOMTree();
  std::unordered_map<size_t, DOMNode *> get_hashes_map();
  DOMNode *get_node(size_t hash);

  DOMNode *dom_root;
  std::shared_ptr<Node<size_t, DOMNode *, B_ELEMS>> persistent_root;
private:
  void _get_hashes_map(DOMNode *node, std::unordered_map<size_t, DOMNode *> &hashes);
};

DOMTree::DOMTree(DOMNode *dom_root,
  std::shared_ptr<Node<size_t, DOMNode *, B_ELEMS>> persistent_root) {
  this->dom_root = dom_root;
  this->persistent_root = persistent_root;
}

void DOMTree::_get_hashes_map(DOMNode *node, std::unordered_map<size_t, DOMNode *> &hashes_map) {
  if (node == NULL) return;

  hashes_map.insert(std::make_pair(node->get_hash(), node));

  for (size_t i = 0; i < node->children.size(); i++) {
    _get_hashes_map(node->children[i], hashes_map);
  }
}

std::unordered_map<size_t, DOMNode *> DOMTree::get_hashes_map() {
  std::unordered_map<size_t, DOMNode *> hashes_map;
  _get_hashes_map(dom_root, hashes_map);
  return hashes_map;
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

// static DOMTree persistent_tree_insert_or_free(DOMNode *node,
//     std::shared_ptr<Node<size_t, DOMNode *, B_ELEMS>> persistent_root) {

//   DOMNode *result_node;
//   if (Find(persistent_root, node->get_hash()) == OPTIONAL_NS::nullopt) {
//     result_node = node;
//     persistent_root = Insert(persistent_root, std::make_pair(node->get_hash(), node));
//   } else {
//     result_node = *Find(persistent_root, node->get_hash()); // TODO only make this call once
//     delete node;
//   }
//   return DOMTree(result_node, persistent_root);
// }

static int indexOf(std::vector<DOMNode *> &nodes, DOMNode *target_node) {
  for (size_t i = 0; i < nodes.size(); i++) {
    if (nodes[i] == target_node) return i;
  }
  assert(false);
  return -1;
}

static std::shared_ptr<Node<size_t, DOMNode *, B_ELEMS>>
merge_new_nodes(DOMNode *dom_parent,
                DOMNode *dom_child,
                std::shared_ptr<Node<size_t, DOMNode *, B_ELEMS>> current_root) {
  assert(dom_parent != NULL);
  // traverse to leaf first
  for (size_t i = 0; i < dom_child->children.size(); i++) {
    current_root = merge_new_nodes(dom_child, dom_child->children[i], current_root);
  }
  // std::cout << "node" << std::endl;

  // if dom_child not in tree, add it
  if (Find(current_root, dom_child->get_hash()) == OPTIONAL_NS::nullopt) {
    // add
    current_root = Insert(current_root, std::make_pair(dom_child->get_hash(), dom_child));
    // std::cout << "inserting" << std::endl;
  } else {
    // delete child and repoint parent
    DOMNode *existing_node = *Find(current_root, dom_child->get_hash());
    assert(existing_node->get_hash() == dom_child->get_hash());
    // get index of dom_child
    int child_idx = indexOf(dom_parent->children, dom_child);
    dom_parent->children[child_idx] = existing_node;
    delete dom_child;
  }

  return current_root;
}

static DOMTree merge_dom_with_persistent_tree(DOMNode *dom_root, DOMTree &original_tree) {
  DOMTree new_tree = DOMTree(dom_root, original_tree.persistent_root);
  std::unordered_map<size_t, DOMNode *> original_hashes_map = original_tree.get_hashes_map();
  std::unordered_map<size_t, DOMNode *> new_hashes_map = new_tree.get_hashes_map();
  std::shared_ptr<Node<size_t, DOMNode *, B_ELEMS>> current_root = original_tree.persistent_root;
  // remove elements from persistent tree that are no longer there
  int remove_cnt = 0;
  for (auto it=original_hashes_map.begin(); it!=original_hashes_map.end(); ++it) {
    auto result = new_hashes_map.find(it->first);
    if (result == new_hashes_map.end()) {
      current_root = Remove(current_root, it->first);
      remove_cnt++;
    }
  }
  // std::cout << "original_size: " << original_hashes_map.size() << ", new_size: " << new_hashes_map.size() << std::endl;
  // std::cout << "removed: " << remove_cnt << std::endl;
  // add all new elements, delete duplicates and redo child pointers
  for (size_t i = 0; i < dom_root->children.size(); i++) {
    current_root = merge_new_nodes(dom_root, dom_root->children[i], current_root);
  }
  if (Find(current_root, dom_root->get_hash()) == OPTIONAL_NS::nullopt) {
    current_root = Insert(current_root, std::make_pair(dom_root->get_hash(), dom_root));
  } else {
    DOMNode *existing_node = *Find(current_root, dom_root->get_hash());
    delete dom_root;
    new_tree.dom_root = existing_node;
  }

  new_tree.persistent_root = current_root;

  return new_tree;
}


static DOMNode *load_dom(const GumboNode* root) {
  assert(root->type == GUMBO_NODE_ELEMENT);

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

  std::shared_ptr<Node<size_t, DOMNode *, B_ELEMS>> persistent_root;
  DOMTree empty_tree = DOMTree(NULL, persistent_root);

  DOMNode *dom_root1 = load_dom(output1->root);
  DOMTree tree1 = merge_dom_with_persistent_tree(dom_root1, empty_tree);
  // std::cout << create_html_str(tree1.dom_root) << std::endl;
  // std::cout << "starting tree 2" << std::endl;

  for (size_t i = 0; i < 10000; i++) {
    DOMNode *dom_root2 = load_dom(output2->root);
    DOMTree tree2 = merge_dom_with_persistent_tree(dom_root2, tree1);
  }
  
  // std::cout << create_html_str(tree2.dom_root) << std::endl;

  gumbo_destroy_output(&kGumboDefaultOptions, output1);
  gumbo_destroy_output(&kGumboDefaultOptions, output2);
  free(input1);
  free(input2);

  std::cout << "done";
  getchar();
  return 0;
}
