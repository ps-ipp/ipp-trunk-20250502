#!/usr/bin/perl

# Copryight (C) 2004-2005  Joshua Hoblitt
#
# $Id: 04_server_replicate_object.t,v 1.16 2008-09-11 22:35:52 jhoblitt Exp $

use strict;
use warnings FATAL => qw( all );

use Test::More tests => 43;

use lib qw( ./t ./lib );

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
    memcached_servers => $NEB_MEMCACHED_SERVERS,
);

Test::Nebulous->setup;
{
    # key
    $neb->create_object("foo");
    my $uri = $neb->replicate_object("foo");

    my ($scheme, $auth, $path, $query, $frag) = uri_split($uri);
    ok(-e $path, "file exists");
    uri_scheme_ok($uri, 'file');

SKIP: {
    skip "requires xattr support", 1 unless $test_xattr;
    is(getfattr($path, 'user.nebulous_key'), 'foo', 'user.nebulous_key xattr');
}
}

Test::Nebulous->setup;
{
    # key, $volume
    $neb->create_object("foo");
    my $uri = $neb->replicate_object("foo", "node01");

    my ($scheme, $auth, $path, $query, $frag) = uri_split($uri);
    ok(-e $path, "file exists");
    uri_scheme_ok($uri, 'file');

SKIP: {
    skip "requires xattr support", 1 unless $test_xattr;
    is(getfattr($path, 'user.nebulous_key'), 'foo', 'user.nebulous_key xattr');
}
}

Test::Nebulous->setup;
{
    # key, $volume is undef
    $neb->create_object("foo");
    my $uri = $neb->replicate_object("foo", undef);

    my ($scheme, $auth, $path, $query, $frag) = uri_split($uri);
    ok(-e $path, "file exists");
    uri_scheme_ok($uri, 'file');

SKIP: {
    skip "requires xattr support", 1 unless $test_xattr;
    is(getfattr($path, 'user.nebulous_key'), 'foo', 'user.nebulous_key xattr');
}
}

Test::Nebulous->setup;
{
    # key, $volume
    $neb->create_object("foo");
    my $uri1 = $neb->replicate_object("foo", "~node01");
    my $uri2 = $neb->replicate_object("foo", "~node02");

    {
        my ($scheme, $auth, $path, $query, $frag) = uri_split($uri1);
        ok(-e $path, "file exists");
        uri_scheme_ok($uri1, 'file');

SKIP: {
    skip "requires xattr support", 1 unless $test_xattr;
        is(getfattr($path, 'user.nebulous_key'), 'foo', 'user.nebulous_key xattr');
}
    }

    {
        my ($scheme, $auth, $path, $query, $frag) = uri_split($uri2);
        ok(-e $path, "file exists");
        uri_scheme_ok($uri2, 'file');

SKIP: {
    skip "requires xattr support", 1 unless $test_xattr;
        is(getfattr($path, 'user.nebulous_key'), 'foo', 'user.nebulous_key xattr');
}
    }
}

Test::Nebulous->setup;

{
    # key, $volume
    $neb->create_object("foo");
    my $uri1 = $neb->replicate_object("foo", "any");

    {
        my ($scheme, $auth, $path, $query, $frag) = uri_split($uri1);
        ok(-e $path, "file exists");
        uri_scheme_ok($uri1, 'file');

SKIP: {
    skip "requires xattr support", 1 unless $test_xattr;
        is(getfattr($path, 'user.nebulous_key'), 'foo', 'user.nebulous_key xattr');
}
    }
}

Test::Nebulous->setup;

{
    # key, $volume
    $neb->create_object("foo");
    my $uri1 = $neb->replicate_object("foo", "~any");

    {
        my ($scheme, $auth, $path, $query, $frag) = uri_split($uri1);
        ok(-e $path, "file exists");
        uri_scheme_ok($uri1, 'file');

SKIP: {
    skip "requires xattr support", 1 unless $test_xattr;
        is(getfattr($path, 'user.nebulous_key'), 'foo', 'user.nebulous_key xattr');
}
    }
}

# hard_volume tests:

# replication with hard_volume:
Test::Nebulous->setup;
{
    my @nodedirs = Test::Nebulous->get_node_dirs();

    # checking on replicate_object: hard_volume as node target
    my $uri1 = $neb->create_object("foo", "~node01");
    my $uri2 = $neb->replicate_object("foo", "~node02");

    my $locations = $neb->find_instances("foo");
    my $Nloc = @$locations;

    is($Nloc, 2, "found 2 instances");
    
    my $loc1 = $locations->[0];
    my $loc2 = $locations->[1];

    like($loc1, qr/$nodedirs[0]/, "instance matches expected node");
    like($loc2, qr/$nodedirs[1]/, "instance matches expected node");
}

# replication to unknown volume: should succeed
Test::Nebulous->setup;
{
    my @nodedirs = Test::Nebulous->get_node_dirs();

    # checking on replicate_object, does this logic make sense?
    my $uri1 = $neb->create_object("foo", "~node01");
    my $uri2 = $neb->replicate_object("foo", "dummy");

    my $locations = $neb->find_instances("foo");
    my $Nloc = @$locations;

    is($Nloc, 2, "found 2 instances");
    
    my $loc1 = $locations->[0];
    my $loc2 = $locations->[1];

    like($loc1, qr/$nodedirs[0]/, "instance matches expected node");
    uri_scheme_ok($loc2, 'file');
}

