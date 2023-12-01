#pragma once

#include <atomic>
#include <iostream>
#include <string>
#include <vector>
#include <iomanip>

#include <map>

#include "utils.h"

class Node: public JsonSerializable {
   public:
    std::string name{};
    int depth{};
    double time{};
    std::vector<Node *> children{};
    Node *parent{nullptr};
    std::atomic<bool> merged{};

    Node(std::string name, int depth, double time, Node *parent): name{name}, depth{depth}, time{time}, parent{parent} {}

    void add_child(Node *child) {
        children.push_back(child);
    }

    void print() {
        std::cout << name << " " << depth << " " << time << std::endl;
    }

    virtual JsonSerializable::JSONType* getJsonData() { 
        JsonSerializable::JSONType *data = new JSONType;
        JSONData *d1 = new JSONData{STRING, (std::uintptr_t)&this->name};
        JSONData *d2 = new JSONData{DOUBLE, (std::uintptr_t)&this->time};
        JSONData *d3 = new JSONData{VECTOR, (std::uintptr_t)&this->children};
        (*data)["name"] = d1;
        (*data)["time"] = d2;
        (*data)["children"] = d3;

        return data;
    }

    // void getJsonData1(std::fstream& file, bool lastChild = false) {
    //     // visited[this->name] = true;
    //     // std::string baseString = "{\"name\": \"" + name + "\",\n";
    //     // baseString += "\"time\": " + std::to_string(time) + ",";
    //     file << "{\"name\": \"" + name + "\",\n";
    //     file << "\"time\": " + std::to_string(time) + ",";
    //     if (children.size() != 0) {
    //         // baseString += "\"children\": [ ";
    //         file << "\"children\": [ ";
    //         for (int i = 0 ; i < children.size(); i++) {
    //             // auto it = visited.find(children[i]->name);
    //             // if (it == visited.end()) 
    //                 children[i]->getJsonData1(file, i == (children.size() - 1));
    //         }

    //         //  baseString += "]\n";
    //         file << "]\n";
    //     }else {
    //         // baseString += "children: []\n";
    //         file << "\"children\": []\n";
    //     }

    //     // if (depth == 0)
    //         // file << "}";
    //     // else
    //     if (!lastChild)
    //         file << "},";
    //     else
    //         file << "}";
    //     // baseString += "}";
    //     // visited.erase(this->name);
    //     // return baseString;
    // }

};


void vitualizeTree(Node *curr) {
    std::stack<Node *> parentHolder;
    parentHolder.push(curr);

    int spaces = 0;
    while (!parentHolder.empty()) {
        Node *temp = parentHolder.top();
        parentHolder.pop();

        for (int i = 0; i < temp->depth; ++i)
            std::cout << " ";
        std::cout << temp->name << "[" << temp->time << "]" << std::endl;

        for (int i = 0; i < temp->children.size(); ++i) {
            parentHolder.push(temp->children[i]);
        }

    }
}

// Single threaded
// Produces finalized merged tree
Node *generateTree(ParsedLine **parsedLines, int numLines) {
    Node *root = new Node("root", 0, 0.0, nullptr);
    std::vector<Node *> stack;
    stack.push_back(root);

#if DEBUG
    std::cout << "Generating tree " << numLines << std::endl;
#endif

    for (int i = 0; i < numLines; ++i) {
        if (parsedLines[i] == nullptr) continue;
        Node *me{nullptr};
#if DEBUG
        std::cout << i + 1 << parsedLines[i]->name << std::endl;
#endif
        auto pl = parsedLines[i];
        if (pl->isEntry) {
            if (stack.empty()) continue; // EVENTS DROPPED 
            
            auto supposedParent = stack.back();
            if (supposedParent == nullptr) continue;

            Node *me{nullptr};

            for (auto &child : supposedParent->children) {
                if (child->name.compare(pl->name) == 0) {
                    me = child;
                    break;
                }
            }

            Node *node;
            if (me != nullptr) {
                node = me;
                // supposedParent->add_child(node);
            } else { 
                node = new Node(pl->name, pl->depth, pl->time, supposedParent);
                supposedParent->add_child(node);
            }

            if (pl->time == 0.0) {
                // non-leaf node
                stack.push_back(node);  
            } else {
                // leaf node
                node->time += pl->time;
            }
        } else {
            if (stack.empty()) continue; // EVENTS DROPPED

            Node *node = stack.back();
            stack.pop_back();
            node->time += pl->time;
        }
    }

    return root;
}