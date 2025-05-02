#!/usr/bin/perl

# Copryight (C) 2004-2005  Joshua Hoblitt
#
# $Id: 61_client_move.t,v 1.1 2005-12-03 02:52:31 jhoblitt Exp $

use strict;
use warnings FATAL => qw( all );

use Apache::Test qw( -withtestmore );

plan tests => 8;

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
    $neb->create( "foo" );

    ok( $neb->move( "foo", "foobar" ), "move object" );

    my $locations1 = $neb->find_instances( "foo" );
    my $locations2 = $neb->find_instances( "foobar" );

    is( $locations1, undef, "old object has no instances" );
    is( scalar @$locations2, 1, "new object has an instances" );
}

Test::Nebulous->setup;

{
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );

    ok( ! $neb->move( "foo", "foobar" ), "move non-existant object" );

    my $locations = $neb->find_instances( "foobar" );

    is( $locations, undef, "new object has no instances" );
}

Test::Nebulous->setup;

eval {
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->move();
};
like( $@, qr/2 were expected/, "no params" );

Test::Nebulous->setup;

eval {
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->move( "foo" );
};
like( $@, qr/2 were expected/, "not enough params" );

Test::Nebulous->setup;

eval {
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->move( "foo", "bar", "baz" );
};
like( $@, qr/2 were expected/, "too many params" );

Test::Nebulous->cleanup;
