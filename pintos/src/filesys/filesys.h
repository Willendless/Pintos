#ifndef FILESYS_FILESYS_H
#define FILESYS_FILESYS_H

#include <stdbool.h>
#include "filesys/off_t.h"

/* Sectors of system file inodes. */
#define FREE_MAP_SECTOR 0       /* Free map file inode sector. */
#define ROOT_DIR_SECTOR 1       /* Root directory file inode sector. */

struct FILE {
  bool is_dir;
  union {
    struct file *file;
    struct dir *dir;
  } ptr;
};

/* Block device that contains the file system. */
struct block *fs_device;

void filesys_init (bool format);
void filesys_done (void);
bool filesys_create (const char *name, off_t initial_size);
struct FILE *filesys_open (const char *name);
bool filesys_remove (const char *name);
bool filesys_mkdir (const char *dir);
bool filesys_chdir (const char *dir);
void filesys_close (struct FILE *);
bool filesys_isdir (struct FILE *);
int filesys_inumber (struct FILE *);
bool filesys_readdir (struct FILE *, char *name);
off_t filesys_tell (struct FILE *f);
off_t filesys_length (struct FILE *f);


#endif /* filesys/filesys.h */
