Final Report for Project 3: File Systems
========================================

## Group Members
* Jiarui Li <jiaruili@berkeley.edu>
* Xiang Zhang <xzhang1048576@berkeley.edu>
* Joel Kattapuram <joelkattapuram@berkeley.edu>
* Kallan Bao <kallanbao@berkeley.edu>

## Changes in design doc

### Buffer Cache

1. Use LRU list instead of Second chance list
    + considering the implementation difficulty of second chance list, we choose to use LRU list
    + export three interfaces for disk sector operation
        1. `cache_get()`: return data within one data sector
        2. `cache_put()`: put data with one data sector
        3. `cache_flush()`: flush all modified and valid cache entry to disk

2. For LRU list's synchronization policy
    + use global lock to guard the whole buffer cache
    + when calling `cache_get()` or `cache_put()`, first acquire global lock then
        1. if hit, try get per entry lock and release global lock
        2. if miss, choose evict entry, try get per entry lock and release global lock
    + if cannot get per entry lock, then wait on the per entry condition variable and release global lock
    + for those thread who have gotten per entry lock
        1. if entry has been modified, write entry to disk 
        2. do R/W operation
        3. get global lock and signal those who wait on the entry variable

### Extensible File

1. Add inode length lock and free map lock
2. Add inode open list lock
3. interfaces for file extend
    + `inode_extend_length()`
    + `inode_extend_sectors()`
    + `inode_extend_sector()`
4. when file extend fails because of length restrict
    + `inode_sector_release()`


### Subdirectories

1. Add `is_dir` element in `struct inode_disk` to represent the type of inode

2. Define a new `struct FILE` to represent both directory and file using union
```c
    struct FILE {
        bool is_dir;
        union {
            struct file *file;
            struct dir *dir;
        } ptr;
    };
```
3. Use FILE to represent thread's file open table

4. Modify interfaces in filesys.c/filesys.h to support FILE operation for both dir and file

5. use `get_next_part()` in filesys.c to walk through path


## Team Collaboration

### Jiarui Li
Designed each part of the project. Build buffer cache, add extensible file and subdirectories support. Add buffer cache hit rate test and buffer cache write through test. Write the project3 design doc and final report.

## Student testing report

### cache hit rate test (cache-hit.c)

1. feature: Verify that the hot cache hit rate will be better than the cold cache hit rate.

2. working mechanics
    1. system call add
        + `cache_flush ()`: atomically flush all buffered entry in cache and reset the hit_cnt, read_cnt and write_cnt for buffer cache
        + `cache_stat ()`: return hit_cnt, read_cnt and write_cnt
    2. test policy:
        + first randomly set a buffer for a new file, create a new file and write the buffer to the file and close the file
        + flush the content in the cache
        + check that hit_cnt, read_cnt and write_cnt are all zeros
        + open that file and sequentially read from that file and call `check_file()` to test file content
        + call cache_stat() and get `cold_hit_rate`
        + open file again and read sequentially
        + call cache_stat() and get `hot_hit_rate`
        + check that `hot_hit_rate` is better `cold_hit_rate`

3. test case output
    + output file:
    ```
    Copying tests/filesys/extended/cache-hit to scratch partition...
    Copying tests/filesys/extended/tar to scratch partition...
    qemu-system-i386 -device isa-debug-exit -hda /tmp/NMsHz_DZXd.dsk -hdb tmp.dsk -m 4 -net none -nographic -monitor null
    PiLo hda1
    Loading............
    Kernel command line: -q -f extract run cache-hit
    Pintos booting with 3,968 kB RAM...
    367 pages available in kernel pool.
    367 pages available in user pool.
    Calibrating timer...  509,542,400 loops/s.
    hda: 1,008 sectors (504 kB), model "QM00001", serial "QEMU HARDDISK"
    hda1: 202 sectors (101 kB), Pintos OS kernel (20)
    hda2: 246 sectors (123 kB), Pintos scratch (22)
    hdb: 5,040 sectors (2 MB), model "QM00002", serial "QEMU HARDDISK"
    hdb1: 4,096 sectors (2 MB), Pintos file system (21)
    filesys: using hdb1
    scratch: using hda2
    Formatting file system...done.
    Boot complete.
    Extracting ustar archive from scratch device into file system...
    Putting 'cache-hit' into the file system...
    Putting 'tar' into the file system...
    Erasing ustar archive...
    Executing 'cache-hit':
    (cache-hit) begin
    (cache-hit) create "cachehit"
    (cache-hit) open "cachehit"
    (cache-hit) init "cachehit"
    (cache-hit) close "cachehit"
    (cache-hit) flush "cachehit"
    (cache-hit) hit_cnt 0 after flush
    (cache-hit) read_cnt 0 after flush
    (cache-hit) write_cnt 0 after flush
    (cache-hit) open "cachehit"
    (cache-hit) cold read "cachehit"
    (cache-hit) open "cachehit" for verification
    (cache-hit) verified contents of "cachehit"
    (cache-hit) close "cachehit"
    (cache-hit) open "cachehit"
    (cache-hit) hot read "cachehit"
    (cache-hit) open "cachehit" for verification
    (cache-hit) verified contents of "cachehit"
    (cache-hit) close "cachehit"
    (cache-hit) compare hit rate (must better)
    (cache-hit) end
    cache-hit: exit(0)
    Execution of 'cache-hit' complete.
    Timer: 104 ticks
    Thread: 41 idle ticks, 48 kernel ticks, 15 user ticks
    hdb1 (filesys): 88 reads, 281 writes
    hda2 (scratch): 245 reads, 2 writes
    Console: 1646 characters output
    Keyboard: 0 keys pressed
    Exception: 0 page faults
    Powering off...
    ```
    + result file:
    ```
    PASS
    ```

