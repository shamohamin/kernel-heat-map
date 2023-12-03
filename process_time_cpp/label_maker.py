import subprocess
import os
import json
import re
import concurrent.futures

cscopeDatabasePath = os.path.dirname(os.path.dirname(__file__))
PATTERN = r"[\(|\)|\{|\}|;]"
PATTERN = re.compile(PATTERN)

functionCache = {}
timeSpentInsideEachCompoenent = {}

def readtrees(filename = "out.json") -> dict:
    data = None

    if not os.path.exists(filename): return None

    with open(filename, "r") as file:
        data = json.load(file)

    return data

def cleanFunctionName(functionName):
    cleanedFunctionName = PATTERN.sub("", functionName)
    return cleanedFunctionName
    
def runCScope(cmd, dirpath):
    with subprocess.Popen(
        cmd, stdout=subprocess.PIPE, shell=True, 
        cwd=dirpath
    ) as proc:
        csoutput = str(proc.stdout.read(), encoding="utf-8").split("\n")
        return csoutput

def queryCScope(functionName) -> set:
    baseCmd = f"cscope -d -l -L -1 {functionName}"
    labels = set()
    functionDefinedLocations = runCScope(baseCmd, cscopeDatabasePath)

    for definedLocation in functionDefinedLocations:
        try:
            filePath = definedLocation.split()[0] # split based on space
            if filePath.find("debian") != -1:
                continue
            parentPath = filePath.split(os.sep)[-2]
            labels.add(parentPath)
        except:
            continue
        
    return labels    

def findLabel(functionName: str):
    cachedLabel = functionCache.get(functionName, None) 
    if cachedLabel is None:
        # use cscope
        labels = queryCScope(functionName)
        functionCache[functionName] = labels
        return labels

    return cachedLabel
        

def processCpuTree(rootNode):
    functionName = cleanFunctionName(rootNode["name"])
    rootNode["name"] = functionName
    if functionName != "root":
        labels = findLabel(functionName)
        rootNode["label"] = labels
        # for label in labels:
        #     timeSpentInsideEachCompoenent[label] = timeSpentInsideEachCompoenent.get(label, 0.0) + rootNode['time']
    
    for child in rootNode["children"]:
        
        processCpuTree(child)

def processCPUTree(cpuTree: dict):
    processCpuTree(cpuTree)    

def processCpusThreaded(cpusData: dict):
    with concurrent.futures.ThreadPoolExecutor(max_workers=5) as executor:
        futures = {executor.submit(processCPUTree, cpusData[cpu]): cpu for cpu in cpusData.keys()}
        for future in concurrent.futures.as_completed(futures):
            future.result()


if __name__ == "__main__":
    data = readtrees()
    if data is None: print("could not read the files"); exit()

    processCpusThreaded(cpusData=data)
    
    class SetEncoder(json.JSONEncoder):
        def default(self, obj):
            if isinstance(obj, set):
                return list(obj)
            return json.JSONEncoder.default(self, obj)
    
    print(timeSpentInsideEachCompoenent)

    with open("final_data.json", "w") as file:
        json.dump(data, file, indent=4, cls=SetEncoder)
    
    