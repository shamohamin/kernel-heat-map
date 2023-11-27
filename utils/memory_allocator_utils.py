# from bcc import BPF
import ctypes as ct
from .data_collector_schema import DataCollector

TASK_COMM_LEN = 16

class MemoryDataStructure(ct.Structure):
    _fields_ = [
        ("pid", ct.c_uint32),
        ("tid", ct.c_uint32),
        ("allocated_size", ct.c_ulonglong),
        ("allocation_time", ct.c_ulonglong),
        ("number_memory_allocation", ct.c_ulonglong),
        ("name", ct.c_char * TASK_COMM_LEN)
    ]


class MemoryAlloctor(DataCollector):
    USER, KERNEL = (0, 1)

    def __init__(self) -> None:        
        super().__init__()
        self.data = {}

    # def checkAllocationData(alloc_type: int):
        
    def __getDefaultValuesForThreads(self):
        return {
            self.USER : {
                'allocation_ts': [],
                'allocated_size': []                    
            },
            self.KERNEL: {
                'allocation_ts': [],
                'allocated_size': []
            }
        }

    def __append_new_data(self, pid, tid, alloc_type, alloc_size, alloc_time):
        self.data[pid][tid][alloc_type]['allocation_ts'].append(alloc_time)
        self.data[pid][tid][alloc_type]['allocated_size'].append(alloc_size)


    def collect(self, perf_data, *args, **kwargs):
        parsed_data = ct.cast(
            perf_data,
            ct.POINTER(MemoryDataStructure)).contents
        
        pid = parsed_data.pid
        tid = parsed_data.tid
        super().collect(None, pid=pid, pname=parsed_data.name)

        memory_allocation_type = kwargs['memory_allocation_type']

        process_data = self.data.get(pid, None)

        if process_data is None:
            self.data[pid] = {
                tid: {
                    **self.__getDefaultValuesForThreads()
                },
                'process_name': parsed_data.name.decode('utf-8'),
            }
        
        thread_data = self.data[pid].get(tid, None)
        if thread_data is None:
            self.data[pid][tid] = {
                **self.__getDefaultValuesForThreads()
            }

    
        self.__append_new_data(
            pid, tid,
            memory_allocation_type,
            parsed_data.allocated_size,
            parsed_data.allocation_time)
        
    
    def save(self):
        import os
        import json
        data_folder = os.path.join(os.path.dirname(os.path.dirname(__file__)), "data")
        if not os.path.exists(data_folder):
            os.mkdir(data_folder)

        with open("memory_allocator.json", "w") as file:
            json.dump(self.data, file)
    