#!/usr/bin/perl

# Copryight (C) 2004-2005  Joshua Hoblitt
#
# $Id: 56_client_find_instances.t,v 1.5 2008-09-10 23:50:26 bills Exp $

use strict;
use warnings FATAL => qw( all );

use Apache::Test qw( -withtestmore );

plan tests => 13;

use lib qw( ./t ./lib );

use Nebulous::Client;
use Nebulous::Util qw( :standard );
use Test::Nebulous;

my $hostport = Apache::Test->config->{ 'hostport' };

Test::Nebulous->setup;
Test::Nebulous->cleanup;

## volume is defined, but it is not valid (should exit)
## Test::Nebulous->setup;
## {
##     my $key = "foo";
##     my $neb = Nebulous::Client->new(
##         proxy => "http://$hostport/nebulous",
##     );
## 
##     my $uri = $neb->create($key, "node01");
## 
##     $neb->replicate($key, "node02");
## 
##     ## set node02 to an invalid state:
##     Test::Nebulous->switch_node_state;
## 
##     # if requested node is invalid, the other instance is kept and the invalid one removed
##     # but since invalid instances are removed with a prune call, they are not counted
##     # in the number of items removed (return by the function call)
##     is($neb->there_can_be_only_one($key, "node02"), 0, "there can be only one!");
##     my $locations = $neb->find_instances( "foo" );
## 
##     is( scalar @$locations, 1, "found 1" );
##     is($locations->[0], $uri, "instance on correct volume" );
## };
## is($@, "", "there_can_be_only_one with supplied volume should succeed");
## die "asdf";

#######

## volume not defined
Test::Nebulous->setup;
{
    my $key = "foo";
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->create($key);
    $neb->replicate($key,);

    is($neb->there_can_be_only_one($key), 1, "there can be only one!");

    my $locations = $neb->find_instances( "foo" );

    is( scalar @$locations, 1, "found 1" );
}

## volume is defined, only valid instances
Test::Nebulous->setup;
eval {
    my $key = "foo";
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->create($key, "node01");

    my $uri = $neb->replicate($key, "node02");
    is($neb->there_can_be_only_one($key, "node02"), 1, "there can be only one!");

    my $locations = $neb->find_instances( "foo" );

    is( scalar @$locations, 1, "found 1" );
    is($locations->[0], $uri, "instance on correct volume" );
};
is($@, "", "there_can_be_only_one with supplied volume should succeed");

## volume is defined, but it is not valid (should exit)
Test::Nebulous->setup;
eval {
    my $key = "foo";
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    my $uri = $neb->create($key, "node01");

    $neb->replicate($key, "node02");

    ## set node02 to an invalid state:
    Test::Nebulous->switch_node_state;

    # if requested node is invalid, nothing is removed
    is($neb->there_can_be_only_one($key, "node02"), 0, "there can be only one!");

    my $locations = $neb->find_instances( "foo" );

    is( scalar @$locations, 1, "found 1" );
    is($locations->[0], $uri, "instance on correct volume" );
};
is($@, "", "there_can_be_only_one with supplied volume should succeed");

Test::Nebulous->setup;

{
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );

    is( $neb->there_can_be_only_one( "foo" ), undef, "storage object does not exist" );
}

Test::Nebulous->setup;

eval {
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );

    $neb->there_can_be_only_one();
};
like( $@, qr/1 - 2 were expected/, "no params" );

Test::Nebulous->setup;

eval {
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );

    $neb->there_can_be_only_one( "foo", 'read', 3 );
};
like( $@, qr/1 - 2 were expected/, "too many params" );

Test::Nebulous->cleanup;
