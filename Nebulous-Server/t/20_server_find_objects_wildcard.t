#!/usr/bin/perl

# Copryight (C) 2004-2005  Joshua Hoblitt
#
# $Id: 12_server_find_objects.t,v 1.4 2008-05-16 20:29:19 jhoblitt Exp $

use strict;
use warnings FATAL => qw( all );

use Test::More tests => 22;

use lib qw( ./t ./lib );

use Nebulous::Server;
use Test::Nebulous;

my $neb = Nebulous::Server->new(
    dsn         => $NEB_DB,
    dbuser      => $NEB_USER,
    dbpasswd    => $NEB_PASS,
);

Test::Nebulous->setup;
Test::Nebulous->cleanup;

&mktestfiles ($neb);
{
    my $keys = $neb->find_objects_wildcard("topdir");

    is(scalar @$keys, 4, 'number of keys found');
    is($keys->[0], "topdir/subdir/", "key name matches");
    is($keys->[1], "topdir/object1", "key name matches");
    is($keys->[2], "topdir/object2", "key name matches");
    is($keys->[3], "topdir/objfoobar", "key name matches");
}

&mktestfiles ($neb);
{
    my $keys = $neb->find_objects_wildcard("topdir/%");

    is(scalar @$keys, 4, 'number of keys found');
    is($keys->[0], "topdir/subdir/", "key name matches");
    is($keys->[1], "topdir/object1", "key name matches");
    is($keys->[2], "topdir/object2", "key name matches");
    is($keys->[3], "topdir/objfoobar", "key name matches");
}

&mktestfiles ($neb);
{
    my $keys = $neb->find_objects_wildcard("topdir/obj%");

    is(scalar @$keys, 3, 'number of keys found');
    is($keys->[0], "topdir/object1", "key name matches");
    is($keys->[1], "topdir/object2", "key name matches");
    is($keys->[2], "topdir/objfoobar", "key name matches");
}

&mktestfiles ($neb);
{
    my $keys = $neb->find_objects_wildcard("topdir/object%");

    is(scalar @$keys, 2, 'number of keys found');
    is($keys->[0], "topdir/object1", "key name matches");
    is($keys->[1], "topdir/object2", "key name matches");
}

&mktestfiles ($neb);
{
    my $keys = $neb->find_objects_wildcard("obj%");

    is(scalar @$keys, 3, 'number of keys found');
    is($keys->[0], "object1", "key name matches");
    is($keys->[1], "object2", "key name matches");
    is($keys->[2], "objfoobar", "key name matches");
}

eval {
    $neb->find_objects_wildcard("foo", 3);
};
like($@, qr/1 was expected/, "too many params");

Test::Nebulous->cleanup;

sub mktestfiles {
    my ($neb) = $_[0];

    Test::Nebulous->setup;
    
    # find_objects_wildcard allows these searches:
    # -> dir : implies dir/* (dir/%)
    # -> dir/%
    # -> dir/sub%
    # -> sub%
    
    # create examples matching the above
    $neb->create_object("object1");
    $neb->create_object("object2");
    $neb->create_object("objfoobar");
    
    $neb->create_object("topdir/object1");
    $neb->create_object("topdir/object2");
    $neb->create_object("topdir/objfoobar");
    
    $neb->create_object("topdir/subdir/object1");
    $neb->create_object("topdir/subdir/object2");
    $neb->create_object("topdir/subdir/objfoobar");
    
    return 1;
}
