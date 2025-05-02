#!/usr/bin/perl

# Copryight (C) 2004-2005  Joshua Hoblitt
#
# $Id: 08_server_delete_instance.t,v 1.10.22.1 2008-12-14 22:52:37 eugene Exp $

use strict;
use warnings FATAL => qw( all );

use Test::More tests => 6;

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
    my $uri1 = $neb->create_object($key, 'node01');
    my $uri2 = $neb->replicate_object($key, 'node02');

    # make one of the instances unavailable
    my $dbh = test_dbh();
    $dbh->do("UPDATE mountedvol SET available = 0 WHERE name = 'node01'");

    ok($neb->prune_object($key), "prune object");

    expected_dataset_ok(
        deleted => [vol_id => 1, uri => $uri1],
    );
    
    expected_dataset_ok(
        instance => [ins_id => 2, so_id => 1, vol_id => 2, uri => $uri2],
    );
}

Test::Nebulous->setup;

{
    my $key = "foo";
    my $uri1 = $neb->create_object($key);

    is($neb->prune_object($key), 0, "no dead instances to remove");
}

Test::Nebulous->setup;

eval {
    $neb->prune_object();
};
like($@, qr/1 was expected/, "no params");

Test::Nebulous->setup;

eval {
    my $key = "foo";
    my $uri1 = $neb->create_object($key);

    $neb->prune_object("foo", 2);
};
like($@, qr/1 was expected/, "too many params");

Test::Nebulous->cleanup;
