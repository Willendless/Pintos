/** Test buffer cache's ability to write full blocks to disk
 *  without reading them first.
 *  @author Jiarui Li
 */

#include <syscall.h>
#include <random.h>
#include "tests/lib.h"
#include "tests/main.h"
#include "tests/filesys/seq-test.h"

#define FILE_SIZE 102400

static char buf[FILE_SIZE];

void
test_main (void)
{
  int fd;
  int brcnt_before, bwcnt_before;
  int brcnt_after, bwcnt_after; 

  random_init (0);
  random_bytes (buf, sizeof buf);

  CHECK (create ("a", 102400), "create \"a\"");
  CHECK ((fd = open ("a")) > 1, "open \"a\"");

  brcnt_before = brcnt ();
  bwcnt_before = bwcnt ();

  if (write (fd, buf, FILE_SIZE) != FILE_SIZE)
    fail ("write failed");

  brcnt_after = brcnt ();
  bwcnt_after = bwcnt ();
  
  CHECK (brcnt_after - brcnt_before <= 3,
        "check block read cnt (must not change)");
  CHECK (bwcnt_after - bwcnt_before >= 200,
        "check block write cnt");

  check_file ("a", buf, FILE_SIZE);
}