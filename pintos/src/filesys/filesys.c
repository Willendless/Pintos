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
#include "threads/malloc.h"

/* Partition that contains the file system. */
struct block *fs_device;


static void do_format (void);
static int get_next_part (char part[NAME_MAX + 1], const char **srcp);
static struct dir * get_last_dir (struct dir *dir, const char *path);
static bool is_last_part (const char *part);
static bool get_last_part (char last_part[NAME_MAX + 1], const char *path);

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

  /* Set cwd for main thread. */
  thread_current ()->cwd = dir_open_root ();
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
  if (!strlen (name)) {
    return false;
  }

  block_sector_t inode_sector = 0;
  struct dir *dir = dir_reopen (thread_current ()->cwd);
  char file_name[NAME_MAX + 1];
  if (name[0] == '/') {
    dir_close (dir);
    dir = dir_open_root ();
  }

  dir = get_last_dir (dir, name);
  if (!get_last_part (file_name, name)) {
    dir_close (dir);
    return false;
  }

  bool success = (dir != NULL
                  && strlen (file_name) > 0
                  && free_map_allocate (1, &inode_sector)
                  && inode_create (inode_sector, initial_size, false)
                  && dir_add (dir, file_name, inode_sector));
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
struct FILE *
filesys_open (const char *name)
{
  if (!strlen (name)) {
    return false;
  }

  struct dir *dir = dir_reopen (thread_current ()->cwd);
  struct inode *inode = NULL;
  char file_name[NAME_MAX + 1];
  struct FILE *f;

  file_name[0] = '\0';
  if (name[0] == '/') {
    dir_close (dir);
    dir = dir_open_root ();
  }

  dir = get_last_dir (dir, name); 
  if (!get_last_part (file_name, name)) {
    dir_close (dir);
    return NULL;
  }

  // open root directory
  if (file_name[0] == '\0') {
    dir_close (dir);
    f = calloc (1, sizeof *f);
    f->is_dir = true;
    f->ptr.dir = dir_open_root ();
    return f;
  }

  if (dir != NULL)
    dir_lookup (dir, file_name, &inode);
  dir_close (dir);

  if (inode == NULL)
    return NULL;

  f = calloc (1, sizeof *f);
  f->is_dir = is_inode_dir (inode);
  if (f->is_dir) {
    f->ptr.dir = dir_open (inode);
  } else {
    f->ptr.file = file_open (inode);
  }

  return f;
}

/* Close FILE F. Type of F may be dir or file. */
void
filesys_close (struct FILE *f)
{
  if (f != NULL) {
    if (f->is_dir) {
      dir_close (f->ptr.dir);
    } else {
      file_close (f->ptr.file);
    }
    free (f);
  }
}


/* Deletes the file named NAME.
   Returns true if successful, false on failure.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
bool
filesys_remove (const char *name)
{
  if (!strlen (name)) {
    return false;
  }

  struct dir *cwd = thread_current ()->cwd;
  struct dir *dir = dir_reopen (cwd);
  struct inode *inode;
  char file_name[NAME_MAX + 1];
  if (name[0] == '/') {
    dir_close (dir);
    dir = dir_open_root ();
  }
  
  file_name[0] = '\0';
  
  dir = get_last_dir (dir, name);

  if (!get_last_part (file_name, name)) {
    dir_close (dir);
    return false;
  }

  if (dir == NULL
      || !dir_lookup (dir, file_name, &inode)
      || inode == NULL) {
    dir_close (dir);
    return false;
  }

  struct dir *old_dir;
  if (is_inode_dir (inode)) {
    old_dir = dir_open (inode);
    if (dir_equals (cwd, old_dir)) {
      dir_close (old_dir);
      dir_close (dir);
      return false;
    }
    dir_close (old_dir);
  }
  
  bool success = dir != NULL
                 && strlen (file_name) > 0
                 && dir_remove (dir, file_name);
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
  bool success = false;

  if (dir[0] == '/') {
    dir_close (dir_);
    dir_ = dir_open_root ();
  }
  
  ASSERT (dir_ != NULL);

  while ((ret = get_next_part (part_path_name, &dir_walk))) {
    if (ret == -1) {
      dir_close (dir_);
      break;
    }
    
    if (!dir_lookup (dir_, part_path_name, &inode)) {
      strlcpy (dir_name, part_path_name, strlen (part_path_name) + 1);
      if (get_next_part (part_path_name, &dir_walk) != 0
          || !free_map_allocate (1, &dir_sector)
          || !dir_create (dir_sector, 2)
          || (new_dir = dir_open (inode_open (dir_sector))) == NULL
          || !dir_add (new_dir, ".", dir_sector)
          || !dir_add (new_dir, "..", inode_get_inumber (dir_get_inode (dir_)))) {
        dir_close (dir_);
      } else {
        dir_add (dir_, part_path_name, dir_sector);
        dir_close (dir_);
        success = true;
      }
      break;
    } else {
      dir_close (dir_);
      if ((dir_ = dir_open (inode)) == NULL) {
        break;
      }
    }
  }
  if (new_dir != NULL)
    dir_close (new_dir);
  return success;
}

/* Changes the current working directory of the process to DIR,
   False if failure. */
