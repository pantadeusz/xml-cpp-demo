#include "tp_tree_xml.hpp"
#include <fstream>

int main(int argc, char **argv) {
  using namespace tp::xml;
  using namespace tp;
  std::string xml_text =
      "<x id=\"ja'nek\" class=\"w\\\"ar>\"> <!DOCTYPE> "
      "sadf>sad </x>oraz<p>element "
      "p</p> no <bla/>i <h>do <p>non br&lt;ea&gt;kable space "
      "example:&nbsp;was here</p> oraz <x>dsfs</x></h>";
  if (argc > 1) {
    std::ifstream t(argv[1]);
    xml_text = std::string((std::istreambuf_iterator<char>(t)),
                           std::istreambuf_iterator<char>());
  }
  std::cout << xml_text << std::endl;
  std::cout << "-------------- A ----------" << std::endl;
  print_tree(tp::xml::helpers::string_to_tree(xml_text));
  std::cout << "-------------- B ----------" << std::endl;
  print_tree(text_to_xml(xml_text));
  std::cout << "-------------- C ----------" << std::endl;
  print_tree(text_to_xml_with_entities(xml_text));

  return -0;
}
