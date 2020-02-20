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
#include "userprog/process.h"
#include <stdbool.h>

static void syscall_handler (struct intr_frame *);
static bool verify_addr (const void *, size_t);
static bool verify_pid (pid_t);
static bool verify_fd (int);

/* process control syscalls */
int syscall_practice (int);
void syscall_halt (void);
tid_t syscall_exec (const char* cmd_line);
int syscall_wait (tid_t tid);
void syscall_exit (int status);

/* file operations syscalls */

static struct lock fs_lock;

bool syscall_create (const char*, unsigned initial_size);
bool syscall_remove (const char*);
int syscall_open (const char*);
int syscall_filesize (int fd);
int syscall_read (int fd, void* buffer, unsigned size);
int syscall_write (int fd, const void* buffer, unsigned size);
void syscall_seek (int fd, unsigned position);
unsigned syscall_tell (int fd);
void syscall_close (int fd);

void
syscall_init (void)
{
  lock_init (&fs_lock);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

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
    case SYS_PRACTICE:
    case SYS_EXIT:
    case SYS_EXEC:
    case SYS_WAIT:
    case SYS_REMOVE:
    case SYS_OPEN:
    case SYS_FILESIZE:
    case SYS_TELL:
    case SYS_CLOSE:
      bad_args = !verify_addr (args + 4, sizeof(uint32_t*));
      break;
    case SYS_CREATE:
    case SYS_SEEK:
      bad_args = !verify_addr (args + 4, 2*sizeof(uint32_t*));
      break;
    case SYS_READ:
    case SYS_WRITE:
      bad_args = !verify_addr (args + 4, 3*sizeof(uint32_t*));
      break;
    default:
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
    // case SYS_WAIT:
    //   syscall_wait();
    //   break;
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
    }
}

/* process control syscalls */
int syscall_practice (int arg) 
{
  return arg + 1;
}

void syscall_halt () 
{
  shutdown_power_off();
}

void syscall_exit (int status)
{
  struct thread *t = thread_current();
  printf ("%s: exit(%d)\n", &t->name, status);
  //t->wait_status->exit_code = status;
  thread_exit();
}


tid_t syscall_exec (const char* cmd_line)
{
  int i = 0, len = 0;
  tid_t tid = -1;
  // TODO: do not know the length of cmd_line
  do {
   if (!verify_addr (cmd_line + i, 1))
    syscall_exit (-1);
  } while (cmd_line[i++] != '\0' && len < 128);
  tid = process_execute(cmd_line);   
  return tid;
}

// TODO
int syscall_wait (tid_t tid);

/* file operations syscalls */

bool syscall_create (const char *file, unsigned initial_size)
{
  int i = 0, len = 0;
  bool result = false;
  do {
    if (!verify_addr (file + i, 1))
      syscall_exit (-1);
  } while (file[i++] != '\0' && len < 128);
  lock_acquire (&fs_lock);
  result = filesys_create (file, initial_size);
  lock_release (&fs_lock);
  return result;
}

bool syscall_remove (const char *file)
{
  int i = 0, len = 0;
  bool result = false;
  do {
    if (!verify_addr (file + i, 1))
      syscall_exit (-1);
  } while (file[i++] != '\0' && len < 128);
  lock_acquire (&fs_lock);
  result = filesys_remove (file);
  lock_release (&fs_lock);
  return result;
}

int syscall_open (const char *file)
{
  int i = 0, len = 0;
  struct file *f;
  int fd = 2;
  do {
    if (!verify_addr (file + i, 1))
      syscall_exit (-1);
  } while (file[i++] != '\0' && len < 128);
  lock_acquire (&fs_lock);
  f = filesys_open (file);
  lock_release (&fs_lock);
  if (f == NULL)
    return -1;
  while (fd < MAX_OPEN_FILES && thread_current ()->open_files[fd] != NULL)
    ++fd;
  if (fd < MAX_OPEN_FILES)
    thread_current ()->open_files[fd] = f;
  else 
    fd = -1;
  return fd;
}

int syscall_filesize (int fd)
{
  int result = -1;
  struct file *f;
  if (!verify_fd (fd) || fd == 0 || fd == 1)
    syscall_exit (-1);
  f = thread_current ()->open_files[fd];
  lock_acquire (&fs_lock);
  result = file_length (f);
  lock_release (&fs_lock);
  return result;
}

int syscall_read (int fd, void* buffer, unsigned size)
{
  int read_len = -1;
  struct thread *t = thread_current ();
  if (size == 0)
    return 0;
  if (!verify_addr (buffer, size) || !verify_fd (fd)) 
    {
      syscall_exit (-1);
    } 
  switch (fd)
    {
      case 0:
        break;
      case 1:
        break;
      default:
        ASSERT(t->open_files != NULL);
        struct file *f = t->open_files[fd];
        lock_acquire (&fs_lock);
        read_len = file_read (f, buffer, size);
        lock_release (&fs_lock);
    }
  return read_len;
}


int syscall_write (int fd, const void* buffer, unsigned size) 
{
  int write_len = -1;
  struct thread *t = thread_current ();
  if (size == 0)
    return 0;
  if (!verify_addr (buffer, size) || !verify_fd (fd)) 
    {
      syscall_exit (-1);
    } 
  switch (fd)
    {
      case 0:
        break;
      case 1:
        putbuf (buffer, size);
        write_len = size;
        break;
      default:
        ASSERT(t->open_files != NULL);
        struct file *f = t->open_files[fd];
        lock_acquire (&fs_lock);
        write_len = file_write (f, buffer, size);
        lock_release (&fs_lock);
    }
  return write_len;
}

void syscall_seek (int fd, unsigned position)
{
  struct file *f;
  if (!verify_fd (fd) || fd == 0 || fd == 1)
    return;
  f = thread_current ()->open_files[fd];
  lock_acquire (&fs_lock);
  file_seek (f, position);
  lock_release (&fs_lock);
}

unsigned syscall_tell (int fd)
{
  struct file *f;
  unsigned offset;
  if (!verify_fd (fd) || fd == 0 || fd == 1)
    syscall_exit (-1);
  f = thread_current ()->open_files[fd];
  lock_acquire (&fs_lock);
  offset = file_tell (f);
  lock_release (&fs_lock);
  return offset;
}

void syscall_close (int fd)
{
  struct file *f;
  if (!verify_fd (fd) || fd == 0 || fd == 1)
    return;
  f = thread_current ()->open_files[fd];
  lock_acquire (&fs_lock);
  file_close (f);
  lock_release (&fs_lock);
  thread_current ()->open_files[fd] = NULL;
}