#!/usr/bin/perl

# Copryight (C) 2004-2005  Joshua Hoblitt
#
# $Id: 08_server_delete_instance.t,v 1.10.22.1 2008-12-14 22:52:37 eugene Exp $

use strict;
use warnings FATAL => qw( all );

use Test::More tests => 11;

use lib qw( ./t ./lib );

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
    my $key = "foo";
    my $uri = $neb->create_object($key);

    ok($neb->delete_instance($key, $uri), "delete instance");
}

Test::Nebulous->setup;

{
    my $key = "foo";
    my $uri1 = $neb->create_object($key);
    my $uri2 = $neb->replicate_object($key);

    ok($neb->delete_instance($key, $uri1), "delete instance");

    my $locations = $neb->find_instances($key);

    is($locations->[0], $uri2, "instance remains");

    ok($neb->delete_instance($key, $uri2), "delete instance");

    eval {
        $neb->find_instances($key);
    };
    like($@, qr/is valid object key/, "storage object was deleted");
}

Test::Nebulous->setup;

{
    my $key = "foo";
    my $uri1 = $neb->create_object($key, 'node01');
    my $uri2 = $neb->replicate_object($key, 'node02');

    # make one of the instances unavailable
    my $dbh = test_dbh();
    $dbh->do("UPDATE mountedvol SET available = 0 WHERE name = 'node01'");

    ok($neb->delete_instance($key, $uri2), "delete instance");

    expected_dataset_ok(
        deleted => [vol_id => 1, uri => $uri1],
    );

    eval {
        $neb->find_instances($key);
    };
    like($@, qr/is valid object key/, "storage object was deleted");
}

Test::Nebulous->setup;

eval {
    my $key = "foo";
    my $uri1 = $neb->create_object($key);

    $neb->delete_instance($key, "file:/foo");
};
like($@, qr/no instance is associated with uri/, "uri does not exist");

Test::Nebulous->setup;

eval {
    $neb->delete_instance();
};
like($@, qr/2 were expected/, "no params");

Test::Nebulous->setup;

eval {
    my $key = "foo";
    my $uri1 = $neb->create_object($key);

    $neb->delete_instance("foo", 2, 3);
};
like($@, qr/2 were expected/, "too many params");

Test::Nebulous->cleanup;
