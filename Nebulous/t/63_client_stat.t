#!/usr/bin/perl

# Copryight (C) 2004-2005  Joshua Hoblitt
#
# $Id: 63_client_stat.t,v 1.3 2007-05-03 22:10:14 jhoblitt Exp $

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
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->create( "foo" );

    my $info = $neb->stat( "foo" );

    is( scalar @$info, 8, "number of columns" );
}

Test::Nebulous->setup;

{
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->create( "foo", "node01" );

    my $info = $neb->stat( "foo" );

    is( scalar @$info, 8,                       "number of columns" );
    is( @$info[0], 1,                           "so_id" );
    is( @$info[1], "foo",                       "ext_id" );
    is( @$info[2], 0,                           "read lock" );
    is( @$info[3], undef,                       "write lock" );
    like( @$info[4], qr/....-..-.. ..:..:../,   "epoch" );
    like( @$info[5], qr/....-..-.. ..:..:../,   "mtime" );
    is( @$info[6], 1,                           "available instances" );
    is( @$info[7], 1,                           "total instances" );
}

Test::Nebulous->setup;

{
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    my $info = $neb->stat( "foo" );

    is( $info, undef, "object does not exist" );
}

Test::Nebulous->setup;

eval {
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->stat();
};
like( $@, qr/1 was expected/, "no params" );

Test::Nebulous->setup;

eval {
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->stat( "foo", 2 );
};
like( $@, qr/1 was expected/, "too many params" );

Test::Nebulous->cleanup;
