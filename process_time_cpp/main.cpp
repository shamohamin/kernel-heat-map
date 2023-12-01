#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector> 
#include <omp.h>
#include <map>
#include "utils.h"
#include "tree.h"


// #define DEBUG 

int main(int argc, char *argv[]) {
    // if (argc != 3) {
    //     std::cout << "Usage: " << argv[0] << " <input_file> <pid>" << std::endl;
    //     return 1;
    // }

    std::string filename = "../process_time/trace.report";
    std::string pidd = "1134987";

    // std::cout << "Hello, World!" << std::endl;
    int pid = 1134987;

    std::ifstream file(filename);
    std::vector<std::string> lines;
    if (file) {
        std::string line;

        // Read file line by line
        while (std::getline(file, line)) {
            lines.push_back(line);
        }
    } else {
        std::cerr << "Could not open file" << std::endl;
    }

    ParsedLine ** parsedLines = (ParsedLine **) malloc(sizeof(ParsedLine *) * lines.size());

    // omp_set_num_threads(140);
    #pragma omp parallel for 
    for (int i = 0; i < lines.size(); i++) {
        parsedLines[i] = parseLine(lines.at(i), pid);
    }


    // parsedLines.shrink_to_fit();
#if DEBUG
    for (int i = 0; i < 500; ++i) {
        if (parsedLines[i] != nullptr) {
            std::cout << parsedLines[i]->name << " " << parsedLines[i]->depth << std::endl;
        }
    }
    std::cout << "processed input, generating tree" << std::endl;
#endif

    
    Node * root = generateTree(parsedLines, lines.size());
#if DEBUG 
    std::cout << "done generating tree" << std::endl;
#endif

    std::fstream my_file;
	// my_file.open("my_file.json", std::ios::out);
	// if (!my_file) {
	// 	std::cout << "File not created!";
	// }
    
    vitualizeTree(root);

    // std::string tmp = JsonSerializable::seriliaze(root);
    // std::cout << tmp << "\n";
    return 0;
}