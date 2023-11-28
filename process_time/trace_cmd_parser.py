from anytree import Node, RenderTree
import re
import json

FUNCTION_OPEN = '{'
FUNCTION_ENTRY = 'funcgraph_entry:'
FUNCTION_EXIT = 'funcgraph_exit:'

class Parser:
    def __init__(self, pid) -> None:
        self.file = None
        self.times_hash_map = {}
        self.pid = int(pid)

    def parse(self, file):
        self.file = file
        self.load_report()
        self.parse_trace_text()    


    def load_report(self) -> None:
        with open(self.file, 'r') as f:
            self.text = f.readlines()

    def add_time(self, key, time):
        if key in self.times_hash_map:
            self.times_hash_map[key] += time
        else:
            self.times_hash_map[key] = time

    def __check_time_usage(self, timepart):
        try:
            return float(timepart[4])
        except:
            return float(timepart[5])

    def parse_trace_text(self, saveTree = False):
        if self.text == []:
            raise ValueError('Trace is empty')
        
        index = 1
        total_line = len(self.text)
        root = Node('root')
        current_parent = root
        function_part = None
        time_stamp_part = None
        pid = None
        line = None

        for i in range(index, total_line):
            try:
                line = self.text[i]

                line = line.strip().split('|')
                function_part = line[1].split()
                time_stamp_part = line[0].split()
                pid = int(time_stamp_part[0].split('-')[-1])
                if pid != self.pid:
                    continue
            except Exception as ex:
                continue
            
            if(FUNCTION_OPEN in function_part):
                function_name = function_part[0]
                node = Node(function_name, parent=current_parent, time=0.0)
                current_parent = node

            elif(FUNCTION_ENTRY in time_stamp_part):
                function_name = function_part[0][:-1]
                time = self.__check_time_usage(time_stamp_part)
                node = Node(function_name, parent=current_parent, time=time)
                self.add_time(key=str(function_name), time=time)

            elif(FUNCTION_EXIT in time_stamp_part):
                if current_parent == root:
                    continue

                time = self.__check_time_usage(time_stamp_part)
                current_parent.time = time
                self.add_time(key=str(current_parent.name), time=time)
                current_parent = current_parent.parent

        with open("a.json", "w") as file:
            json.dump(self.times_hash_map, file, indent=4)

        if saveTree:
            with open("afrin.txt", "w") as file:
                import sys
                sys.stdout = file
                sys.stdout.write(str(RenderTree(root)))
                sys.stdout.flush()
                # print(RenderTree(root))
