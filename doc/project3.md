Design Document for Project 3: File Systems
===========================================

## Group Members

* Jiarui Li <jiaruili@berkeley.edu>
* Xiang Zhang <xzhang1048576@berkeley.edu>
* Joel Kattapuram <joelkattapuram@berkeley.edu>
* Kallan Bao <kallanbao@berkeley.edu>

## Design Overview

### Task 1: Buffer Cache

#### Data structures and functions

***1. filesys/buffer_cache.c, filesys/buffer_cache.h:***

*Adding following data strutures:*

+ struct cache_entry
    ```c
    #define CACHE_SIZE 64

    static struct lock cache_lock;              /* global cache lock */

    static struct list active_list;             /* FIFO list */
    static struct list second_chance_list;      /* LRU list */

    struct cache_entry {
        struct list_elem elem;
        uint8_t buffer[BLOCK_SECTOR_SIZE];
        bool modified;                          /* if valid */
        bool valid;                             /* if modified */
        bool active;                            /* if entry in active list */
        block_sector_t sector;                  /* sector number */
        struct block* block;                    /* device */
        struct lock* lock;                      /* per entry lock */
    }
    ```
    + use 64 `struct cache_entry` to store each cached block, manage them with FIFO queue `active_list` and LRU queue `second_chance_list` each of them are fixed sized with 32 cache entry

*Adding following functions:*
+ cache_init ()
    ```c
    void cache_init (void);
    ```
    + init 64 cache entries with `valid` equals to false and place them in `second_chance_list` and `active list`

+ cache_get ()
    ```c
    int cache_get (struct block *block, block_sector_t sector, uint8_t *buffer);
    ```
    + iterate 64 cache entries in `active_list` and `second_chance_list` and return the one with sector number equals to SECTOR

+ cache_put ()
    ```c
    int cache_put (struct block *block, block_sector_t sector, off_t sector_ofs, uint8_t *buffer, off_t size);
    ```
    + write SECTOR to corresponding cache entry from buffer which must be `BLOCK_SECOTR_SIZE` bytes

+ cache_replace ()
    ```c
    static void cache_replace (struct block *block, block_sector_t sector);
    ```
    + replace the least used cache with SECTOR on disk


***2. filesys/inode.c:***

*Modifying following data structures:*

+ struct inode
    ```c
    /* In-memory inode. */
    struct inode
    {
        struct list_elem elem;              /* Element in inode list. */
        block_sector_t sector;              /* Sector number of disk location. */
        int open_cnt;                       /* Number of openers. */
        bool removed;                       /* True if deleted, false otherwise. */
        int deny_write_cnt;                 /* 0: writes ok, >0: deny writes. */
        //struct inode_disk data;             /* Inode content. */
    };
    ```
    + use buffer cache to store on-disk inode

*Modifying following functions:*

+ inode_read_at()
    ```c
    off_t inode_read_at (struct inode *inode, void *buffer_, off_t size, off_t offset);
    ```
    + use buffer cache get interface instead of `block_read()`

+ inode_write_at()
    ```c
    off_t inode_write_at (struct inode *inode, const void *buffer_, off_t size, off_t offset);
    ```
    + use buffer cache put interface insread of `block_write()`

+ inode_open () 
    ```c
    struct inode * inode_open (block_sector_t sector)
    ```
    + use `cache_get()` insread of `block_read()`

+ inode_create ()
    ```c
    bool inode_create (block_sector_t sector, off_t length)
    ```
    + use `cache_put()` instead of `block_write()`

+ inode_length ()
    ```c
    off_t inode_length (const struct inode *inode);
    ```
    + first use `cache_get()` to read on-disk inode, then return inode data length 

#### Algorithms

*second chance list:*

Active list uses FIFO strategy and SC list uses LRU strategy.

First acquire global `cache_lock`, then execute:
1. traverse active list
    1. if hit, call `lock_try_acquire()` on `entry_lock`  
        1. if success, release `cache_lock` and execute IO operation  
        1. if `entry_lock` held by another thread, release `cache_lock` then call lock_acquire() on `entry_lock` after return, double check if `entry_lock` really hit and in active list  
            + if hit, then execute IO operation  
            + if miss, redo traverse  
    2. if miss, goto 2  
2. traverse second chance list  
    1. if hit, first move entry to head of active list and move tail of active list to head of second chance list, then acquire `entry_lock`, release `cache_lock` and do IO operation  
    2. if miss, first modify field of tail of second chance list and move it to the head of active list, move the tail of the active list, then acquire `entry_lock`, release `cache_lock`, then  
        + if mofified bit is set, then write back  
        + execute IO operation  
    
special case:  
> generally, when doing write operation, if miss we should first read the corresponding sector from the disk, then overwrite it. However, if the write size equals to length of sector then there is no need to read the sector before write to the buffer cache entry, we can directly write the sector to the buffer

#### Synchronization

+ *Using two-level locking to implement thread-safe buffer cache:*
    1. global buffer cache lock
        + protect the list structure of active list and second chance list
        + guarantee that at any time there can only be one thread modifying the structure of the lists
    2. fine granularity per entry lock
        + guarantee that at ant time there can only be one operation executed on a single entry 

