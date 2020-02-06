Design Document for Project 1: User Programs
============================================

## Group Members

* Jiarui Li <jiaruili@berkeley.edu>
* FirstName LastName <email@domain.example>
* FirstName LastName <email@domain.example>
* FirstName LastName <email@domain.example>

## Design Overview

### Task 1: Argument Passing

#### Data structures and functions

1. PROCESS_EXEUTE
```c
tid_t process_execute (const char *file_name);
```
+ change the FILE_NAME argument to thread_create to the correct FILE_NAME

2.  START_PROCESS
```c
static void start_process (void *file_name_);
```
+ using STRTOK_R to get every argument from FILE_NAME and place them into stack frame.


#### Algorithms

#### Synchronization

#### Rationale

### Task 2: Process Control Syscalls

#### Data structures and functions

1. SYSCALL_INIT
```c
void syscall_init (void);
```

+ init process control syscall

2. SYSCALL_HANDLER
```c
static void syscall_handler (struct intr_frame *f UNUSED);
```

+ verify esp

#### Algorithms

#### Synchronization

#### Rationale

### Task 3: File Operation Syscalls

#### Data structures and functions

1. SYSCALL_INIT
```c
void syscall_init (void);
```

+ init file operation syscall

#### ALgorithms

#### Synchronization

#### Rationale

## Additional Questions

### Question 1

+ name: sc-bad-sp.c
    + in line 18, using x86 assembly code, first store the number  and then executing systam call. 

### Question 2

+ name: sc-boundary-3.c
    + TODO

### Question 3

+ name: remove
    + TODO
