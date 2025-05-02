#!/usr/bin/perl

# Copryight (C) 2004-2005  Joshua Hoblitt
#
# $Id: 56_client_find_instances.t,v 1.5 2008-09-10 23:50:26 bills Exp $

use strict;
use warnings FATAL => qw( all );

use Apache::Test qw( -withtestmore );

plan tests => 35;

use lib qw( ./t ./lib );

use Nebulous::Client;
use Nebulous::Util qw( :standard );
use Test::Nebulous;

my $hostport = Apache::Test->config->{ 'hostport' };

### XXX EAM : this test suite finds real errors in the implementation.  

## the API is now slightly different (what is the state of the API in the ipp ops code?)
## find_instances ("key", "volume", find_invalid)

## this returns a result even if we request a hard volume unless the hard volume is not valid

Test::Nebulous->setup;
Test::Nebulous->cleanup;

### TEST BLOCK for find_invalid
## Test::Nebulous->setup;
## {
##     # volume/key, volume override
##     my $neb = Nebulous::Client->new(
##         proxy => "http://$hostport/nebulous",
##     );
##     $neb->create( "foo", "~node02" );
##     $neb->replicate( "foo", "~node01" );
## 
##     Test::Nebulous->switch_node_state;
##     my $tmphost;
## 
## #   my $locations = $neb->find_instances( "neb:///foo", "~node02", 1); # result : no instance
## #   my $locations = $neb->find_instances( "neb:///foo", "~node02", 0); # result : no instance
## #   my $locations = $neb->find_instances( "neb:///foo", "~node02"); # result : no instance
## 
## #   my $locations = $neb->find_instances( "neb:///foo", "node02", 1); # result : no instance
## #   my $locations = $neb->find_instances( "neb:///foo", "node02", 0); # result : no instance
##     my $locations = $neb->find_instances( "neb:///foo", "node02"); # result : no instances
## 
## #   my $locations = $neb->find_instances( "neb:///foo", undef, 1);
## #   my $locations = $neb->find_instances( "neb:///foo", undef, 0);
## #   my $locations = $neb->find_instances( "neb:///foo", $tmphost, 0);
## 
## #   my $locations = $neb->find_instances( "neb:///foo", "bebaz", 1);
## #   my $locations = $neb->find_instances( "neb:///foo", "bebaz", 0);
## 
## #   my $locations = $neb->find_instances( "neb:///foo");
## #   my $locations = $neb->find_instances( "neb://bebaz/foo");
## 
## #   my $locations = $neb->find_instances( "neb:///foo");
## #   my $locations = $neb->find_instances( "neb:///foo", undef, 0);
## 
##     foreach my $f (@$locations) {
## 	print "location: $f\n";
##     }
## 
##     is( scalar @$locations, 1, "found 1" );
##     like( @$locations[0], qr/file:/, "URIs match" );
##     ok( -e _get_file_path( @$locations[0] ), "URI matches file" );
## }
## die "quit";

## Test::Nebulous->setup;
## eval {
##     # volume/key, volume override
##     my $neb = Nebulous::Client->new(
##         proxy => "http://$hostport/nebulous",
##     );
##     $neb->create( "foo", "~node01" );
## 
##     my $locations_1 = $neb->find_instances( "neb://~invalid/foo", "~invalid");
##     is($locations_1, undef, "no instances in invalid node" );
## 
##     # EAM: we fail this test because the logic of find_instances has
##     # changed: as long as there is an instance on a valid node, the
##     # instance gets returned
##     my $locations = $neb->find_instances( "neb://node01/foo", "~node02" );
##     is($locations, undef, "no instances on specified volume" );
## };
##print "result: $@\n";
##like( $@, qr/result message/, "my message" );

# find a single instance by raw name (foo)
Test::Nebulous->setup;
{
    # key
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->create( "foo" );

    my $locations = $neb->find_instances( "foo" );

    is( scalar @$locations, 1, "found 1" );
    like( @$locations[0], qr/file:/, "URIs match" );
    ok( -e _get_file_path( @$locations[0] ), "URI matches file" );
}

# find a both instances of a duplicated object
Test::Nebulous->setup;
{
    # key
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->create( "foo" );
    $neb->replicate( "foo" );

    my $locations = $neb->find_instances( "foo" );

    is( scalar @$locations, 2, "found 2" );
    like( @$locations[0], qr/file:/, "URIs match" );
    like( @$locations[1], qr/file:/, "URIs match" );
    ok( -e _get_file_path( @$locations[0] ), "URI matches file" );
    ok( -e _get_file_path( @$locations[1] ), "URI matches file" );
}

