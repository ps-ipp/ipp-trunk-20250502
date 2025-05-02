#!/usr/bin/perl

# Copryight (C) 2004-2005  Joshua Hoblitt
#
# $Id: 64_client_find_objects.t,v 1.3 2008-05-15 03:26:11 jhoblitt Exp $

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
    $neb->create( "foo" );

    my $keys = $neb->find_objects();

    # currently returning everything is turned off
    is($keys, undef, 'no keys found');
}

Test::Nebulous->setup;

{
    # key
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->create( "foo" );

    my $keys = $neb->find_objects( "foo" );

    is(scalar @$keys, 1, 'number of keys found');
    is($keys->[0], "foo", "key name");
}

Test::Nebulous->setup;

{
    # key
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->create( "foo" );
    $neb->replicate( "foo" );

    my $keys = $neb->find_objects( "foo" );

    is(scalar @$keys, 1, 'number of keys found');
    is($keys->[0], "foo", "key name");
}

Test::Nebulous->setup;

{
    # key does not exist
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );

    my $keys = $neb->find_objects( "bar" );

    is($keys, undef, 'no keys found');
}

Test::Nebulous->setup;

eval {
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );

    $neb->find_objects( "foo", 3 );
};
like( $@, qr/1 was expected/, "too many params" );

Test::Nebulous->cleanup;
