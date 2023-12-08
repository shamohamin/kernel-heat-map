from bcc import BPF
import ctypes as ct
from utils.memory_allocator_utils import MemoryAlloctor
import time

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

start = time.time()

while 1:
    try:
        if time.time() - start >= 100:
            memAllocator.save()
            break
    
        bpf.perf_buffer_poll()
    except:
        memAllocator.save()
        break
    