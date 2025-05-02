#!/usr/bin/perl

# Copryight (C) 2004-2005  Joshua Hoblitt
#
# $Id: 03_server_create_object.t,v 1.30 2008-09-11 22:35:52 jhoblitt Exp $

use strict;
use warnings FATAL => qw( all );

use Test::More tests => 99;

use lib qw( ./t ./lib );

use File::Basename qw( basename );
use File::ExtAttr qw( getfattr );
use Nebulous::Server;
use Test::Nebulous;
use Test::URI;
use URI::Split qw( uri_split );

my $test_xattr = undef;

my $neb = Nebulous::Server->new(
    dsn         => $NEB_DB,
    dbuser      => $NEB_USER,
    dbpasswd    => $NEB_PASS,
);

use Test::DBUnit dsn => $NEB_DB, username => $NEB_USER, password => $NEB_PASS;

Test::Nebulous->setup;
{
    # key
    my $uri = $neb->create_object("foo");

    my ($scheme, $auth, $path, $query, $frag) = uri_split($uri);
    ok(-e $path, "file exists");
    ok($neb->find_instances("foo"), 'object key exists');
    uri_scheme_ok($uri, 'file');

  SKIP: {
      skip "requires xattr support", 1 unless $test_xattr;
      is(getfattr($path, 'user.nebulous_key'), 'foo', 'user.nebulous_key xattr');
    }
}

Test::Nebulous->setup;
{
    # key
    my $uri = $neb->create_object("neb:/foo");

    my ($scheme, $auth, $path, $query, $frag) = uri_split($uri);
    ok(-e $path, "file exists");
    ok($neb->find_instances("foo"), 'object key exists');
    uri_scheme_ok($uri, 'file');

  SKIP: {
      skip "requires xattr support", 1 unless $test_xattr;
      is(getfattr($path, 'user.nebulous_key'), 'foo', 'user.nebulous_key xattr');
    }
}

Test::Nebulous->setup;

{
    # key
    my $uri = $neb->create_object("neb://node01/foo");

    my ($scheme, $auth, $path, $query, $frag) = uri_split($uri);
    ok(-e $path, "file exists");
    ok($neb->find_instances("foo"), 'object key exists');
    uri_scheme_ok($uri, 'file');

SKIP: {
    skip "requires xattr support", 1 unless $test_xattr;
    is(getfattr($path, 'user.nebulous_key'), 'foo', 'user.nebulous_key xattr');
}
}

Test::Nebulous->setup;

{
    # key
    my $uri = $neb->create_object("neb://node02/foo");

    my ($scheme, $auth, $path, $query, $frag) = uri_split($uri);
    ok(-e $path, "file exists");
    ok($neb->find_instances("foo"), 'object key exists');
    uri_scheme_ok($uri, 'file');

SKIP: {
    skip "requires xattr support", 1 unless $test_xattr;
    is(getfattr($path, 'user.nebulous_key'), 'foo', 'user.nebulous_key xattr');
}
}

Test::Nebulous->setup;

{
    # key
    my $uri = $neb->create_object("/foo");

    my ($scheme, $auth, $path, $query, $frag) = uri_split($uri);
    ok(-e $path, "file exists");
    ok($neb->find_instances("foo"), 'object key exists');
    uri_scheme_ok($uri, 'file');

SKIP: {
    skip "requires xattr support", 1 unless $test_xattr;
    is(getfattr($path, 'user.nebulous_key'), '/foo', 'user.nebulous_key xattr');
}
}

Test::Nebulous->setup;
{
    # key
    my $uri = $neb->create_object("neb:///foo");

    my ($scheme, $auth, $path, $query, $frag) = uri_split($uri);
    ok(-e $path, "file exists");
    ok($neb->find_instances("foo"), 'object key exists');
    uri_scheme_ok($uri, 'file');

SKIP: {
    skip "requires xattr support", 1 unless $test_xattr;
    is(getfattr($path, 'user.nebulous_key'), 'foo', 'user.nebulous_key xattr');
}
}

