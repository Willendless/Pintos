#include "filesys/filesys.h"
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "filesys/file.h"
#include "filesys/free-map.h"
#include "filesys/inode.h"
#include "filesys/directory.h"
#include "filesys/buffer-cache.h"
#include "threads/thread.h"

/* Partition that contains the file system. */
struct block *fs_device;

static void do_format (void);
static int get_next_part (char part[NAME_MAX + 1], const char **srcp);

/* Initializes the file system module.
   If FORMAT is true, reformats the file system. */
void
filesys_init (bool format)
{
  fs_device = block_get_role (BLOCK_FILESYS);
  if (fs_device == NULL)
    PANIC ("No file system device found, can't initialize file system.");

  inode_init ();
  free_map_init ();
  cache_init ();

  if (format)
    do_format ();

  free_map_open ();
  
}

/* Shuts down the file system module, writing any unwritten data
   to disk. */
void
filesys_done (void)
{
  free_map_close ();
}

/* Creates a file named NAME with the given INITIAL_SIZE.
   Returns true if successful, false otherwise.
   Fails if a file named NAME already exists,
   or if internal memory allocation fails. */
bool
filesys_create (const char *name, off_t initial_size)
{
  block_sector_t inode_sector = 0;
  struct dir *dir = dir_open_root ();
  bool success = (dir != NULL
                  && free_map_allocate (1, &inode_sector)
                  && inode_create (inode_sector, initial_size)
                  && dir_add (dir, name, inode_sector));
  if (!success && inode_sector != 0)
    free_map_release (inode_sector, 1);
  dir_close (dir);

  return success;
}

/* Opens the file with the given NAME.
   Returns the new file if successful or a null pointer
   otherwise.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
struct file *
filesys_open (const char *name)
{
  struct dir *dir = dir_open_root ();
  struct inode *inode = NULL;

  if (dir != NULL)
    dir_lookup (dir, name, &inode);
  dir_close (dir);

  return file_open (inode);
}

/* Deletes the file named NAME.
   Returns true if successful, false on failure.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
bool
filesys_remove (const char *name)
{
  struct dir *dir = dir_open_root ();
  bool success = dir != NULL && dir_remove (dir, name);
  dir_close (dir);

  return success;
}


/* Create the dir named DIR.
   Returns true if successful, false on failure.
   Fails if DIR is valid,
   or if dir has already existed. */
bool
filesys_mkdir (const char *dir)
{
  int ret;
  const char *dir_walk  = dir;
  struct dir *dir_      = dir_reopen (thread_current()->cwd);
  struct dir *new_dir   = NULL;
  struct inode *inode;
  char part_path_name[NAME_MAX + 1];
  char dir_name[NAME_MAX + 1];
  block_sector_t dir_sector;
  bool success = true;

  if (dir[0] == '/')
    dir_ = dir_open_root ();
  
  if (dir_ == NULL)
    success = false;

  while ((ret = get_next_part (part_path_name, &dir_walk))) {
    if (ret == -1) {
      dir_close (dir_);
      success = false;
      break;
    }
    
    if (!dir_lookup (dir_, part_path_name, &inode)) {
      strlcpy (dir_name, part_path_name, strlen (part_path_name));
      if (!get_next_part (part_path_name, &dir_walk) != 0
          || !free_map_allocate (1, &dir_sector)
          || !dir_create (dir_sector, 2)
          || (new_dir = dir_open (inode_open (dir_sector))) == NULL
          || !dir_add (new_dir, ".", dir_sector)
          || !dir_add (new_dir, "..", inode_get_inumber (dir_get_inode (new_dir)))) {
        dir_close (dir_);
        success = false;
        break;
      }  
    } else {
      dir_close (dir_);
      if ((dir_ = dir_open (inode)) == NULL) {
        success = false;
        break;
      }
    }
  }
  return success && new_dir != NULL;
}


/* Formats the file system. */
static void
do_format (void)
{
  printf ("Formatting file system...");
  free_map_create ();
  if (!dir_create (ROOT_DIR_SECTOR, 16))
    PANIC ("root directory creation failed");
  free_map_close ();
  printf ("done.\n");
}

/* Extracts a file name part from *SRCP into PART, and updates *SRCP so that
   the next call will return the next file name part. Returns 1 if successful,
   0 at end of string, -1 for a too-long file name part. */
static int
get_next_part (char part[NAME_MAX + 1], const char **srcp)
{
  const char *src = *srcp;
  char *dst = part;

  /* Skip leading slashes. If it's all slashes, we're done. */
  while (*src == '/')
    ++src;
  if (*src == '\0')
    return 0;
  
  /* Copy up ot NAME_MAX character from SRC to DST. Add null terminator. */
  while (*src != '/' && *src != '\0') {
    if (dst < part + NAME_MAX)
      (*dst++) = *src;
    else
      return -1;
    src++;
  }
  *dst = '\0';

  /* Advance source pointer. */
  *srcp = src;
  return 1;
}
