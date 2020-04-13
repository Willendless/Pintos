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
        bool modified;
        bool valid;
        block_sector_t sector;                  /* sector number */
        struct block* block;
        struct lock* lock;
        struct condition* cond;
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
    + use buffer cache get interface instead of block_read()

+ inode_write_at()
    ```c
    off_t inode_write_at (struct inode *inode, const void *buffer_, off_t size, off_t offset);
    ```
    + use buffer cache put interface insread of block_write()

#### Algorithms

*second chance list:*

Active list uses FIFO strategy and SC list uses LRU strategy.
1. cache buffer hit
    + read:
        1. active list hit: directly return data
        2. sc list hit: put elem into head of active list and return data
    + write:
        1. active list hit: write data and set modified bit
        2. sc list hit: put elem into head of active list and write data and set modified bit
2. cache buffer miss
    + read:
        1. replace
    + write: least used cache, then the same as cache hit.
        1. if write size equals to BLOCK_SECTOR_SIZE 

3. replace the least used cache without reading from disk
        2. else replace the least used cache with reading from disk
        3. the same as cache hitif active list has more node the sc list, move the first node of active list to sc list.

+ cache_search()
```c
static struct cache_entry *cache_search(sector)
{
    struct cache_entry *entry;
    lock_acquire(&cache_lock);
    /* 
        1. traverse active list to find entry
            + if hit, set entry
            + if miss, traverse SC list to find entry 
                + if hit, move entry to head of active list
                + if miss, move tail of second chance list to head of active list
    */
    lock_release(&cache_lock);
    return entry
}
```

+ cache_get()
```c
int cache_get (sector, buffer)
{
    cache_entry *entry = cache_search();
    lock_acquire(&entry->lock);
    /* 
        if entry hit:
        1. copy data to buffer
    */
    /* 
        if entry miss
        1. if modified bit set, write back data
        2. call block_read() to feed entry data
        3. copy data to buffer
    */
    lock_release(&entry->lock);
}

```

+ cache_put()
```c
int cache_put (sector, buffer, size)
{
    cache_entry *entry = cache_search();
    lock_require(&entry->lock);
    /* 
        if entry hit:
        1. copy data from buffer to entry data
    */
    /* 
        if entry miss:
        1. if modified bit set, write back data
        2. if size smaller than BLOCK_SECTOR_SIZE call block_read() to feed entry data
        3. write data to entry and set modified bit 
    */
    lock_release(&entry->lock);
}
```

+ inode_open () 
    ```c
    struct inode * inode_open (block_sector_t sector)
    ```
    + use `cache_get()` insread of `block_read()`

+ inode_create ()
    ```c
    bool inode_create (block_sector_t sector, off_t length)
    ```
    + use cache_put instead of block_write

+ inode_length ()
    ```c
    off_t inode_length (const struct inode *inode);
    ```
    + first use cache_get() to read on-disk inode, then return inode data length 

#### Synchronization

Using two-level locking to implement thread-safe buffer cache:
    1. global buffer cache lock
        + protect the list structure of active list and second chance list
    2. fine granularity per entry lock
        + guarantee the automic access of cached block
    
1. active_list

2. second_chance_list

3. cache_entry

#### Rationale

1. global cache_lock guarantees there is only one thread can modify list structure at any time
    invariant: no other thread is modifying list

2. use active_list to avoid frequently moving node to the end of list if the cache is visited frequently.

### Task 2: Extensible Files

#### Data structures and functions

***filesys/inode.c***

*Adding following data structures*

```c
static struct internal_node
{
  block_sector_t ptr[128];
}
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

+ inode_write_at ()
    ```c
    off_t inode_write_at (struct inode *inode, const void *buffer_, off_t size, off_t offset)
    ```
    + dynamic calculate the write destination sector and extends the file if necessary

+ inode_close ()
    ```c
    void inode_close (struct inode *inode)
    ```
    + if release resources, release all sectors assigned to the inode  

#### Algorithms
    
#### Synchronization
    
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