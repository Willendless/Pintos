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

***filesys/buffer_cache.c, filesys/buffer_cache.h:***

*Adding following data strutures:*

```c
#define CACHE_SIZE 64

static struct list active_list;
static struct list second_chance_list;

struct cache_entry {
    struct list_elem elem;
    uint8_t buffer[BLOCK_SECTOR_SIZE];
    bool modified;
    bool valid;
    block_sector_t sector;
}
```

*Adding following functions:*
+ cache_init()
```c
void cache_init (void);
```

+ cache_find()
```c
uint8_t *cache_find (block_sector_t sector);
```

+ cache_eviction ()
```c
bool *cache_eviction (uint8_t *buffer, off_t size);
```

+ cache_write_back ()
```c
static void cache_write_back ();
```

***filesys/inode.c:***


*Modifying following data structures:*

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

*Modifying following functions:*

+ inode_read_at()
```c
off_t inode_read_at (struct inode *inode, void *buffer_, off_t size, off_t offset);
```

+ inode_write_at()
```c
off_t inode_write_at (struct inode *inode, const void *buffer_, off_t size, off_t offset);
```



#### Algorithms

*second chance list*



#### Synchronization
    
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
