## -*- perl -*-
use strict;
use warnings;
use tests::tests;
use tests::random;
my ($a) = random_bytes (102400);
check_archive ({"a" => [$a]});
pass;
