Design Document for Project 2: Threads
======================================

## Group Members

* Jiarui Li <jiaruili@berkeley.edu>
* Xiang Zhang <xzhang1048576@berkeley.edu>
* Joel Kattapuram <joelkattapuram@berkeley.edu>
* Kallan Bao < kallanbao@berkeley.edu>

## Design Overview

### Task 1: Efficient Alarm Clock

#### Data structures and functions

*Adding following data structure:*
+ struct list wait_list
    ```c
    struct list wait_list;
    ```
    + Use ordered list **wait_list** to store sleep threads waiting for timer to wake up.

*Modifying following data structure:*
+ struct thread
    ```c
    struct thread
      {
        /* ... */
        int64_t wakeup_time;                /* Thread wake up time.*/
        /* ... */
      };
    ```
    + Add **wakeup_time** to store sleep thread's wake up time. Undefined when thread is not in state **THREAD_BLOCKED**.
    
*Modifying following functions:*
+ thread_tick
    ```c
    void thread_tick (void);
    ```
    + Examine **wait_list** and put threads to *ready_list* if wake_time is reached.

+ timer_sleep
    ```c
    void timer_sleep (int64_t ticks);
    ```
    + Initiate **wakeup_time** and insert current thread to **wait_list**.

#### Algorithms
1. thread_tick
    + while **wait_list** is not empty
        + if the first thread reached its **wakeup_time**:
            + put it to **ready_list**
            + remove it from **wait_list**
        + else:
            + break

2. timer_sleep
    + Calculate **wakeup_time** for current thread.
    + Set state of current thread to **THREAD_BLOCKED**.
    + Insert thread into **wait_list**. 
    + thread_yield()
    + Yield
    
#### Synchronization

#### Rationale

### Task 2: Priority Scheduler

#### Data structures and functions

#### Algorithms

#### Synchronization

#### Rationale

## Aditional Questions

### Question 1

1. kernel thread
    + program counter:
    + stack pointer:
    + registers:

2. user thread
    + program counter:
    + stack pointer:
    + registers:

### Question 2

1. when
    + Next thread has just schduled to run from  
2. where 
    + thread.c: thread_schedule_tail()
3. why

### Question 3


### Question 4


