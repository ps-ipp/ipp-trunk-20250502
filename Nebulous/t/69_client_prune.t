#!/usr/bin/perl

# Copryight (C) 2004-2009  Joshua Hoblitt

use strict;
use warnings FATAL => qw( all );

use Apache::Test qw( -withtestmore );

plan tests => 5;

use lib qw( ./t ./lib );


use Nebulous::Client;
use Nebulous::Util qw( :standard );
use Test::Nebulous;

my $hostport = Apache::Test->config->{ 'hostport' };

my $neb = Nebulous::Client->new(
    proxy => "http://$hostport/nebulous",
);

# note: Test::DBUnit::expected_dataset_ok() does not seem to work under
# Apache::Test for some reason
#
use Test::DBUnit dsn => $NEB_DB, username => $NEB_USER, password => $NEB_PASS;

Test::Nebulous->setup;

{
    my $key = "foo";
    my $uri1 = $neb->create($key, 'node01');
    my $uri2 = $neb->replicate($key, 'node02');

    # make one of the instances unavailable
    my $dbh = test_dbh();
    $dbh->do("UPDATE mountedvol SET available = 0 WHERE name = 'node01'");

    ok($neb->prune($key), "prune object");

    is(scalar @{$neb->find_instances($key)}, 1, "# of instances");
}

Test::Nebulous->setup;

{
    my $key = "foo";
    my $uri1 = $neb->create($key);

    is($neb->prune($key), 0, "no dead instances to remove");
}

Test::Nebulous->setup;

eval {
    $neb->prune();
};
like($@, qr/1 was expected/, "no params");

Test::Nebulous->setup;

eval {
    my $key = "foo";
    my $uri1 = $neb->create($key);

    $neb->prune("foo", 2);
};
like($@, qr/1 was expected/, "too many params");

Test::Nebulous->cleanup;
