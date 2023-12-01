#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector> 
#include <omp.h>
#include "utils.h"
#include "tree.h"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " <input_file> <pid>" << std::endl;
        return 1;
    }

    std::cout << "Hello, World!" << std::endl;
    int pid = std::stoi(argv[2]);

    std::ifstream file(argv[1]);
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


    for (int i = 100; i < 120; ++i) {
        parseLine(lines.at(i), 1);
    }

    ParsedLine ** parsedLines = (ParsedLine **) malloc(sizeof(ParsedLine *) * lines.size());

    omp_set_num_threads(140);
    #pragma omp parallel for 
    for (int i = 0; i < lines.size(); i++) {
        parsedLines[i] = parseLine(lines.at(i), pid);
    }


    // parsedLines.shrink_to_fit();

    for (int i = 0; i < 500; ++i) {
        if (parsedLines[i] != nullptr) {
            std::cout << parsedLines[i]->name << " " << parsedLines[i]->depth << std::endl;
        }
    }

    std::cout << "processed input, generating tree" << std::endl;
    auto root = generateTree(parsedLines, lines.size());
    std::cout << "done generating tree" << std::endl;

    return 0;
}