bool 
filesys_chdir (const char *dir)
{
  if (dir[0] == '\0')
    return false;

  char dir_name[NAME_MAX + 1];
  struct dir *dir_ = dir_reopen (thread_current ()->cwd);
  struct inode* inode;

  dir_name[0] = '\0';

  if (dir[0] == '/') {
    dir_close (dir_);
    dir_ = dir_open_root ();
  }
  
  ASSERT (dir_ != NULL);

  dir_ = get_last_dir (dir_, dir);
  if (dir_ == NULL) {
    return false;
  }

  if (!get_last_part (dir_name, dir)) {
    dir_close (dir_);
    return false;
  }

  // chdir to root dir
  if (!strlen (dir_name)) {
    dir_close (dir_);
    dir_close (thread_current ()->cwd);
    thread_current ()->cwd = dir_open_root ();
    return true;
  }

  if (!dir_lookup (dir_, dir_name, &inode)
      || inode == NULL) {
    dir_close (dir_);
    return false;
  }

  dir_close (dir_);
  dir_close (thread_current ()->cwd);
  thread_current ()->cwd = dir_open (inode);
  return true;
}

off_t
filesys_tell (struct FILE *f)
{
  if (f != NULL) {
    if (f->is_dir) {
      return dir_tell (f->ptr.dir);
    } else {
      return file_tell (f->ptr.file);
    }
  }
  return -1;
}

bool
filesys_isdir (struct FILE *f)
{
  return f->is_dir;
}

int
filesys_inumber (struct FILE *f)
{
  if (f != NULL) {
    if (f->is_dir) {
      return dir_get_inumber (f->ptr.dir);
    } else {
      return file_get_inumber (f->ptr.file);
    }
  }
  return -1;
}

bool
filesys_readdir (struct FILE* f, char *name)
{
  ASSERT (f != NULL);
  ASSERT (f->is_dir);

  return dir_readdir (f->ptr.dir, name);
}

off_t
filesys_length (struct FILE *f)
{
  ASSERT (f != NULL);

  if (f->is_dir) {
    return dir_length (f->ptr.dir);
  } else {
    return file_length (f->ptr.file);
  }
}

/* Formats the file system. */
static void
do_format (void)
{
  printf ("Formatting file system...");
  free_map_create ();
  if (!dir_root_init ())
    PANIC ("root directory creation failed");
  free_map_close ();
  printf ("done.\n");
}

/* Get last part in PATH. */
static bool
get_last_part (char last_part[NAME_MAX + 1], const char *path)
{
  int ret;
  while ((ret = get_next_part (last_part, &path))) {
    if (ret == -1)
      return false;
  }
  return true;
}


/* Get last dir in PATH. */
static struct dir *
get_last_dir (struct dir *dir, const char *path)
{
  ASSERT (dir != NULL);

  if (is_last_part (path))
    return dir;

  int ret;
  struct dir *dir_ = dir;
  char part_name[NAME_MAX + 1];
  struct inode *inode;

  while ((ret = get_next_part (part_name, &path))) {
    if (ret == -1) {
      dir_close (dir_);
      return NULL;
    }

    if (is_last_part (path)) {
      return dir_;
    }
    
    if (!dir_lookup (dir_, part_name, &inode)) {
      dir_close (dir_);
      return NULL;
    } else {
      dir_close (dir_);
      if ((dir_ = dir_open (inode)) == NULL)
        return NULL;
    }
  }
  /* Won't reach here. */
  ASSERT (true);
  return NULL;
}


/* If PART is last path part. */
static bool
is_last_part (const char *path)
{
  if (*path == '/') {
    while (*path == '/')
      ++path;
    if (*path == '\0')
      return true;
  } else {
    while (*path != '/' && *path != '\0')
      ++path;
    if (*path == '\0')
      return true;
  }
  return false;
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
