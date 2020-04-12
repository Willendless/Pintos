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

    static struct lock cache_lock;
    static struct condition entry_evict;

    static struct list active_list;             /* FIFO list */
    static struct list second_chance_list;      /* LRU list */

    struct cache_entry {
        struct list_elem elem;
        uint8_t buffer[BLOCK_SECTOR_SIZE];
        bool modified;
        bool valid;
        bool currently_used;
        block_sector_t sector;
    }
    ```
    + use 64 `struct cache_entry` to store each cached block, manage them with FIFO queue `active_list` and LRU queue `second_chance_list` each of them are fixed sized with 32 cache entry

*Adding following functions:*
+ cache_init ()
    ```c
    void cache_init (void);
    ```
    + init 64 cache entries with `valid` equals to false and place them in `second_chance_list`

+ cache_get ()
    ```c
    int cache_get (block_sector_t sector, void *buffer);
    ```
    + iterate 64 cache entries in `active_list` and `second_chance_list` and return the one with sector number equals to SECTOR if exists

+ cache_put ()
    ```c
    int cache_put (block_sector_t sector, uint8_t *buffer);
    ```
    + write SECTOR to corresponding cache entry from buffer which must be `BLOCK_SECOTR_SIZE` bytes

+ cache_eviction ()
    ```c
    static void cache_eviction ();
    ```
    + 

+ cache_write_back ()
    ```c
    static void cache_write_back ();
    ```
    +

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
    + 

+ inode_write_at()
    ```c
    off_t inode_write_at (struct inode *inode, const void *buffer_, off_t size, off_t offset);
    ```
    +



#### Algorithms

*second chance list*

+ cache_get()
    1. acquire cache_lock
    2. search cached sector in active list
        + if cached sector exists
            1. copy data to buffer
        + if cached sector doesn't exist
            1. search cached sector in second chance list
                + if cache sector exists
                + if cache sector

     release cache_lock

+ cache_put()
    + acquire cache_lock

+ cache_eviction()

4. cache_write_back()



#### Synchronization
    
1. active_list

2. second_chance_list

3. cache_entry




#### Rationale

### Task 2: Extensible Files

#### Data structures and functions

***filesys/inode.c***

*Adding following data structures*

```c
static struct internal_node
{
  uint32_t ptr[BLOCK_SECTOR_SIZE / 4];
}
```

*Modifying following data structures*

```c
#define DIRECT_PTR_SIZE 12
struct inode_disk
{
  block_sector_t start;               /* First data sector. */
  off_t length;                       /* File size in bytes. */
  uint32_t direct_ptr[12];
  uint32_t indirect_ptr;
  uint32_t dbl_indirect_ptr;
  unsigned magic;                     /* Magic number. */
  uint32_t unused[111];               /* Not used. */
 };
```


#### Algorithms
    
#### Synchronization
    
#### Rationale

### Task 3: Subdirectories

#### Data structures and functions

#### Algorithms
    
#### Synchronization
    
#### Rationale


## Aditional Questions
