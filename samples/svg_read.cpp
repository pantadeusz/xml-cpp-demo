#include "tp_tree_xml.hpp"
#include <fstream>
/*

    MoveTo: M, m
    LineTo: L, l, H, h, V, v
    Cubic Bézier Curve: C, c, S, s
    Quadratic Bézier Curve: Q, q, T, t
    Elliptical Arc Curve: A, a
    ClosePath: Z, z

*/
auto parse_path_to_cmnds = [](auto pth) {
  std::vector<std::pair<char, std::list<std::string>>> cmnds;
  std::vector<std::pair<char, std::list<double>>> cmnds_ret;
  
  for (auto c : pth) {
    if (((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z'))) {
      cmnds.push_back({c, {""}});
    } else if (c == ',') {
      cmnds.back().second.push_back("");
    } else {
      if (((c >= '0') && (c <= '9')) || (c == '.') || (c == '-')){
        if (cmnds.size() == 0) throw std::invalid_argument("first you must specify command in svg path!!");
        if (cmnds.back().second.size() == 0) throw std::invalid_argument("first you must specify command in svg path!!");
        cmnds.back().second.back().push_back(c);
      }
    }
  }
  for (auto &[k, v] : cmnds) {
    std::list<double> v2;
    std::transform(v.begin(), v.end(), std::back_inserter(v2),[](auto e) -> double { 
      if (e.size() == 0) return 0.0;
      return std::stof(e); 
    });
    cmnds_ret.push_back({k, v2});
  }
  return cmnds_ret;
};

int main(int argc, char **argv) {
  using namespace tp::xml;
  using namespace tp;
  std::string xml_text;
  if (argc > 1) {
    std::ifstream t(argv[1]);
    xml_text = std::string((std::istreambuf_iterator<char>(t)),
                           std::istreambuf_iterator<char>());
  } else {
    std::cout << "svg file is needed" << std::endl;
    return -1;
  }
  auto tree = text_to_xml_with_entities(xml_text);
  int prev_d = -1;
  walk_tree(tree, [&prev_d](auto &element, auto d) {
    if (element.index() == 1) {
      tag_t tag = std::get<1>(element);
      if (tag.tag == "path") {
        //        std::cout << tag.attr["d"] << std::endl;
        auto path_commands = parse_path_to_cmnds(tag.attr["d"]);
        for (auto &[c, n] : path_commands)
          std::cout << " " << c << ":" << n.size();
        std::cout << std::endl;
        
      }
    }
  });
  // print_tree(text_to_xml_with_entities(xml_text));

  return -0;
}
