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


#### Algorithms

*second chance list*



#### Synchronization
    
#### Rationale

### Task 2: Extensible Files

#### Data structures and functions

#### Algorithms
    
#### Synchronization
    
#### Rationale

### Task 3: Subdirectories

#### Data structures and functions

#### Algorithms
    
#### Synchronization
    
#### Rationale


## Aditional Questions
