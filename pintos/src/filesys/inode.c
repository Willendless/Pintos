#include "filesys/inode.h"
#include <list.h>
#include <debug.h>
#include <round.h>
#include <string.h>
#include "filesys/filesys.h"
#include "filesys/free-map.h"
#include "filesys/buffer-cache.h"
#include "threads/malloc.h"
#include <stdio.h>

/* Identifies an inode. */
#define INODE_MAGIC 0x494e4f44

#define FILE_MAX_BLOCK_SIZE 16384
#define DIRECT_PTR_SIZE 12
#define PTR_BLOCK_SIZE ((BLOCK_SECTOR_SIZE)/(sizeof(block_sector_t)))
#define INDIRECT_PTR_SIZE PTR_BLOCK_SIZE
#define DBL_INDIRECT_PTR_SIZE ((PTR_BLOCK_SIZE)*(PTR_BLOCK_SIZE))

/* On-disk inode.
   Must be exactly BLOCK_SECTOR_SIZE bytes long. */
struct inode_disk
  {
    off_t length;                               /* File size in bytes. */
    uint32_t is_dir;                               /* If file is a directory. */
    uint32_t sectors;                              /* Sector num in bytes. */
    block_sector_t direct_ptr[DIRECT_PTR_SIZE]; /* Inode direct pointer. */
    block_sector_t indirect_ptr;                /* Inode indirect pointer. */     
    block_sector_t dbl_indirect_ptr;              /* Inode double direct pointer. */
    unsigned magic;                             /* Magic number. */
    uint32_t unused[110];                       /* Not used. */
  };

/* On-disk ptr stored block. 
   Must be exactly BLOCK_SECTOR_SIZE bytes long. 
   May point to another ptr_block or data block. */
struct ptr_block
  {
    block_sector_t ptr[PTR_BLOCK_SIZE];
  };

static void inode_sector_release (block_sector_t sector, int begin, int end, int level);
static int inode_extend_length (block_sector_t inode_sector, size_t lengh);
static void inode_sector_remove (struct inode *inode);
static bool inode_extend_sectors (block_sector_t inode_sector, int num);
static int inode_extend_sector (block_sector_t sector, off_t sector_ofs);


/* Returns the number of sectors to allocate for an inode SIZE
   bytes long. */
static inline size_t
bytes_to_sectors (off_t size)
{
  return DIV_ROUND_UP (size, BLOCK_SECTOR_SIZE);
}

/* In-memory inode. */
struct inode
  {
    struct list_elem elem;              /* Element in inode list. */
    block_sector_t sector;              /* Sector number of disk location. */
    int open_cnt;                       /* Number of openers. */
    bool removed;                       /* True if deleted, false otherwise. */
    int deny_write_cnt;                 /* 0: writes ok, >0: deny writes. */
    struct lock inode_lock;             /* Guard inode. */
    int is_dir;                         /* file type. */
  };


/* Returns the block device sector that contains byte offset POS
   within INODE.
   Returns -1 if INODE does not contain data for a byte at offset
   POS. */
static int
byte_to_sector (const struct inode *inode, off_t pos)
{
  ASSERT (inode != NULL);
  ASSERT (true);

  int length = inode_length (inode);
  size_t index = pos / BLOCK_SECTOR_SIZE;
  block_sector_t dbl_indirect_ptr;
  block_sector_t indirect_ptr;
  block_sector_t ptr;

  if (pos > length) {
    return -1;
  }

  if (index < DIRECT_PTR_SIZE) {
    cache_get (fs_device,
               inode->sector,
               offsetof (struct inode_disk, direct_ptr) + index * sizeof (block_sector_t),
               (void *)&ptr, sizeof(block_sector_t));
    return ptr;
  }
  index -= DIRECT_PTR_SIZE;
  if (index < INDIRECT_PTR_SIZE) {
    cache_get (fs_device, inode->sector, offsetof (struct inode_disk, indirect_ptr),
              (void *)&indirect_ptr, sizeof(block_sector_t));
    cache_get (fs_device, indirect_ptr, offsetof (struct ptr_block, ptr[index]),
               (void *)&ptr, sizeof(block_sector_t));
    return ptr;
  }
  index -= INDIRECT_PTR_SIZE;
  if (index < DBL_INDIRECT_PTR_SIZE) {
    cache_get (fs_device, inode->sector, offsetof (struct inode_disk, dbl_indirect_ptr),
              (void *)&dbl_indirect_ptr, sizeof(block_sector_t));
    cache_get (fs_device, dbl_indirect_ptr, index / BLOCK_SECTOR_SIZE * sizeof (block_sector_t),
              (void *)&indirect_ptr, sizeof(block_sector_t));
    cache_get (fs_device, indirect_ptr, index % BLOCK_SECTOR_SIZE * sizeof (block_sector_t),
              (void *)&ptr, sizeof(block_sector_t));
    return ptr;
  }
  ASSERT (true);
  return -1;
}