Test::Nebulous->setup;

{
    # key
    my $uri = $neb->create_object("/foo/");

    my ($scheme, $auth, $path, $query, $frag) = uri_split($uri);
    ok(-e $path, "file exists");
    ok($neb->find_instances("foo"), 'object key exists');
    uri_scheme_ok($uri, 'file');

SKIP: {
    skip "requires xattr support", 1 unless $test_xattr;
    is(getfattr($path, 'user.nebulous_key'), '/foo/', 'user.nebulous_key xattr');
}
}

Test::Nebulous->setup;

{
    # key
    my $uri = $neb->create_object("neb:/foo/");

    my ($scheme, $auth, $path, $query, $frag) = uri_split($uri);
    ok(-e $path, "file exists");
    ok($neb->find_instances("foo"), 'object key exists');
    uri_scheme_ok($uri, 'file');

SKIP: {
    skip "requires xattr support", 1 unless $test_xattr;
    is(getfattr($path, 'user.nebulous_key'), '/foo/', 'user.nebulous_key xattr');
}
}

Test::Nebulous->setup;

{
    # key
    my $uri = $neb->create_object("foo/");

    my ($scheme, $auth, $path, $query, $frag) = uri_split($uri);
    ok(-e $path, "file exists");
    ok($neb->find_instances("foo"), 'object key exists');
    uri_scheme_ok($uri, 'file');

SKIP: {
    skip "requires xattr support", 1 unless $test_xattr;
    is(getfattr($path, 'user.nebulous_key'), 'foo/', 'user.nebulous_key xattr');
}
}

Test::Nebulous->setup;

{
    # key
    my $uri = $neb->create_object("foo/bar");

    my ($scheme, $auth, $path, $query, $frag) = uri_split($uri);
    ok(-e $path, "file exists");
    ok($neb->find_instances("foo/bar"), 'object key exists');
    uri_scheme_ok($uri, 'file');

SKIP: {
    skip "requires xattr support", 1 unless $test_xattr;
    is(getfattr($path, 'user.nebulous_key'), 'foo/bar', 'user.nebulous_key xattr');
}
}

Test::Nebulous->setup;

{
    # key
    my $uri = $neb->create_object("/foo/bar");

    my ($scheme, $auth, $path, $query, $frag) = uri_split($uri);
    ok(-e $path, "file exists");
    ok($neb->find_instances("foo/bar"), 'object key exists');
    uri_scheme_ok($uri, 'file');

SKIP: {
    skip "requires xattr support", 1 unless $test_xattr;
    is(getfattr($path, 'user.nebulous_key'), '/foo/bar', 'user.nebulous_key xattr');
}
}

Test::Nebulous->setup;

{
    # key, volume
    my $uri = $neb->create_object("foo", "node01");

    my ($scheme, $auth, $path, $query, $frag) = uri_split($uri);
    ok(-e $path, "file exists");
    ok($neb->find_instances("foo"), 'object key exists');
    uri_scheme_ok($uri, 'file');
}

Test::Nebulous->setup;

{
    # volume name override
    my $uri = $neb->create_object("neb://node02/foo", "node01");

    my ($scheme, $auth, $path, $query, $frag) = uri_split($uri);
    ok(-e $path, "file exists");
    ok($neb->find_instances("foo"), 'object key exists');
    uri_scheme_ok($uri, 'file');
}

Test::Nebulous->setup;

{
    # volume name override 
    # OK because the volume arg overrides the key's  implied volume
    my $uri = $neb->create_object("neb://99/foo", "node01");

    my ($scheme, $auth, $path, $query, $frag) = uri_split($uri);
    ok(-e $path, "file exists");
    ok($neb->find_instances("foo"), 'object key exists');
    uri_scheme_ok($uri, 'file');
}

Test::Nebulous->setup;

