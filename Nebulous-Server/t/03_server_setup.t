#!/usr/bin/perl

# Copryight (C) 2004-2005  Joshua Hoblitt
#
# $Id: 02_server_setup.t,v 1.8 2008-12-14 22:54:25 eugene Exp $

use strict;
use warnings;

use Test::More tests => 6;

use lib qw( ./t ./lib );

use Nebulous::Server;
use Test::Nebulous;

Test::Nebulous->show_setup;
die "test";

Test::Nebulous->setup;

# ->new()

{
    my $neb = Nebulous::Server->new(
        dsn         => $NEB_DB,
        dbuser      => $NEB_USER,
        dbpasswd    => $NEB_PASS,
    );

    ok($neb, "set database");
}

Test::Nebulous->setup;

{
    my $neb = Nebulous::Server->new(
        dsn         => $NEB_DB,
        dbuser      => $NEB_USER,
        dbpasswd    => $NEB_PASS,
        trace       => 'off',
    );

    ok($neb, "set log level");
}

Test::Nebulous->setup;

# add dbs after ->new()
{
    my $config = Nebulous::Server::Config->new;

    ok($config->add_db(
        dbindex    => 0,
        dsn         => $NEB_DB,
        dbuser      => $NEB_USER,
        dbpasswd    => $NEB_PASS,
    ), "add db");
    
    ok($config->add_db(
        dbindex    => 1,
        dsn         => $NEB_DB,
        dbuser      => $NEB_USER,
        dbpasswd    => $NEB_PASS,
    ), "add dbs");

    is($config->n_db, 2, "n dbs");

    my $neb = Nebulous::Server->new_from_config($config);
     
    isa_ok($neb, 'Nebulous::Server');
}

Test::Nebulous->cleanup;
