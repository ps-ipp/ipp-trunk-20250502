#!/usr/bin/perl

# Copryight (C) 2004-2005  Joshua Hoblitt
#
# $Id: 06_server_unlock_object.t,v 1.10 2008-03-20 21:10:14 jhoblitt Exp $

use strict;
use warnings FATAL => qw( all );

use Test::More tests => 14;

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

    $neb->lock_object("foo", "read");

    ok($neb->unlock_object("foo", "read"), "read unlock");
}

Test::Nebulous->setup;

{
    $neb->create_object("foo");

    $neb->lock_object("foo", "read");
    $neb->lock_object("foo", "read");

    ok($neb->unlock_object("foo", "read"), "read unlock");
    ok($neb->unlock_object("foo", "read"), "read unlock");
}

Test::Nebulous->setup;

{
    $neb->create_object("foo");

    $neb->lock_object("foo", "write");

    ok($neb->unlock_object("foo", "write"), "write unlock");
}

Test::Nebulous->setup;

eval {
    $neb->unlock_object("foo", "read");
};
like($@, qr/is valid object key/, "storage object does not exist");

Test::Nebulous->setup;

eval {
    $neb->unlock_object("foo", "write");
};
like($@, qr/is valid object key/, "storage object does not exist");

Test::Nebulous->setup;

eval {
    $neb->create_object("foo");

    $neb->unlock_object("foo", "read");
};
like($@, qr/can not remove non-existant read lock/, "no lock set");

Test::Nebulous->setup;

eval {
    $neb->create_object("foo");

    $neb->unlock_object("foo", "write");
};
like($@, qr/can not remove non-existant write lock/, "no lock set");

Test::Nebulous->setup;

eval {
    $neb->create_object("foo");
    $neb->lock_object("foo", "write");

    $neb->unlock_object("foo", "read");
};
like($@, qr/can not have a read lock under a write lock/, "read unlock under write lock");

Test::Nebulous->setup;

eval {
    $neb->create_object("foo");
    $neb->lock_object("foo", "read");

    $neb->unlock_object("foo", "write");
};
like($@, qr/can not have a write lock under a read lock/, "write unlock under read lock");

Test::Nebulous->setup;

eval {
    $neb->unlock_object();
};
like($@, qr/2 were expected/, "no params");

Test::Nebulous->setup;

eval {
    $neb->create_object("foo");

    $neb->unlock_object("foo");
};
like($@, qr/2 were expected/, "not enough params");

Test::Nebulous->setup;

eval {
    $neb->create_object("foo");

    $neb->unlock_object("foo", "both");
};
like($@, qr/is read or write/, "not read or write");

Test::Nebulous->setup;

eval {
    $neb->create_object("foo");

    $neb->unlock_object("foo", 'read', 3);
};
like($@, qr/2 were expected/, "too many params");

Test::Nebulous->cleanup;
