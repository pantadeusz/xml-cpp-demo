
/*
TYPES:

template <class T> class tree_elem_t {
public:
  T value;
  std::list<tree_elem_t> children;
};

struct tag_t {
  std::map<std::string, std::string> attr;
  std::string tag;
};
using text_t = std::string;
using element_t = std::variant<text_t, tag_t>; // element variant

FUNCTIONS:

inline tree_elem_t<element_t> text_to_xml(const std::string &xml_text);
inline tree_elem_t<element_t> text_to_xml_with_entities(const std::string
&xml_text);

*/

#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <regex>
#include <streambuf>
#include <string>
#include <variant>
#include <vector>

namespace tp {

template <class T> class tree_elem_t {
public:
  T value;
  std::list<tree_elem_t> children;
};

/**
 * t - tree root
 * f - callback on element
 * d - depth in tree
 * */
auto walk_tree = [](auto &t, auto f, int d = 0) -> void {
  f(t.value, d);
  for (const auto &e : t.children) {
    walk_tree(e, f, d + 1);
  }
};
auto walk_tree_io = [](auto &t, auto f_pre, auto f_post, int d = 0) -> void {
  f_pre(t.value, d);
  for (const auto &e : t.children) {
    walk_tree_io(e, f_pre, f_post, d + 1);
  }
  f_post(t.value, d);
};

/**
 * transform one type to another inside tree
 *
 * t - tree root
 * f - callback on element
 * d - depth in tree
 *
 * for example convert from elements of type string into elements of type
 * element_t
 *
 * auto e2 = transform_tree<std::string, element_t>(elements, [](auto &a, auto
 * d) { return str_to_element(a); });
 * */
template <class T, class D>
inline tree_elem_t<D> transform_tree(const tree_elem_t<T> &t,
                                     std::function<D(const T &, int)> f,
                                     int d = 0) {
  tree_elem_t<D> ret = {f(t.value, d), {}};
  for (const tree_elem_t<T> &e : t.children) {
    ret.children.push_back(transform_tree(e, f, d + 1));
  }
  return ret;
}

auto print_tree = [](auto elements) {
  walk_tree(elements, [](auto &a, auto d) {
    for (int i = 0; i < d; i++)
      std::cout << ".\t";
    std::cout << a << std::endl;
  });
};

namespace xml {

struct tag_t {
  std::map<std::string, std::string> attr;
  std::string tag;
};
using text_t = std::string;
using element_t = std::variant<text_t, tag_t>; // element variant

inline std::ostream &operator<<(std::ostream &o, const element_t &e) {
  //    o << "<(" << e.type << ")" << e.tag << ">" << e.value;
  if (e.index() == 0) {
    o << "*" << std::get<0>(e) << "*";
  } else {
    o << "<" << std::get<1>(e).tag << " ";
    for (auto &[k, v] : std::get<1>(e).attr) {
      o << " `" << k << "`===`" << v << "`";
    }
    o << ">";
  }
  return o;
}

namespace helpers {
/**
 * @brief parses xml string into tree of strings - elements in < and >, and
 * other parts
 *
 * TODO: too big recurrence in regex parsing
 */
auto simple_parse_xml = [](const std::string xmltxt0, auto on_fragment) {
  static const std::string regex_string = R"([<][!][-][-].*[-][-][>])";
  static const std::regex r(regex_string);
  std::string xmltxt =   std::regex_replace(xmltxt0, r, "");
  char in_string = 0;
  char escape = 0;
  std::string fragment = "";
  for (auto &c : xmltxt) {
    if (c == '<') {
      if (fragment.size() > 0)
        on_fragment(fragment);
      fragment = c;
    } else if (fragment[0] == '<') {
      if (in_string) {
        if (escape == '\\') {
          switch (c) {
          case '\\':
            fragment += c;
            break;
          case 'n':
            fragment += '\n';
            break;
          case 'r':
            fragment += '\r';
            break;
          case 't':
            fragment += '\t';
            break;
          case 'b':
            fragment += '\b';
            break;
          default:
            fragment += c;
          }
          escape = 0;
        } else {
          if (c != '\\') {
            fragment += c;
            if (c == in_string)
              in_string = 0;
          } else
            fragment += c;
            escape = c;
        }
      } else if (c == '>') {
        fragment += c;
        if (fragment.size() > 0)
          on_fragment(fragment);
        fragment = "";
      } else if ((c == '\"') || (c == '\'')) {
        in_string = c;
        fragment += c;
      } else {
        fragment += c;
      }
    } else {
      fragment += c;
    }
  }
  if (fragment.size() > 0)
    on_fragment(fragment);
};
/**
 * for given xml string generates the tree (it does not interpret xml)
 * */
auto string_to_tree = [](auto xml_string) {
  tree_elem_t<std::string> elements;
  std::map<tree_elem_t<std::string> *, tree_elem_t<std::string> *> parents = {
      {&elements, &elements}};
  tree_elem_t<std::string> *current_element = &elements;
  simple_parse_xml(xml_string, [&elements, &current_element,
                                &parents](std::string s) {
    if (s.size()) {
      std::cout << "EEE::" << s << std::endl;
      if ((s.size() > 2) && (s[0] == '<') && (s.back() == '>') && (s[1] == '/'))
        current_element = parents.at(current_element);
      else {
        current_element->children.push_back({s, {}});
        parents[&(current_element->children.back())] = current_element;
        if ((s[0] == '<') && (s.back() == '>') && (s[s.size() - 2] != '/') &&
            (s[1] != '!'))
          current_element = &(current_element->children.back());
      }
    }
  });
  return elements;
};

auto entities_convert = [](const std::string &str) -> std::string {
  static std::map<std::string, std::string> entity = []() {
    std::map<std::string, std::string> entity;
    entity["&nbsp;"] = " ";
    entity["&lt;"] = "<";
    entity["&gt;"] = ">";
    entity["&amp;"] = "&";
    entity["&quot;"] = "\"";
    entity["&apos;"] = "'";
    entity["&cent;"] = "¢";
    entity["&pound;"] = "£";
    entity["&yen;"] = "¥";
    entity["&euro;"] = "€";
    entity["&copy;"] = "©";
    entity["&reg;"] = "®";
    entity["&#160;"] = " ";
    entity["&#60;"] = "<";
    entity["&#62;"] = ">";
    entity["&#38;"] = "&";
    entity["&#34;"] = "\"";
    entity["&#39;"] = "'";
    entity["&#162;"] = "¢";
    entity["&#163;"] = "£";
    entity["&#165;"] = "¥";
    entity["&#8364;"] = "€";
    entity["&#169;"] = "©";
    entity["&#174;"] = "®";
    return entity;
  }();
  std::stringstream ret;
  std::string stringfrag;
  for (auto e : str) {
    if ((stringfrag.size() == 0) && (e != '&')) {
      ret << e;
    } else {
      stringfrag += e;
      if (stringfrag[0] == '&') {
        if (e == ';') {
          ret << entity[stringfrag];
          stringfrag = "";
        }
      }
    }
  }
  return ret.str();
};

auto str_to_element = [](const std::string &txt) -> element_t {
  element_t ret;
  if ((txt.front() == '<') && (txt.back() == '>')) {
    if ((txt[1] == '!') || (txt[1] == '?')) {
      tag_t ret_tag;
      ret_tag.tag = txt;
      ret = ret_tag;
    } else {

      std::string name;
      std::string value;
      std::size_t p = 1;
      // go to next important fragment (skip spaces)
      auto skip_white_space = [&]() {
        while ((p < txt.size()) && ((txt[p] == ' ') || (txt[p] == '\t') ||
                                    (txt[p] == '\n') || (txt[p] == '\r')))
          p++;
      };
      // extract string into name
      auto _name = [&]() {
        skip_white_space();
        int a = p;
        while ((p < txt.size()) &&
               (((txt[p] >= '-') && (txt[p] <= ':')) || ((txt[p] == '!')) ||
                ((txt[p] >= '@') && (txt[p] <= 'z'))))
          p++;
        name = txt.substr(a, p - a);
      };
      // extract string into value. Interpret equal sign properly
      auto _value = [&]() {
        value = "";
        while ((p < txt.size()) && (txt[p] != '='))
          p++;
        skip_white_space();
        if (p >= txt.size()) {
          return;
        }
        p++;
        int a = p;
        // std::cout << "p " << p << " = " << txt[p] << std::endl;
        if ((p >= txt.size()) || ((txt[p] != '"') && (txt[p - 1] != '\''))) {
          value = "";
          return;
        }
        p++;
        value = "";
        while ((p < txt.size()) && (txt[p] != txt[a])) {
          value = value + ((txt[p] == '\\') ? txt[p + 1] : txt[p]);
          p = p + ((txt[p] == '\\') ? 2 : 1);
        }
        // std::cout << "VALUE (" << a << "-" << p << "): " << value <<
        // std::endl;
      };
      skip_white_space();
      _name();
      tag_t ret_tag;
      ret_tag.tag = name;
      while (p < txt.size()) {
        _name();
        _value();
        if (name.size() > 0) {
          ret_tag.attr[name] = value;
        }
        p++;
      }
      ret = ret_tag;
    }
  } else {
    ret = txt;
  }
  return ret;
};

} // namespace helpers

inline tree_elem_t<element_t> text_to_xml(const std::string &xml_text) {
  auto elements = helpers::string_to_tree(xml_text);
  return transform_tree<std::string, element_t>(
      elements, [](auto &a, auto d) { return helpers::str_to_element(a); });
}

inline tree_elem_t<element_t>
text_to_xml_with_entities(const std::string &xml_text) {
  auto elements = text_to_xml(xml_text);
  return transform_tree<element_t, element_t>(
      elements, [](const auto &a, int d) {
        element_t r = a;
        switch (r.index()) {
        case 0:
          r = helpers::entities_convert(std::get<0>(r));
          break;
        case 1:
          for (auto &[k, v] : std::get<1>(r).attr) {
            v = helpers::entities_convert(v);
          }
          break;
        }
        return r;
      });
}

} // namespace xml
} // namespace tp
