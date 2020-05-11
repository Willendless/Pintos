## -*- perl -*-
use strict;
use warnings;
use tests::tests;
use tests::random;
my ($cachehit) = random_bytes (10240);
check_archive ({"cachehit" => [$cachehit]});
pass;
