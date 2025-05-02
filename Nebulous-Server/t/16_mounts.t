#!/usr/bin/perl

# Copryight (C) 2008  Joshua Hoblitt
#
# $Id: 15_mounts.t,v 1.3 2008-09-11 22:35:52 jhoblitt Exp $

use strict;
use warnings FATAL => qw( all );

use Test::More tests => 18;

use lib qw( ./t ./lib );

use Nebulous::Server;
use Test::URI;
use Test::Nebulous;

my $neb = Nebulous::Server->new(
    dsn         => $NEB_DB,
    dbuser      => $NEB_USER,
    dbpasswd    => $NEB_PASS,
);

Test::Nebulous->setup;

{
    # key
    my $mounts = $neb->mounts();

    is(scalar @$mounts, 8, "number of rows");

    my %row;
    # first row
    @row{qw(mountpoint total used vol_id name host path allocate available xattr)}
        = @{$mounts->[0]};
    
    is($row{total},     100000000000, "total bytes");
    is($row{used},      100000000, "used bytes");
    is($row{vol_id},    1, "vol id");
    is($row{name},      "node01", "name");
    is($row{host},      "node01", "host");
    is($row{allocate},  1, "is allocated");
    is($row{available}, 1, "is available");
    is($row{xattr},     0, "has no xattr");

    # 2nd row
    @row{qw(mountpoint total used vol_id name host path allocate available xattr)}
        = @{$mounts->[1]};

    is($row{total},     100000000000, "total bytes");
    is($row{used},      1000000000, "used bytes");
    is($row{vol_id},    2, "vol id");
    is($row{name},      "node02", "name");
    is($row{host},      "node02", "host");
    is($row{allocate},  1, "is allocated");
    is($row{available}, 1, "is available");
    is($row{xattr},     0, "has no xattr");
}

Test::Nebulous->setup;

eval {
    $neb->mounts("foo");
};
like($@, qr/0 were expected/, "no params");

Test::Nebulous->cleanup;