#### Rationale

1. we can modify the size of active list and the second chance list to test the performance of different size combination 

2. use active_list to avoid frequently moving node to the end of list if the cache is visited frequently.

3. use second chance list to guarantee that when evicting a entry no other thread is executing IO operation on that evicted entry 

### Task 2: Extensible Files

#### Data structures and functions

***filesys/inode.c***

*Adding following data structures*

```c
struct ptr_block_t
{
  block_sector_t ptr[128];
};
```

*Modifying following data structures*

```c
#define DIRECT_PTR_SIZE 12

struct inode_disk
{
  off_t length;                       /* File size in bytes. */
  block_sector_t direct_ptr[DIRECT_PTR_SIZE];
  block_sector_t indirect_ptr;
  block_sector_t dbl_indirect_ptr;
  unsigned magic;                     /* Magic number. */
  uint32_t unused[112];               /* Not used. */
 };
```

```c
/* In-memory inode. */
struct inode
{
    struct list_elem elem;              /* Element in inode list. */
    block_sector_t sector;              /* Sector number of disk location. */
    int open_cnt;                       /* Number of openers. */
    bool removed;                       /* True if deleted, false otherwise. */
    int deny_write_cnt;                 /* 0: writes ok, >0: deny writes. */

    struct lock *lock;                  /* inode is shared data structure. */            
};
```

*Adding following functions*

+ syscall_inumber ()
    ```c
    int inumber (int fd)
    ```
    + return the inode of the file indexed by fd in current thread's `open_files[MAX_OPEN_FILES]`

*Modifying following functions*

+ filesys_create ()
    ```c
    bool filesys_create (const char *name, off_t initial_size);
    ```
    + assign direct/indirect ptrs to initial file's on-disk inode according to the initial_size

+ byte_to_sector ()
    ```c
    static block_sector_t byte_to_sector (const struct inode *inode, off_t pos)
    ```
    + first read on-disk inode, then calculate the sector number according to file length and pos

+ inode_read_at () 
    ```c
    off_t inode_read_at (struct inode *inode, void *buffer_, off_t size, off_t offset)
    ```
    + dyncamically get the read destination sector use `locate_inode_sector()` and read data

+ inode_write_at ()
    ```c
    off_t inode_write_at (struct inode *inode, const void *buffer_, off_t size, off_t offset)
    ```
    + dynamically calculate the write destination sector use `locate_inode_sector()` and extends the file if necessary

+ inode_close ()
    ```c
    void inode_close (struct inode *inode)
    ```
    + if release resources, release all sectors assigned to the inode  

#### Algorithms

1. *byte_to_sector(used in both read & write operation):*
    1. use offset to calculate which level indirect ptr pointing to the sector of data
    2. use `cache_get()` to first get on-disk inode then get corresponding sector number by traverse entry `struct internel_node::ptr[128]`

2. *handle disk exhaustion:*
    + in `inode_write_at()` 
        1. calculate the number of new sectors current write operation needs 
        2. check if there is enough free sector in `free_map` for allocating
            + if yes, continuing write
            + if no, abort the write operation

#### Synchronization

*file system synchronization:*

1. when threads access in-memory inode, they should acquire the inode lock first
    
#### Rationale

### Task 3: Subdirectories

#### Data structures and functions

*Modifying following data structures:*

+ struct thread
    ```c
    struct thread {
        /*...*/
        block_sector_t cwd;
        /*...*/
    }
    ```

*Modifying following functions:*

+ filesys_open ()
```c
struct file *filesys_open (const char *name)
```

*Adding following functions:*

+ find_inode_by_relative_path ()
    ```c
    struct inode *find_inode_by_relative_path (char* path)
    ```
    + find the inode indicated by relative path. return NULL if not exist.
    
+ find_inode_by_absolute_path ()
    ```c
    struct inode *find_inode_by_absolute_path (char* path)
    ```
    + find the inode indicated by absolute path. return NULL if not exist.

+ find_inode_by_path ()
    ```c
    struct inode *find_inode_by_path (char* path)
    ```
    + identify the path and call relative or absolute path finder.


#### Algorithms

+ find_inode_by_relative_path ()
  1. break the path into parts separated by '\\\\' or '/'.
  2. starting from cwd, visit the inode according to each part
  3. return the inode if the file exist.

+ find_inode_by_absolute_path ()
  1. break the path into parts separated by '\\\\' or '/'.
  2. starting from root, visit the inode according to each part
  3. return the inode if the file exist.

+ find_inode_by_path ()
  1. check if path begins by '\\\\' or '/'
  2. if true, call find_inode_by_absolute_path ()
  3. else call find_inode_by_relative_path ()

#### Synchronization
    
#### Rationale


## Aditional Questions

1. write behind
    + on initialization of file system, create a thread which does the following:
    + check if the disk is not busy at every time tick.
    + if not busy, flush several dirty cache back to disk.
2. read ahead
    + on reading a block from a file, start a thread which does the following:
    + load the following blocks (maximum half of cache size) into cache.