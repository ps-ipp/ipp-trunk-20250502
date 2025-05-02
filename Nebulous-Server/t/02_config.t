#!/usr/bin/perl

# Copryight (C) 2004-2005  Joshua Hoblitt
#
# $Id: 02_config.t,v 1.1.2.2 2008-12-14 22:52:37 eugene Exp $

use strict;
use warnings;

use Test::More tests => 22;

use lib qw( ./t ./lib );

use Nebulous::Server::Config;

isa_ok(Nebulous::Server::Config->new(), "Nebulous::Server::Config");

{
    my $config = Nebulous::Server::Config->new;

    ok($config->add_db(
        dbindex     => 0,
        dsn         => "DBI:mysql:database=foobar:host=localhost",
        dbuser      => "baz",
        dbpasswd    => "boo",
    ), "add a db");

    is($config->n_db, 1, "number of dbs");
}


{
    my $config = Nebulous::Server::Config->new;

    ok($config->add_db(
        dbindex     => 0,
        dsn         => "DBI:mysql:database=foobar:host=localhost",
        dbuser      => "baz",
        dbpasswd    => "boo",
    ), "add a db");

    ok($config->add_db(
        dbindex     => 0,
        dsn         => "DBI:mysql:database=foobar:host=localhost",
        dbuser      => "baz",
        dbpasswd    => "boo",
    ), "add a db");

    is($config->n_db, 1, "number of dbs");
}

{
    my $config = Nebulous::Server::Config->new;

    ok($config->add_db(
        dbindex     => 0,
        dsn         => "DBI:mysql:database=foobar:host=localhost",
        dbuser      => "baz",
        dbpasswd    => "boo",
    ), "add a db");

    ok($config->add_db(
        dbindex     => 1,
        dsn         => "DBI:mysql:database=foobar:host=localhost",
        dbuser      => "baz",
        dbpasswd    => "boo",
    ), "add a db");

    is($config->n_db, 2, "number of dbs");
}

{
    my $config = Nebulous::Server::Config->new;

    $config->add_db(
        dbindex     => 0,
        dsn         => "DBI:mysql:database=foobar:host=localhost",
        dbuser      => "baz",
        dbpasswd    => "boo",
    );

    $config->add_db(
        dbindex     => 1,
        dsn         => "DBI:mysql:database=foobar:host=localhost2",
        dbuser      => "baz2",
        dbpasswd    => "boo2",
    );

    # default should be 0
    my $config_db = $config->db();

    isa_ok($config_db, "Nebulous::Server::Config::DB");
    is($config_db->dsn, "DBI:mysql:database=foobar:host=localhost", "dsn");
    is($config_db->dbuser, "baz", "dbuser");
    is($config_db->dbpasswd, "boo", "dbpasswd");

    my $config_db0 = $config->db(0);

    isa_ok($config_db0, "Nebulous::Server::Config::DB");
    is($config_db0->dsn, "DBI:mysql:database=foobar:host=localhost", "dsn");
    is($config_db0->dbuser, "baz", "dbuser");
    is($config_db0->dbpasswd, "boo", "dbpasswd");

    my $config_db1 = $config->db(1);

    isa_ok($config_db1, "Nebulous::Server::Config::DB");
    is($config_db1->dsn, "DBI:mysql:database=foobar:host=localhost2", "dsn");
    is($config_db1->dbuser, "baz2", "dbuser");
    is($config_db1->dbpasswd, "boo2", "dbpasswd");
}

# memcached_servers
{
    my $config = Nebulous::Server::Config->new(
            memcached_servers => ['127.0.0.1:11211'],
        );

    is_deeply($config->memcached_servers, ['127.0.0.1:11211'], "memcached_servers");
}
