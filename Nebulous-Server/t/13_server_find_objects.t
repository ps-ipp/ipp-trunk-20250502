#!/usr/bin/perl

# Copryight (C) 2004-2005  Joshua Hoblitt
#
# $Id: 12_server_find_objects.t,v 1.4 2008-05-16 20:29:19 jhoblitt Exp $

use strict;
use warnings FATAL => qw( all );

use Test::More tests => 36;

use lib qw( ./t ./lib );

use Nebulous::Server;
use Test::Nebulous;

my $neb = Nebulous::Server->new(
    dsn         => $NEB_DB,
    dbuser      => $NEB_USER,
    dbpasswd    => $NEB_PASS,
);

Test::Nebulous->setup;

# search for a regex of '' should match nothing
eval {
    $neb->create_object("foo");

    my $keys = $neb->find_objects();
};
like($@, qr/no keys found/, "no keys found");

Test::Nebulous->setup;

{
    $neb->create_object("foo");

    my $keys = $neb->find_objects("foo");

    is(scalar @$keys, 1, 'number of keys found');
    is($keys->[0], "foo", "key name");
}

Test::Nebulous->setup;

{
    # key
    $neb->create_object("foo");
    $neb->replicate_object("foo");

    my $keys = $neb->find_objects("foo");

    is(scalar @$keys, 1, 'number of keys found');
    is($keys->[0], "foo", "key name");
}

Test::Nebulous->setup;

{
    # key
    $neb->create_object("foo");
    $neb->create_object("bar");

    my $keys = $neb->find_objects("foo");

    is(scalar @$keys, 1, 'number of keys found');
    is($keys->[0], "foo", "key name");
}

# test recursive dir searching
Test::Nebulous->setup;

{
    $neb->create_object("a/foo");

    my $keys = $neb->find_objects("a");

    is(scalar @$keys, 1, 'number of keys found');
    is($keys->[0], "a/foo", "key name");
}

Test::Nebulous->setup;

{
    $neb->create_object("a/foo");
    $neb->create_object("b/foo");

    my $keys = $neb->find_objects("a");

    is(scalar @$keys, 1, 'number of keys found');
    is($keys->[0], "a/foo", "key name");
}

Test::Nebulous->setup;

{
    $neb->create_object("a/foo");
    $neb->create_object("a/b/foo");

    my $keys = $neb->find_objects("a");

    is(scalar @$keys, 2, 'number of keys found');
    is($keys->[0], "a/b/", "key name");
    is($keys->[1], "a/foo", "key name");
}

Test::Nebulous->setup;

{
    $neb->create_object("a/foo");
    $neb->create_object("a/bar");

    my $keys = $neb->find_objects("a");

    is(scalar @$keys, 2, 'number of keys found');
    is($keys->[0], "a/bar", "key name");
    is($keys->[1], "a/foo", "key name");
}

Test::Nebulous->setup;

{
    $neb->create_object("a/foo");
    $neb->create_object("bar");

    my $keys = $neb->find_objects("a");

    is(scalar @$keys, 1, 'number of keys found');
    is($keys->[0], "a/foo", "key name");
}

Test::Nebulous->setup;

{
    $neb->create_object("a/foo");
    $neb->create_object("foo");
    $neb->create_object("bar");

    my $keys = $neb->find_objects("/");

    is(scalar @$keys, 3, 'number of keys found');
    is($keys->[0], "a/", "key name");
    is($keys->[1], "bar", "key name");
    is($keys->[2], "foo", "key name");
}

Test::Nebulous->setup;

{
    $neb->create_object("a/foo");
    $neb->create_object("foo");
    $neb->create_object("bar");

    my $keys = $neb->find_objects(".");

    is(scalar @$keys, 3, 'number of keys found');
    is($keys->[0], "a/", "key name");
    is($keys->[1], "bar", "key name");
    is($keys->[2], "foo", "key name");
}

Test::Nebulous->setup;

{
    $neb->create_object("a/foo");
    $neb->create_object("foo");
    $neb->create_object("bar");

    my $keys = $neb->find_objects("..");

    is(scalar @$keys, 3, 'number of keys found');
    is($keys->[0], "a/", "key name");
    is($keys->[1], "bar", "key name");
    is($keys->[2], "foo", "key name");
}

Test::Nebulous->setup;

{
    $neb->create_object("a/bar");
    $neb->create_object("b/foo");
    $neb->create_object("foo");

    my $keys = $neb->find_objects("/");

    is(scalar @$keys, 3, 'number of keys found');
    is($keys->[0], "a/", "key name");
    is($keys->[1], "b/", "key name");
    is($keys->[2], "foo", "key name");
}

Test::Nebulous->setup;

eval {
    $neb->find_objects("foo", 3);
};
like($@, qr/1 was expected/, "too many params");

Test::Nebulous->cleanup;
