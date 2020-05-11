# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected (IGNORE_EXIT_CODES => 1, [<<'EOF']);
(cache-wthrough) begin
(cache-wthrough) create "a"
(cache-wthrough) open "a"
(cache-wthrough) check block read cnt (must not change)
(cache-wthrough) check block write cnt
(cache-wthrough) open "a" for verification
(cache-wthrough) verified contents of "a"
(cache-wthrough) close "a"
(cache-wthrough) end
EOF
pass;