{
    # undef volume name
    # OK because the undef is ignored
    my $uri = $neb->create_object("neb://node01/foo", undef);

    my ($scheme, $auth, $path, $query, $frag) = uri_split($uri);
    ok(-e $path, "file exists");
    ok($neb->find_instances("foo"), 'object key exists');
    uri_scheme_ok($uri, 'file');
}

Test::Nebulous->setup;

{
    # soft volume name request
    my $uri = $neb->create_object("neb://~node01/foo");

    my ($scheme, $auth, $path, $query, $frag) = uri_split($uri);
    ok(-e $path, "file exists");
    ok($neb->find_instances("foo"), 'object key exists');
    uri_scheme_ok($uri, 'file');
}

Test::Nebulous->setup;

{
    # soft volume name request
    my $uri = $neb->create_object("neb://node01/foo", "~node02");

    my ($scheme, $auth, $path, $query, $frag) = uri_split($uri);
    ok(-e $path, "file exists");
    ok($neb->find_instances("foo"), 'object key exists');
    uri_scheme_ok($uri, 'file');
}

Test::Nebulous->setup;

{
    # any volume name request
    my $uri = $neb->create_object("neb://any/foo");

    my ($scheme, $auth, $path, $query, $frag) = uri_split($uri);
    ok(-e $path, "file exists");
    ok($neb->find_instances("foo"), 'object key exists');
    uri_scheme_ok($uri, 'file');
}

Test::Nebulous->setup;

{
    # any volume name request
    my $uri = $neb->create_object("neb://~any/foo");

    my ($scheme, $auth, $path, $query, $frag) = uri_split($uri);
    ok(-e $path, "file exists");
    ok($neb->find_instances("foo"), 'object key exists');
    uri_scheme_ok($uri, 'file');
}

Test::Nebulous->setup;

{
    # any volume name request
    my $uri = $neb->create_object("neb://node01/foo", "any");

    my ($scheme, $auth, $path, $query, $frag) = uri_split($uri);
    ok(-e $path, "file exists");
    ok($neb->find_instances("foo"), 'object key exists');
    uri_scheme_ok($uri, 'file');
}

Test::Nebulous->setup;

{
    # any volume name request
    my $uri = $neb->create_object("neb://node01/foo", "~any");

    my ($scheme, $auth, $path, $query, $frag) = uri_split($uri);
    ok(-e $path, "file exists");
    ok($neb->find_instances("foo"), 'object key exists');
    uri_scheme_ok($uri, 'file');
}

# test for properly row creation in the directories table

Test::Nebulous->setup;

{
    my $key = "foo";
    $neb->create_object($key);

    expected_dataset_ok(
        directory       => [dir_id => 1, dirname => '/', parent_id => 1],
        storage_object  => [so_id => 1, ext_id => $key, dir_id => 1],
    );
}

Test::Nebulous->setup;

{
    my $key = "a/foo";
    $neb->create_object($key);

    expected_dataset_ok(
        directory       => [dir_id => 1, dirname => '/', parent_id => 1],
        directory       => [dir_id => 2, dirname => 'a', parent_id => 1],
        storage_object  => [so_id => 1, ext_id => $key, ext_id_basename => basename($key), dir_id => 2],
    );

}

Test::Nebulous->setup;

{
    my $key = "a/b/foo";
    $neb->create_object($key);

    expected_dataset_ok(
        directory       => [dir_id => 1, dirname => '/', parent_id => 1],
        directory       => [dir_id => 2, dirname => 'a', parent_id => 1],
        directory       => [dir_id => 3, dirname => 'b', parent_id => 2],
        storage_object  => [so_id => 1, ext_id => $key, ext_id_basename => basename($key), dir_id => 3],
    );
}

Test::Nebulous->setup;

