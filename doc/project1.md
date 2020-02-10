Design Document for Project 1: User Programs
============================================

## Group Members

* Jiarui Li <jiaruili@berkeley.edu>
* Xiang Zhang <xzhang1048576@berleley.edu>
* FirstName LastName <email@domain.example>
* FirstName LastName <email@domain.example>

## Design Overview

### Task 1: Argument Passing

#### Data structures and functions
*Modifying following functions:*
1. PROCESS_EXEUTE
    ```c
    tid_t process_execute (const char *file_name);
    ```
    + Use the correct file name as the argument to THREA_CREATE()

2.  START_PROCESS
    ```c
    static void start_process (void *file_name_);
    ```
    + Pass the correct file name to LOAD() and after successful LOAD() set user stack frame 

#### Algorithms

1. in PROCESS_EXECUTE() before calling THREAD_CREATE(), get the correct filename from FILE_NAME and pass it into THREAD_CREATE().
    1. use char* res = STRPBRK(FILE_NAME, " ") to detect the first space in FILE_NAME
    2. if res == NULL, assign word_len with strlen(FILE_NAME), otherwise assign word_len with res - FILE_NAME  
    3. use STRLCPY(buf, FILE_NAME, word_len) to copy the program name to allocated char buf[MAX_WORD_LEN]
    4. append the buf with '\0' and use it as the first argument in THREAD_CREATE()

2. in START_PROCESS() before calling LOAD(), get the correct filename from FILE_NAME_ and pass it into LOAD(), then after returning from LOAD(), if LOAD() returned successfully, using STRTOK_R to get every token from FILE_NAME_ and push them into stack. Otherwise free the page and exit this thread.
    1. argv[.][.]: push every string in reverse order using strlcopy
    2. padding: align stack so that satisfy 16-byte align requirement
    3. argv[argc]: push null
    4. argv[.]: push every string address according to the length of every string 
    5. argv: push address of argv[0]
    6. ra: push 0 as dumb return address

#### Synchronization

1. fn_copy

#### Rationale

1. Use local variable but not global variable to store temporary string token
    + Local variable can be released automatically after function return
    + Dynamic memory must be guaranteed to be released before THREAD_EXIT()

2. Use function provided by string.c instead of writing them from scratch


### Task 2: Process Control Syscalls

#### Data structures and functions

*Adding following data structure*:
+ struct wait_status
    ```c
    struct wait_status
    {
        struct list_elem elem;              /* 'children' list elem */
        struct lock lock;                   /* Protects ref_cnt */
        int ref_cnt;                        /* 2=child and parent both alive, 1=either child or parent alive, 0=child and parent both dead */
        tid_t tid;                          /* Child thread id */
        int exit _code;                     /* Child exit code, if dead. */
        struct semaphore dead;              /* 1=child alive, 0=child dead. */
    }
    ```


