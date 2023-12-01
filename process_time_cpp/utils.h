#pragma once 

#include <iostream>
#include <regex>
#include <string>
#include <typeinfo>


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

class JsonSerializable {
public:
    enum TypeDef {
        STRING,
        DOUBLE,
        VECTOR
    };

    struct JSONData {
        TypeDef typedefinition;
        std::uintptr_t actualdata;

        JSONData(TypeDef tf, std::uintptr_t d): 
            typedefinition(tf), actualdata(d) {};
        
        JSONData(const JSONData &jd) {
            this->actualdata = jd.actualdata;
            this->typedefinition = jd.typedefinition;
        }
    };
    typedef std::map<std::string, JSONData *> JSONType; 

    virtual JSONType* getJsonData() = 0;

    // template<class T, class ... Args>
    static std::string seriliaze(JsonSerializable * root, bool lastChild = true) {
        
        JsonSerializable::JSONType* data = root->getJsonData();
        std::string baseString = "{";
        int index = 0;
        
        for (auto it = data->begin(); it != data->end(); it++) {
            std::string key = it->first;
            JSONData* val = it->second;
            baseString += "\"" + key + "\":";
            if ((*val).typedefinition == STRING) {
                std::string* d = (std::string *)(*val).actualdata;
                baseString += "\"" + std::string{*(d)} + "\"";
            } else if ((*val).typedefinition == DOUBLE) {
                double* d = (double *)(*val).actualdata;
                double tmp = *d;
                baseString += std::to_string(tmp);
            } else if ((*val).typedefinition == VECTOR) {
                baseString += "[";
                std::vector<JsonSerializable *>* v = (std::vector<JsonSerializable *>*)(*val).actualdata;

                for (int i = 0; i < v->size(); i++) {
                    baseString += seriliaze((*v)[i], i == v->size() - 1);
                }
                baseString += "]";
            }

            index += 1;
            if (index == data->size()) baseString += "}";
            else  baseString += ",";
        }

        if (!lastChild) baseString += ",";
        
        return baseString;
    }


};