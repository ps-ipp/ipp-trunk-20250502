#!/usr/bin/perl

# Copryight (C) 2004-2005  Joshua Hoblitt
#
# $Id: 05_server_lock_object.t,v 1.10 2008-03-20 21:10:14 jhoblitt Exp $

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

    ok($neb->lock_object("foo", "read"), "read lock");
}

Test::Nebulous->setup;

{
    $neb->create_object("foo");

    ok($neb->lock_object("foo", "read"), "read lock");
    ok($neb->lock_object("foo", "read"), "read lock");
}

Test::Nebulous->setup;

{
    $neb->create_object("foo");

    ok($neb->lock_object("foo", "write"), "write lock");
}

Test::Nebulous->setup;

eval {
    $neb->lock_object("foo", "read");
};
like($@, qr/is valid object key/, "storage object does not exist");

Test::Nebulous->setup;

eval {
    $neb->lock_object("foo", "write");
};
like($@, qr/is valid object key/, "storage object does not exist");

Test::Nebulous->setup;

eval {
    $neb->create_object("foo");

    $neb->lock_object("foo", "write");
    $neb->lock_object("foo", "write");
};
like($@, qr/can not write lock twice/, "can not write lock twice");

Test::Nebulous->setup;

eval {
    $neb->create_object("foo");

    $neb->lock_object("foo", "read");
    $neb->lock_object("foo", "write");
};
like($@, qr/can not write lock after read lock/, "can not write lock after read lock");

Test::Nebulous->setup;

eval {
    $neb->create_object("foo");

    $neb->lock_object("foo", "write");
    $neb->lock_object("foo", "read");
};
like($@, qr/can not read lock after write lock/, "can not read lock after write lock");

Test::Nebulous->setup;

eval {
    $neb->lock_object();
};
like($@, qr/2 were expected/, "no params");

Test::Nebulous->setup;

eval {
    $neb->create_object("foo");

    $neb->lock_object("foo");
};
like($@, qr/2 were expected/, "not enough params");

Test::Nebulous->setup;

eval {
    $neb->create_object("foo");

    $neb->lock_object("foo", "both");
};
like($@, qr/is read or write/, "not read or write");

Test::Nebulous->setup;

eval {
    $neb->create_object("foo");

    $neb->lock_object("foo", 'read', 3);
};
like($@, qr/2 were expected/, "too many params");

Test::Nebulous->cleanup;
