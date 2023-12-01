import re 
from joblib import Parallel, delayed
import time 
from anytree import Node, RenderTree

# pattern = '(\S+)\s*\[(\d+)\]\s*(\d+\.\d+):\s*(\w+):\s*(.*)\s*\|\s*(.*)'

# with open('trace.report', 'r') as fp: 
#     lines = fp.readlines()

# def sleep(): 
#     time.sleep(0.01)
# print(len(lines))
# start = time.time()
# jobs = 20 
# results = Parallel(n_jobs=jobs)(delayed(re.match)(pattern, line) for line in lines)
# print(len(results))
# end = time.time()
# print(f"{jobs} jobs")
# print(end - start)

# dummy = [None] * 1000
# start = time.time()
# results = Parallel(n_jobs=jobs)(delayed(sleep)() for _ in dummy)
# end = time.time()
# print(f"{jobs} jobs")
# print(end - start)

root = Node("root")
node1 = Node("node1", parent=root)
node2 = Node("node2", parent=root)
print(RenderTree(root))