/** Try extend inode legth by LENGTH.
 *  Return the lengh actually extended. */
static int
inode_extend_length (block_sector_t inode_sector, size_t length)
{
  int old_length, new_length;
  
  cache_get (fs_device, inode_sector,
             offsetof (struct inode_disk, length),
             &old_length, sizeof(int));

  int sector_left = (old_length + BLOCK_SECTOR_SIZE - 1)
                    / BLOCK_SECTOR_SIZE * BLOCK_SECTOR_SIZE - old_length;
  if (sector_left >= (int)length) {
    new_length = old_length + length;
    cache_put (fs_device, inode_sector,
               offsetof (struct inode_disk, length),
               &new_length, sizeof(int));
    return length;
  }

  length -= sector_left;
  if (!inode_extend_sectors (inode_sector,
                            (length + BLOCK_SECTOR_SIZE - 1)
                            / BLOCK_SECTOR_SIZE)) {
    new_length = old_length;
  } else {
    new_length = old_length + length;
  }

  cache_put (fs_device, inode_sector,
             offsetof (struct inode_disk, length),
             &new_length,
             sizeof(int));
  return new_length - old_length;
}


/* Try extend inode with num sectors.
   Return false if failed.
*/
static bool
inode_extend_sectors (block_sector_t inode_sector, int num)
{
  int begin, index;
  int length;
  int allocated_size = 0;
  int indirect_ptr;
  int dbl_indirect_ptr;

  cache_get (fs_device, inode_sector,
             offsetof (struct inode_disk, length), (void *)&length, sizeof (int));

  if (num == 0)
    return true;

  if (num < 0 || num > (int)DBL_INDIRECT_PTR_SIZE) {
    return false;
  }

  // first sector needed to be allocated
  begin = index = (length + BLOCK_SECTOR_SIZE - 1) / BLOCK_SECTOR_SIZE;

  while (allocated_size < num && index < DIRECT_PTR_SIZE) {
    if (inode_extend_sector (inode_sector,
                              offsetof (struct inode_disk, direct_ptr)
                              + index * sizeof(block_sector_t)) == -1) {
      inode_sector_release (inode_sector, begin, index, 0);
      return false;
    }
    ++allocated_size;
    ++index;
  }
  index -= DIRECT_PTR_SIZE;
  if (index == 0 && allocated_size < num) {
    if (inode_extend_sector (inode_sector,
                             offsetof (struct inode_disk, indirect_ptr)) == -1) {
      inode_sector_release (inode_sector, begin, index + DIRECT_PTR_SIZE, 1);
      return false;
    }
  }
  if (index >= 0 && index < (int)INDIRECT_PTR_SIZE && allocated_size < num) {
    cache_get (fs_device, inode_sector,
                offsetof (struct inode_disk, indirect_ptr),
                (void *)&indirect_ptr, sizeof (block_sector_t));
    while (allocated_size < num && index < (int)INDIRECT_PTR_SIZE) {
      if (inode_extend_sector (indirect_ptr,
                                offsetof (struct ptr_block, ptr[index])) == -1) {
        inode_sector_release (inode_sector, begin, index + INDIRECT_PTR_SIZE, 1);
        return false;
      }
      ++allocated_size;
      ++index;
    }
  }
  index -= INDIRECT_PTR_SIZE;
  if (index == 0 && allocated_size < num) {
    if (inode_extend_sector (inode_sector,
                              offsetof (struct inode_disk, dbl_indirect_ptr)) == -1) {
      inode_sector_release (inode_sector, begin, index + DIRECT_PTR_SIZE + INDIRECT_PTR_SIZE, 2);
      return false;
    }
  }
  cache_get (fs_device, inode_sector,
              offsetof (struct inode_disk, dbl_indirect_ptr),
              (void *)&dbl_indirect_ptr, sizeof (block_sector_t));
  int indirect_ptr_block;
  while (allocated_size < num && index >= 0
          && index < (int)DBL_INDIRECT_PTR_SIZE) {
    if (index % PTR_BLOCK_SIZE == 0) {
      if ((indirect_ptr_block
            = inode_extend_sector (dbl_indirect_ptr,
                                   (index / PTR_BLOCK_SIZE) * sizeof(block_sector_t))) == -1) {
        inode_sector_release (inode_sector, begin, index + DIRECT_PTR_SIZE + INDIRECT_PTR_SIZE, 1);
        return false;
      }        
    } else {
      cache_get (fs_device, dbl_indirect_ptr,
                (index / PTR_BLOCK_SIZE) * sizeof(block_sector_t),
                (void *)&indirect_ptr_block, sizeof(block_sector_t));
    }
    if (inode_extend_sector (indirect_ptr_block,
                              (index % PTR_BLOCK_SIZE) * sizeof(block_sector_t)) == -1) {
        inode_sector_release (inode_sector, begin, index + DIRECT_PTR_SIZE + INDIRECT_PTR_SIZE, 0);
        return false;
    }
    ++allocated_size;
    ++index;
  }

  ASSERT (allocated_size == num);
  return true;
}

