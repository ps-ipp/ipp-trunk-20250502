#!/usr/bin/perl

# Copryight (C) 2004-2005  Joshua Hoblitt
#
# $Id: 11_server_is_valid_object_key.t,v 1.3 2008-03-20 21:10:14 jhoblitt Exp $

use strict;
use warnings FATAL => qw( all );

use Test::More tests => 2;

use lib qw( ./t ./lib );

use Nebulous::Server;
use Test::Nebulous;

my $neb = Nebulous::Server->new(
    dsn         => $NEB_DB,
    dbuser      => $NEB_USER,
    dbpasswd    => $NEB_PASS,
);

Test::Nebulous->setup;

{
    $neb->create_object('foo');

    ok($neb->_is_valid_object_key('foo'), "valid object key");
}

Test::Nebulous->setup;

{
    ok(!$neb->_is_valid_object_key('foo'), "invalid object key");
}

Test::Nebulous->cleanup;
