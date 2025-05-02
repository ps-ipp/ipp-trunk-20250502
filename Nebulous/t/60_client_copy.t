#!/usr/bin/perl

# Copryight (C) 2004-2005  Joshua Hoblitt
#
# $Id: 60_client_copy.t,v 1.2 2007-04-27 23:50:57 jhoblitt Exp $

use strict;
use warnings FATAL => qw( all );

use Apache::Test qw( -withtestmore );

plan tests => 11;

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

    ok( $neb->copy( "foo", "foobar" ), "copied object" );

    my $locations1 = $neb->find_instances( "foobar" );
    my $locations2 = $neb->find_instances( "foobar" );

    is( scalar @$locations1, 1, "old object has an instances" );
    is( scalar @$locations2, 1, "new object has an instances" );
}

Test::Nebulous->setup;

{
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->create( "foo" );

    ok( $neb->copy( "foo", "foobar", "node01" ), "copied object" );

    my $locations1 = $neb->find_instances( "foobar" );
    my $locations2 = $neb->find_instances( "foobar" );

    is( scalar @$locations1, 1, "old object has an instances" );
    is( scalar @$locations2, 1, "new object has an instances" );
}

Test::Nebulous->setup;

{
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );

    ok( ! $neb->copy( "foo", "foobar" ), "copy non-existant object" );

    my $locations = $neb->find_instances( "foobar" );

    is( $locations, undef, "new object has no instances" );
}

Test::Nebulous->setup;

eval {
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->copy();
};
like( $@, qr/2 - 3 were expected/, "no params" );

Test::Nebulous->setup;

eval {
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->copy( "foo" );
};
like( $@, qr/2 - 3 were expected/, "not enough params" );

Test::Nebulous->setup;

eval {
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->copy( "foo", "bar", "baz", "bong" );
};
like( $@, qr/2 - 3 were expected/, "too many params" );

Test::Nebulous->cleanup;