/**
 * Try allocate a sector to offset.
 * Return allocated sector number and -1 if failed.
 */
static int
inode_extend_sector (block_sector_t sector, off_t sector_ofs)
{
  block_sector_t allocated_sector;
  if (!free_map_allocate (1, &allocated_sector)) {
    return -1;
  }
  cache_put (fs_device, sector, sector_ofs, (void *)&allocated_sector, sizeof(block_sector_t));
  return allocated_sector;
}

/**
 * Release block backward from end index to begin index.
 * level = 0, data sector allocate failed.
 * level = 1, indirect ptr block allocate failed.
 * level = 2, dbl-indirect-ptr block allocate failed.
 */
static void
inode_sector_release (block_sector_t sector, int begin, int end, int level)
{
  int index;
  int dbl_index = end - DIRECT_PTR_SIZE - INDIRECT_PTR_SIZE;
  int dbl_beg = begin - DIRECT_PTR_SIZE - INDIRECT_PTR_SIZE;
  dbl_beg = dbl_beg < 0 ? 0 : dbl_beg;
  int in_index = dbl_index >= 0 ? (int)INDIRECT_PTR_SIZE - 1 : end - (int)DIRECT_PTR_SIZE;
  int in_beg = dbl_beg > 0 ? (int)INDIRECT_PTR_SIZE + 1 : begin - (int)DIRECT_PTR_SIZE;
  in_beg = in_beg < 0 ? 0 : in_beg;
  int dir_index = in_index >= 0 ? (int)DIRECT_PTR_SIZE - 1: end;
  int dir_beg = in_beg > 0 ? (int)DIRECT_PTR_SIZE + 1 : begin;
  if (begin == end) 
    return;

  // dbl-indirect-ptr process
  index = dbl_index;
  if (index >= dbl_beg) {
    block_sector_t dbl_indirect_ptr;
    block_sector_t indirect_ptr;
    block_sector_t ptr;
    cache_get (fs_device, sector,
               offsetof (struct inode_disk, dbl_indirect_ptr),
               (void *)&dbl_indirect_ptr, sizeof(block_sector_t));
    while (index >= dbl_beg) {
      if (level == 2) {
        level = -1;
        break;
      }
      cache_get (fs_device, dbl_indirect_ptr,
                 index / PTR_BLOCK_SIZE * sizeof(block_sector_t),
                 (void *)&indirect_ptr, sizeof(block_sector_t));
      if (level == 1) {
        if (index == 0) {
          free_map_release (dbl_indirect_ptr, 1);
        }
        --index;
        level = -1;
        continue;
      }
      cache_get (fs_device, ptr,
                 index % PTR_BLOCK_SIZE * sizeof(block_sector_t),
                 (void *)&ptr, sizeof(block_sector_t));
      if (level == 0) {
        if (index % PTR_BLOCK_SIZE == 0) {
          free_map_release (indirect_ptr, 1);
        }
        --index;
        level = -1;
        continue;
      }
      free_map_release (ptr, 1);
      if (index % PTR_BLOCK_SIZE == 0) {
        free_map_release (indirect_ptr, 1);
      }
      if (index / PTR_BLOCK_SIZE == 0) {
        free_map_release (dbl_indirect_ptr, 1);
      }
      --index;
    }
  }

  // indirect-ptr process
  index = in_index;
  if (in_index >= in_beg) {
    block_sector_t indirect_ptr;
    block_sector_t ptr;
    cache_get (fs_device, sector,
                offsetof (struct inode_disk, indirect_ptr),
                (void *)&indirect_ptr,
                sizeof(block_sector_t));
    while (index >= in_beg) {
      if (level == 1) {
        level = -1;
        break;
      }
      cache_get (fs_device, indirect_ptr,
                  index * sizeof(block_sector_t),
                  (void *)&ptr,
                  sizeof(block_sector_t));
      if (level == 0) {
        if (index == 0) {
          free_map_release (indirect_ptr, 1);
        }
        --index;
        level = -1;
        continue;
      }
      free_map_release (ptr, 1);
      if (index == 0) {
        free_map_release (indirect_ptr, 1);
      }
      --index;
    }
  }

  // direct-ptr process
  index = dir_index;
  while (index >= dir_beg) {
    block_sector_t ptr;
    if (level == 0) {
      --index;
      level = -1;
      continue;
    }
    cache_get (fs_device, sector,
                offsetof (struct inode_disk, direct_ptr) + index * sizeof(block_sector_t),
                (void *)&ptr,
                sizeof(block_sector_t));
    free_map_release (ptr, 1);
    --index;
  }
}


