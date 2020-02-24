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

1.  In void syscall_exit(int status), we didn't change thread_current()->wait_status->exit_code to status.

### Task 3

1. 

### Questions

1. If syscall failed should we return -1, or exit the thread?