Design Document for Project 2: Threads
======================================

## Group Members

* Jiarui Li <jiaruili@berkeley.edu>
* Xiang Zhang <xzhang1048576@berkeley.edu>
* Joel Kattapuram <joelkattapuram@berkeley.edu>
* Kallan Bao <kallanbao@berkeley.edu>

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
    
*Adding following functions:*
+ wait_list_less();
    ```c
    bool wait_list_less(const struct list_elem *a,
                        const struct list_elem *b,
                        void *aux));
    ```
    + *list_insert_ordered()* helper function: Compare relative order of two thread in wait_list.

*Modifying following functions:*
+ thread_tick()
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
            + pop front thread
            + insert it into **ready_list**
        + else:
            + break

2. timer_sleep
    + Calculate **wakeup_time** for current thread.
    + Disable interrupt
    + Set state of current thread to **THREAD_BLOCKED**.
    + Insert thread into **wait_list** order by **wakeup_time** ascend.
    + Enable interrupt 
    + thread_yield()
    
#### Synchronization

1. struct list wait_list
    + insert into wait_list in timer_sleep(), interrupt is disabled so it is atomic.
    + pop wait_list in thread_tick(). It is an interrupt so no other thread will be excecuting timer_sleep() or thread_tick()
    

#### Rationale

+ Use a sorted list ordered by `wakeup_time` so each insert cost O(n), pop cost O(1). Because thread_tick() is more often excuted, this implementation accelerates performance.
+ 

### Task 2: Priority Scheduler

#### Data structures and functions
*Modifying following data structure:*
+ struct thread
    ```c
    struct thread
      {
        /* ... */
        int64_t wakeup_time;                /* Thread wake up time. */
        int base_priority;                  /* Thread base priority. */
        int effective_priority;             /* Thread effective priority. */
        struct list owned_locks;            /* Thread current owned locks. */
        struct lock *wait_lock;             /* Thread current wait lock. */
        /* ... */
      };
    ```

+ struct lock
    ```c
    struct lock
      {
        struct list_elem elem;      /* Used in thread's owned_locks. */
        struct thread *holder;      /* Thread holding lock (for debugging). */
        struct semaphore semaphore; /* Binary semaphore controlling access. */
      };
    ```

*Adding following functions:*

+ thread_recursive_donate()
  ```c
  void thread_recursive_donate(struct thread* t, int donate_priority)
  ```
  + Update the **effective_priority** of thread t. If **effective_priority** changes and the thread is waiting for a lock, call recursively on the lock owner.

*Modifying following functions*

1. **INIT -> READY**

+ thread_create()
  ```c
  tid_t thread_create (const char *name, int priority,
                       thread_func *function, void *aux);
  ```
  + if new thread's priority is higher than current thread, then call `thread_yield()`

2. **WAIT -> READY**

+ thread_unblock() 
  ```c
  void thread_unblock (struct thread *t)
  ```
  + use `list_insert_ordered()` to insert blocked thread into ready list for a ordered ready list

+ thread_tick()
  ```c
  void thread_tick(void);
  ```
  + if unblocked thread's priority is bigger than current thread, then call `thread_yield()`

+ sema_up()
  ```c
  void sema_up (struct semaphore *sema)
  ```
  + yield CPU if the unblocked thread has a higher priority.

+ lock_release()
  ```c
  void lock_release (struct lock *lock)
  ```
  + change current thread's `effective_priority` accordingly.

3. **RUNNING -> READY**

+ thread_yield()
  ```c
  void thread_yield (void)
  ```
  +  orderly insert current thread into ready_list by **effective_priority**

4. **RUNNING -> WAIT**

+ sema_down()
  ```c
  void sema_down (struct semaphore *sema)
  ```
  + if need waiting, insert into `waiters` ordered by `effective_priority`

+ lock_acquire()
  ```c
  void lock_acquire (struct lock *lock);
  ```
  + if lock has holder, recursively donate current thread's priority

+ cond_wait()
  ```c
  void cond_wait (struct condition *cond, struct lock *lock);
  ```
  + orderly insert current thread into `cond->waiters`

3. **PRIORITY CHANGE**

+ thread_set_priority()
  ```c
  void thread_set_priority (int new_priority);
  ```
  + set `base_priority` and may set `effective_priority`

+ thread_get_priority()
  ```c
  int thread_get_priority ();
  ```
  + get the thread's effective priority instead of base priority
  
#### Algorithms

#### Choosing the next thread to run

+ keep ready_list and wait_list ordered by **effective_priority** 
1. thread_create()
  + if new thread's priority is higher than current thread, immediately yield to let scheduler run the new thread.

