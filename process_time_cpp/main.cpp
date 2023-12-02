#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector> 
#include <omp.h>
#include <map>
#include "utils.h"
#include "tree.h"
#include <chrono>
#include <unordered_map>


// #define DEBUG 

int main(int argc, char *argv[]) {
    // if (argc != 3) {
    //     std::cout << "Usage: " << argv[0] << " <input_file> <pid>" << std::endl;
    //     return 1;
    // }

    std::string filename = "../process_time/trace.report";
    std::string pidd = "2659";

    // std::cout << "Hello, World!" << std::endl;
    int pid = 2659;

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

    auto start = std::chrono::high_resolution_clock::now();
    omp_set_num_threads(140);
    #pragma omp parallel for 
    for (int i = 0; i < lines.size(); i++) {
        parsedLines[i] = parseLineNoRegex(lines.at(i), pid);
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Time taken to parse lines: " << elapsed.count() << " s\n";

    std::cout << "Separating lines by CPU\n";

    std::unordered_map<int, std::vector<ParsedLine*>> parsedLinesPerCpu;
    for (int i = 0; i < lines.size(); i++) {
        if (parsedLines[i] == nullptr) {
            continue;
        }
        if (parsedLinesPerCpu.find(parsedLines[i]->cpu) == parsedLinesPerCpu.end()) {
            parsedLinesPerCpu[parsedLines[i]->cpu] = std::vector<ParsedLine*>();
        }
        parsedLinesPerCpu[parsedLines[i]->cpu].push_back(parsedLines[i]);
    }

    Node **roots = (Node **) malloc(sizeof(Node *) * parsedLinesPerCpu.size());
    std::cout << "Generating trees\n";
    # pragma omp parallel for
    for (int i = 0; i < parsedLinesPerCpu.size(); i++) {
        roots[i] = generateTree(parsedLinesPerCpu[i].data(), parsedLinesPerCpu[i].size());
    }
    
    // std::fstream my_file;
	// // my_file.open("my_file.json", std::ios::out);
	// // if (!my_file) {
	// // 	std::cout << "File not created!";
	// // }
    
    // // vitualizeTree(root);

    // std::string tmp = JsonSerializable::seriliaze(root);
    // std::cout << tmp << "\n";
    return 0;
}