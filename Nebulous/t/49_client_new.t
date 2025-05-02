#!/usr/bin/perl

# Copryight (C) 2004-2005  Joshua Hoblitt
#
# $Id: 50_client_new.t,v 1.1 2005-12-03 02:52:31 jhoblitt Exp $

use strict;
use warnings FATAL => qw( all );

use Apache::Test qw( -withtestmore );

plan tests => 4;

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

    isa_ok( $neb, "Nebulous::Client" );
}

Test::Nebulous->setup;

{
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
        uri   => "http://example.com/PS/IPP/Nebulous/Client",
    );

    isa_ok( $neb, "Nebulous::Client" );
}

Test::Nebulous->setup;

eval {
    Nebulous::Client->new;
};
like( $@, qr/Mandatory parameter/, "no proxy" );

Test::Nebulous->setup;

eval {
    Nebulous::Client->new(
        proxy => "foo",
        dog => "do"
    );
};
like( $@, qr/not listed in the validation options/, "bad param" );

Test::Nebulous->cleanup;
