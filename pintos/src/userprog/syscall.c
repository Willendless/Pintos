#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "devices/shutdown.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "filesys/buffer-cache.h"
#include "userprog/process.h"
#include <stdbool.h>

#define READDIR_MAX_LEN 14

static void syscall_handler (struct intr_frame *);
static bool verify_addr (const void *, size_t);
static bool verify_tid (tid_t);
static bool verify_fd (int);
static bool verify_str (const char *);

struct lock fs_lock;

void
syscall_init (void)
{
  lock_init (&fs_lock);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

/* helper functions to verify variables */

static bool
verify_addr (const void *p, size_t len)
{
  ASSERT (len > 0);
  struct thread *t = thread_current();
  if (p == NULL)
    return false;
  else if (!is_user_vaddr (p) || !is_user_vaddr (p + len - 1))
    return false;
  else if (pagedir_get_page (t->pagedir, p) == NULL
          || pagedir_get_page (t->pagedir, p + len - 1) == NULL)
    return false;
  else
    return true;
}

static bool
verify_fd (int fd)
{
  return (fd == 0) || (fd == 1) || ((fd > 1) && (fd < MAX_OPEN_FILES) 
          && (thread_current ()->open_files[fd] != NULL));
}

static bool
verify_str (const char *str)
{
  const int MAX_LEN = 128;
  int len = 0;
  if (str == NULL)
    return false;
  do {
    if(!verify_addr(str, sizeof(char)))
      return false;
  } while (*(str++)!='\0' && (++len) < MAX_LEN);
  return true;
}

static bool
verify_tid (tid_t tid) 
{
  struct list_elem *e;
  struct thread *cur = thread_current();
  for (e = list_begin (&cur->children); e != list_end (&cur->children); e = list_next (e)) {
    struct wait_status *ws = list_entry (e, struct wait_status, elem);
    if (ws->tid == tid) {
      return true;
    }
  } 
  return false;
}

static void
syscall_handler (struct intr_frame *f)
{
  bool bad_args = false;
  uint32_t* args = ((uint32_t*) f->esp);

  /*
   * The following print statement, if uncommented, will print out the syscall
   * number whenever a process enters a system call. You might find it useful
   * when debugging. It will cause tests to fail, however, so you should not
   * include it in your final submission.
   */

  //printf("System call number: %d\n", args[0]);

  /* verify syscall number */
  if (!verify_addr(args, sizeof(uint32_t*))) {
    syscall_exit (-1);
  }

  /* verify arguments addr*/
  switch (args[0])
    {
    case SYS_HALT:
    case SYS_CACHE_FLUSH:
    case SYS_BWCNT:
    case SYS_BRCNT:
      /* have no argument */
      break;
    case SYS_PRACTICE:
    case SYS_EXIT:
    case SYS_EXEC:
    case SYS_WAIT:
    case SYS_REMOVE:
    case SYS_OPEN:
    case SYS_FILESIZE:
    case SYS_TELL:
    case SYS_CLOSE:
    case SYS_CHDIR:
    case SYS_MKDIR:
    case SYS_ISDIR:
    case SYS_INUMBER:
      /* these cases have one argument */
      bad_args = !verify_addr (args + 4, sizeof(uint32_t*));
      break;
    case SYS_CREATE:
    case SYS_SEEK:
    case SYS_READDIR:
      /* these cases have two arguments */
      bad_args = !verify_addr (args + 4, 2*sizeof(uint32_t*));
      break;
    case SYS_READ:
    case SYS_WRITE:
    case SYS_CACHE_STAT:
      /* these cases have three arguments */
      bad_args = !verify_addr (args + 4, 3*sizeof(uint32_t*));
      break;
    default:
      /* Incorrect syscall number */
      bad_args = true;
    }

  if (bad_args)
    {
      syscall_exit(-1);
    }

  /* call syscall_func */
  switch (args[0]) 
    {
    case SYS_PRACTICE: 
      f->eax = syscall_practice (args[1]); 
      break; 
    case SYS_HALT: 
      syscall_halt (); 
      break;
    case SYS_EXIT: 
      syscall_exit (args[1]);
      break;
    case SYS_EXEC: 
      f->eax = syscall_exec ((char *)args[1]); 
      break;
    case SYS_WAIT:
      f->eax = syscall_wait ((tid_t)args[1]);
      break;
    case SYS_CREATE: 
      f->eax = syscall_create ((char *)args[1], args[2]);
      break;
    case SYS_REMOVE:
      f->eax = syscall_remove ((char *)args[1]);
      break;
    case SYS_OPEN:
      f->eax = syscall_open ((char *)args[1]);
      break;
    case SYS_FILESIZE:
      f->eax = syscall_filesize (args[1]);
      break;
    case SYS_READ:
      f->eax = syscall_read (args[1], (char *)args[2], args[3]);
      break;
    case SYS_WRITE: 
      f->eax = syscall_write (args[1], (char *)args[2], args[3]); 
      break;
    case SYS_SEEK:
      syscall_seek (args[1], args[2]);
      break;
    case SYS_TELL:
      f->eax = syscall_tell (args[1]);
      break;
    case SYS_CLOSE:
      syscall_close (args[1]);
      break;
    case SYS_CHDIR:
      f->eax = syscall_chdir ((char *)args[1]);
      break;
    case SYS_MKDIR:
      f->eax = syscall_mkdir ((char *)args[1]);
      break;
    case SYS_READDIR:
      f->eax = syscall_readdir (args[1], (char *)args[2]);
      break;
    case SYS_ISDIR:
      f->eax = syscall_isdir (args[1]);
      break;
    case SYS_INUMBER:
      f->eax = syscall_inumber (args[1]);
      break;
    case SYS_CACHE_FLUSH:
      syscall_cache_flush ();
      break;
    case SYS_CACHE_STAT:
      syscall_cache_stat (args[1], args[2], args[3]);
      break;
    case SYS_BRCNT:
      f->eax = syscall_brcnt ();
      break;
    case SYS_BWCNT:
      f->eax = syscall_bwcnt ();
      break;
    default:
      ASSERT (false);
    }
}

/* process control syscalls */

int
syscall_practice (int arg) 
{
  return arg + 1;
}

void
syscall_halt () 
{
  shutdown_power_off();
}

void
syscall_exit (int status)
{
  struct thread *t = thread_current();
  printf ("%s: exit(%d)\n", t->name, status);
  t->wait_status->exit_code = status;
  thread_exit();
}

tid_t
syscall_exec (const char* cmd_line)
{
  tid_t tid = -1;
  if (!verify_str(cmd_line))
    syscall_exit(-1);
  tid = process_execute (cmd_line);
  return tid;
}

int
syscall_wait (tid_t tid)
{
  int exit_code = 0;
  exit_code = process_wait (tid);
  return exit_code;
}

/* file operations syscalls */

bool
syscall_create (const char *file, unsigned initial_size)
{
  bool result = false;
  if(!verify_str(file))
    syscall_exit(-1);
  result = filesys_create (file, initial_size);
  return result;
}

bool
syscall_remove (const char *file)
{
  bool result = false;
  if(!verify_str(file))
    syscall_exit(-1);
  result = filesys_remove (file);
  return result;
}

int
syscall_open (const char *file)
{
  struct FILE *f;
  int fd = 2;
  if(!verify_str(file))
    syscall_exit(-1);
  f = filesys_open (file);
  
  if (f == NULL)
    return -1;
  while (fd < MAX_OPEN_FILES && thread_current ()->open_files[fd] != NULL)
    ++fd;
  if (fd < MAX_OPEN_FILES)
    thread_current ()->open_files[fd] = f;
  else {
    filesys_close(f);
    fd = -1;
  }
    
  return fd;
}

int
syscall_filesize (int fd)
{
  int result = -1;
  struct FILE *f;
  if (!verify_fd (fd) || fd == 0 || fd == 1)
    syscall_exit (-1);
  f = thread_current ()->open_files[fd];
  result = filesys_length (f);
  return result;
}

int
syscall_read (int fd, void* buffer, unsigned size)
{
  int read_len = -1;
  struct thread *t = thread_current ();
  if (size == 0)
    return 0;
  if (!verify_addr (buffer, size) || !verify_fd (fd)) 
    syscall_exit (-1);
  switch (fd)
    {
      case 0:
        // Read from stdin
        read_len = 0;
        while (read_len < size){
          ((char*)buffer)[read_len++] = (char)input_getc();
        }
        break;
      case 1:
        // Nothing happens if read from stdout
        break;
      default:
        ASSERT(t->open_files != NULL);
        struct FILE *f = t->open_files[fd];
        if (filesys_isdir(f)) {
          return 0;
        }
        struct file *file = f->ptr.file;
        read_len = file_read (file, buffer, size);
    }
  return read_len;
}

int
syscall_write (int fd, const void* buffer, unsigned size) 
{
  int write_len = -1;
  struct thread *t = thread_current ();
  if (size == 0)
    return 0;
  if (!verify_addr (buffer, size) || !verify_fd (fd)) 
    syscall_exit (-1);
  switch (fd)
    {
      case 0:
        // Nothing happens if write to stdin
        break;
      case 1:
        // Write to stdout
        putbuf (buffer, size);
        write_len = size;
        break;
      default:
        ASSERT (t->open_files != NULL);
        struct FILE *f = t->open_files[fd];
        if (filesys_isdir (f)) {
          return -1;
        }
        struct file *file = f->ptr.file;
        write_len = file_write (file, buffer, size);
    }
  return write_len;
}

void
syscall_seek (int fd, unsigned position)
{
  struct file *f;
  if (!verify_fd (fd) || fd == 0 || fd == 1)
    return;
  f = thread_current ()->open_files[fd]->ptr.file;
  file_seek (f, position);
}

unsigned
syscall_tell (int fd)
{
  struct FILE *f;
  unsigned offset;
  if (!verify_fd (fd) || fd == 0 || fd == 1)
    syscall_exit (-1);
  f = thread_current ()->open_files[fd];
  offset = filesys_tell (f);
  return offset;
}

void
syscall_close (int fd)
{
  struct FILE *f;
  if (!verify_fd (fd) || fd == 0 || fd == 1)
    return;
  f = thread_current ()->open_files[fd];
  filesys_close (f);
  thread_current ()->open_files[fd] = NULL;
}

bool
syscall_mkdir (const char *dir)
{
  if (!verify_str (dir))
    return false;
  return filesys_mkdir(dir);
}

bool
syscall_chdir (const char *dir)
{
  if (!verify_str (dir))
    return false;
  return filesys_chdir(dir);
}

bool
syscall_readdir (int fd, char *name)
{
  if (!verify_fd (fd) || fd == 0 || fd == 1)
    syscall_exit (-1);
  if (!verify_addr (name, READDIR_MAX_LEN + 1))
    syscall_exit (-1);
  if (!syscall_isdir (fd))
    syscall_exit (-1);
  
  return filesys_readdir (thread_current ()->open_files[fd], name);
}

bool
syscall_isdir (int fd)
{
  if (!verify_fd (fd) || fd == 0 || fd == 1)
    syscall_exit (-1);
  return filesys_isdir (thread_current ()->open_files[fd]);
}

int
syscall_inumber (int fd)
{
  if (!verify_fd (fd) || fd == 0 || fd == 1)
    syscall_exit (-1);
  
  return filesys_inumber (thread_current ()->open_files[fd]);
}

void
syscall_cache_flush ()
{
  cache_flush (fs_device);
}

void
syscall_cache_stat (int *hit_cnt, int *read_cnt, int *write_cnt)
{
  if (!verify_addr (hit_cnt, sizeof (uint32_t))
      || !verify_addr (read_cnt, sizeof (uint32_t))
      || !verify_addr (write_cnt, sizeof (uint32_t)))
      syscall_exit (-1);
  cache_stat (hit_cnt, read_cnt, write_cnt);
}

unsigned long long
syscall_brcnt ()
{
  return block_read_cnt (fs_device);
}

unsigned long long
syscall_bwcnt ()
{
  return block_write_cnt (fs_device);
}
