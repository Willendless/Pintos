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

1. We implement most of our features inside process_*** functions, let syscall_*** functions only verify argument and call process_***.

### Task 3

1. When loading executables, we need to call filesys_open. So a filesys lock is needed in process.c. We used extern statement to refer to the lock in syscall.c.