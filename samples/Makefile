all: print_xml_tree svg_read

print_xml_tree: print_xml_tree.cpp
	g++ -std=c++17 -I../ print_xml_tree.cpp -o print_xml_tree
svg_read: distance/distance_t.cpp svg_read.cpp
	g++ -std=c++17 -I../ -Idistance distance/distance_t.cpp svg_read.cpp -o svg_read

clean:
	rm -f print_xml_tree 
	rm -f svg_read
