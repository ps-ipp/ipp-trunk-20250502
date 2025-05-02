#!/usr/bin/perl

# Copryight (C) 2007  Joshua Hoblitt
#
# $Id: 13_server_rename_object.t,v 1.5 2008-05-16 20:29:19 jhoblitt Exp $

use strict;
use warnings FATAL => qw( all );

use Test::More tests => 8;

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
    my $key = "bar";
    my $uri = $neb->create_object("foo");

    $neb->rename_object("foo", $key);

    expected_dataset_ok(
        directory       => [dir_id => 1, dirname => '/', parent_id => 1],
        storage_object  => [so_id => 1, ext_id => $key, ext_id_basename => basename($key), dir_id => 1],
    );
}

Test::Nebulous->setup;

{
    my $key = "a/bar";
    my $uri = $neb->create_object("foo");

    $neb->rename_object("foo", $key);

    expected_dataset_ok(
        directory       => [dir_id => 1, dirname => '/', parent_id => 1],
        directory       => [dir_id => 2, dirname => 'a', parent_id => 1],
        storage_object  => [so_id => 1, ext_id => $key, ext_id_basename => basename($key), dir_id => 2],
    );
}

Test::Nebulous->setup;

{
    my $key = "bar";
    my $uri = $neb->create_object("a/foo");

    $neb->rename_object("a/foo", $key);

    expected_dataset_ok(
        directory       => [dir_id => 1, dirname => '/', parent_id => 1],
        directory       => [dir_id => 2, dirname => 'a', parent_id => 1],
        storage_object  => [so_id => 1, ext_id => $key, ext_id_basename => basename($key), dir_id => 1],
    );
}

Test::Nebulous->setup;

# destination key exists
eval {
    $neb->create_object('foo');
    $neb->create_object('bar');

    $neb->rename_object('foo', 'bar');
};
like($@, qr/is not valid object key/, "too few params");

Test::Nebulous->setup;

eval {
    $neb->rename_object();
};
like($@, qr/2 were expected/, "no params");

Test::Nebulous->setup;

eval {
    $neb->create_object("foo");

    $neb->rename_object("foo");
};
like($@, qr/2 were expected/, "too few params");

Test::Nebulous->setup;

eval {
    $neb->create_object("foo");

    $neb->rename_object("foo", "bar", "baz");
};
like($@, qr/2 were expected/, "too many params");

# test attempting to rename a key in such a way to cause the distributed
# storage db to change
# this must be the last test as we're messing with the $neb object
eval {
    $neb->config->add_db(
        dbindex     => 1,
        dsn         => $NEB_DB,
        dbuser      => $NEB_USER,
        dbpasswd    => $NEB_PASS,
    );

    $neb->create_object("a/foo");
    $neb->rename_object("a/foo", "g/bar");
};
like($@, qr/rename objects across distributed database boundaries/, "rename between databases");

Test::Nebulous->cleanup;
