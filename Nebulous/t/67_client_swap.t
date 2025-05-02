#!/usr/bin/perl

# Copryight (C) 2004-2005  Joshua Hoblitt
#
# $Id: 67_client_swap.t,v 1.1 2008-10-13 21:20:33 jhoblitt Exp $

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

    my $uri1 = $neb->create( "foo1" );
    my $uri2 = $neb->create( "foo2" );

    ok($neb->swap( "foo1", "foo2" ), "swap objects");

    my $new_uri1 = ($neb->find_instances( "foo1" ))->[0];
    my $new_uri2 = ($neb->find_instances( "foo2" ))->[0];

    is($uri1, $new_uri2, "key1 -> key2");
    is($uri2, $new_uri1, "key2 -> key1");
}

Test::Nebulous->setup;

{
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );

    $neb->create('bar');

    ok(!$neb->swap('foo', 'bar'), "key1 doesn't exist" );
}

Test::Nebulous->setup;

{
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );

    $neb->create('foo');

    ok(!$neb->swap('foo', 'bar'), "key2 doesn't exist" );
}

Test::Nebulous->setup;

eval {
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );

    $neb->swap();
};
like( $@, qr/2 were expected/, "no params" );

Test::Nebulous->setup;

eval {
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );

    $neb->swap('foo');
};
like( $@, qr/2 were expected/, "not enough params" );

Test::Nebulous->setup;

eval {
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );

    $neb->create('foo');
    $neb->create('bar');

    $neb->swap('foo', 'bar', 'baz');
};
like( $@, qr/2 were expected/, "too many params" );

Test::Nebulous->cleanup;
