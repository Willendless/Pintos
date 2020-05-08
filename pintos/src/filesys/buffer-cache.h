#ifndef BUFFER_CACHE_H
#define BUFFER_CACHE_H

#include <list.h>
#include <stdbool.h>
#include "devices/block.h"
#include "threads/synch.h"
#include "filesys/off_t.h"

#define CACHE_SIZE 64

void cache_init (void);
int cache_get (struct block* block, block_sector_t sector, off_t sector_ofs,
               void *buffer, off_t size);
int cache_put (struct block *block, block_sector_t sector, off_t sector_ofs,
               const void *buffer, off_t size); 

#endif
