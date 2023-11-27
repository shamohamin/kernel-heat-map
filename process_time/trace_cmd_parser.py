from anytree import Node, RenderTree

FUNCTION_OPEN = '{'
FUNCTION_ENTRY = 'funcgraph_entry:'
FUNCTION_EXIT = 'funcgraph_exit:'

class Parser:
    def __init__(self) -> None:
        self.file = None
        self.times_hash_map = {}


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

    def parse_trace_text(self):
        if self.text == []:
            raise ValueError('Trace is empty')
        
        index = 1
        total_line = len(self.text)
        root = Node('root')
        current_parent = root
        
        for i in range(index, total_line):
            line = self.text[i].strip().split('|')
            function_part = line[1].split()
            time_stamp_part = line[0].split()
            if(FUNCTION_OPEN in function_part):
                function_name = function_part[0]
                node = Node(function_name, parent=current_parent, time=0.0)
                current_parent = node

            elif(FUNCTION_ENTRY in time_stamp_part):
                function_name = function_part[0][:-1]
                time = float(time_stamp_part[4])
                node = Node(function_name, parent=current_parent, time=time)
                self.add_time(key=function_name, time=time)

            elif(FUNCTION_EXIT in time_stamp_part):
                time = float(time_stamp_part[4])
                current_parent.time = time
                self.add_time(key=current_parent.name, time=time)
                current_parent = current_parent.parent

        print(self.times_hash_map)