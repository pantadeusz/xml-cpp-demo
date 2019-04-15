
#include <iostream>
#include <vector>
#include <map>
#include <list>
#include <map>
#include <functional>

template <class T>
class element_t
{
  public:
    T value;
    std::list<element_t> children;
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
inline element_t<D> transform_tree(element_t<T> &t, std::function<D(T &, int)> f, int d = 0)
{
    element_t<D> ret = {f(t.value, d), {}};
    for (element_t<T> &e : t.children)
    {
        ret.children.push_back(transform_tree(e, f, d + 1));
    }
    return ret;
}

auto simple_parse_xml = [](std::string xmltxt, auto on_fragment) {
    std::string fragment = "", current_f = "text";
    std::map<std::string, std::function<void(char c)>> walker;
    walker["text"] = [&](char c) -> void {
        if (c == '<')
        {
            on_fragment(fragment);
            fragment = "";
            current_f = "tag";
        }
        fragment.push_back(c);
    };
    walker["tag"] = [&](char c) -> void {
        fragment.push_back(c);
        if (c == '>')
        {
            on_fragment(fragment);
            fragment = "";
            current_f = "text";
        }
        else if (c == '"' || c == '\'')
            current_f = {c};
    };
    walker["\""] = walker["\'"] = [&](char c) -> void {
        fragment.push_back(c);
        if (c == current_f[0])
            current_f = "tag";
    };
    for (char c : xmltxt)
        walker.at(current_f)(c);
};

/**
 * for given xml string generates the tree (it does not interpret xml)
 * */
auto string_to_tree = [](auto xml_string) {
    element_t<std::string> elements;
    std::map<element_t<std::string> *, element_t<std::string> *> parents = {{&elements, &elements}};
    element_t<std::string> *current_element = &elements;
    simple_parse_xml(xml_string, [&elements, &current_element, &parents](auto s) {
        if (s.size())
        {
            if ((s.size() > 2) && (s[0] == '<') && (s.back() == '>') && (s[1] == '/'))
                current_element = parents.at(current_element);
            else
            {
                current_element->children.push_back({s, {}});
                parents[&(current_element->children.back())] = current_element;
                if ((s[0] == '<') && (s.back() == '>') && (s[s.size() - 2] != '/') && (s[1] != '!'))
                    current_element = &(current_element->children.back());
            }
        }
    });
    return elements;
};

auto print_tree = [](auto elements) {
    walk_tree(elements, [](auto &a, auto d) {
        for (int i = 0; i < d; i++) std::cout << "\t" ; std::cout << "{" << a << "}" << std::endl; });
};

int main()
{
    auto elements = string_to_tree("<x id=\"ja'nek\" class=\"war>\"> <!DOCTYPE> sadf>sad </x>oraz<p>element p</p> no <bla/>i <h>do <p> sfs</p> oraz <x>dsfs</x></h>");

    print_tree(elements);
    auto e2 = transform_tree<std::string, int>(elements, [](auto &a, auto d) {
        return a.size();
    });
    print_tree(e2);
    return -0;
}
