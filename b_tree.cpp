#include "b_tree.h"
#include "gumbo.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <set>
#include <unordered_map>
#include <sstream>
#include <iostream>
#include <functional>
#include <string>
#include <sys/types.h>
#include <dirent.h>
#include <fstream>

#define B_ELEMS 2

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
  size_t num_nodes_added;
  std::shared_ptr<Node<size_t, DOMNode *, B_ELEMS>> persistent_root;
private:
  void _get_hashes_map(DOMNode *node, std::unordered_map<size_t, DOMNode *> &hashes);
};

DOMTree::DOMTree(DOMNode *dom_root,
  std::shared_ptr<Node<size_t, DOMNode *, B_ELEMS>> persistent_root) {
  this->dom_root = dom_root;
  this->persistent_root = persistent_root;
  num_nodes_added = 0;
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
                std::shared_ptr<Node<size_t, DOMNode *, B_ELEMS>> current_root,
                size_t &num_nodes_added) {
  assert(dom_parent != NULL);
  // traverse to leaf first
  for (size_t i = 0; i < dom_child->children.size(); i++) {
    current_root = merge_new_nodes(dom_child, dom_child->children[i], current_root, num_nodes_added);
  }

  // if dom_child not in tree, add it
  if (Find(current_root, dom_child->get_hash()) == OPTIONAL_NS::nullopt) {
    // add
    current_root = Insert(current_root, std::make_pair(dom_child->get_hash(), dom_child));
    num_nodes_added++;
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
  std::shared_ptr<Node<size_t, DOMNode *, B_ELEMS>> current_root = original_tree.persistent_root;

  // add all new elements, delete duplicates and redo child pointers
  for (size_t i = 0; i < dom_root->children.size(); i++) {
    current_root = merge_new_nodes(dom_root, dom_root->children[i], current_root, new_tree.num_nodes_added);
  }
  if (Find(current_root, dom_root->get_hash()) == OPTIONAL_NS::nullopt) {
    current_root = Insert(current_root, std::make_pair(dom_root->get_hash(), dom_root));
    new_tree.num_nodes_added++;
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
  if (root == NULL) return "";
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

static void _get_elements_by_attr(DOMNode *root,
                                  std::string &key,
                                  std::string &value,
                                  bool exact_match,
                                  std::vector<DOMNode *> &matches) {
  if (root == NULL) return;
  // check match
  for (size_t i = 0; i < root->attrs.size(); i++) {
    if (exact_match) {
      if (root->attrs[i].first == key && root->attrs[i].second == value) {
        matches.push_back(root);
      }
    } else {
      if (root->attrs[i].first == key && root->attrs[i].second.find(value) != std::string::npos) {
        matches.push_back(root);
      }
    }

  }
  for (size_t i = 0; i < root->children.size(); i++) {
    _get_elements_by_attr(root->children[i], key, value, exact_match, matches);
  }
}

static std::vector<DOMNode *> get_elements_by_attr(DOMNode *root,
                                                    std::string key,
                                                    std::string value,
                                                    bool exact_match) {
  std::vector<DOMNode *> matches;
  _get_elements_by_attr(root, key, value, exact_match, matches);
  return matches;
}

static void interactive_query(std::vector<DOMTree> &tree_history) {
  while (true) {
    std::cout << "select action: (q to quit)" << std::endl;
    std::cout << "1) query attr exact match" << std::endl;
    std::cout << "2) query attr contains" << std::endl;
    std::cout << "3) construct version" << std::endl;
    std::string ans;
    std::cin >> ans;
    bool exact_match;
    if (ans == "1") {
      exact_match = true;
    } else if (ans == "2") {
      exact_match = false;
    } else if (ans == "3") {
      int version;
      std::cout << "select version [0," << (tree_history.size() - 1) << "]: ";
      std::cin >> version;
      std::ofstream myfile;
      std::string filename = "version" + std::to_string(version) + ".html";
      myfile.open (filename);
      myfile << create_html_str(tree_history[version].dom_root) << std::endl;
      myfile.close();
      std::cout << "created file: " << filename << std::endl;
      continue;
    } else if (ans == "q") {
      break;
    } else {
      continue;
    }

    std::cout << "select versions [0," << (tree_history.size() - 1) << "] or \"all\": " << std::endl;
    std::cout << "low: ";
    std::string low;
    int low_idx;
    int high_idx;
    std::cin >> low;
    if (low == "all") {
      low_idx = 0;
      high_idx = tree_history.size() - 1;
    } else {
      std::stringstream low_ss(low);
      low_ss >> low_idx;
      std::cout << "high: ";
      std::cin >> high_idx;  
    }
    

    std::string key;
    std::cout << "key: ";
    std::cin >> key;
    if (key == "q") break;
    std::string value;
    std::cout << "value: ";
    std::cin >> value;
    std::cout << "searching: \"" << key << "\": \"" << value << "\"" << std::endl;

    for (int i = low_idx; i <= high_idx; i++) {
      std::cout << "============ version: " << i << " =============" << std::endl;
      std::vector<DOMNode *> matches = get_elements_by_attr(tree_history[i].dom_root, key, value, exact_match);
      for (size_t i = 0; i < matches.size(); i++) {
        std::cout << matches[i]->info() << std::endl;
      }
      std::cout << "=============" << std::endl;
    }
  }
}

static void log_estimated_memory_usage(std::vector<DOMTree> &tree_history) {
  size_t total_dom_nodes = 0;
  for (size_t i = 0; i < tree_history.size(); i++) {
    total_dom_nodes += tree_history[i].num_nodes_added;
  }
  // a little tricky to get node size using sizeof.... this should be equivalent
  double b_tree_node_size = sizeof(void *) * 2 + sizeof(void *) * 2 * B_ELEMS - 1 + sizeof(void *) * 2 * B_ELEMS + sizeof(int);
  double numerator = total_dom_nodes * sizeof(DOMNode) + // size of all DOMNodes
                      tree_history.size() * sizeof(DOMTree) + sizeof(std::vector<DOMTree>) + // size tree_history
                      (double)total_dom_nodes/B_ELEMS * b_tree_node_size; // b tree overhead

  double size_in_mb = numerator/ 1000.0;
  std::cout << "Memory usage: " << size_in_mb << " kb" << std::endl;
}

int main(int argc, const char** argv) {
  if (argc != 2) {
    printf("Usage: b_tree <html director>.\n");
    exit(EXIT_FAILURE);
  }
  std::vector<std::string> file_paths;
  DIR *dir;
  struct dirent *ent;
  const char *dir_name = argv[1];
  if ((dir = opendir (dir_name)) != NULL) {
    /* print all the files and directories within directory */
    while ((ent = readdir (dir)) != NULL) {
      std::string path = dir_name;
      path.append(ent->d_name);
      if (path.find(".htm") != std::string::npos) {
        file_paths.push_back(path);
      }
    }
    closedir (dir);
  } else {
    /* could not open directory */
    printf("Failed to open dir %s", dir_name);
    return EXIT_FAILURE;
  }

  std::sort(file_paths.begin(), file_paths.end());
  std::vector<DOMTree> tree_history;
  std::shared_ptr<Node<size_t, DOMNode *, B_ELEMS>> persistent_root;
  DOMTree empty_tree = DOMTree(NULL, persistent_root);
  tree_history.push_back(empty_tree);
  for (size_t j = 0; j < 20; j++) {
    for (size_t i = 0; i < file_paths.size(); i++) {
      const char *filename = file_paths[i].c_str();
      std::cout << "loading: " << filename << std::endl;
      // open file
      FILE* fp = fopen(filename, "r");
      if (!fp) {
        printf("File %s not found!\n", filename);
        exit(EXIT_FAILURE);
      }
      char* input;
      int input_length;
      read_file(fp, &input, &input_length);
      // parse
      GumboOutput* output = gumbo_parse_with_options(
          &kGumboDefaultOptions, input, input_length);

      // convert to DOMTree
      DOMNode *dom_root = load_dom(output->root);
      tree_history.push_back(merge_dom_with_persistent_tree(dom_root, tree_history.back()));

      // free
      gumbo_destroy_output(&kGumboDefaultOptions, output);
      free(input);
    }
  }

  log_estimated_memory_usage(tree_history);

  interactive_query(tree_history);

  return 0;
}
