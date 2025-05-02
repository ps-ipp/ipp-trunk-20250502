#!/usr/bin/perl

# Copryight (C) 2004-2005  Joshua Hoblitt
#
# $Id: 52_client_replicate.t,v 1.2 2008-09-24 00:36:56 jhoblitt Exp $

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

{
    # key
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->create( "foo" );

    my $uri = $neb->replicate( "foo" );

    ok( $uri, "good replication" );
    ok( -e _get_file_path( $uri ), "file exists" );
}

Test::Nebulous->setup;

{
    # key, $volume
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->create( "foo" );

    my $uri = $neb->replicate( "foo", "node01" );

    ok( $uri, "good replication" );
    ok( -e _get_file_path( $uri ), "file exists" );
}

Test::Nebulous->setup;

{
    # key, $volume = undef (same as saying no volume)
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->create( "neb://node01/foo" );

    my $uri = $neb->replicate( "neb://node01/foo", undef );

    ok( $uri, "good replication" );
    ok( -e _get_file_path( $uri ), "file exists" );
}

Test::Nebulous->setup;

{
    # key, $volume
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->create( "foo" );

    my $uri1 = $neb->replicate( "foo", "node01" );
    my $uri2 = $neb->replicate( "foo", "node02" );

    ok( $uri1, "good replication" );
    ok( $uri2, "good replication" );
    ok( -e _get_file_path( $uri1 ), "file exists" );
    ok( -e _get_file_path( $uri2 ), "file exists" );
}

Test::Nebulous->setup;

{
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    my $uri = $neb->replicate( "foo" );

    is( $uri, undef, "storage object does not exist" );
}

Test::Nebulous->setup;

eval {
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->replicate();
};
like( $@, qr/1 - 2 were expected/, "no params" );

Test::Nebulous->setup;

eval {
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->replicate( 1, 2, 3 );
};
like( $@, qr/1 - 2 were expected/, "too many params" );

Test::Nebulous->cleanup;
