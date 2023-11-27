from bcc import BPF
import ctypes as ct
from utils.memory_allocator_utils import MemoryAlloctor

TASK_COMM_LEN = 16

# class MemoryDataStructure(ct.Structure):
#     _fields_ = [
#         ("pid", ct.c_uint32),
#         ("tid", ct.c_uint32),
#         ("stack_id", ct.c_ulonglong),
#         ("allocated_size", ct.c_ulonglong),
#         ("allocation_time", ct.c_ulonglong),
#         ("number_memory_allocation", ct.c_ulonglong),
#         # ("allocation_type", ct.c_uint32),
#         # ("", ct.c_ulonglong),
#         ("name", ct.c_char * TASK_COMM_LEN)
#     ]


memAllocator = MemoryAlloctor()

bpf = BPF(src_file="ebpf_source/memory_alloc.c")

# def process_data_user(data):
    # per_data = bpf["__alloc_buffer_USER"].event(data)
    # parsed_event = ct.cast(data, ct.POINTER(MemoryDataStructure)).contents

bpf.attach_uprobe(name='c', sym="malloc", fn_name="malloc_enter")
bpf.attach_kprobe(event="kmem_cache_alloc", fn_name="kmem_cache_alloc_enter")
#bpf.attach_tracepoint("kmem:kmalloc", fn_name="kmalloc_enter")
bpf["__alloc_buffer_USER"].open_perf_buffer(
    lambda cpu, data, size: memAllocator.collect(
        data, memory_allocation_type=MemoryAlloctor.USER
    )
)

bpf["__alloc_buffer_KERNEL"].open_perf_buffer(
    lambda cpu, data, size: memAllocator.collect(
        data, memory_allocation_type=MemoryAlloctor.KERNEL
    )
)

while 1:
    try:
        bpf.perf_buffer_poll()
    except:
        memAllocator.save()
        break
    