*Modifying following data structure*:
+ struct thread
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

        /* Modification here*/
        struct list children;               /* Completion status of children. */
        struct wait_status *wait_status;    /* This process's completion status. */
        struct list_elem child_elem;        /* List element for parent childs list. */
    #endif

        /* Owned by thread.c. */
        unsigned magic;                     /* Detects stack overflow. */
    };
    ```

*Adding following functions:*
+ PRACTICE
    ```c
    int syscall_practice(int arg) {
        return arg + 1;
    }
    ```
    + Return arg plus one.

+ HALT
    ```c
    void syscall_halt(void) {
        shut_down_power_off ();
    }
    ```
    + Terminate the system.

+ EXEC
    ```c
    pid_t syscall_exec(const char* cmd_line) {
        pit_t pid = -1;
        if (pointer_valid (cmd_line))
            pid = process_exec (cmd_line);
        return pid;
    }
    ```
    + Run the program in cmd_line with subsequent arguments, returning its program id.

+ EXIT
    ```c
    void syscall_exit(int status) {
        thread_current ()->exit_status = status;
        printf ("%s: exit(%d)\n", &thread_current ()->name, args[1]);
        thread_exit ();
    }
    ```
    + Terminate current user program and return to kernel.

+ WAIT
    ```c
    int syscall_wait(pid_t pid) {
        return process_wait ();
    }
    ```
    + Wait for process pid to finish and get its return value.

*Modifying following function:*
+ SYSCALL_HANDLER
    ```c
    static void syscall_handler (struct intr_frame *f UNUSED);
    ```
    + Dispatch system call.
+ PROCESS_EXEUTE
    ```c
    tid_t thread_create (const char *name, int priority,
               thread_func *function, void *aux);
    ```
    + Initialize struct THREAD for child process.
+ PROCESS_EXECUTE
    ```c
    tid_t process_execute (const char *file_name);
    ```
    + Synchronize parent and child process, parent process waits for child's LOAD().
+ START_PROCESS
    ```c
    static void start_process (void *file_name_);
    ```
    + Synchronize parent and child process
+ PROCESS_WAIT
    ```c
    int process_wait (tid_t child_tid UNUSED);
    ```
    + Verify TID and block in struct semaphore PARENT_WAIT.
5. PROCESS_EXIT
    ```c
    void process_exit (void);
    ```
    + Release resources after process exits.

#### Algorithms

1. syscall_handler
    1. verify the pointer f->esp
        + if null
        + if point to user addr 
            + using IS_USER_ADDR()
        + if point to mapped page 
            + using PAGEDIR_GET_PAGE()
        + if cross boundary
            + verify address of last byte
    2. dispatch syscall according to syscall number in lib/syscall-nr.h
    3. verify pointer before execute syscall
2. exec
    + call PROCESS_EXECUTE()
        1. init CHILDS list
        2. init semaphore PARENT_WAIT
        3. add CHILD_ELEM into current's CHILDS
3. exit
    + call PROCESS_EXIT()
        1. for every thread struct in the CHILD list, release it if its state is THREAD_DYING
        2. change status of current thread
        3. if parent_wait->waiters is not empty, SEMA_UP(parent_wait)
4. wait
    1. call PROCESS_WAIT()
        1. if CHILD_TID is not a child thread or invalid(search it in the CHILD list), return -1, otherwise find the child's thread struct
        2. SEMA_DOWN(parent_wait)
    2. after child changes to THREAD_DYING, release its struct THREAD from CHILD lists
 

#### Synchronization

1. struct semaphore parent_wait;

#### Rationale

1. initialize the PARENT_WAIT to 0
    + loading child program into memory can happen only once 
    + only parent can call wait and block in the semaphore
2. in PROCESS_EXIT(), first release dying child then change thread_status and at last release waiting parent if it exists. Also, wait until the state of child thread changes to THREAD_DYING then release its TLB
    + if releasing waiting parent first, parent may release child's TLB before child release other resources

### Task 3: File Operation Syscalls

#### Data structures and functions

*Adding following data structure*:
+ file lock
    ```c
    static struct lock file_lock;           /* Filesystem operation global lock */
    ```


*Modifying following data structure*:
+ struct thread
    ```c
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

        /* Modification here*/
        struct list children;               /* Completion status of children. */
        struct wait_status *wait_status;    /* This process's completion status. */
        struct list_elem child_elem;        /* List element for parent childs list. */
    #endif

        /* Owned by thread.c. */
        unsigned magic;                     /* Detects stack overflow. */
    };
    ```
    ```

*Adding following functions:*
+ SYSCALL_CREATE
    ```c
    bool syscall_create(const char *file, unsigned initial_size) {

    }
    ```

+ SYSCALL_REMOVE
    ```c
    bool syscall_remove(const char *file) {

    }
    ```

+ SYSCALL_OPEN
    ```c
    int syscall_open(const char *file) {

    }
    ```

+ SYSCALL_FILESIZE
    ```c
    int filesize(int fd) {

    }
    ```

+ SYSCALL_READ
    ```c
    int syscall_read(int fd, const void *buffer, unsigned size) {

    }
    ```

+ SYSCALL_WRITE
    ```c
    int syscall_write(int fd, const void *buffer, unsigned size) {

    }
    ```

+ SYSCALL_SEEK
    ```c
    void seek(int fd, unsigned position) {

    }
    ```

+ SYSCALL_TELL
    ```c
    unsigned tell(int fd) {

    }
    ```

+ SYSCALL_CLOSE
    ```c
    void close(int fd) {

    }
    ```

*Modifying following functions:*




#### Algorithms

#### Synchronization

#### Rationale

1. Using lock but not semaphore
    + only the owner of lock can release it.

## Additional Questions

### Question 1

name: sc-bad-sp.c
+ in line 18, using inline assembly code, first store a invalid number into *%esp* and then executing systam call. 

### Question 2

+ name: sc-boundary-3.c
    + TODO

### Question 3

+ name: remove
    + TODO
