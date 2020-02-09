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
+ correct the FILE_NAME argument passed to thread_create

2.  START_PROCESS
```c
static void start_process (void *file_name_);
```
+ using STRTOK_R to get first argument from FILE_NAME to load file and push all into stack frame 

#### Algorithms

1. in PROCESS_EXECUTE() before calling THREAD_CREATE(), get the correct filename from FILE_NAME and pass it into THREAD_CREATE().
    + using STRPBRK() and STRLCPY()

2. in START_PROCESS() before calling LOAD(), get the correct filename from FILE_NAME_ and pass it into LOAD(), then after return from LOAD(), if LOAD() successfully return, using STRTOK_R to get every token from FILE_NAME_ and push them into stack
    1. argv[.][.] push every string in reverse order using strlcopy
    2. align stack so that satisfy 16-byte align requirement
    4. argv[argc]: push null
    3. argv[.]: push every string address according to the length of every string 
    3. argv: push address of argv[0]
    4. ra: push 0 as dump return address

#### Synchronization

1. temporary

2. fn_copy

#### Rationale

1. using local variable but not global variable to store temporary token
    + if using local variable, it can be released automatically after function return
    + if using dynamic memory, it must be guaranteed to be released before THREAD_EXIT()

2. using function provided by string.c not write them from scratch


### Task 2: Process Control Syscalls

#### Data structures and functions

1. SYSCALL_HANDLER
```c
static void syscall_handler (struct intr_frame *f UNUSED);
```

+ verify esp



#### Algorithms

#### Synchronization

#### Rationale

### Task 3: File Operation Syscalls

#### Data structures and functions

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