Test::Nebulous->setup;
{
    # key, volume
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->create( "foo" );

    my $locations = $neb->find_instances( "foo", "node01" );

    is( scalar @$locations, 1, "found 1" );
    like( @$locations[0], qr/file:/, "URIs match" );
    ok( -e _get_file_path( @$locations[0] ), "URI matches file" );
}

Test::Nebulous->setup;

{
    # key, volume == undef
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->create( "foo" );
    $neb->set_log_level ('DEBUG');

    my $locations = $neb->find_instances( "foo", undef );

    is( scalar @$locations, 1, "found 1" );
    like( @$locations[0], qr/file:/, "URIs match" );
    ok( -e _get_file_path( @$locations[0] ), "URI matches file" );
}

Test::Nebulous->setup;

{
    # volume/key
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->create( "foo" );

    my $locations = $neb->find_instances( "neb://node01/foo");

    is( scalar @$locations, 1, "found 1" );
    like( @$locations[0], qr/file:/, "URIs match" );
    ok( -e _get_file_path( @$locations[0] ), "URI matches file" );
}

# find instance using hard_volume for wrong node
# (should still return a valid instance)
Test::Nebulous->setup;
{
    # volume/key, volume override
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->create( "foo", "~node01" );
    $neb->set_log_level ('DEBUG');

    my $locations = $neb->find_instances( "neb://node01/foo", "~node02" );

    is( scalar @$locations, 1, "found 1" );
    like( @$locations[0], qr/file:/, "URIs match" );
    ok( -e _get_file_path( @$locations[0] ), "URI matches file" );
#   the API used to return undef if we used a hard_volume but it was not there
#   is($locations, undef, "no instances on specified volume" );
}

# find instance using hard_volume for unknown node
# (should fail)
Test::Nebulous->setup;
{
    # volume/key, volume override
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->create( "foo", "~node01" );
    $neb->set_log_level ('DEBUG');

    my $locations = $neb->find_instances( "neb://node01/foo", "~nobody" );
    is($locations, undef, "no instances on specified volume" );
#   is( scalar @$locations, 0, "found 0" );
#   under Ubuntu (Perl 5.26.1), @$locations evaluates to 0 if $locations is undef,
#   but not under Gentoo (Perl 5.6.6)
}

# find instance using hard_volume for unavailable node
# (should fail)
Test::Nebulous->setup;
{
    # volume/key, volume override
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->create( "foo", "~node01" );
    $neb->set_log_level ('DEBUG');

    my $locations = $neb->find_instances( "neb://node01/foo", "~node04" );

    is( scalar @$locations, 1, "found 1" );
    like( @$locations[0], qr/file:/, "URIs match" );
    ok( -e _get_file_path( @$locations[0] ), "URI matches file" );

#   the API used to return undef if we used a hard_volume but it was not there
#   is($locations, undef, "no instances on specified volume" );
}

Test::Nebulous->setup;

{
    # volume/key, volume override
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->create( "foo", "node01" );

    my $locations = $neb->find_instances( "neb://node02/foo", "any" );

    is( scalar @$locations, 1, "found 1" );
    like( @$locations[0], qr/file:/, "URIs match" );
    ok( -e _get_file_path( @$locations[0] ), "URI matches file" );
}

Test::Nebulous->setup;

{
    # key
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->create( "foo", "node01" );
    $neb->replicate( "foo", "node01" );

    my $locations = $neb->find_instances( "foo", "node01" );

    is( scalar @$locations, 2, "found 2" );
    like( @$locations[0], qr/file:/, "URIs match" );
    like( @$locations[1], qr/file:/, "URIs match" );
    ok( -e _get_file_path( @$locations[0] ), "URI matches file" );
    ok( -e _get_file_path( @$locations[1] ), "URI matches file" );
}

Test::Nebulous->setup;
{
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );

    is( $neb->find_instances( "foo" ), undef, "storage object does not exist" );
}

Test::Nebulous->setup;

eval {
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );

    $neb->find_instances();
};
like( $@, qr/1 - 3 were expected/, "no params" );

Test::Nebulous->setup;

eval {
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );

    $neb->find_instances( "foo", 'read', 3, 5 );
};
like( $@, qr/1 - 3 were expected/, "too many params" );

Test::Nebulous->cleanup;
