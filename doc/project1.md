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
Modifying following functions:
1. PROCESS_EXEUTE
    ```c
    tid_t process_execute (const char *file_name);
    ```
    + It should be the program name instead of FILE_NAME as the name of the new thread in thread_create()

2.  START_PROCESS
    ```c
    static void start_process (void *file_name_);
    ```
    + Use STRTOK_R to get every argument from FILE_NAME using the first one as argument to LOAD() file and after successful LOAD() push all into stack frame 

#### Algorithms

1. in PROCESS_EXECUTE() before calling THREAD_CREATE(), get the correct filename from FILE_NAME and pass it into THREAD_CREATE().
    1. use char* res = STRPBRK(FILE_NAME, " ") to detect the first space in FILE_NAME
    2. if res == NULL, assign word_len with strlen(FILE_NAME), otherwise assign word_len with res - FILE_NAME  
    3. use STRLCPY(buf, FILE_NAME, word_len) to copy the program name to allocated char buf[MAX_WORD_LEN]
    4. append the buf with '\0' and use it as the first argument in THREAD_CREATE()

2. in START_PROCESS() before calling LOAD(), get the correct filename from FILE_NAME_ and pass it into LOAD(), then after returning from LOAD(), if LOAD() returned successfully, using STRTOK_R to get every token from FILE_NAME_ and push them into stack. Otherwise free the page and exit this thread.
    1. argv[.][.] push every string in reverse order using strlcopy
    2. align stack so that satisfy 16-byte align requirement
    3. argv[argc]: push null
    4. argv[.]: push every string address according to the length of every string 
    5. argv: push address of argv[0]
    6. ra: push 0 as dumb return address

#### Synchronization

1. temporary
2. fn_copy

#### Rationale

1. Use local variable but not global variable to store temporary string token
    + Local variable can be released automatically after function return
    + Dynamic memory must be guaranteed to be released before THREAD_EXIT()

2. Use function provided by string.c instead of writing them from scratch


### Task 2: Process Control Syscalls

#### Data structures and functions
Modifying following functions:
+ SYSCALL_HANDLER
    ```c
    static void syscall_handler (struct intr_frame *f UNUSED);
    ```
    1. verify esp
    2. dispatch syscall according to argv[0]
    3. verify argument if syscall has pointer argument

+ PROCESS_EXECUTE
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

+ PROCESS_WAIT
    ```c
    int process_wait (tid_t child_tid UNUSED);
    ```

Adding following functions:
+ PRACTICE
    ```c
    int practice(int arg);
    ```
    Increase arg by 1

+ HALT
    ```c
    void halt(void);
    ```
    Terminate the system

+ EXEC
    ```c
    pid_t exec(const char* cmd_line);
    ```
    Run the program in cmd_line with subsequent arguments, returning its program id

+ EXIT
    ```c
    void exit(int status);
    ```
    Terminate current user program and return to kernel

+ WAIT
    ```c
    int wait(pid_t pid);
    ```
    Wait for process pid to finish and get its return value

Modifying following data structure:
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
        struct list childs;
        struct semaphore parent_wait;
        struct list_elem child_elem;
    #endif

        /* Owned by thread.c. */
        unsigned magic;                     /* Detects stack overflow. */
    };
    ```


#### Algorithms

1. in syscall_handler() after args is assigned, add some verification. Then call corresponding syscall according to argv[0]. Verify argv[1] if it is the argument of syscall.
    1. Verify the pointer f->esp
        + if null
        + if point to user addr 
            + using IS_USER_ADDR()
        + if point to mapped page 
            + using PAGEDIR_GET_PAGE()
        + if cross boundary
            + verify address of last byte
    2. Call syscall and verify argument
        + switch args[0]
        + if syscall requires an argument, verify args[1] with steps in #1
        + call syscall

2. practice
    + return arg+1

3. halt
    + call shutdown_power_off()

4. exec
    + //TODO:implement exec

5. exit
    + //TODO:implement exit

6. wait
    + //TODO:implement wait

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