/* List of open inodes, so that opening a single inode twice
   returns the same `struct inode'. */
static struct list open_inodes;
static struct lock open_inodes_lock;

/* Initializes the inode module. */
void
inode_init (void)
{
  list_init (&open_inodes);
  lock_init (&open_inodes_lock);
}

/* Initializes an inode with LENGTH bytes of data and
   writes the new inode to sector SECTOR on the file system
   device.
   Returns true if successful.
   Returns false if memory or disk allocation fails. */
bool
inode_create (block_sector_t sector, off_t length, bool is_dir)
{
  struct inode_disk *disk_inode = NULL;
  bool success = false;
  uint32_t magic = INODE_MAGIC;
  size_t sectors = bytes_to_sectors (length);
  uint32_t is_dir_ = is_dir ? 1 : 0; 

  ASSERT (length >= 0);

  /* If this assertion fails, the inode structure is not exactly
     one sector in size, and you should fix that. */
  ASSERT (sizeof *disk_inode == BLOCK_SECTOR_SIZE);

  success = inode_extend_sectors (sector, sectors);

  if (success) {
    cache_put (fs_device, sector,
              offsetof(struct inode_disk, length), &length, sizeof(off_t));
    cache_put (fs_device, sector, 
              offsetof(struct inode_disk, magic), &magic, sizeof(uint32_t));
    cache_put (fs_device, sector,
              offsetof(struct inode_disk, sectors), &sectors, sizeof(uint32_t));
    cache_put (fs_device, sector,
               offsetof(struct inode_disk, is_dir), &is_dir_, sizeof(uint32_t));
  }
  return success;
}

/* Reads an inode from SECTOR
   and returns a `struct inode' that contains it.
   Returns a null pointer if memory allocation fails. */
struct inode *
inode_open (block_sector_t sector)
{
  struct list_elem *e;
  struct inode *inode;

  /* Check whether this inode is already open. */
  lock_acquire (&open_inodes_lock);
  for (e = list_begin (&open_inodes); e != list_end (&open_inodes);
       e = list_next (e))
    {
      inode = list_entry (e, struct inode, elem);
      if (inode->sector == sector)
        {
          lock_release (&open_inodes_lock);
          inode_reopen (inode);
          return inode;
        }
    }
  lock_release (&open_inodes_lock);

  /* Allocate memory. */
  inode = malloc (sizeof *inode);
  if (inode == NULL)
    return NULL;

  /* Initialize. */
  lock_acquire (&open_inodes_lock);
  list_push_front (&open_inodes, &inode->elem);
  lock_release (&open_inodes_lock);
  inode->sector = sector;
  inode->open_cnt = 1;
  inode->deny_write_cnt = 0;
  inode->removed = false;
  lock_init(&inode->inode_lock);
  cache_get (fs_device, sector,
             offsetof (struct inode_disk, is_dir),
             &inode->is_dir,
             sizeof(int));
  return inode;
}

/* Reopens and returns INODE. */
struct inode *
inode_reopen (struct inode *inode)
{
  if (inode != NULL) {
    lock_acquire (&inode->inode_lock);
    inode->open_cnt++;
    lock_release (&inode->inode_lock);
  }
  return inode;
}

/* Returns INODE's inode number. */
block_sector_t
inode_get_inumber (const struct inode *inode)
{
  return inode->sector;
}

/* Closes INODE and writes it to disk.
   If this was the last reference to INODE, frees its memory.
   If INODE was also a removed inode, frees its blocks. */
