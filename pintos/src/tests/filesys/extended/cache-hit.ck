# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected (IGNORE_EXIT_CODES => 1, [<<'EOF']);
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
EOF
pass;