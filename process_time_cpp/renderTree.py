import anytree
import json
import sys

class Node(anytree.NodeMixin): 
    def  __init__(self, name, time, parent=None, children=None, labels=[]):
        super(Node, self).__init__()
        self.name = name
        self.time = time
        self.labels = labels
        self.parent = parent
        if children:
            self.children = children
            
    def __str__(self): 
        return f"{self.name} ({self.time})"


def loadJson(filename="out.json"):
    with open(filename, "r") as file:
        data = json.load(file)
        return data
    

def traverseTree(data, parent):
    # if 
    n = Node(data["name"], data["time"], parent, labels=data["labels"])

    for child in data["children"]:
        traverseTree(child, n)



data = loadJson()
# 
# print(data.keys())
roots = []
for cpu in data.keys():
    node = Node(f"{cpu}", 0.0, None)
    traverseTree(data[cpu], node)
    roots.append(node)

for i in range(len(roots)):
    root = roots[i]
    cpu = list(data.keys())[i]
    with open(f"{cpu}.txt", "w") as file:
        for pre, _, node in anytree.RenderTree(root):
            treestr = u"%s%s [%s] [%s]" % (pre, node.name, node.time, ",".join(node.labels))        
            file.write(treestr.ljust(8) + "\n")
                # sys.stdout = file
            # sys.stdout.wr/ite()
            # sys.stdout.??
            # print()

# print(anytree.RenderTree(node))