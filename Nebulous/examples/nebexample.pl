#!/usr/bin/env perl

use strict;
use warnings FATAL => qw( all );

use Nebulous::Client;

my $neb = Nebulous::Client->new(
    proxy   => 'http://podb1.ifa.hawaii.edu:80/nebulous'
);

my $key = shift || 'foobarbaz';

# make sure there isn't already a file named "foobarbaz" so this example
# doesn't cause an error
$neb->delete($key);

my $fh = $neb->create($key);
die "can't create file $key " unless $fh;
close($fh);

if (!$neb->replicate($key)) {
    die "can't replicate object $key";
}

if (!$neb->delete($key)) {
    die "can't delete object $key";
}
