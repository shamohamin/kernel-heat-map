#pragma once 

#include <iostream>
#include <regex>
#include <string>

class ParsedLine {
   public:
    std::string name{};
    int depth{};
    double time{};
    bool isEntry{};

    ParsedLine(std::string name, int depth, double time) : name{name}, depth{depth}, time{time} {}
};

std::regex re{"^\\s*(\\S+)\\s*\\[(\\d+)\\]\\s*(\\d+\\.\\d+):\\s*(\\w+):\\s*(.*)\\s*\\|\\s*(.*)\\s*$"};
std::regex number{"(\\d+\\.\\d+)"};
std::regex pidre{"\\d+"};

ParsedLine *parseLine(std::string& line, int pid) {
    std::smatch matches;
    ParsedLine *pl = new ParsedLine("", 0, 0);
    int procPid{};

    if (std::regex_search(line, matches, re)) {
        std::smatch number_matches;
        double time{};
        
        std::string time_string{matches[5]};
        if (std::regex_search(time_string, number_matches, number)) {
            time = std::stod(number_matches[0]);
        }

        std::string name{matches[6]};
        std::string funcg{matches[4]};
        pl->time = time;
        pl->name = name;
        pl->isEntry = funcg == "funcgraph_entry";
        
        std::string proc{matches[1]};
        if (std::regex_search(proc, number_matches, pidre)) {
            procPid = std::stoi(number_matches[0]);
        }

        int idxOfPipe = line.find("|");
        for (int i = idxOfPipe+1; i < line.length(); ++i) {
            if (line[i] == ' ') {
                pl->depth++;
            }
            else { 
                break;
            }
        }
        pl->depth = pl->depth / 2;
    }
    return procPid == pid ? pl : nullptr;
}

