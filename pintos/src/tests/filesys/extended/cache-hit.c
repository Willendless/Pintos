/** Tries to resest the buffer cache. Open a file,
 *  read it sequentially and determine the hit rate
 *  for a cold cache. Then, close it, re-open it and
 *  read it sequentially again. The cache hit rate must
 *  improve.
 *  @author: Jiarui Li
 */

#include <syscall.h>
#include <random.h>
#include "tests/lib.h"
#include "tests/main.h"
#include "threads/fixed-point.h"

#define FILE_SIZE 10240
static char buf[FILE_SIZE];
static char rbuf[FILE_SIZE];

static void
write_some_bytes (const char *file_name, int fd, const char *buf, size_t *ofs)
{
  if (*ofs < FILE_SIZE)
    {
      size_t block_size = random_ulong () % (FILE_SIZE / 8) + 1;
      size_t ret_val;
      if (block_size > FILE_SIZE - *ofs)
        block_size = FILE_SIZE - *ofs;

      ret_val = write (fd, buf + *ofs, block_size);
      if (ret_val != block_size)
        fail ("write %zu bytes at offset %zu in \"%s\" returned %zu",
              block_size, *ofs, file_name, ret_val);
      *ofs += block_size;
    }
}

static void
read_some_bytes (const char *file_name, int fd, char *buf, size_t *ofs)
{
  if (*ofs < FILE_SIZE)
    {
      size_t block_size = random_ulong () % (FILE_SIZE / 8) + 128;
      size_t ret_val;
      if (block_size > FILE_SIZE - *ofs)
        block_size = FILE_SIZE - *ofs;
      
      ret_val = read (fd, buf + *ofs, block_size);
      if (ret_val != block_size)
        fail ("read %zu bytes at offset %zu in \"%s\" returned %zu",
              block_size, *ofs, file_name, ret_val);
      *ofs += block_size;
    }

}

void
test_main (void)
{
  int fd;
  size_t w_ofs, r_ofs;
  int read_cnt, write_cnt, hit_cnt;
  int hot_hit_rate, cold_hit_rate;
  w_ofs = r_ofs = 0;

  random_init (0);
  random_bytes (buf, sizeof buf);

  CHECK (create ("cachehit", 0), "create \"cachehit\"");
  CHECK ((fd = open ("cachehit")) > 1, "open \"cachehit\"");

  msg ("init \"cachehit\"");
  while (w_ofs < FILE_SIZE)
    {
      write_some_bytes ("cachehit", fd, buf, &w_ofs);
    }
  
  msg ("close \"cachehit\"");
  close (fd);

  msg ("flush \"cachehit\"");
  cache_flush ();

  cache_stat (&hit_cnt, &read_cnt, &write_cnt);
  CHECK ((hit_cnt == 0), "hit_cnt 0 after flush");
  CHECK ((read_cnt == 0), "read_cnt 0 after flush");
  CHECK ((write_cnt == 0), "write_cnt 0 after flush");

  CHECK ((fd = open ("cachehit")) > 1, "open \"cachehit\"");
  msg ("cold read \"cachehit\"");
  while (r_ofs < FILE_SIZE)
    {
      read_some_bytes ("cachehit", fd, rbuf, &r_ofs);
    }
  check_file ("cachehit", rbuf, 10240);

  cache_stat (&hit_cnt, &read_cnt, &write_cnt);
  cold_hit_rate = hit_cnt * 100 / (read_cnt + write_cnt);

  msg ("close \"cachehit\"");
  close (fd);

  CHECK ((fd = open ("cachehit")) > 1, "open \"cachehit\"");
  msg ("hot read \"cachehit\"");
  r_ofs = 0;
  while (r_ofs < FILE_SIZE)
    {
      read_some_bytes ("cachehit", fd, buf, &r_ofs);
    }
  check_file ("cachehit", rbuf, 10240);

  cache_stat (&hit_cnt, &read_cnt, &write_cnt);
  hot_hit_rate = hit_cnt * 100 / (read_cnt + write_cnt);

  msg ("close \"cachehit\"");
  close (fd);

  CHECK (cold_hit_rate < hot_hit_rate, "compare hit rate (must better)");
}