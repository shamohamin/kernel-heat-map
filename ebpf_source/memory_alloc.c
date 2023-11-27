#include <uapi/linux/ptrace.h>
#include <linux/sched.h>
#include <asm/io.h>
// #include <linux/mm.h>
// #include <linux/kasan.h?>
// #include <linux/slab.h>

// #include <linux/slab_def.h>
#include <linux/mm.h>
#include <linux/kasan.h>

// memcg_cache_params is a part of kmem_cache, but is not publicly exposed in
// kernel versions 5.4 to 5.8.  Define an empty struct for it here to allow the
// bpf program to compile.  It has been completely removed in kernel version
// 5.9, but it does not hurt to have it here for versions 5.4 to 5.8.
struct memcg_cache_params {};

// introduce kernel interval slab structure and slab_address() function, solved
// 'undefined' error for >=5.16. TODO: we should fix this workaround if BCC
// framework support BTF/CO-RE.
struct slab {
    unsigned long __page_flags;

#if defined(CONFIG_SLAB)

    struct kmem_cache *slab_cache;
    union {
        struct {
            struct list_head slab_list;
            void *freelist; /* array of free object indexes */
            void *s_mem;    /* first object */
        };
        struct rcu_head rcu_head;
    };
    unsigned int active;

#elif defined(CONFIG_SLUB)

    struct kmem_cache *slab_cache;
    union {
        struct {
            union {
                struct list_head slab_list;
#ifdef CONFIG_SLUB_CPU_PARTIAL
                struct {
                    struct slab *next;
                        int slabs;      /* Nr of slabs left */
                };
#endif
            };
            /* Double-word boundary */
            void *freelist;         /* first free object */
            union {
                unsigned long counters;
                struct {
                    unsigned inuse:16;
                    unsigned objects:15;
                    unsigned frozen:1;
                };
            };
        };
        struct rcu_head rcu_head;
    };
    unsigned int __unused;

#elif defined(CONFIG_SLOB)

    struct list_head slab_list;
    void *__unused_1;
    void *freelist;         /* first free block */
    long units;
    unsigned int __unused_2;

#else
#error "Unexpected slab allocator configured"
#endif

    atomic_t __page_refcount;
#ifdef CONFIG_MEMCG
    unsigned long memcg_data;
#endif
};

// slab_address() will not be used, and NULL will be returned directly, which
// can avoid adaptation of different kernel versions
static inline void *slab_address(const struct slab *slab)
{
    return NULL;
}

#ifdef CONFIG_64BIT
typedef __uint128_t freelist_full_t;
#else
typedef u64 freelist_full_t;
#endif

typedef union {
	struct {
		void *freelist;
		unsigned long counter;
	};
	freelist_full_t full;
} freelist_aba_t;

#ifdef CONFIG_SLUB
#include <linux/slub_def.h>
#else
#include <linux/slab_def.h>
#endif

#define CACHE_NAME_SIZE 32

#define SAMPLE_RATE 50000

typedef struct _allocation_key_t {
    u32 pid, tid;
} allocation_key_t;

typedef struct _shared_info_t {
    allocation_key_t process_identifier;
    char name[TASK_COMM_LEN]; // contains the name of executable
} shared_info_t;

typedef struct _allocation_info_t {
    shared_info_t process_info;
    
    uint buff_counter;

    u64 requested_size; 
    u64 allocation_time;
} allocation_info_t;

// typedef enum _Mem_ALLOC_TYPE {
//     KERNEL = 0,
//     USER
// } Mem_ALLOC_TYPE; 

#define KERNEL 0
#define USER 1

typedef struct _submit_data_t {
    u32 pid;
    u32 tid;

    u64 allocated_size; 
    u64 allocation_time;
    u64 number_memory_allocation;
    
    char name[TASK_COMM_LEN];
} submit_data_t;

/**
 * Defining DATA_STRUCTURES
 *  - __alloc_buffer is for retriving the data when a buffer is full.
 *  - __allocation_data the buffer that holds data until it reaches the buffer size. 
 *  - __process_info hashes process identifier to process binary-name
*/
BPF_PERF_OUTPUT(__alloc_buffer_USER);
BPF_PERF_OUTPUT(__alloc_buffer_KERNEL);
BPF_HASH(__allocation_user_data, u32, allocation_info_t);
BPF_HASH(__allocation_kernel_data, u32, allocation_info_t);
BPF_HASH(__process_info, u32, shared_info_t);


static __always_inline 
void __save_process_name(allocation_key_t *key)
{
    void *found = __process_info.lookup(&(key->pid));
    if (!found) { // not found
        shared_info_t info;
        info.process_identifier = (allocation_key_t) {.pid = key->pid, .tid = key->tid};
        bpf_get_current_comm(&info.name, sizeof(info.name));

        __process_info.insert(&(key->pid), &info);
    }

    return;
}

static __always_inline  
void __retrive_process_info(u32 *pid, u32 *tid) {
    u64 id = bpf_get_current_pid_tgid();
    *tid = id;
    *pid = id >> 32;
    return;
}


static __always_inline
int __enter_handler_alloc_funcs(struct pt_regs *ctx, size_t size, int alloc_type) {
    allocation_key_t key;
    allocation_info_t *alloc_info;

    u64 curr_time, diff_time;    

    __retrive_process_info(&(key.pid), &(key.tid));
    // __save_process_name(&key);
    
    if (alloc_type == USER) 
        alloc_info = __allocation_user_data.lookup(&(key.pid));
    else 
        alloc_info = __allocation_kernel_data.lookup(&(key.pid));

    if (!alloc_info) { // data not found
        allocation_info_t ait = {
            .buff_counter = 1
        };
        ait.process_info.process_identifier = key;

        bpf_get_current_comm(ait.process_info.name, sizeof(ait.process_info.name));

        ait.requested_size = size;
        ait.allocation_time = bpf_ktime_get_ns();
        if (alloc_type == USER) 
            __allocation_user_data.insert(&(key.pid), &ait);
        else
            __allocation_kernel_data.insert(&(key.pid), &ait);

    } else {
        curr_time = bpf_ktime_get_ns();
        alloc_info->requested_size += size;
        diff_time = curr_time - alloc_info->allocation_time;
        alloc_info->buff_counter++;

        if (diff_time > SAMPLE_RATE) {
            alloc_info->allocation_time = curr_time;

            submit_data_t data = {
                .pid = key.pid,
                .tid = key.tid,
                .allocation_time = curr_time,
                .allocated_size = alloc_info->requested_size,
                .number_memory_allocation = alloc_info->buff_counter,
            };
            bpf_get_current_comm(data.name, sizeof(data.name));

            if (alloc_type == USER) 
                __alloc_buffer_USER.perf_submit(ctx, &data, sizeof(submit_data_t));
            else
                __alloc_buffer_KERNEL.perf_submit(ctx, &data, sizeof(submit_data_t));

        }
    }

    return 0;
}

int malloc_enter(struct pt_regs *ctx, size_t size) {
    return __enter_handler_alloc_funcs(ctx, size, USER);
}

int kmem_cache_alloc_enter(struct pt_regs *ctx, struct kmem_cache *cachep) {
    int size = cachep->object_size;
    return __enter_handler_alloc_funcs(ctx, size, KERNEL);
}