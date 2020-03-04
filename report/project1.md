Final Report for Project 1: User Programs
=========================================

## Group Members

* Jiarui Li <jiaruili@berkeley.edu>
* Xiang Zhang <xzhang1048576@berkeley.edu>
* Joel Kattapuram <joelkattapuram@berkeley.edu>
* Kallan Bao <kallanbao@berkeley.edu>

## Changes in Design Doc

### Task 1

1. We didn't use strpbrk(), for there could be several spaces before the program name. strpbrk() can only detect the first space which is before the program name in this case.

2. After discovering that C99 supports variable-length array, malloc become less tempting. We use arrays instead to prevent memory leak.

### Task 2

1. We implemented most of our features inside process_\*\*\*(), let *syscall_\*\*\*()* functions only verify argument and call *process_\*\*\*()*.

2. In *process_wait()*:
    + first remove child's wait_status from children list and then destory it if condition is met.

3. In *process_exit()*:
    + decrease *ref_cnt* of every child process, if it equals to 0, then free child process's *wait_status*
    + decrease its own *wait_status*'s *ref_cnt*, if it equals to 0, then free it
    + call sema_up() to unlock the waiting parent if it exists

4. In *page_fault()*When user space exception triggers page fault, terminate user process by calling thread_exit(). 

### Task 3

1. We added FILE* this_executable in struct thread to record source executable if this thread is loaded from it. In order to keep the executable deny_write()d, the file must remain opened. So we need a FILE* to store it and close it in process_exit().

2. When loading executables, we need to call *filesys_open()*. So a filesys lock is needed in process.c. We used extern statement to refer to the lock in syscall.c.

3. In *process_exit()*, any file opened(which is in thread::open_files) should be close at this time. We added code to reclaim the resources.

## Team collaboration

### Jiarui Li
Designed each part of the project. Implemented the syscall handler, argument-verifying helper functions. Work with Xiang to implement syscalls in task 2(halt, exec, wait, practice) and task 3(create, remove, open, filesize, read, write, seek, tell, close). Debugged with Xiang to verify the functionality of syscalls. 

### Xiang Zhang
Work with Jiarui to implement syscalls in task 2(halt, exec, wait, practice) and task 3(create, remove, open, filesize, read, write, seek, tell, close). Debugged with Jiarui to verify the functionality of syscalls. Wrote the documents(design doc and report).

### Joel Kattapuram
Implemented the part to extract program name from command. Debugged with Kallan to verify the functionality of task1(Argument Passing).

### Kallan Bao
Implemented the argument passing part(moving arguments to stack). Debugged with Joel to verify the functionality of task1(Argument Passing)

## Summary and reflection on this project

1. We managed to pass all the test before deadline! Generally we went quite smoothly.

2. Different functions are responsible for different tasks, which should be made clear in design doc(but we didn't). For example, syscall_exit() is calling thread_exit() which calls process_exit(). Though they sound the same, syscall_exit() should verify argument and process_exit() should clean #USERPROG resources. We should clarify their responsibilities before coding to avoid inconsistent code style and potential bugs.

3. It takes too much time to debug. So it is essential that we understand what the skeleton code and our code is doing before actual coding.

4. We should work together so that bugs can be detected more easily with many eyes on screen.

