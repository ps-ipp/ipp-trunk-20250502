#!/usr/bin/perl

# Copryight (C) 2004-2005  Joshua Hoblitt
#
# $Id: 57_client_find.t,v 1.3 2008-07-10 02:38:24 jhoblitt Exp $

use strict;
use warnings FATAL => qw( all );

use Apache::Test qw( -withtestmore );

plan tests => 7;

use lib qw( ./t ./lib );

use Nebulous::Client;
use Nebulous::Util qw( :standard );
use Test::Nebulous;

my $hostport = Apache::Test->config->{ 'hostport' };

Test::Nebulous->setup;

{
    # key
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    my $uri = $neb->create( "foo" );

    my $path = $neb->find( "foo" );

    ok( -e $path, "file exists" );
}

Test::Nebulous->setup;

{
    # key, volume
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    my $uri = $neb->create( "foo", "node01" );

    my $path = $neb->find( "foo", "node01" );

    ok( -e $path, "file exists" );
}

Test::Nebulous->setup;

{
    # key, volume does not hold key but this works anyways ans find will fall
    # back to looking for "any"
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    my $uri = $neb->create( "foo", "node01" );

    my $path = $neb->find( "foo", "node02" );

    ok( -e $path, "file exists" );
}

Test::Nebulous->setup;

{
    # key, volume any
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    my $uri = $neb->create( "foo", "node01" );

    my $path = $neb->find( "foo", "any" );

    ok( -e $path, "file exists" );
}

Test::Nebulous->setup;

{
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );

    my $path = $neb->find( "foo" );

    is($path, undef, "file doesn't exist" );
}

Test::Nebulous->setup;

eval {
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );

    $neb->find();
};
like( $@, qr/1 - 2 were expected/, "no params" );

Test::Nebulous->setup;

eval {
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );

    $neb->find( "foo", "bar", "bong" );
};
like( $@, qr/1 - 2 were expected/, "too many params" );

Test::Nebulous->cleanup;
