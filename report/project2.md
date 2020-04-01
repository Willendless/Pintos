Final Report for Project 2: Threads
===================================

## Group Members
* Jiarui Li <jiaruili@berkeley.edu>
* Xiang Zhang <xzhang1048576@berkeley.edu>
* Joel Kattapuram <joelkattapuram@berkeley.edu>
* Kallan Bao <kallanbao@berkeley.edu>

## Changes in Design Doc
1. make sure we modify ready list atomically:
    + add element into ready list: use thread_unblock()
        + thread_tick()
    + pop element from ready list: use thread_yield()

2. condvar implementation
    + add field (priority) in struct semaphore_elem
        + when insert semaphore_elem into cond's wait queue, current thread hasn't been blocked in sema.

3. sema's waiters may dynamically change
    + we cannot keep the wait queue ordered, so we use `list_max()` instead of `list_pop_front()`

## Team Collaboration

### Jiarui Li
Designed each part of the project. Added priority check to thread_waking functions for they may have a higher priority than current thread. Implemented the lock tree to enable priority donation, avoiding deadlocks. Debugged with Xiang to verify the functionality of the priority scheduler. Wrote part of schedlab (Implement simulation of fairness of CPU Burst and answer part of questions) with Xiang and Joel.

### Xiang Zhang
Implemented the priority scheduler, which choose the thread with highest `effective_priority` to run. Debugged with Jiarui to verify the functionality of the priority scheduler. Wrote part of documents(design doc and report) with Kallan and Joel. Wrote part of schedlab (Implement simulation of fairness of CPU Burst and answer part of questions) with Jiarui and Joel.

### Joel Kattapuram
Implemented `thread_tick()` function to wake up threads at each tick. Debugged with Kallan to verify the functionality of task1(Effective alarm clock). Wrote part of documents(design doc and report) with Xiang and Kallan. Implemented part of schedlab with Xiang and Jiarui (Implement SRTF, MLFQ and simulation of CPU Utilization).

### Kallan Bao
Implemented `timer_sleep()` function to insert thread into `wait_list`. Debugged with Joel to verify the functionality of task1(Effective alarm clock). Wrote part of documents(design doc and report) with Xiang and Joel.

## Summary and reflection on this project

As expected, working as a group on this project in the midst of the current pandemic had its share of challenges. Unable to physically meet and work together, much of the actions of the group were much more disjointed - especially when taking into consideration differing time zones. Despite these challenges, we are proud to have managed to come together as a group and complete project 2.