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

#define FILE_SIZE 102400
static char buf[FILE_SIZE];

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
      size_t block_size = random_ulong () % (FILE_SIZE / 8) + 1;
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

  CHECK (create ("cache-hit", 0), "create \"cache-hit\"");
  CHECK ((fd = open ("cache-hit")) > 1, "open \"cache-hit\"");

  msg ("init \"cache-hit\"");
  while (w_ofs < FILE_SIZE)
    {
      write_some_bytes ("cache-hit", fd, buf, &w_ofs);
    }
  
  msg ("close \"cache-hit\"");
  close (fd);

  cache_flush ();

  CHECK ((fd = open ("cache-hit")) > 1, "open \"cache-hit\"");
  msg ("cold read \"cache-hit\"");
  while (r_ofs < FILE_SIZE)
    {
      read_some_bytes ("cache-hit", fd, buf, &r_ofs);
    }

  cache_stat (&hit_cnt, &read_cnt, &write_cnt);
  cold_hit_rate = hit_cnt * 100 / (read_cnt + write_cnt);

  msg ("cold-cache stat:");
  msg ("hit rate: %d%%, hit cnt: %d, read cnt: %d, write cnt: %d", 
       cold_hit_rate, hit_cnt, read_cnt, write_cnt);
  
  close (fd);

  CHECK ((fd = open ("cache-hit")) > 1, "open \"cache-hit\"");
  msg ("hot read \"cache-hit\"");
  r_ofs = 0;
  while (r_ofs < FILE_SIZE)
    {
      read_some_bytes ("cache-hit", fd, buf, &r_ofs);
    }

  cache_stat (&hit_cnt, &read_cnt, &write_cnt);
  hot_hit_rate = hit_cnt * 100 / (read_cnt + write_cnt);

  msg ("hot cache rate:");
  msg ("hit rate: %d%%, hit cnt: %d, read cnt: %d, write cnt: %d", 
       hot_hit_rate, hit_cnt, read_cnt, write_cnt);

  CHECK (cold_hit_rate < hot_hit_rate, "compare hit rate");
}