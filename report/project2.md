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


## Changes in Design Doc

## Team Collaboration

## Summary and reflection on this project

As expected, working as a group on this project in the midst of the current pandemic had its share of challenges. Unable to physically meet and work together, much of the actions of the group were much more disjointed - especially when taking into consideration differing time zones. Despite these challenges, we are proud to have managed to come together as a group and complete project 2.
