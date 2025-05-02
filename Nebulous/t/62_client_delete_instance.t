#!/usr/bin/perl

# Copryight (C) 2004-2005  Joshua Hoblitt
#
# $Id: 62_client_delete_instance.t,v 1.1.38.1 2008-12-14 22:03:08 eugene Exp $

use strict;
use warnings FATAL => qw( all );

use Apache::Test qw( -withtestmore );

plan tests => 10;

use lib qw( ./t ./lib );

use Nebulous::Client;
use Nebulous::Util qw( :standard );
use Test::Nebulous;

my $hostport = Apache::Test->config->{ 'hostport' };

Test::Nebulous->setup;

{
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );

    my $key = "foo";
    $neb->create($key);

    my $locations = $neb->find_instances($key);

    my $uri = $neb->delete_instance($key, @$locations[0]);

    is( $uri, @$locations[0], "delete instance" );
}

Test::Nebulous->setup;

{
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    my $key = "foo";
    $neb->create($key);
    $neb->replicate($key);

    my $uri1 = $neb->find_instances( "foo" )->[0];

    ok( $neb->delete_instance($key, $uri1), "delete instance" );

    my $uri2 = $neb->find_instances( "foo" )->[0];

    isnt( $uri1, $uri2, "other instance remains" );

    ok( $neb->delete_instance($key, $uri2), "delete instance" );

    my $locations = $neb->find_instances( "foo" );

    is( $locations, undef, "no remaining instances" );
}

Test::Nebulous->setup;

{
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    my $key = "foo";
    $neb->create($key);
    my $uri = $neb->delete_instance($key, "file:/foo" );

    is( $uri, undef, "uri does not exist" );
}

Test::Nebulous->setup;

eval {
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    my $uri = $neb->delete_instance("foo", "file:/foo" );
};
like( $@, qr/is valid object key/, "bad object key" );

Test::Nebulous->setup;

eval {
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    my $uri = $neb->delete_instance("foo");
};
like( $@, qr/2 were expected/, "missing second param" );

Test::Nebulous->setup;

eval {
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->delete_instance();
};
like( $@, qr/2 were expected/, "no params" );

Test::Nebulous->setup;

eval {
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->delete_instance("foo", 2, 3);
};
like( $@, qr/2 were expected/, "too many params" );

Test::Nebulous->cleanup;