{
    my $key = "a/b/c/foo";
    $neb->create_object($key);

    expected_dataset_ok(
        directory       => [dir_id => 1, dirname => '/', parent_id => 1],
        directory       => [dir_id => 2, dirname => 'a', parent_id => 1],
        directory       => [dir_id => 3, dirname => 'b', parent_id => 2],
        directory       => [dir_id => 4, dirname => 'c', parent_id => 3],
        storage_object  => [so_id => 1, ext_id => $key, ext_id_basename => basename($key), dir_id => 4],
    );
}

Test::Nebulous->setup;

{
    my $key1 = "a/b/c/foo";
    my $key2 = "foo";
    $neb->create_object($key1);
    $neb->create_object($key2);

    expected_dataset_ok(
        directory       => [dir_id => 1, dirname => '/', parent_id => 1],
        storage_object  => [so_id => 2, ext_id => $key2, ext_id_basename => basename($key2), dir_id => 1],
        directory       => [dir_id => 2, dirname => 'a', parent_id => 1],
        directory       => [dir_id => 3, dirname => 'b', parent_id => 2],
        directory       => [dir_id => 4, dirname => 'c', parent_id => 3],
        storage_object  => [so_id => 1, ext_id => $key1, ext_id_basename => basename($key1), dir_id => 4],
    );
}

Test::Nebulous->setup;

{
    my $key1 = "a/b/c/foo";
    my $key2 = "a/foo";
    $neb->create_object($key1);
    $neb->create_object($key2);

    expected_dataset_ok(
        directory       => [dir_id => 1, dirname => '/', parent_id => 1],
        directory       => [dir_id => 2, dirname => 'a', parent_id => 1],
        storage_object  => [so_id => 2, ext_id => $key2, ext_id_basename => basename($key2), dir_id => 2],
        directory       => [dir_id => 3, dirname => 'b', parent_id => 2],
        directory       => [dir_id => 4, dirname => 'c', parent_id => 3],
        storage_object  => [so_id => 1, ext_id => $key1, ext_id_basename => basename($key1), dir_id => 4],
    );
}

Test::Nebulous->setup;

{
    my $key1 = "a/b/c/foo";
    my $key2 = "d/foo";
    $neb->create_object($key1);
    $neb->create_object($key2);

    expected_dataset_ok(
        directory       => [dir_id => 1, dirname => '/', parent_id => 1],
        directory       => [dir_id => 2, dirname => 'a', parent_id => 1],
        directory       => [dir_id => 3, dirname => 'b', parent_id => 2],
        directory       => [dir_id => 4, dirname => 'c', parent_id => 3],
        storage_object  => [so_id => 1, ext_id => $key1, ext_id_basename => basename($key1), dir_id => 4],
        directory       => [dir_id => 5, dirname => 'd', parent_id => 1],
        storage_object  => [so_id => 2, ext_id => $key2, ext_id_basename => basename($key2), dir_id => 5],
    );
}

Test::Nebulous->setup;

{
    my $key1 = "a/b/c/foo";
    my $key2 = "a/d/foo";
    $neb->create_object($key1);
    $neb->create_object($key2);

    expected_dataset_ok(
        directory       => [dir_id => 1, dirname => '/', parent_id => 1],
        directory       => [dir_id => 2, dirname => 'a', parent_id => 1],
        directory       => [dir_id => 3, dirname => 'b', parent_id => 2],
        directory       => [dir_id => 4, dirname => 'c', parent_id => 3],
        storage_object  => [so_id => 1, ext_id => $key1, ext_id_basename => basename($key1), dir_id => 4],
        directory       => [dir_id => 5, dirname => 'd', parent_id => 2],
        storage_object  => [so_id => 2, ext_id => $key2, ext_id_basename => basename($key2), dir_id => 5],
    );
}

Test::Nebulous->setup;

