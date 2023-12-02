#pragma once

#include <atomic>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "utils.h"

class Node : public JsonSerializable {
   public:
    std::string name{};
    int depth{};
    double time{};
    std::vector<Node *> children{};
    Node *parent{nullptr};
    std::atomic<bool> merged{};

    Node(std::string name, int depth, double time, Node *parent) : name{name}, depth{depth}, time{time}, parent{parent} {}

    void add_child(Node *child) {
        children.push_back(child);
    }

    void print() {
        std::cout << name << " " << depth << " " << time << std::endl;
    }

    virtual JsonSerializable::JSONType *getJsonData() {
        JsonSerializable::JSONType *data = new JSONType;
        JSONData *d1 = new JSONData{STRING, (std::uintptr_t) & this->name};
        JSONData *d2 = new JSONData{DOUBLE, (std::uintptr_t) & this->time};
        JSONData *d3 = new JSONData{VECTOR, (std::uintptr_t) & this->children};
        (*data)["name"] = d1;
        (*data)["time"] = d2;
        (*data)["children"] = d3;

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
            if (stack.empty()) {  // EVENTS DROPPED
                // while (parsedLines[i] != nullptr && parsedLines[i]->depth != 2) {
                //     i++;
                // }
                // i -= 1;
                // stack.push_back(root);
                // continue;
            }

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