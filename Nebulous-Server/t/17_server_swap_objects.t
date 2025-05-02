#!/usr/bin/perl

# Copryight (C) 2007  Joshua Hoblitt
#
# $Id: 16_server_swap_objects.t,v 1.1 2008-10-13 20:41:17 jhoblitt Exp $

use strict;
use warnings FATAL => qw( all );

use Test::More tests => 13;

use lib qw( ./t ./lib );

use File::Basename qw( basename );
use Nebulous::Server;
use Test::Nebulous;

my $neb = Nebulous::Server->new(
    dsn         => $NEB_DB,
    dbuser      => $NEB_USER,
    dbpasswd    => $NEB_PASS,
);

use Test::DBUnit dsn => $NEB_DB, username => $NEB_USER, password => $NEB_PASS;

Test::Nebulous->setup;

{
    my $key1 = "foo1";
    my $key2 = "foo2";
    my $uri1 = $neb->create_object($key1);
    my $uri2 = $neb->create_object($key2);

    ok($neb->swap_objects($key1, $key2), "swap succeeded");

    my $new_uri1 = ($neb->find_instances($key1))->[0];
    my $new_uri2 = ($neb->find_instances($key2))->[0];

    expected_dataset_ok(
        directory       => [dir_id => 1, dirname => '/', parent_id => 1],
        storage_object  => [so_id => 1, ext_id => $key2, ext_id_basename => basename($key2), dir_id => 1],
        storage_object  => [so_id => 2, ext_id => $key1, ext_id_basename => basename($key1), dir_id => 1],
    );

    is($uri1, $new_uri2, "key1 -> key2");
    is($uri2, $new_uri1, "key2 -> key1");
}

Test::Nebulous->setup;

{
    my $key1 = "foo1";
    my $key2 = "a/foo2";
    my $uri1 = $neb->create_object($key1);
    my $uri2 = $neb->create_object($key2);

    ok($neb->swap_objects($key1, $key2), "swap succeeded");

    my $new_uri1 = ($neb->find_instances($key1))->[0];
    my $new_uri2 = ($neb->find_instances($key2))->[0];

    expected_dataset_ok(
        directory       => [dir_id => 1, dirname => '/', parent_id => 1],
        directory       => [dir_id => 2, dirname => 'a', parent_id => 1],
        storage_object  => [so_id => 1, ext_id => $key2, ext_id_basename => basename($key2), dir_id => 2],
        storage_object  => [so_id => 2, ext_id => $key1, ext_id_basename => basename($key1), dir_id => 1],
    );

    is($uri1, $new_uri2, "key1 -> key2");
    is($uri2, $new_uri1, "key2 -> key1");
}

Test::Nebulous->setup;

eval {
    $neb->create_object('bar');

    $neb->swap_objects('foo', 'bar');
};
like($@, qr/is valid object key/, "key1 doesn't exist");

Test::Nebulous->setup;

eval {
    $neb->create_object('foo');

    $neb->swap_objects('foo', 'bar');
};
like($@, qr/is valid object key/, "key2 doesn't exist");

Test::Nebulous->setup;

eval {
    $neb->swap_objects();
};
like($@, qr/2 were expected/, "no params");

Test::Nebulous->setup;

eval {
    $neb->create_object("foo");

    $neb->swap_objects("foo");
};
like($@, qr/2 were expected/, "too few params");

Test::Nebulous->setup;

eval {
    $neb->create_object("foo");
    $neb->create_object("bar");

    $neb->swap_objects("foo", "bar", "baz");
};
like($@, qr/2 were expected/, "too many params");

Test::Nebulous->cleanup;
