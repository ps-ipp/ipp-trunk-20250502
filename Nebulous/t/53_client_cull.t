#!/usr/bin/perl

# Copryight (C) 2004-2005  Joshua Hoblitt
#
# $Id: 53_client_cull.t,v 1.5 2007-05-03 03:21:28 jhoblitt Exp $

use strict;
use warnings FATAL => qw( all );

use Apache::Test qw( -withtestmore );

plan tests => 18;

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
    $neb->create("foo");
    $neb->replicate("foo");

    my $uri = $neb->cull("foo");

    ok($uri, "good cull");
    ok(! -e _get_file_path($uri), "file doesn't exist");
}

Test::Nebulous->setup;
{
    # key, $volume
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->create("foo", "node01");
    $neb->replicate("foo", "node02");
    
    my $uri = $neb->cull("foo", "node02");

    ok($uri, "good cull");
    ok(! -e _get_file_path($uri), "file exists");
}

Test::Nebulous->setup;
{
    # key, $volume
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->create("foo", "node01");
    $neb->replicate("foo", "node02");

    # cull with soft volume should succeed even if node does not have instance
    my $uri = $neb->cull("foo", "node03");

    ok($uri, "good cull");
    ok(! -e _get_file_path($uri), "file exists");
}

Test::Nebulous->setup;
{
    # key, $volume
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->create("foo", "node01");
    $neb->replicate("foo", "node02");

    # cull with hard volume should not succeed if node does not have instance
    my $uri = $neb->cull("foo", "~node03");

    is($uri, undef, "nothing to cull on this node");
}

Test::Nebulous->setup;

{
    # key
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->create("foo");

    $neb->replicate("foo", "node01");
    $neb->replicate("foo", "node02");

    my $uri1 = $neb->cull("foo");
    my $uri2 = $neb->cull("foo");

    ok($uri1, "good cull");
    ok($uri2, "good cull");
    ok(! -e _get_file_path($uri1), "file exists");
    ok(! -e _get_file_path($uri2), "file exists");
}

Test::Nebulous->setup;

{

    # key, $volume
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->create( "foo" );

    $neb->replicate( "foo", "node01" );
    $neb->replicate( "foo", "node02" );

    my $uri1 = $neb->cull( "foo", "node01" );
    my $uri2 = $neb->cull( "foo", "node02" );

    ok( $uri1, "good cull" );
    ok( $uri2, "good cull" );
    ok( ! -e _get_file_path( $uri1 ), "file exists" );
    ok( ! -e _get_file_path( $uri2 ), "file exists" );
}

Test::Nebulous->setup;

{
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    my $uri = $neb->cull( "foo" );

    is( $uri, undef, "storage object does not exist" );
}

Test::Nebulous->setup;

eval {
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    my $uri = $neb->cull();

    is ($uri, undef, "no params given to cull");
};
like( $@, qr/1 - 3 were expected/, "no params" );

Test::Nebulous->setup;

eval {
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    my $uri = $neb->cull( 1, 2, 3, 4 );

    is ($uri, undef, "no params given to cull");
};
like( $@, qr/1 - 3 were expected/, "too many params" );

Test::Nebulous->cleanup;
