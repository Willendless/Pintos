Final Report for Project 2: Threads
===================================

Replace this text with your final report.

1. make sure atomically modify ready list:
    + add element into ready list: use thread_unblock()
        + thread_tick()
    + pop element from ready list: use thread_yield()

2. condvar implementation
    + add field (priority) in struct semaphore_elem
        + when insert semaphore_elem into cond's wait queue, current thread hasn't been blocked in sama.

3. sema's waiters may dynamically change