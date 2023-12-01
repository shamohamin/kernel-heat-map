from anytree import NodeMixin, RenderTree
from threading import Thread 
from joblib import Parallel, delayed
import re
import concurrent.futures 
# REGEX PLAYGROUND: 
# https://regex101.com/r/WTDMfp/1


# extending the node class by anytree to add functionality
class Node(NodeMixin): 
    def  __init__(self, name, time, parent=None, children=None):
        super(Node, self).__init__()
        self.name = name
        self.time = time
        self.parent = parent
        if children:
            self.children = children
            
    def __str__(self): 
        return f"{self.name} ({self.time})"
            

class ParsedLine: 
    
    def __init__(self, process_part, bracket_part, timestamp, funcgraph_part, time, function_name, depth):
        self.pid = process_part.strip().split("-")[-1]
        self.bracket_part = bracket_part.strip()
        self.timestamp = timestamp.strip()
        self.funcgraph_part = funcgraph_part.strip()
        if "us" in time:
            self.time = time.strip().replace("us", "").strip()
            self.time = self.time.replace("!", "").replace("+", "").replace("#", "").strip()
            self.time = float(self.time)
        else: 
            self.time = None 
        self.function_name = function_name.strip()
        self.depth = depth
    
    def __str__(self): 
        return f"{self.pid} {self.bracket_part} {self.timestamp} {self.funcgraph_part} {self.time} {self.function_name}"


class TreeParser:
    
    def __init__(self, pid, file_name): 
        self.pid = pid 
        self.file_name = file_name 
        self.pattern = '(\S+)\s*\[(\d+)\]\s*(\d+\.\d+):\s*(\w+):\s*(.*)\s*\|\s*(.*)'
        self.file_content = []
        self.parsed_lines = []
        self.root = None 
        
        
    def read_file(self): 
        with open(self.file_name, 'r') as f: 
            print("REading file", flush=True)
            self.file_content = f.readlines()
            self.parsed_lines = [None] * len(self.file_content)
    
    def parse_line(self, idx):
        if idx % 1000 == 0: 
            print(f"Processing line {idx}", flush=True)
        line = self.file_content[idx].strip()
        k = line.index("|")
        depth = 0
        while line[k+1].isspace(): 
            k += 1
            depth += 1
            
        match = re.match(self.pattern, line)
        if match: 
            process_part = match.group(1)
            bracket_part = match.group(2)
            timestamp = match.group(3)
            funcgraph_part = match.group(4)
            time = match.group(5)
            function_name = match.group(6)
            pl = ParsedLine(process_part, bracket_part, timestamp, funcgraph_part, time, function_name, depth//2)
            if pl.pid == self.pid: 
                return pl
        return None
    
    def parse_few_lines(self, start, length): 
        res = []
        for i in range(start, start + length): 
            res.append(self.parse_line(i))
        return res
            
    def parse_lines(self): 
        # # Counting on you!
        # # https://stackoverflow.com/questions/56659294/does-joblib-parallel-keep-the-original-order-of-data-passed
        # print("HELLLOOOOO", flush=True)
        # self.parsed_lines = Parallel(n_jobs=16)(delayed(self.parse_line)(idx) for idx in range(len(self.file_content)))    
        # # for i, line in enumerate(self.file_content): 
        #     # pl = self.parse_line(i)
        #     # self.parsed_lines[i] = pl
        
        with concurrent.futures.ProcessPoolExecutor(max_workers=128) as executor:
            # futures = {executor.submit(self.parse_line, idx): idx for idx in range(len(self.file_content))}
            # for future in concurrent.futures.as_completed(futures):
            #     idx = futures[future]
            #     pl = future.result()
            #     self.parsed_lines[idx] = pl
            futures = {executor.submit(self.parse_few_lines, idx, 10000): idx for idx in range(0, len(self.file_content), 10000)}
            for future in concurrent.futures.as_completed(futures):
                idx = futures[future]
                pls = future.result()
                for i, pl in enumerate(pls): 
                    self.parsed_lines[idx + i] = pl
  
    def generate_tree(self): 
        self.root = Node("root", 0.0)
        stack = [self.root]
        
        for i, pl in enumerate(self.parsed_lines):
            if not pl: 
                continue
            if "funcgraph_entry" in pl.funcgraph_part:
                supposed_parent = stack[-1]
                me = None 
                for child in supposed_parent.children:
                    if child.name == pl.function_name: 
                        me = child
                        break 
                if me: 
                    node = me  
                else: 
                    node = Node(pl.function_name, 0.0, parent=supposed_parent)
                    
                # node = Node(pl.function_name, 0.0, parent=supposed_parent)
                if pl.time is None: # non-null pl.time in funcgraph_entry means that the function is a leaf node
                    stack.append(node) 
                    # time will be set whenever it's popped
                else: 
                    node.time += pl.time
            elif "funcgraph_exit" in pl.funcgraph_part:
                node = stack.pop()
                node.time += pl.time
         
    
    def execute(self): 
        print("executing tree parser", flush=True)
        self.read_file()
        self.parse_lines()
        
        for line in self.parsed_lines: 
            if line:
                print(line.depth)
        
        # self.generate_tree()
        # # print(RenderTree(self.root))
        
        # for pre, _, node in RenderTree(self.root):
        #     treestr = u"%s%s [%s]" % (pre, node.name, node.time)
        #     print(treestr.ljust(8))
        