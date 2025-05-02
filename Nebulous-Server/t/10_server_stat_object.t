#!/usr/bin/perl

# Copryight (C) 2004-2005  Joshua Hoblitt
#
# $Id: 09_server_stat_object.t,v 1.15 2008-05-16 20:29:19 jhoblitt Exp $

use strict;
use warnings FATAL => qw( all );

use Test::More tests => 13;

use lib qw( ./t ./lib );

use Nebulous::Server;
use Test::Nebulous;

my $neb = Nebulous::Server->new(
    dsn         => $NEB_DB,
    dbuser      => $NEB_USER,
    dbpasswd    => $NEB_PASS,
);

Test::Nebulous->setup;
{
    $neb->create_object("foo");

    my $info = $neb->stat_object("foo");

    is(scalar @$info, 8, "number of columns");
}

Test::Nebulous->setup;

{
    $neb->create_object("foo", "node01");

    my $info = $neb->stat_object("foo");

    is(scalar @$info, 8,                       "number of columns");
    is(@$info[0], 1,                           "so_id");
    is(@$info[1], "foo",                       "ext_id");
    is(@$info[2], 0,                           "read lock");
    is(@$info[3], undef,                       "write lock");
    like(@$info[4], qr/....-..-.. ..:..:../,   "epoch");
    like(@$info[5], qr/....-..-.. ..:..:../,   "mtime");
    is(@$info[6], 1,                           "available instances");
    is(@$info[7], 1,                           "total instances");
}

Test::Nebulous->setup;

eval {
    my $stat = $neb->stat_object("foo");
};
like($@, qr/is valid object key/, "no params");

Test::Nebulous->setup;

eval {
    $neb->stat_object();
};
like($@, qr/1 was expected/, "no params");

Test::Nebulous->setup;

eval {
    $neb->create_object("foo");
    $neb->stat_object("foo", 2);
};
like($@, qr/1 was expected/, "too many params");

Test::Nebulous->cleanup;
