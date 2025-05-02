#!/usr/bin/perl

# Copryight (C) 2004-2005  Joshua Hoblitt
#
# $Id: 10_server_is_valid_volume_name.t,v 1.7 2008-12-14 22:54:25 eugene Exp $

use strict;
use warnings FATAL => qw( all );

use Test::More tests => 5;

use lib qw( ./t ./lib );

use Nebulous::Server;
use Test::Nebulous;

my $neb = Nebulous::Server->new(
    dsn         => $NEB_DB,
    dbuser      => $NEB_USER,
    dbpasswd    => $NEB_PASS,
);

Test::Nebulous->setup;

ok($neb->_is_valid_volume_name('foo', 'node01'), "valid volume name");

Test::Nebulous->setup;

ok($neb->_is_valid_volume_name('foo', 'node02'), "valid volume name");

Test::Nebulous->setup;

is($neb->_is_valid_volume_name('foo', 'node99'), 0, "invalid volume name");

Test::Nebulous->setup;

# key does not need to exist (is hashed to select db server)
ok($neb->_is_valid_volume_name('bax', 'node01'), "valid volume name");

Test::Nebulous->setup;

# 'any' should always be a valid name
ok($neb->_is_valid_volume_name('foo', 'any'), "valid volume name");

Test::Nebulous->cleanup;
