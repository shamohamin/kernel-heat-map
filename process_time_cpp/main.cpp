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
#include <mutex>


// #define DEBUG 

int main(int argc, char *argv[]) {
    // if (argc != 3) {
    //     std::cout << "Usage: " << argv[0] << " <input_file> <pid>" << std::endl;
    //     return 1;
    // }
    // std::cout << exec("cd /home/amin/workspace/kernel-heat-map && cscope -d -l -L -1 nf_conn") << "\n";
    // exit(1);
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
#if DEBUG_ERR
        std::cerr << "Could not open file" << std::endl;
#endif
    }

    ParsedLine ** parsedLines = (ParsedLine **) malloc(sizeof(ParsedLine *) * lines.size());

    auto start = std::chrono::high_resolution_clock::now();
    omp_set_num_threads(24);
    #pragma omp parallel for 
    for (int i = 0; i < lines.size(); i++) {
        parsedLines[i] = parseLineNoRegex(lines.at(i), pid);
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
#if DEBUG
    std::cout << "Time taken to parse lines: " << elapsed.count() << " s\n";

    std::cout << "Separating lines by CPU\n";
#endif
    std::unordered_map<int, std::vector<ParsedLine*>> parsedLinesPerCpu;
    std::vector<int> cpus;
    int i = 0;
    for (i = 0; i < lines.size(); i++) {
        if (parsedLines[i] == nullptr) {
            continue;
        }
        if (parsedLinesPerCpu.find(parsedLines[i]->cpu) == parsedLinesPerCpu.end()) {
            parsedLinesPerCpu[parsedLines[i]->cpu] = std::vector<ParsedLine*>();
            cpus.push_back(parsedLines[i]->cpu); 
            if (parsedLines[i]->depth != 2) { // check if the there are some inconsistent depthes
                while(parsedLines[i]->depth != 2 || !parsedLines[i]->isEntry) {
                    i++;
                }
            }
        }
        parsedLinesPerCpu[parsedLines[i]->cpu].push_back(parsedLines[i]);
    }

    Node **roots = (Node **) malloc(sizeof(Node *) * parsedLinesPerCpu.size());
    std::map<std::string, bool> functionNameCaches{};
    std::map<std::string, std::set<std::string> > nameToLabels{};
    std::map<std::string, double> labelCacheTime{};
    std::mutex cacheGaurd; 
#if DEBUG
    std::cout << "Generating trees\n";
#endif
    # pragma omp parallel for
    for (int i = 0; i < cpus.size(); i++) {
        auto cpu = cpus.at(i);
#if DEBUG
        std::cout << "processing cpu " << cpu << std::endl;
#endif 
        if (parsedLinesPerCpu.find(cpu) == parsedLinesPerCpu.end()) {
            continue;
        }
        if (parsedLinesPerCpu[cpu].size() == 0) {
            roots[i] = nullptr;
            continue;
        }
        roots[cpu] = generateTreeNoDrops(parsedLinesPerCpu[cpu]);
        std::map<std::string, bool> cpuFunctionNameCache{};

        
        if (roots[cpu] != nullptr) {
            extractFunctionNames(roots[cpu], cpuFunctionNameCache);
        //     std::map<std::string, bool> functionName{};
        //     labelFunctions(roots[cpu], functionName);
        }
        cacheGaurd.lock();
        for(auto &[key, val]: cpuFunctionNameCache) {
            functionNameCaches[std::string{key}] = true;
        }
        cacheGaurd.unlock();   
    }
    std::vector<std::string> names{};
    for(auto item: functionNameCaches) {
        names.push_back(std::string{item.first});
    }

    
    # pragma omp parallel for
    for(int i = 0; i < names.size(); i++) {
        std::set<std::string> labels = queryCScope(names.at(i));
        nameToLabels[names.at(i)] = labels; // names are different so thread-safe
        for (auto &label: labels) {
            labelCacheTime[label] = 0.0; // thread safe it only initializes no WORRIES then
        }
    }

    // for (int i = 0; i < cpus.size(); i++)
    //     assignLabels(roots[cpus.at(i)]);

    std::cout << "{";
    for (int i = 0; i < cpus.size(); i++) {
        auto cpu = cpus.at(i);
        auto root = roots[cpu];
        std::cout << "\"" << cpu  << "\"" << ": ";
        if (root == nullptr) {
            std::cout << "null";
        } else {
            assignLabels(root, nameToLabels);
            std::cout << JsonSerializable::seriliaze(root);
        }

        if (i != cpus.size() - 1)
            std::cout << ", ";
    }
    std::cout << "}\n";
    return 0;
}