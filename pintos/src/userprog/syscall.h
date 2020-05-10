#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include <stdbool.h>
typedef int tid_t;

void syscall_init (void);

/* process control syscalls */
int syscall_practice (int);
void syscall_halt (void);
tid_t syscall_exec (const char* cmd_line);
int syscall_wait (tid_t tid);
void syscall_exit (int status);


/* file operations syscalls */
bool syscall_create (const char*, unsigned);
bool syscall_remove (const char*);
int syscall_open (const char*);
int syscall_filesize (int);
int syscall_read (int, void*, unsigned);
int syscall_write (int, const void*, unsigned);
void syscall_seek (int, unsigned);
unsigned syscall_tell (int);
void syscall_close (int);
bool syscall_mkdir (const char*);
bool syscall_chdir (const char*);
bool syscall_readdir (int fd, char *name);
bool syscall_isdir (int fd);
int syscall_inumber (int fd);

/* buffer cache backend. */
void syscall_cache_flush (void);
void syscall_cache_stat (int *, int *, int *);

unsigned long long syscall_bwcnt (void);
unsigned long long syscall_brcnt (void);

#endif /* userprog/syscall.h */
