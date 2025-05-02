#!/usr/bin/perl

# Copryight (C) 2004-2005  Joshua Hoblitt
#
# $Id: 58_client_open.t,v 1.2 2008-04-18 00:08:23 jhoblitt Exp $

use strict;
use warnings FATAL => qw( all );

use Apache::Test qw( -withtestmore );

plan tests => 10;

use lib qw( ./t ./lib );

use Nebulous::Client;
use Nebulous::Util qw( :standard );
use Test::Nebulous;
use File::stat;

my $hostport = Apache::Test->config->{ 'hostport' };

Test::Nebulous->setup;

{
    # key
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->create( "foo" );
    my $fh = $neb->open( "foo", 'read' );

    is( ref $fh, 'GLOB', "good filehandle" );
}

Test::Nebulous->setup;

{
    # key
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->create( "foo" );
    my $fh = $neb->open( "foo", 'write' );

    is( ref $fh, 'GLOB', "good filehandle" );
}

Test::Nebulous->setup;

{
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );

    is( ref $neb->open( "foo", 'write' ), 'GLOB', "create new object" );
}

Test::Nebulous->setup;

{
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );

    is( $neb->open( "foo", 'read' ), undef, "can't create new object" );
}

Test::Nebulous->setup;

# truncation test
{
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    my $fh = $neb->open( "foo", 'write' );
    print $fh "foobar";
    close $fh;

    $fh = $neb->open( "foo", '>' );
    ok($fh, "open in trucate mode" );
    my $sb = stat($fh);
    is($sb->size, 0, "file was truncated");
}

Test::Nebulous->setup;

{
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->create( "foo" );
    $neb->replicate( "foo" );

    is( $neb->open( "foo", 'write' ), undef,
        "write to object with multiple instances" );
}

Test::Nebulous->setup;

eval {
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->open( "foo", 'bar' );
};
like( $@, qr/is read or write/, "2nd params not read or write" );

Test::Nebulous->setup;

eval {
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->open();
};
like( $@, qr/2 were expected/, "no params" );

Test::Nebulous->setup;

eval {
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->open( 1, 'read', 3 );
};
like( $@, qr/2 were expected/, "too many params" );

Test::Nebulous->cleanup;
