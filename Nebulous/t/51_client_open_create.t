#!/usr/bin/perl

# Copryight (C) 2004-2005  Joshua Hoblitt
#
# $Id: 51_client_open_create.t,v 1.4 2008-05-15 02:40:23 jhoblitt Exp $

use strict;
use warnings FATAL => qw( all );

use Apache::Test qw( -withtestmore );

plan tests => 5;

use lib qw( ./t ./lib );

use Nebulous::Client;
use Test::Nebulous;

my $hostport = Apache::Test->config->{ 'hostport' };

Test::Nebulous->setup;

{
    # key
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );

    my $fh = $neb->open_create("foo");

    is(ref $fh, 'GLOB', "good filehandle");
}

Test::Nebulous->setup;

{
    # key, volume
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );

    my $fh = $neb->open_create("foo", "node01");

    is(ref $fh, 'GLOB', "good filehandle");
}

Test::Nebulous->setup;

{
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );

    $neb->open_create("foo");
    is($neb->open_create("foo"), undef, "object already exists");
}

Test::Nebulous->setup;

eval {
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );

    $neb->open_create();
};
like($@, qr/1 - 2 were expected/, "no params");

Test::Nebulous->setup;

eval {
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );

    $neb->open_create(1, 2, 3);
};
like($@, qr/1 - 2 were expected/, "too many params");

Test::Nebulous->cleanup;
