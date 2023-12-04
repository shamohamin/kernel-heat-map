#pragma once

#include <atomic>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <set>
#include "utils.h"
#include <mutex>

class Node : public JsonSerializable {
   public:
    std::string name{};
    int depth{};
    double time{};
    std::vector<Node *> children{};
    Node *parent{nullptr};
    std::atomic<bool> merged{};
    std::set<std::string> labels{};

    Node(std::string name, int depth, double time, Node *parent) : name{name}, depth{depth}, time{time}, parent{parent} {}

    void add_child(Node *child) {
        children.push_back(child);
    }

    void add_label(std::string label) {
        labels.insert(label);
    }

    void print() {
        std::cout << name << " " << depth << " " << time << std::endl;
    }

    virtual JsonSerializable::JSONType *getJsonData() {
        JsonSerializable::JSONType *data = new JSONType;

        std::string *labelSerialized = new std::string{"["};
        int index = 0;
        for (auto label: labels) {
            (*labelSerialized) += "\"" + label + "\"";
            if (index != labels.size() - 1) (*labelSerialized) += ",";
            index++;
        }
        (*labelSerialized) += "]";

        JSONData *d1 = new JSONData{STRING, (std::uintptr_t) & this->name};
        JSONData *d2 = new JSONData{DOUBLE, (std::uintptr_t) & this->time};
        JSONData *d3 = new JSONData{VECTOR, (std::uintptr_t) & this->children};
        JSONData *d4 = new JSONData{STRING_VEC, (std::uintptr_t) labelSerialized};
        (*data)["name"] = d1;
        (*data)["time"] = d2;
        (*data)["children"] = d3;
        (*data)["labels"] = d4;

        return data;
    }
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
        std::cout << temp->name << "[" << temp->time << "] ";
        std::cout << "labels -> [";
        for (auto &label: temp->labels) std::cout << label << ",";
        std::cout << "]" << std::endl;

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
    int a{};
    bool handlingDrops{};
    int lastDepth{};

#if DEBUG
    std::cout << "Generating tree " << numLines << std::endl;
#endif
    int i = 0;
    for (i = 0; i < numLines; ++i) {
        auto pl = parsedLines[i];

        if (pl == nullptr) continue;
        if (pl->name == "DROPPED") {
            handlingDrops = true;
            continue;
        }
        if (handlingDrops) {
            if (pl->depth == lastDepth) {
                handlingDrops = false;
            } else {
                continue;
            }
        }
        Node *me{nullptr};
#if DEBUG
        std::cout << i + 1 << pl->name << std::endl;
#endif
        lastDepth = pl->depth;
        if (pl->isEntry) {
            if (stack.empty()) continue;  // EVENTS DROPPED

            auto supposedParent = stack.back();

            if (supposedParent == nullptr) continue;

            Node *me{nullptr};

            for (int i = 0; supposedParent->children.size(); i++) {
                Node *child = supposedParent->children[i];
                if (child == nullptr) 
                    continue;
                if (child->name.compare(pl->name) == 0) {
                    me = supposedParent->children[i];
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
            Node *node = stack.back();
            stack.pop_back();
            node->time += pl->time;
            if (node->name == "root") {
                a += 1;
            }
        }
    }

    return root;
}

Node *generateTreeNoDrops(std::vector<ParsedLine *> &parsedLines) {
    Node *root = new Node("root", 0, 0.0, nullptr);
    std::vector<Node *> stack;
    stack.push_back(root);

    for (int i = 0; i < parsedLines.size(); ++i) {
        auto pl = parsedLines[i];

        if (pl == nullptr) continue;
        if (pl->name == "DROPPED") {
            std::cout << "PANIC" << std::endl;
        }

        if (pl->isEntry) {
            auto supposedParent = stack.back();
            if (supposedParent == nullptr) continue;
            Node *me{nullptr};
#if DEBUG
            std::cout << i << " " << parsedLines[i] << supposedParent->name << "\n";
#endif
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
            Node *node = stack.back();
            stack.pop_back();
            node->time += pl->time;
        }
    }
    return root;
}

void extractFunctionNames(Node *rootNode, std::map<std::string, bool>& functionCache) {
    if (rootNode->name != "root") {
        functionCache[rootNode->name] = true;
    }

    for(int i = 0; i < rootNode->children.size(); i++) {
        extractFunctionNames(rootNode->children[i], functionCache);
    }
}


void labelFunctions(Node *rootNode, std::map<std::string, bool>& functionName) {
    bool shouldSearch = functionName.find(rootNode->name) == functionName.end();

    if (rootNode->name != "root") {
        std::set<std::string> labels;
        if (shouldSearch) {
            labels = queryCScope(rootNode->name);
            for(auto label: labels) rootNode->add_label(label);
            functionName[rootNode->name] = true;
        }        
    }

    if (shouldSearch) {
        for (auto &child: rootNode->children) {
            labelFunctions(child, functionName);
        }
    }
}


void assignLabels(Node *root, std::map<std::string, std::set<std::string> >& nameToLabels) {
    if(root->name != "root") {
        root->labels = nameToLabels[root->name];
    }

    for(int i = 0; i < root->children.size(); i++) {
        assignLabels(root->children[i], nameToLabels);
    }
}

void queryTreeForLabel(Node *root, const std::string &label, double &time) {
    if (root->name != "root") {
        if (root->labels.find(label) != root->labels.end()) {
            time += root->time;
        }
    }

    for(int i = 0; i < root->children.size(); i++) {
        queryTreeForLabel(root->children[i], label, time);
    }
}