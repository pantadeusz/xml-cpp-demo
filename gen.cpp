
#include <fstream>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <regex>
#include <streambuf>
#include <string>
#include <vector>

template <class T>
class tree_elem_t
{
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
    for (const auto &e : t.children)
    {
        walk_tree(e, f, d + 1);
    }
};

/**
 * transform one type to another inside tree
 *
 * t - tree root
 * f - callback on element
 * d - depth in tree
 * */
template <class T, class D>
inline tree_elem_t<D> transform_tree(tree_elem_t<T> &t,
                                     std::function<D(T &, int)> f, int d = 0)
{
    tree_elem_t<D> ret = {f(t.value, d), {}};
    for (tree_elem_t<T> &e : t.children)
    {
        ret.children.push_back(transform_tree(e, f, d + 1));
    }
    return ret;
}

auto simple_parse_xml = [](std::string xmltxt, auto on_fragment) {
    std::string regex_string =
        R"(([<](([^"'>]+|(["][^"]*["])|(['][^']*[']))+)[>])|([^<]+))";
    std::regex r(regex_string);
    std::smatch sm;
    while (std::regex_search(xmltxt, sm, r))
    {
        for (auto x : sm)
        {
            on_fragment(x);
            break;
        }
        xmltxt = sm.suffix().str();
    }
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
        if (s.size())
        {
            if ((s.size() > 2) && (s[0] == '<') && (s.back() == '>') && (s[1] == '/'))
                current_element = parents.at(current_element);
            else
            {
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

auto print_tree = [](auto elements) {
    walk_tree(elements, [](auto &a, auto d) {
        for (int i = 0; i < d; i++)
            std::cout << "\t";
        std::cout << a << std::endl;
    });
};

struct element_t
{
    std::map<std::string, std::string> attr;
    std::string value;
    std::string type;
    std::string tag;
};

std::ostream &operator<<(std::ostream &o, const element_t &e)
{
    //    o << "<(" << e.type << ")" << e.tag << ">" << e.value;
    if (e.type == "text")
    {
        o << e.value;
    }
    else
    {
        o << "<" << e.tag << " ";
        for (auto &[k, v] : e.attr)
        {
            o << " " << k << "=" << v;
        }
        o << ">" << e.value;
    }
    return o;
}
element_t str_to_element(std::string &txt)
{
    element_t ret;
    if ((txt.front() == '<') && (txt.back() == '>'))
    {
        std::string name;
        std::string value;
        std::size_t p = 1;
        auto skip_white_space = [&]() {
            while ((p < txt.size()) && ((txt[p] == ' ') || (txt[p] == '\t') ||
                                        (txt[p] == '\n') || (txt[p] == '\r')))
                p++;
        };
        auto _name = [&]() {
            skip_white_space();
            int a = p;
            while ((p < txt.size()) && 
            (((txt[p] >= '-') && (txt[p] <= ':')) ||
             ((txt[p] == '!')) ||
             ((txt[p] >= '@') && (txt[p] <= 'z'))))
                p++;
            name = txt.substr(a, p - a);
            std::cout << "NAE (" << a << "-" << p << "):" << name << ":" << std::endl;
        };
        auto _value = [&]() {
            value = "";
            while ((p < txt.size()) && (txt[p] != '='))
                p++;
            skip_white_space();
            if (p >= txt.size())
            {
                return;
            }
            p++;
            int a = p;
            std::cout << "p " << p << " = " << txt[p] << std::endl;
            if ((p >= txt.size()) || ((txt[p] != '"') && (txt[p - 1] != '\'')))
            {
                value = "";
                return;
            }
            p++;
            value = "";
            while ((p < txt.size()) && (txt[p] != txt[a]))
            {
                value = value + ((txt[p] == '\\') ? txt[p + 1] : txt[p]);
                p = p + ((txt[p] == '\\') ? 2 : 1);
            }
            std::cout << "VALUE (" << a << "-" << p << "): " << value << std::endl;
        };
        skip_white_space();
        _name();
        ret.tag = name;
        ret.type = "tag";
        ret.value = "";
        while (p < txt.size())
        {
            _name();
            _value();
            if (name.size() > 0)
            {
                ret.attr[name] = value;
            }
            p++;
        }
    }
    else
    {
        ret.tag = "";
        ret.type = "text";
        ret.value = txt;
    }
    return ret;
}

int main(int argc, char **argv)
{
    std::string xml_text =
        "<x id=\"ja'nek\" class=\"w\\\"ar>\"> <!DOCTYPE> sadf>sad </x>oraz<p>element "
        "p</p> no <bla/>i <h>do <p> sfs</p> oraz <x>dsfs</x></h>";
    if (argc > 1)
    {
        std::ifstream t(argv[1]);
        xml_text = std::string((std::istreambuf_iterator<char>(t)),
                               std::istreambuf_iterator<char>());
    }
    auto elements = string_to_tree(xml_text);

    print_tree(elements);
    auto e2 = transform_tree<std::string, element_t>(
        elements, [](auto &a, auto d) { return str_to_element(a); });
    print_tree(e2);

    return -0;
}