{
    my $key1 = "a/b/c/foo";
    my $key2 = "d/a/foo";
    $neb->create_object($key1);
    $neb->create_object($key2);

    expected_dataset_ok(
        directory       => [dir_id => 1, dirname => '/', parent_id => 1],
        directory       => [dir_id => 2, dirname => 'a', parent_id => 1],
        directory       => [dir_id => 3, dirname => 'b', parent_id => 2],
        directory       => [dir_id => 4, dirname => 'c', parent_id => 3],
        storage_object  => [so_id => 1, ext_id => $key1, ext_id_basename => basename($key1), dir_id => 4],
        directory       => [dir_id => 5, dirname => 'd', parent_id => 1],
        directory       => [dir_id => 6, dirname => 'a', parent_id => 5],
        storage_object  => [so_id => 2, ext_id => $key2, ext_id_basename => basename($key2), dir_id => 6],
    );
}

Test::Nebulous->setup;

{
    my $key1 = "a/b/c/foo";
    my $key2 = "a/b/c/d/foo";
    $neb->create_object($key1);
    $neb->create_object($key2);

    expected_dataset_ok(
        directory       => [dir_id => 1, dirname => '/', parent_id => 1],
        directory       => [dir_id => 2, dirname => 'a', parent_id => 1],
        directory       => [dir_id => 3, dirname => 'b', parent_id => 2],
        directory       => [dir_id => 4, dirname => 'c', parent_id => 3],
        storage_object  => [so_id => 1, ext_id => $key1, ext_id_basename => basename($key1), dir_id => 4],
        directory       => [dir_id => 5, dirname => 'd', parent_id => 4],
        storage_object  => [so_id => 2, ext_id => $key2, ext_id_basename => basename($key2), dir_id => 5],
    );
}

Test::Nebulous->setup;

eval {
    $neb->create_object("foo");
    $neb->create_object("foo");
};
like($@, qr/Duplicate entry/, "object already exists");

Test::Nebulous->setup;

eval {
    $neb->create_object("neb:/foo");
    $neb->create_object("neb:/foo");
};
like($@, qr/Duplicate entry/, "object already exists");

Test::Nebulous->setup;

eval {
    $neb->create_object("foo", '~node07');
};
like($@, qr/node07 is not available/, "request volume which is full");

Test::Nebulous->setup;

eval {
    $neb->create_object("neb://~node07/foo");
};
like($@, qr/node07 is not available/, "request volume which is full");

Test::Nebulous->setup;

eval {
    $neb->create_object("foo", '~node04');
};
like($@, qr/node04 is not available/, "request volume with allocate = FALSE, available = FALSE");

Test::Nebulous->setup;

eval {
    $neb->create_object("neb://~node04/foo");
};
like($@, qr/node04 is not available/, "request volume with allocate = FALSE, available = FALSE");

Test::Nebulous->setup;

eval {
    $neb->create_object("foo", '~node05');
};
like($@, qr/node05 is not available/, "request volume with allocate = FALSE , available = TRUE");

Test::Nebulous->setup;

eval {
    $neb->create_object("neb://~node05/foo");
};
like($@, qr/node05 is not available/, "request volume with allocate = FALSE , available = TRUE");

Test::Nebulous->setup;

eval {
    $neb->create_object("foo", '~node06');
};
like($@, qr/node06 is not available/, "request volume with allocate = TRUE, available = FALSE");

Test::Nebulous->setup;

eval {
    $neb->create_object("neb://~node06/foo");
};
like($@, qr/node06 is not available/, "request volume with allocate = TRUE, available = FALSE");

Test::Nebulous->setup;

{
    ok($neb->create_object("foo", 99));
}

Test::Nebulous->setup;

eval {
    $neb->create_object("foo", "~99");
};
like($@, qr/is not a valid volume name/, "volume name doesn't exist");

Test::Nebulous->setup;

eval {
    $neb->create_object("neb://~99/foo");
};
like($@, qr/is not a valid volume name/, "volume name doesn't exist");

Test::Nebulous->setup;

eval {
    $neb->create_object();
};
like($@, qr/1 - 2 were expected/, "no params");

Test::Nebulous->setup;

eval {
    $neb->create_object(1, "node01", 3);
};
like($@, qr/1 - 2 were expected/, "too many params");

Test::Nebulous->cleanup;
