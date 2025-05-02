#!/usr/bin/env perl

use strict;
use warnings;

use URI;

my $u1 = URI->new("neb://www.perl.com/bar");

print $u1->scheme, "\n";
print $u1->opaque, "\n";
print $u1->path, "\n";

my $u2 = URI->new("neb:/www.perl.com");
print $u2->scheme, "\n";
print $u2->opaque, "\n";
print $u2->path, "\n";

