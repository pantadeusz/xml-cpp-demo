#include <distance_t.hpp>
#include <tp_tree_xml.hpp>

#include <fstream>

using point_2d_t = raspigcd::generic_position_t<double, 2>;
enum step_type_e { GOTO, PLOT, MARK };
using plot_step_callback_t = std::function<void(step_type_e, point_2d_t)>;
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
  std::vector<std::pair<char, std::vector<double>>> cmnds_ret;

  for (auto c : pth) {
    if (((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')) ||
        (cmnds.size() == 0)) {
      if (cmnds.size() > 0) {
        if ((cmnds.back().second.size() > 0) &&
            (cmnds.back().second.back().size() == 0))
          cmnds.back().second.pop_back();
      }
      cmnds.push_back({c, {""}});
    } else {
      if (((c >= '0') && (c <= '9')) || (c == '.') ||
          ((c == '-') && (cmnds.back().second.back().size() == 0))) {
        if (cmnds.size() == 0)
          throw std::invalid_argument(
              "first you must specify command in svg path!!");
        if (cmnds.back().second.size() == 0)
          throw std::invalid_argument(
              "first you must specify command in svg path!!");
        cmnds.back().second.back().push_back(c);
      } else {
        if (cmnds.back().second.back().size() > 0)
          cmnds.back().second.push_back((c == '-') ? "-" : "");
      }
    }
  }
  if (cmnds.size() > 0) {
    if ((cmnds.back().second.size() > 0) &&
        (cmnds.back().second.back().size() == 0))
      cmnds.back().second.pop_back();
  }
  for (auto &[k, v] : cmnds) {
    std::vector<double> v2;
    std::transform(v.begin(), v.end(), std::back_inserter(v2),
                   [](auto e) -> double {
                     if (e.size() == 0)
                       return 0.0;
                     return std::stof(e);
                   });
    cmnds_ret.push_back({k, v2});
  }
  return cmnds_ret;
};

auto path_noop = [](auto p, auto on_plot_step) { return p; };

auto path_move_to = [](auto t, auto current_point, point_2d_t new_point,
                       auto on_plot_step) {
  on_plot_step(t, new_point);
  return new_point;
};

auto path_bezier_cubic = [](auto movetype, auto current_point,
                            std::vector<double> args, auto on_plot_step,
                            double dt) {
  std::vector<point_2d_t> pts = {current_point,
                                 {args[0], args[1]},
                                 {args[2], args[3]},
                                 {args[4], args[5]}};

  for (double t = dt; t <= 1.0; t += dt) {
    current_point = raspigcd::bezier(pts, t);
    on_plot_step(movetype, current_point);
  }
  if (!(pts.back() == current_point))
    on_plot_step(movetype, pts.back());
  return pts.back();
};

point_2d_t
interpret_svg_path_command(point_2d_t current_point,
                           std::pair<char, std::vector<double>> command,
                           plot_step_callback_t on_plot_step, double dt,
                           point_2d_t *current_shape_start_point) {

  auto &[c, args] = command;

  std::cerr << c;
  for (auto v : args) {
    std::cerr << " " << v;
  }
  std::cerr << std::endl;

  if ((c == 'm') || (c == 'l') || (c == 'c') || (c == 's')) {
    int i = 1;
    for (auto &e : args)
      e = e + current_point[++i % 2];
  } else if (c == 'h') {
    for (auto &e : args)
      e = e + current_point[0];
  } else if (c == 'v') {
    for (auto &e : args)
      e = e + current_point[1];
  }
  if ((c >= 'a') && (c <= 'z'))
    c = c - 'a' + 'A';

  switch (c) {
  case 'M': {
    step_type_e tpy = GOTO;
    while (args.size() >= 2) {
      current_point = path_move_to(tpy, current_point, {args.at(0), args.at(1)},
                                   on_plot_step);
      args = std::vector<double>(args.begin() + 2, args.end());
      if (tpy == GOTO) {
        tpy = PLOT;
        *current_shape_start_point = current_point;
      }
    }
    return current_point;
  }
  case 'L': {
    while (args.size() >= 2) {
      current_point = path_move_to(PLOT, current_point,
                                   {args.at(0), args.at(1)}, on_plot_step);
      args = std::vector<double>(args.begin() + 2, args.end());
    }
    return current_point;
  }
  case 'C': {
    while (args.size() >= 6) {
      current_point =
          path_bezier_cubic(PLOT, current_point, args, on_plot_step, dt);
      args = std::vector<double>(args.begin() + 6, args.end());
    }
    return current_point;
  }
  case 'S': {
    while (args.size() >= 4) {
      current_point = path_move_to(PLOT, current_point,
                                   {args.at(2), args.at(3)}, on_plot_step);
      args = std::vector<double>(args.begin() + 4, args.end());
    }
    return current_point;
  }
  case 'V': {
    while (args.size() >= 1) {
      current_point = path_move_to(
          PLOT, current_point, {current_point[0], args.at(0)}, on_plot_step);
      args = std::vector<double>(args.begin() + 1, args.end());
    }
    return current_point;
  }
  case 'H': {
    while (args.size() >= 1) {
      current_point = path_move_to(
          PLOT, current_point, {args.at(0), current_point[1]}, on_plot_step);
      args = std::vector<double>(args.begin() + 1, args.end());
    }
    return current_point;
  }
  case 'z':
  case 'Z':
    return path_move_to(PLOT, current_point, *current_shape_start_point,
                        on_plot_step);
  default: {
    std::cerr << c;
    for (auto v : args) {
      std::cerr << " " << v;
    }
    std::cerr << std::endl;
    return path_noop(current_point, on_plot_step);
  }
  }
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

  double work_depth = -0.1;
  double fly_high = 10.0;

  auto tree = text_to_xml_with_entities(xml_text);
  std::map<int, std::pair<point_2d_t, point_2d_t>> shift_and_scale;
  walk_tree(tree, [&](auto &element, auto d) {
    if (shift_and_scale.size() == 0)
      shift_and_scale[d] = {{0.0, 0.0}, {1.0, 1.0}};
    else
      shift_and_scale[d] = shift_and_scale[d - 1];
    if (element.index() == 1) {
      tag_t tag = std::get<1>(element);
      if (tag.tag == "path") {
        //        std::cout << tag.attr["d"] << std::endl;
        auto path_commands = parse_path_to_cmnds(tag.attr["d"]);
        point_2d_t current_point = {};
        point_2d_t current_shape_start_point = {};
        raspigcd::distance_t current_point_3d = {};
        for (auto &c : path_commands) {
          current_point = interpret_svg_path_command(
              current_point, c,
              [&](step_type_e sttp, point_2d_t p) {
                if (true)
                  switch (sttp) {
                  case GOTO:
                    if (!(current_point == p)) {
                      std::cout << "G0Z" << fly_high << std::endl;
                      std::cout << "G0"
                                << "X" << p[0] << "Y" << -p[1] << std::endl;
                      std::cout << "G0"
                                << "Z" << 0.0 << std::endl;
                      current_point_3d[2] = 0.0;
                    }
                    break;
                  case PLOT:
                    if (!(current_point == p)) {
                      if (current_point_3d[2] > work_depth) {
                        std::cout << "G1"
                                  << "Z" << work_depth << std::endl;
                        current_point_3d[2] = work_depth;
                      }
                      std::cout << "G1"
                                << "X" << p[0] << "Y" << -p[1] << std::endl;
                    }
                    break;
                  }
                current_point = p;
                current_point_3d[0] = current_point[0];
                current_point_3d[1] = current_point[1];
              },
              0.05, &current_shape_start_point);
        }
        std::cout << std::endl;
      }
    }
  });
  // print_tree(text_to_xml_with_entities(xml_text));

  return -0;
}
