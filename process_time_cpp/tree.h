#pragma once

#include <atomic>
#include <iostream>
#include <string>
#include <vector>

#include "utils.h"

class Node {
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
};

// Single threaded
// Produces finalized merged tree
Node *generateTree(ParsedLine **parsedLines, int numLines) {
    Node *root = new Node("root", 0, 0.0, nullptr);
    std::vector<Node *> stack;
    stack.push_back(root);

    std::cout << "Generating tree " << numLines << std::endl;
    for (int i = 0; i < numLines; ++i) {
        if (parsedLines[i] == nullptr) continue;
        auto pl = parsedLines[i];
        if (pl->isEntry) {
            auto supposedParent = stack.back();
            Node *me{nullptr};

            for (auto &child : supposedParent->children) {
                if (child->name == pl->name) {
                    me = child;
                    break;
                }
            }

            Node *node;
            if (me != nullptr) {
                node = me;
                supposedParent->add_child(node);
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