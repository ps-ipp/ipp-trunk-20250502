#!/usr/bin/perl

# Copryight (C) 2004-2005  Joshua Hoblitt
#
# $Id: 55_client_unlock.t,v 1.1 2005-12-03 02:52:31 jhoblitt Exp $

use strict;
use warnings FATAL => qw( all );

use Apache::Test qw( -withtestmore );

plan tests => 14;

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
    $neb->lock( "foo", "read" );

    ok( $neb->unlock( "foo", "read" ), "read unlock" );
}

Test::Nebulous->setup;

{
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->create( "foo" );
    $neb->lock( "foo", "read" );
    $neb->lock( "foo", "read" );

    ok( $neb->unlock( "foo", "read" ), "read unlock" );
    ok( $neb->unlock( "foo", "read" ), "read unlock" );
}

Test::Nebulous->setup;

{
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->create( "foo" );
    $neb->lock( "foo", "write" );

    ok( $neb->unlock( "foo", "write" ), "write unlock" );
}

Test::Nebulous->setup;

{
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );

    is( $neb->unlock( "foo", "read" ), undef, "storage object does not exist" );
}

Test::Nebulous->setup;

{
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );

    is( $neb->unlock( "foo", "write" ), undef, "storage object does not exist" );
}

Test::Nebulous->setup;

{
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->create( "foo" );

    is( $neb->unlock( "foo", "read" ), undef, "no lock set" );
}

Test::Nebulous->setup;

{
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->create( "foo" );

    is( $neb->unlock( "foo", "write" ), undef, "no lock set" );
}

Test::Nebulous->setup;

{
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->create( "foo" );
    $neb->lock( "foo", "write" );

    is( $neb->unlock( "foo", "read" ), undef, "read unlock under write lock" );
}

Test::Nebulous->setup;

{
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->create( "foo" );
    $neb->lock( "foo", "read" );

    is( $neb->unlock( "foo", "write" ), undef, "write unlock under read lock" );
}

Test::Nebulous->setup;

eval {
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );

    $neb->unlock();
};
like( $@, qr/2 were expected/, "no params" );

Test::Nebulous->setup;

eval {
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );

    $neb->unlock( "foo" );
};
like( $@, qr/2 were expected/, "not enough params" );

Test::Nebulous->setup;

eval {
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );

    $neb->unlock( "foo", "both" );
};
like( $@, qr/is read or write/, "not read or write" );

Test::Nebulous->setup;

eval {
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );

    $neb->unlock( "foo", 'read', 3 );
};
like( $@, qr/2 were expected/, "too many params" );

Test::Nebulous->cleanup;
