#!/usr/bin/perl

# Copryight (C) 2004-2005  Joshua Hoblitt
#
# $Id: 51_client_create.t,v 1.6 2008-05-15 03:24:58 jhoblitt Exp $

use strict;
use warnings FATAL => qw( all );

use Apache::Test qw( -withtestmore );

plan tests => 13;

use lib qw( ./t ./lib );

use Nebulous::Client;
use Test::Nebulous;
use Test::URI;
use URI::Split qw( uri_split );

# this returns the test apache server location
my $hostport = Apache::Test->config->{ 'hostport' };

Test::Nebulous->setup;

{
    # key
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );

    my $uri = $neb->create("foo");
    if (not defined $uri) { die "failure to create basic file (does it already exist?)"; }

    my ($scheme, $auth, $path, $query, $frag) = uri_split($uri);
    ok(-e $path, "good filename");

    uri_scheme_ok($uri, 'file');
    
    # need to remove the test entry we made above (create fails if file exists)
    ok( $neb->delete( "foo" ), "delete object" );
}

Test::Nebulous->setup;

{
    # key, volume
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );

    my $uri = $neb->create("foo", "node01");

    my ($scheme, $auth, $path, $query, $frag) = uri_split($uri);
    ok(-e $path, "good filename");
    uri_scheme_ok($uri, 'file');

    # need to remove the test entry we made above (create fails if file exists)
    ok( $neb->delete( "foo" ), "delete object" );
}

Test::Nebulous->setup;

{
    # key, volume == undef
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );

    my $uri = $neb->create("foo", undef);

    my ($scheme, $auth, $path, $query, $frag) = uri_split($uri);
    ok(-e $path, "good filename");
    uri_scheme_ok($uri, 'file');

    # need to remove the test entry we made above (create fails if file exists)
    ok( $neb->delete( "foo" ), "delete object" );
}

Test::Nebulous->setup;

{
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );

    $neb->create("foo");
    is($neb->create("foo"), undef, "object already exists");

    # need to remove the test entry we made above (create fails if file exists)
    ok( $neb->delete( "foo" ), "delete object" );
}

Test::Nebulous->setup;

eval {
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );

    $neb->create();
};
like($@, qr/1 - 2 were expected/, "no params");

Test::Nebulous->setup;

eval {
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );

    $neb->create(1, 2, 3);
};
like($@, qr/1 - 2 were expected/, "too many params");

Test::Nebulous->cleanup;
