#include "b_tree.h"
#include "gumbo.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

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

static const char* find_title(const GumboNode* root) {
  assert(root->type == GUMBO_NODE_ELEMENT);
  assert(root->v.element.children.length >= 2);

  const GumboVector* root_children = &root->v.element.children;
  // GumboNode* head = NULL;
  for (size_t i = 0; i < root_children->length; ++i) {
    GumboNode* child = (GumboNode*)root_children->data[i];
    std::cout << gumbo_normalized_tagname(child->v.element.tag) << ", # children: " << child->v.element.children.length << std::endl;
    if (child->type == GUMBO_NODE_ELEMENT &&
        child->v.element.tag == GUMBO_TAG_BODY) {
        const GumboVector* body_children = &child->v.element.children;
        for (size_t j = 0; j < body_children->length; j++) {
          GumboNode* bodyChild = (GumboNode*)body_children->data[j];
          // std::cout << bodyChild << std::endl;
          std::cout << gumbo_normalized_tagname(bodyChild->v.element.tag);
          GumboNode* c = (GumboNode*)bodyChild->v.element.children.data[0];
          std::cout << " " << (c->type == GUMBO_NODE_TEXT) << " " << c->v.text.text << std::endl;
        }
    }
    // if (child->type == GUMBO_NODE_ELEMENT &&
    //     child->v.element.tag == GUMBO_TAG_HEAD) {
    //   head = child;
    //   break;
    // }
  }
  
  return "<no title found>";
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

  // std::cout << find_title(output->root) << std::endl;
  gumbo_destroy_output(&kGumboDefaultOptions, output);
  free(input);
  return 0;
}