2. priority scheduler
  + Save previous thread's PC, SP, etc
  + Choose the thread with maximum **effective_priority** from **ready_list**(pop the first one)
  + Set PC, SP, etc with current thread's PC, SP, etc.

#### Acquiring a lock

+ Disable interrupt
+ If lock is owned, call `thread_recursive_donate()` on the lock owner.
+ Enable interrupt
+ `sema_down()`
+ set lock_holder
+ insert lock into owned locks orderly

#### Releasing a lock

+ disable interrupt
+ remove lock from current thread's `owned_locks`
+ get largest `effective priority` from other locks in `owned_locks`
+ change current thread's `effective priority`
+ enable interrupt

#### Computing the effective priority

1. When thread is initialized in `thread_create()`, `effective_priority` is the same as `base priority`
2. Updating **effective_priority** using thread_recursive_donate()
  + Assert the interrupt is disabled
  + Set t's **effective_priority** as the maximum of **donate_priority** and t's **effective_priority**
  + If t's **effective_priority** has changed, and t is waiting for a lock:
    + call *thread_recursive_donate* on the lock's owner with **donate_priority**

#### Priority scheduling for semaphores and locks

1. sema_up
  + If the thread unblocked has a higher `effective_priority` than current thread, then after increasing `sema->value`, call `thread_yield()` before `intr_set_level()`.

2. sema_down
  + if `sema->value == 0` use `list_insert_ordered()` to insert current thread into `sema->waiters`.


#### Priority scheduling for condition variables

1. cond_wait
  + use `list_insert_ordered()` to insert current thread into `cond->waiters`. 

#### Setting and getting thread's priority

1. thread_set_priority
  + if **new_priority** >= **effective_priority**, set **effective_priority** = **new_priority**
  + else if **base_priority** == **effective_priority**
    + get the highest **effective_priority** from the waiters of each lock in **owned_locks**
    + set **effective_priority** = the maximum of these priorities and **new_priority**
  + Set current **base_priority** as **new_priority**  

2. thread_get_priority
  + Return the **effective_priority**

#### Synchronization

1. lock tree
  + One thread can own many locks but can only wait for one lock, thus forming a tree with owner as the parent and owned locks as the children. Only the root of the tree is able to execute.
  + On waiting for the lock, the thread should donate its **effective_priority** up the tree to ensure the root has the highest priority.
  + On releasing the lock, the thread should subtract potentially gained priority, and the child having the highest priority becomes the new root.

2. wait_list
  + use `intr_disable()` and `intr_set_level()` to guarantee mutual exclusion.

3. ready_list
  + use `intr_disable()` and `intr_set_level()` to guarantee mutual exclusion.


#### Rationale

## Aditional Questions

### Question 1

1. kernrl thread in kernel mode
    1. other kernel thread
        + program counter: no need to store, if switch, will start from same position
        + stack pointer: *struct thread::stack*
        + registers: top of other kernel threads' stack
    2. corresponding user thread
        + program counter: current kernel thread's stack in *struct intr_frame*
        + stack pointer: current kernel thread's stack in *struct intr_frame*
        + registers: current kernel thread's stack in *struct intr_frame*
    3. other user thread:
        + program counter: other kernel thread's stack in *struct intr_frame*
        + stack pointer: other kernel thread's stack in *struct intr_frame*
        + registers: other kernel thread's stack in *struct intr_frame*

2. user process in user mode
    1. corresponding kernel thread
        + program counter: get from IDT(interrupt descriptor table) 
        + stack pointer: tss
        + registers: initialize when enter into interrupt
    2. other kernel thread
        + program counter: no need to store, if switch, will start from same position
        + stack pointer: *struct thread:stack*
        + registers: top of ohter kernel thread's stack
    3. other user thread
        + program counter: other kernel thread's stack in *struct intr_frame*
        + stack pointer: other kernel thread's stack in *struct intr_frame*
        + registers: other kernel thread's stack in *struct intr_frame*

### Question 2

+ When: The page containing stack and TCB is freed when next scheduled thread begin to execute and the state of prev thread is THREAD_DYING.
+ Where: Next thread will execute **palloc_free_page()** in thread.c: **thread_schedule_tail()**
+ Theard can not free its own kernel page before next thread run, because after thread free its own kernel page, it cannot call other functions or assign other variables, which means it cannot schedule next thread and execute switch_threads.

### Question 3

+ current kernel thread stack

### Question 4

+ Consider the following case:
    D --> B --> A <-- C
    A is holding a lock which B and C is waiting for. While B has lower base priority than C, D with the highest priority is waiting for a lock holding by B, so B has a higher effective priority than C. When A releases the lock, wrong implementation will result in C going first, but in fact it should be B. We can make B output "B" and C output "C" after acquiring the lock, so the correct answer is "BC" and the wrong one is "CB".


