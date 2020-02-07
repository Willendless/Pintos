Design Document for Project 1: User Programs
============================================

## Group Members

* Jiarui Li <jiaruili@berkeley.edu>
* Xiang Zhang <xzhang1048576@berkeley.edu>
* Joel Kattapuram <joelkattapuram@berkeley.edu>
* Kallan Bao <kallanbao@berkeley.edu>

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

*Adding following data structure:*
+ struct wait_status
    ```c
    struct wait_status
    {
        struct list_elem elem;              /* 'children' list elem */
        struct lock lock;                   /* Protects ref_cnt */
        int ref_cnt;                        /* 2=child and parent both alive, 1=either child or parent alive, 0=child and parent both dead */
        tid_t tid;                          /* Child thread id */
        int exit_code;                     /* Child exit code, if dead. */
        struct semaphore dead;              /* 1=child alive, 0=child dead. */
    }
    ```

*Modifying following data structure:*
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
        thread_current ()->wait_status->exit_code = status;
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

*Modifying following functions*
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
*Adding following global variable:*
+ lock file_system_lock
    ```c
    static struct lock file_system_lock;           /* Filesystem operation global lock */
    ```

*Modifying following data structure:*
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
        file * open_files[MAX_OPEN_FILES];		/* An array storing file* which is opened in the process. */

    #endif

        /* Owned by thread.c. */
        unsigned magic;                     /* Detects stack overflow. */
    };
    ```

*Adding following functions:*
+ SYSCALL_CREATE
    ```c
    bool syscall_create(const char *file, unsigned initial_size);
    ```
    Create a new file named *file* and initialize its size as initial_size bytes. Return true if succeeded, otherwise false.

+ SYSCALL_REMOVE
    ```c
    bool syscall_remove(const char *file);
    ```
    Delete the file named *file*. Return true if succeeded, otherwise false.

+ SYSCALL_OPEN
    ```c
    int syscall_open(const char *file);
    ```
    Open the file named file_name and return its file descriptor, return -1 if failed.

+ SYSCALL_FILESIZE
    ```c
    int filesize(int fd);
    ```
    Return the size of the file opened as fd.

+ SYSCALL_READ
    ```c
    int syscall_read(int fd, void *buffer, unsigned size);
    ```
    Read size of bytes from the file opened as fd and store it in the buffer. Return the size actually read, or -1 if could not read.

+ SYSCALL_WRITE
    ```c
    int syscall_write(int fd, const void *buffer, unsigned size);
    ```
    Write size of bytes from the buffer to the file opened as fd. Return the size actually write, or -1 if could not write.

+ SYSCALL_SEEK
    ```c
    void syscall_seek(int fd, unsigned position);
    ```
    Change the file pointer to the beginning of the file + position offset.

+ SYSCALL_TELL
    ```c
    unsigned syscall_tell(int fd);
    ```
    Return the offset of the file pointer to the beginning of the file.

+ SYSCALL_CLOSE
    ```c
    void syscall_close(int fd);
    ```
    Close the file opened as fd.

*Modifying following functions:*
+ SYSCALL_HANDLER
    ```c
    static void syscall_handler (struct intr_frame *f UNUSED);
    ```
    Add switch cases for file syscalls

+ LOAD
    ```c
    bool load (const char *file_name, void (**eip) (void), void **esp)
    ```
    Aquire file_system_lock before opening the executable and release it after the file is closed

#### Algorithms
+ First, set file_system_lock as a global lock in syscall.c

+ Following modifications in syscall_handler() in Task 2, we shall add more switch options 
    1. Verify esp. If valid then cast it as (uint_32 *) args, else kill process. (Same as Task2)
    2. Switch args[0] to find corresponding syscall. (Same as Task2)
    3. Add SYS_CREATE, SYS_REMOVE ...etc to switch cases.
    4. Before calling each file system calls, if it has more arguments, verify args[1], args[2] ...etc to see if it has exceeded the memory limit. If it has, terminate the process.
    5. Call corresponding syscalls. Add args if syscall needs arguments.

+ bool syscall_create(const char *file, unsigned initial_size)
    1. Acquire the file system lock
    2. Call filesys_create(file, initial_size)
    3. Release the file system lock
    4. If filesys_create failed, return false. Otherwise return true

+ bool syscall_remove(const char *file)
    1. Acquire the file system lock
    2. Call filesys_remove(file)
    3. Release the file system lock
    4. If filesys_remove failed, return false. Otherwise return true

+ int syscall_open(const char *file)
    1. Acquire the file system lock
    2. Call filesys_open(file) and store return value as pf
    3. Release the file system lock
    4. If pf == NULL, return -1.
    5. Append the pf in struct thread::open_files(begin searching from 2, find the first NULL in open_files and replace it with pf)
    6. Return the position of pf in open_files

+ int filesize(int fd)
    1. Check if 1 < fd  < MAX_OPEN_FILES and open_files[fd] is not NULL. Return -1 if not.
    2. Get file* pf = open_files[fd]
    3. Acquire the file system lock
    4. Call file_length(pf) and store return value as file_len
    5. Release the file system lock
    6. Return file_len

+ int syscall_read(int fd, void *buffer, unsigned size)
    1. Check if buffer and buffer+size is valid. Return -1 if not.
    2. if fd == 0
        1. while(i-->size) use input_getc() to get a char from keyboard input and put it to *(buffer+i). If input_getc() failed to get char, break.
        2. return i.
    3. else if 1 < fd < MAX_OPEN_FILES && open_files[fd] is not NULL
        1. Get file* pf = list[fd]
        2. Acquire the file system lock
        3. Call file_read(pf, buffer, size) and store return value as read_len
        4. Release the file system lock
        5. Return read_len
    4. else
        1. Return -1
 
+ int syscall_write(int fd, const void *buffer, unsigned size)
    1. Check if buffer and buffer+size is valid. Return -1 if not.
    2. if fd == 1
        1. Use putbuf() to write all data in buffer
        2. Return the value actually written
    3. else if 1 < fd < MAX_OPEN_FILES && open_files[fd] is not NULL
        1. Get file* pf = list[fd]
        2. Acquire the file system lock
        3. Call file_write(pf, buffer, size) and store return value as write_len
        4. Release the file system lock
        5. Return write_len
    4. else
        1. Return -1

+ void syscall_seek(int fd, unsigned position)
    1. Check if 1 < fd  < MAX_OPEN_FILES and open_files[fd] is not NULL. Return -1 if not.
    2. Get file* pf = open_files[fd]
    3. Acquire the file system lock
    4. Call file_seek(pf, position)
    5. Release the file system lock

+ unsigned syscall_tell(int fd)
    1. Check if 1 < fd  < MAX_OPEN_FILES and open_files[fd] is not NULL. Return -1 if not.
    2. Get file* pf = open_files[fd]
    3. Acquire the file system lock
    4. Call file_tell(pf) and store return value as file_pos
    5. Release the file system lock
    6. Return file_pos

+ void syscall_close(int fd)
    1. Check if 1 < fd  < MAX_OPEN_FILES and open_files[fd] is not NULL. Return -1 if not.
    2. Get file* pf = open_files[fd]
    3. Acquire the file system lock
    4. Call file_close(pf)
    5. Release the file system lock
    6. Set open_files[fd] to NULL

+ In bool load (const char *file_name, void (**eip) (void), void **esp)
    1. Acquire the file system lock before calling file_read()
    2. Release the file system lock after the file_close()

#### Synchronization
+ lock file_system_lock
    Used to allow only one thread at a time to access file syscalls to avoid racing issue.
    Because it's time-consuming to read/write from disk, locking won't affect much on performance.

#### Rationale

1. Use lock instead of semaphore
    + only the owner of lock can release it.

2. Create different functions for different kinds of syscall
    + to simplify syscall_handle() and make different syscalls easier to debug

## Additional Questions

### Question 1

+ In child-bad.c, line 12:
    ```c
    asm volatile ("movl $0x20101234, %esp; int $0x30");
    ```
    movl sets esp to an invaild value(0x20101234), then int invokes the syscall. Because esp is invaild, syscall should kill the process. 
    
+ In sc-bad-sp.c, line 18:
    ```c
    asm volatile ("movl $.-(64*1024*1024), %esp; int $0x30");
    ```
    First store a invalid number into *%esp* and then execute systam call. Also result in being killed. 

### Question 2

+ In sc-boundary-3.c, line 13:
    ```c
    char *p = get_bad_boundary ();
    p--;
    *p = 100;

    /* Invoke the system call. */
    asm volatile ("movl %0, %%esp; int $0x30" : : "g" (p));
    ```
    The first three lines makes p point to the byte just below the boundary and assign the byte with 100. The assembly code assigns esp with p so the stack is only 1 byte in depth and is unable to pop a 32-bit int as the syscall argument, which resulted in killing this process.

### Question 3

+ REMOVE
    The test suite don't cover tests in bool remove(const char *file) syscall, which is used to delete files. OS might run into trouble if:
    1. file is pointing to a invalid memory address
    2. file is not a acceptable string(string reaches memory boundary before '\0' or contains invalid character)
    3. file does not exist in file system
    4. file is being opened
    5. file not allowed to be deleted by user programs(such as system files)
    The first four cases should be tested. For the fifth test, it may cause damage in system. So better use a dummy system file to test.