void
inode_close (struct inode *inode)
{
  /* Ignore null pointer. */
  if (inode == NULL)
    return;

  /* Release resources if this was the last opener. */
  lock_acquire (&open_inodes_lock);
  lock_acquire (&inode->inode_lock);
  if (--inode->open_cnt == 0)
    {
      /* Remove from inode list and release lock. */
      list_remove (&inode->elem);
      lock_release (&open_inodes_lock);

      /* Deallocate blocks if removed. */
      if (inode->removed)
        {
          inode_sector_remove (inode);
          free_map_release (inode->sector, 1);
          // free_map_release (inode_disk.start,
          //                   bytes_to_sectors (inode_disk.length));
        }

      lock_release (&inode->inode_lock);
      free (inode);
    } else {
      lock_release (&inode->inode_lock);
      lock_release (&open_inodes_lock);
    }
}

/* Remove all sectors in INODE.
*/
static void
inode_sector_remove (struct inode *inode)
{
  ASSERT (inode->removed);
  int sectors;
  cache_get (fs_device, inode->sector,
             offsetof (struct inode_disk, sectors),
             (void *)&sectors,
             sizeof(uint32_t));
  inode_sector_release (inode->sector, 0, sectors - 1, -1);
}

/* Marks INODE to be deleted when it is closed by the last caller who
   has it open. */
void
inode_remove (struct inode *inode)
{
  ASSERT (inode != NULL);
  inode->removed = true;
}

/* Reads SIZE bytes from INODE into BUFFER, starting at position OFFSET.
   Returns the number of bytes actually read, which may be less
   than SIZE if an error occurs or end of file is reached. */
off_t
inode_read_at (struct inode *inode, void *buffer_, off_t size, off_t offset)
{
  uint8_t *buffer = buffer_;
  off_t bytes_read = 0;

  while (size > 0)
    {
      /* Disk sector to read, starting byte offset within sector. */
      block_sector_t sector_idx = byte_to_sector (inode, offset);
      int sector_ofs = offset % BLOCK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually copy out of this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;
      
      cache_get (fs_device, sector_idx, sector_ofs, (void *)(buffer + bytes_read),
                 chunk_size);
      
      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_read += chunk_size;
    }

  return bytes_read;
}

/* Writes SIZE bytes from BUFFER into INODE, starting at OFFSET.
   Returns the number of bytes actually written, which may be
   less than SIZE if end of file is reached or an error occurs.
   (Normally a write at end of file would extend the inode, but
   growth is not yet implemented.) */
off_t
inode_write_at (struct inode *inode, const void *buffer_, off_t size,
                off_t offset)
{
  const uint8_t *buffer = buffer_;
  off_t bytes_written = 0;

  if (inode->deny_write_cnt)
    return 0;

  
  off_t inode_left = inode_length (inode) - offset;
  if (inode_left < 0) {
    if (!inode_extend_length (inode->sector, -inode_left + size)) {
      return 0;
    }
  } else if (size - inode_left > 0) {
    if (!inode_extend_length (inode->sector, size - inode_left)) {
      return 0;
    }
  }


  while (size > 0)
    {
      /* Sector to write, starting byte offset within sector. */
      int sector_idx = byte_to_sector (inode, offset);
      int sector_ofs = offset % BLOCK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;

      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually write into this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0) {
        if (!inode_extend_length (inode->sector, size))
          break;
      }

      cache_put (fs_device, sector_idx, sector_ofs, 
                 buffer + bytes_written, chunk_size);

      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_written += chunk_size;
    }

  return bytes_written;
}

/* Disables writes to INODE.
   May be called at most once per inode opener. */
void
inode_deny_write (struct inode *inode)
{
  inode->deny_write_cnt++;
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
}

/* Re-enables writes to INODE.
   Must be called once by each inode opener who has called
   inode_deny_write() on the inode, before closing the inode. */
void
inode_allow_write (struct inode *inode)
{
  ASSERT (inode->deny_write_cnt > 0);
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
  inode->deny_write_cnt--;
}

/* Returns the length, in bytes, of INODE's data. */
off_t
inode_length (const struct inode *inode)
{
  int length;
  cache_get (fs_device, inode->sector, offsetof (struct inode_disk, length),
             (void *)&length, sizeof(int));
  return length;
}

/* Returns the length, in sectors, of INODE's data. */
off_t
inode_size (const struct inode *inode)
{
  int sectors;
  cache_get (fs_device, inode->sector, offsetof (struct inode_disk, sectors),
             (void *)&sectors, sizeof(int));
  return sectors;
}

/* Return inode type, true if directory. */
bool
is_inode_dir (const struct inode *inode)
{
  ASSERT (inode != NULL);

  return inode->is_dir;
}

int
inode_open_cnt (const struct inode *inode)
{
  ASSERT (inode != NULL);
  return inode->open_cnt;
}
