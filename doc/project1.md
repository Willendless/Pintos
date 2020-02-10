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
    + using STRTOK_R to get every argument from FILE_NAME using the first one as argument to LOAD() file and after successful LOAD() push all into stack frame 

#### Algorithms

1. in PROCESS_EXECUTE() before calling THREAD_CREATE(), get the correct filename from FILE_NAME and pass it into THREAD_CREATE().
    + using STRPBRK() and STRLCPY()

2. in START_PROCESS() before calling LOAD(), get the correct filename from FILE_NAME_ and pass it into LOAD(), then after return from LOAD(), if LOAD() successfully return, using STRTOK_R to get every token from FILE_NAME_ and push them into stack
    1. argv[.][.] push every string in reverse order using strlcopy
    2. align stack so that satisfy 16-byte align requirement
    4. argv[argc]: push null
    3. argv[.]: push every string address according to the length of every string 
    3. argv: push address of argv[0]
    4. ra: push 0 as dumb return address

#### Synchronization

1. temporary

2. fn_copy

#### Rationale

1. using local variable but not global variable to store temporary string token
    + if using local variable, it can be released automatically after function return
    + if using dynamic memory, it must be guaranteed to be released before THREAD_EXIT()

2. using function provided by string.c not write them from scratch


### Task 2: Process Control Syscalls

#### Data structures and functions

1. SYSCALL_HANDLER
    ```c
    static void syscall_handler (struct intr_frame *f UNUSED);
    ```
    1. verify esp
    2. dispatch syscall according to argv[0]
    3. verify argument if syscall has pointer argument

2. struct thread
    ```c
    struct thread
    {
        /* Owned by thread.c. */
        tid_t tid;                          /* Thread identifier. */
        enum thread_status status;          /* Thread state. */
        char name[16];                      /* Name (for debugging purposes). */
        uint8_t *stack;                     /* Saved stack pointer. */
        int priority;                       /* Priority. */
        struct list_elem allelem;           /* List element for all threads list. */

        /* Shared between thread.c and synch.c. */
        struct list_elem elem;              /* List element. */ 

    #ifdef USERPROG
        /* Owned by userprog/process.c. */
        uint32_t *pagedir;                  /* Page directory. */

        /*modified*/
        struct list childs;        
        struct semaphore parent_wait;
        struct list_elem child_elem;
    #endif

        /* Owned by thread.c. */
        unsigned magic;                     /* Detects stack overflow. */
    };
    ```
3. PROCESS_EXEUTE
    ```c
    tid_t thread_create (const char *name, int priority,
               thread_func *function, void *aux);
    ```
    + initialize CHILDS list
    + init the child's semaphore to 0 not the temporary
    + add the child thread's CHILD_ELEM into current_thread's CHILDS list

    ```c
    tid_t process_execute (const char *file_name);
    ```
    + if child thread creates successfully, then parent thread SEMA_DOWN(child), other wise return -1

    ```c
    static void start_process (void *file_name_);
    ```
    + after child thread successfully load program file into memory, SEMA_UP(parent_wait) 

4. PROCESS_WAIT
    ```c
    int process_wait (tid_t child_tid UNUSED);
    ```


#### Algorithms

1. Accessing user memory(pointer verify)
    + if null
    + if point to user addr 
        + using IS_USER_ADDR()
    + if point to mapped page 
        + using PAGEDIR_GET_PAGE()
    + if cross boundary
        + verify address of last byte

2. exec
    + verify argument
    + 

#### Synchronization

#### Rationale

1. initialize the PARENT_WAIT to 0
    + loading child program into memory can happen only once 

### Task 3: File Operation Syscalls

#### Data structures and functions

#### Algorithms

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
