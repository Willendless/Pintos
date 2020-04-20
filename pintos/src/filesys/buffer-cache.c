#include "filesys/buffer-cache.h"
#include "threads/synch.h"

/* Global buffer cache lock. */
static struct lock cache_lock;

/* Second chance list data structure. */
static struct list active_list;         /* FIFO list */
static struct list second_chance_list;  /* LRU list */

void
cache_init ()
{

}

int
cache_get ()
{

}

int cache_put ()
{

}

static void cache_replace ()
{

}
