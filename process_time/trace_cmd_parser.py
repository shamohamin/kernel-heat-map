from anytree import Node, RenderTree
import re
import json
from threading import Thread
import gc

FUNCTION_OPEN = '{'
FUNCTION_ENTRY = 'funcgraph_entry:'
FUNCTION_EXIT = 'funcgraph_exit:'
ROOT_SPACE_COUNT = 2

TIME_KEY = "time_in_us"
PARENT_KEY = "parent"


class SplitTracingFile:
    def __init__(self, file_content, number_of_threads = 2) -> None:
        self.file_content = file_content
        self.number_of_threads = number_of_threads        
    
    def get_thread_ranges(self):
        number_of_lines = len(self.file_content)
        thread_ranges = []
        start_index = 0
        end_index = 0
        step = number_of_lines//self.number_of_threads
        end_of_file = None

        for _ in range(1, self.number_of_threads):
            end_index += step
            print(thread_ranges, step)
            end_index, end_of_file = self.__find_root_parrent_node(end_index, number_of_lines)
            if end_of_file: break

            thread_ranges.append((start_index, end_index))

            start_index = end_index
        
        thread_ranges.append((start_index, len(self.file_content)))
        print(thread_ranges)
        return thread_ranges

                        
    def __number_of_spaces(self, function_part_cmd: str):
        space_count = 0
        for charactar in function_part_cmd:
            if charactar.isspace():
                space_count += 1
            else:
                break
        
        return space_count

    def __find_root_parrent_node(self, start_position, total_index):
        root_line_index = start_position
        line = None
        splited_line = None

        while True:
            # check the root
            if root_line_index >= total_index:
                return root_line_index, True

            line = self.file_content[root_line_index]
            splited_line = line.split('|')

            if len(splited_line) == 2:
                function_part = splited_line[-1]
                time_stamp_part = splited_line[0].split()
                if FUNCTION_ENTRY in time_stamp_part and \
                    self.__number_of_spaces(function_part) == ROOT_SPACE_COUNT: 
                    break

            root_line_index += 1    
        
        return root_line_index, False   


class ParserThread(Thread):
    def __init__(self, pid, file_content, parse_range: tuple, *args, **kwargs) -> None:
        Thread.__init__(self, *args, **kwargs)
        self.fn_hash_map = {}
        self.pid = int(pid)
        self.parse_range = parse_range
        self.file_content = file_content

        self.daemon = True 
        self.start()

    def run(self) -> None:
        self.__parse_trace_text()

    def __add_fn_entry(self, key, time, parent_name, parent_depth):
        fn_value = self.fn_hash_map.get(key, None)
        if fn_value is None:
            self.fn_hash_map[key] = {
                TIME_KEY: time,
                PARENT_KEY: set([f'{parent_name}-{parent_depth}'])
            }
        else:
            self.fn_hash_map[key][TIME_KEY] += time
            self.fn_hash_map[key][PARENT_KEY].add(f'{parent_name}-{parent_depth}')

    def __check_time_usage(self, timepart):
        try:
            return float(timepart[4])
        except:
            return float(timepart[5])

    def __get_function_name(self, fn_name: str):
        return re.sub(r'(\(|\))', '', fn_name)

    def __parse_trace_text(self):
        if len(self.file_content) == 0:
            raise ValueError('Trace is empty')
        
        root = Node('root')
        current_parent = root
        function_part = None
        time_stamp_part = None
        pid = None
        line = None

        for i in range(self.parse_range[0], self.parse_range[-1]):
            try:
                line = self.file_content[i]

                line = line.strip().split('|')
                function_part = line[1].split()
                time_stamp_part = line[0].split()
                pid = int(time_stamp_part[0].split('-')[-1])
                if pid != self.pid:
                    continue
            except Exception as ex:
                continue
            
            if(FUNCTION_OPEN in function_part):
                function_name = self.__get_function_name(function_part[0])
                node = Node(function_name, parent=current_parent, time=0.0)
                current_parent = node

            elif(FUNCTION_ENTRY in time_stamp_part):
                function_name = self.__get_function_name(function_part[0][:-1])
                time = self.__check_time_usage(time_stamp_part)
                node = Node(function_name, parent=current_parent, time=time)
                self.__add_fn_entry(key=str(function_name), time=time,
                                     parent_name=current_parent.name, parent_depth=current_parent.depth)

            elif(FUNCTION_EXIT in time_stamp_part):
                if current_parent == root:
                    continue

                time = self.__check_time_usage(time_stamp_part)
                current_parent.time = time
                self.__add_fn_entry(key=str(current_parent.name), time=time,
                                     parent_name=current_parent.parent.name, parent_depth=current_parent.depth)
                current_parent = current_parent.parent


class Parser:
    def __init__(self, pid, filename, number_of_threads = 5) -> None:
        self.filename = filename
        self.number_of_threads = number_of_threads
        self.pid = pid
        self.__trace_splinter = None
        self.__aggeregated_fn_entries = {}
    
    def __aggregate_times(self, fn_entry: dict):
        for key in fn_entry.keys():
            fn_value = self.__aggeregated_fn_entries.get(key, None)
            if fn_value is None:
                self.__aggeregated_fn_entries[key] = fn_entry[key]
            else:
                self.__aggeregated_fn_entries[key][TIME_KEY] += fn_entry[key][TIME_KEY]
                self.__aggeregated_fn_entries[key][PARENT_KEY].union(fn_entry[key][PARENT_KEY])
                
    def __read_file(self):
        with open(self.filename, 'r') as f:
            self.file_content = f.readlines()

    def execute(self):
        self.__read_file()
        self.__trace_splinter = SplitTracingFile(self.file_content, self.number_of_threads)
        thread_ranges = self.__trace_splinter.get_thread_ranges()
        parsers = []

        for i in range(len(thread_ranges)):
            parser = ParserThread(self.pid, self.file_content, thread_ranges[i])
            parsers.append(parser)

        # join
        for parser in parsers:
            assert type(parser) is ParserThread 
            parser.join()

        for parser in parsers:
            assert type(parser) is ParserThread 
            self.__aggregate_times(parser.fn_hash_map)


        class SetEncoder(json.JSONEncoder):
            def default(self, obj):
                if isinstance(obj, set):
                    return list(obj)
                return json.JSONEncoder.default(self, obj)
        
        with open("b.json", "w") as file:
            json.dump(self.__aggeregated_fn_entries, file, indent=4, cls=SetEncoder)

        