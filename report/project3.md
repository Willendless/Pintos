Final Report for Project 3: File Systems
========================================

Replace this text with your final report.


### Task 2 extensable file


synchronous:

1. file create
    + freemap lock
2. file open
3. file write
4. read extension field


operation:

1. file create
    + inode_create()
        + inode_extend()
            + each time allocate one block
            + if allocate fail, then roll back and release previous allocated block

2. 
