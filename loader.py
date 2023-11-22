from bcc import BPF
import ctypes as ct


TASK_COMM_LEN = 16

class MemoryDataStructure(ct.Structure):
    _fields_ = [
        ("pid", ct.c_uint32),
        ("tid", ct.c_uint32),
        # ("alloc_type", ct.c_uint32),
        ("allocated_size", ct.c_ulonglong),
        ("allocation_time", ct.c_ulonglong),
        ("number_memory_allocation", ct.c_ulonglong),
        # ("allocation_type", ct.c_uint32),
        ("name", ct.c_char * TASK_COMM_LEN)
    ]


bpf = None
# with open('ebpf_source/memory_alloc.c') as file:
bpf = BPF(src_file="ebpf_source/memory_alloc.c")

def process_data_user(data):
    # if kernel
    per_data = bpf["__alloc_buffer_USER"].event(data)
    parsed_event = ct.cast(data, ct.POINTER(MemoryDataStructure)).contents
    # print(parsed_event.pid, parsed_event.allocated_size, parsed_event.number_memory_allocation, str(parsed_event.name))


def process_data_kernel(data):
    per_data = bpf["__alloc_buffer_KERNEL"].event(data)
    parsed_event = ct.cast(data, ct.POINTER(MemoryDataStructure)).contents
    print("kernel", parsed_event.pid, parsed_event.allocated_size, parsed_event.number_memory_allocation, str(parsed_event.name))


bpf.attach_uprobe(name='c', sym="malloc", fn_name="malloc_enter")
bpf.attach_kprobe(event="kmem_cache_alloc", fn_name="kmalloc_exit")
bpf["__alloc_buffer_USER"].open_perf_buffer(
    lambda cpu, data, size: process_data_user(data)
)

bpf["__alloc_buffer_KERNEL"].open_perf_buffer(
    lambda cpu, data, size: process_data_kernel(data)
)

while 1:
    # print(bpf.trace_fields())
    bpf.perf_buffer_poll()
    
    # exit(1)