4. + If kernel did not reset the buffer cache hit, read and write cnt, then cnt check will fail
    + If kernel did not flush buffer cache, then hot hit rate will not be better than hot hit rate


### cache write through test (cache-wthrough.c)

1. feature: Verify that write operations with BLOCK_SECTOR_SIZE length each time will only cause disk write operation instead of disk read operation.

2. working mechanics
    1. system call add
        + `brcnt()`: return `block_ read_cnt`
        + `bwcnt()`: return `block_write_cnt`
    2. test policy
        + first randomly set a buffer for a new file with length equal to 200 blocks (102400 bytes), create a new file and write the buffer to the file and close the file
        + call `brcnt()` to get `brcnt_before` and call `bwcnt()` to get `bwcnt_before`
        + write buffer to file
        + call `brcnt()` to get `brcnt_after` and call `bwcnt()` to get `bwcnt_after`
        + check that *brcnt_after - brcnt_before <= 3*
        + check that *bwcnt_after - bwcnt_before >= 200*
        + check that file content is same as buffer and close file using `check_file ()`

3. test case output
    + output file:
    ```
    Copying tests/filesys/extended/cache-wthrough to scratch partition...
    Copying tests/filesys/extended/tar to scratch partition...
    qemu-system-i386 -device isa-debug-exit -hda /tmp/lc89qRXEMI.dsk -hdb tmp.dsk -m 4 -net none -nographic -monitor null
    PiLo hda1
    Loading............
    Kernel command line: -q -f extract run cache-wthrough
    Pintos booting with 3,968 kB RAM...
    367 pages available in kernel pool.
    367 pages available in user pool.
    Calibrating timer...  524,288,000 loops/s.
    hda: 1,008 sectors (504 kB), model "QM00001", serial "QEMU HARDDISK"
    hda1: 202 sectors (101 kB), Pintos OS kernel (20)
    hda2: 241 sectors (120 kB), Pintos scratch (22)
    hdb: 5,040 sectors (2 MB), model "QM00002", serial "QEMU HARDDISK"
    hdb1: 4,096 sectors (2 MB), Pintos file system (21)
    filesys: using hdb1
    scratch: using hda2
    Formatting file system...done.
    Boot complete.
    Extracting ustar archive from scratch device into file system...
    Putting 'cache-wthrough' into the file system...
    Putting 'tar' into the file system...
    Erasing ustar archive...
    Executing 'cache-wthrough':
    (cache-wthrough) begin
    (cache-wthrough) create "a"
    (cache-wthrough) open "a"
    (cache-wthrough) check block read cnt (must not change)
    (cache-wthrough) check block write cnt
    (cache-wthrough) open "a" for verification
    (cache-wthrough) verified contents of "a"
    (cache-wthrough) close "a"
    (cache-wthrough) end
    cache-wthrough: exit(0)
    Execution of 'cache-wthrough' complete.
    Timer: 98 ticks
    Thread: 31 idle ticks, 48 kernel ticks, 19 user ticks
    hdb1 (filesys): 255 reads, 458 writes
    hda2 (scratch): 240 reads, 2 writes
    Console: 1293 characters output
    Keyboard: 0 keys pressed
    Exception: 0 page faults
    Powering off...
    ```
    + result file:
    ```
    PASS
    ```

4. + If buffer cache first read block into buffer cache than write content to buffer cache entry, then *brcnt_after - brcnt_before* will be larger than 200
    + If buffer cache first read block into buffer cache than write content to buffer cache entry, then *bwcnt_after - bwcnt_before* will be smaller than 200

### test written experience

+ When writing tests for Pintos, I first refered to existed test to know some general method to use like sequentially read and test a file, randomly init a buffer and know about how to use the `CHECK` macro (just like the assertTrue in junit). Based on existed test lib, it is much easy to build own test.
+ If we can add our own kernel tests instead of just user mode test then it might be more easier, since we don't need to add all kinds of system calls, but it may required us to hardcode our test cases in kernel.
+ lesson:
    1. Macros and general method are really good things to build c test.
    2. It is important to keep number of system call not too much and just implement mechanism instead of policy.