# replication to unknown volume with hard_volume: should fail
Test::Nebulous->setup;
eval {
    my @nodedirs = Test::Nebulous->get_node_dirs();

    # checking on replicate_object, does this logic make sense?
    my $uri1 = $neb->create_object("foo", "~node01");
    my $uri2 = $neb->replicate_object("foo", "~dummy");

    my $locations = $neb->find_instances("foo");
};
like($@, qr/is not a valid volume name/, "replicate to unknown node with hard_volume should fail");

# replication to unavailable volume: should succeed
Test::Nebulous->setup;
{
    my @nodedirs = Test::Nebulous->get_node_dirs();

    # checking on replicate_object, does this logic make sense?
    my $uri1 = $neb->create_object("foo", "~node01");
    my $uri2 = $neb->replicate_object("foo", "node04");

    my $locations = $neb->find_instances("foo");
    my $Nloc = @$locations;

    is($Nloc, 2, "found 2 instances");
    
    my $loc1 = $locations->[0];
    my $loc2 = $locations->[1];

    like($loc1, qr/$nodedirs[0]/, "instance matches expected node");
    uri_scheme_ok($loc2, 'file');
}

# replication to unavailable volume with hard_volume: should fail
Test::Nebulous->setup;
eval {
    my @nodedirs = Test::Nebulous->get_node_dirs();

    # checking on replicate_object, does this logic make sense?
    my $uri1 = $neb->create_object("foo", "~node01");
    my $uri2 = $neb->replicate_object("foo", "~node04");

    my $locations = $neb->find_instances("foo");
};
like($@, qr/is not available/, "replicate to unknown node with hard_volume should fail");

### END hard_volume tests

### location-aware tests (do we respect the cab_id / vol_id exclusions?)

# here are the volume definitions (Test/Nebulous.pm):
# name   : vol_id : cab_id : site_id : xattr
# node01 :      1 :      1 :       1 :     0
# node02 :      2 :      2 :       1 :     0
# node03 :      3 :      3 :       2 :     0
# node04 :      4 :      1 :       1 :     0
# node05 :      5 :      1 :       1 :     0
# node06 :      6 :      1 :       1 :     0
# node07 :      7 :      1 :       1 :     0
# node08 :      8 :      4 :       2 :     0

# replication without volume 
Test::Nebulous->setup;
{
    my @nodedirs = Test::Nebulous->get_node_dirs();

    # checking on replicate_object: hard_volume as node target
    my $uri1 = $neb->create_object("foo", "~node01");
    my $uri2 = $neb->replicate_object("foo");

    my $locations = $neb->find_instances("foo");
    my $Nloc = @$locations;

    is($Nloc, 2, "found 2 instances");
    
    my $loc1 = $locations->[0];
    my $loc2 = $locations->[1];

    # if the volume is not explicitly specified, then the first copy
    # should go to a different volume, cabinet, and site
    # if first is one node01, replication should go to node03 or node08
    # since those both have different site_id values
    like($loc1, qr/$nodedirs[0]/, "instance matches expected node");

    my $isnode03 = $loc2 =~ m/$nodedirs[2]/;
    my $isnode08 = $loc2 =~ m/$nodedirs[7]/;
    ok($isnode03 || $isnode08, "instance matches expected node");
}

# replication without volume
Test::Nebulous->setup;
{
    my @nodedirs = Test::Nebulous->get_node_dirs();

    # checking on replicate_object, does this logic make sense?
    my $uri1 = $neb->create_object("foo", "~node03");
    my $uri2 = $neb->replicate_object("foo");

    my $locations = $neb->find_instances("foo");
    my $Nloc = @$locations;

    is($Nloc, 2, "found 2 instances");
    
    my $loc1 = $locations->[0];
    my $loc2 = $locations->[1];

    # if the volume is not explicitly specified, then the first copy
    # should go to a different volume, cabinet, and site
    # if first is one node03, replication should go to node01 or node02
    # since those both have different site_id values
    like($loc1, qr/$nodedirs[2]/, "instance matches expected node");

    my $isnode01 = $loc2 =~ m/$nodedirs[0]/;
    my $isnode02 = $loc2 =~ m/$nodedirs[1]/;
    ok($isnode01 || $isnode02, "instance matches expected node");
}

Test::Nebulous->setup;

eval {
    $neb->replicate_object('foo');
};
like($@, qr/is valid object key/, 'storage object does not exist');

Test::Nebulous->setup;

eval {
    $neb->create_object('foo');
    ok($neb->replicate_object('foo', 'bar'),'soft fake storage volume');
};

Test::Nebulous->setup;

eval {
    $neb->create_object('foo');
    $neb->replicate_object('foo', '~bar');
};
like($@, qr/is not a valid volume name/, 'storage volume does not exist');

Test::Nebulous->setup;

eval {
    $neb->replicate_object();
};
like($@, qr/1 - 2 were expected/, 'no params');

Test::Nebulous->setup;

eval {
    $neb->create_object('foo');
    $neb->replicate_object('foo', 'node01', 3);
};
like($@, qr/1 - 2 were expected/, 'too many params');

Test::Nebulous->cleanup;
