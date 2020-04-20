#ifndef BUFFER_CACHE_H
#define BUFFER_CACHE_H

#include <list.h>
#include "devices/block.h"
#include "threads/synch.h"

#define CACHE_SIZE 64

struct cache_entry
{
    struct list_elem elem;
    uint8_t buffer[BLOCK_SECTOR_SIZE];
    bool modified;
    bool valid;
    bool active;
    block_sector_t sector;
    struct lock *lock;
}


#endif