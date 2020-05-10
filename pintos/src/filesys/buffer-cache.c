#include "filesys/buffer-cache.h"
#include "threads/synch.h"
#include "threads/malloc.h"
#include <stdio.h>
#include <string.h>

/* Global buffer cache lock. */
static struct lock cache_lock;

/* Second chance list data structure. */
static struct list cache_list;  /* LRU list */

typedef struct {
    struct list_elem elem;
    uint8_t buffer[BLOCK_SECTOR_SIZE];
    bool modified;
    bool valid;
    block_sector_t sector;
    block_sector_t old_sector;
    struct lock lock;
    struct condition cond;
} cache_entry_t;

/* Create new cache entry. */
static cache_entry_t *cache_entry_init (void);
static void get_buffer (cache_entry_t *, off_t, uint8_t *, off_t);
static void put_buffer (cache_entry_t *entry,  off_t sector_ofs, const uint8_t *buffer,
                        off_t size);
static bool get_cache_entry (block_sector_t sector, cache_entry_t **entry);

/* Init buffer cache. */
void
cache_init (void)
{
  int i;
  lock_init (&cache_lock);
  list_init (&cache_list);
  for (i = 0; i < CACHE_SIZE; ++i)
    list_push_back (&cache_list, &cache_entry_init ()->elem);
}

static cache_entry_t *
cache_entry_init (void)
{
  cache_entry_t *new = (cache_entry_t *)malloc (sizeof(cache_entry_t));
  if (new == NULL) {
    return NULL;
  }
  lock_init (&new->lock);
  cond_init (&new->cond);
  new->modified = false;
  new->valid = false;
  return new;
}

int
cache_get (struct block* block, block_sector_t sector, off_t sector_ofs,
          void *buffer, off_t size)
{
  ASSERT (sector_ofs + size <= BLOCK_SECTOR_SIZE);
  cache_entry_t *hit_entry;
  if (get_cache_entry (sector, &hit_entry)) {
    get_buffer (hit_entry, sector_ofs, buffer, size);
  } else {
    if (hit_entry->modified) {
      block_write (block, hit_entry->old_sector, hit_entry->buffer);
    }
    block_read (block, sector, hit_entry->buffer);
    hit_entry->modified = false;
    get_buffer (hit_entry, sector_ofs, buffer, size);
  }
  return 0;
}

int
cache_put (struct block *block, block_sector_t sector, off_t sector_ofs,
           const void *buffer, off_t size)
{
  cache_entry_t *hit_entry;
  if (get_cache_entry (sector, &hit_entry)) {
    put_buffer (hit_entry, sector_ofs, buffer, size);
    hit_entry->modified = true;
  } else if (sector_ofs == 0 && size == BLOCK_SECTOR_SIZE) {
    if (hit_entry->modified) {
      block_write (block, hit_entry->old_sector, hit_entry->buffer);
    }
    hit_entry->modified = true;
    put_buffer (hit_entry, sector_ofs, buffer, size);
  } else {
    if (hit_entry->modified) {
      block_write (block, hit_entry->old_sector, hit_entry->buffer);
    }
    block_read (block, sector, hit_entry->buffer);
    hit_entry->modified = true;
    put_buffer (hit_entry, sector_ofs, buffer, size);
  }
  return 0;
}

static void 
get_buffer (cache_entry_t *entry, off_t sector_ofs, uint8_t *buffer,
            off_t size)
{
  ASSERT (sector_ofs + size <= BLOCK_SECTOR_SIZE);
  ASSERT (lock_held_by_current_thread (&entry->lock));

  memcpy (buffer, entry->buffer + sector_ofs, size);
  lock_release (&entry->lock);
  lock_acquire (&cache_lock);
  cond_broadcast (&entry->cond, &cache_lock);
  lock_release (&cache_lock);
}

static void
put_buffer (cache_entry_t *entry,  off_t sector_ofs, const uint8_t *buffer,
            off_t size)
{
  ASSERT (sector_ofs + size <= BLOCK_SECTOR_SIZE);
  ASSERT (lock_held_by_current_thread (&entry->lock));

  memcpy (entry->buffer + sector_ofs, buffer, size);
  lock_release (&entry->lock);
  lock_acquire (&cache_lock);
  cond_broadcast (&entry->cond, &cache_lock);
  lock_release (&cache_lock);
}

static bool
get_cache_entry (block_sector_t sector, cache_entry_t **entry)
{
  struct list_elem *e;
  bool hit = false;
  cache_entry_t *hit_entry;

  lock_acquire (&cache_lock);
loop:
  for (e = list_begin (&cache_list); e != list_end (&cache_list);
       e = list_next (e))
    {
      cache_entry_t *en = list_entry (e, cache_entry_t, elem);
      if (en->valid == false) 
        break;
      /* hit */
      if (en->sector == sector) {
        while (!lock_try_acquire (&en->lock)) {
          cond_wait (&en->cond, &cache_lock);
        }
        if (en->sector != sector) {
          lock_release (&en->lock);
          goto loop;
        }
        /* Move to head of cache list. */
        list_remove (&en->elem);
        list_push_front (&cache_list, &en->elem);
        hit = true;
        hit_entry = en;
        break;
      }
    }
  if (hit == false) {
    /* Try evict back of cahce_list. */
    hit_entry = list_entry (list_back (&cache_list), cache_entry_t, elem);
    while (!lock_try_acquire (&hit_entry->lock)) {
      cond_wait (&hit_entry->cond, &cache_lock);
    }
    if (&hit_entry->elem != list_back (&cache_list)) {
      goto loop;
    } else {
      hit_entry->old_sector = hit_entry->sector;
      hit_entry->sector = sector;
      hit_entry->valid = true;
      list_remove (&hit_entry->elem);
      list_push_front (&cache_list, &hit_entry->elem);
    }
  }
  lock_release (&cache_lock);
  *entry = hit_entry;
  return hit;
}

void
cache_flush (struct block* block)
{
  struct list_elem *e;
  for (e = list_begin (&cache_list); e != list_end (&cache_list);
       e = list_next (e))
    {
      cache_entry_t *en = list_entry (e, cache_entry_t, elem);
      if (en->valid && en->modified) {
        block_write (block, en->sector, en->buffer);
        en->modified = false;
      }
    }
}
