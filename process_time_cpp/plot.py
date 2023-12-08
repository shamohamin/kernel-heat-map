import matplotlib.pyplot as plt
import json

topN = 10
components = {}
with open('components.txt') as file:
    lines = file.readlines()

for line in lines:
    splitedComponent = line.split(":")
    name = splitedComponent[0]
    time = float(splitedComponent[-1])
    components[name] = time

components = dict(sorted(components.items(), key=lambda x: x[-1]))

def getTopN(topN, data: dict, ifOthers = False, ifMili = True):
    componentsOth = {}
    if ifOthers:
        componentsOth = {'others': 0.0}
    keys = list(data.keys())

    denominator = 1e3 if ifMili else 1.

    if ifOthers:
        for i in range(len(keys) - topN):
            componentsOth["others"] += data[keys[i]] / denominator
    
    for i in range(len(keys) - topN, len(keys)):
        componentsOth[keys[i]] = data[keys[i]] / denominator

    return  componentsOth

def qeuryTreeForLabel(data, label, output):
    name = data["name"] 

    if label in data["labels"]:
        output[name]  = output.get(name, 0.0) + (data["time"] / float(1e3))
    
    for child in data["children"]:
        qeuryTreeForLabel(child, label, output)

with open('tree.json', "r") as file:
    treeData = json.load(file)


output = {}
for cpu in treeData.keys():
    qeuryTreeForLabel(treeData[cpu], "ipv4", output)

output = dict(sorted(output.items(), key=lambda x: x[-1]))

componentsOth = getTopN(5, components, True)
functionsTopN = getTopN(2, output, ifMili=False)

plt.figure(figsize=(8, 7))
plt.bar(list(componentsOth.keys()), list(componentsOth.values()))
plt.title("Redis Time Distribution inside Kernel")
plt.xlabel("Subsytem")
plt.ylabel("Time(ms)")

plt.savefig('linux_components.png')

plt.figure(figsize=(12, 7))
plt.bar(list(functionsTopN.keys()), list(functionsTopN.values()))
plt.title("Redis Time Distribution inside fs component")
plt.xlabel("Function Name")
plt.ylabel("Time(us)")

#plt.grid()
#plt.show()

plt.savefig('fs_components.png')
print(componentsOth)

