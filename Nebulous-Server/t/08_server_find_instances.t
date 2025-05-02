#!/usr/bin/perl

# Copryight (C) 2004-2005  Joshua Hoblitt
#
# $Id: 07_server_find_instances.t,v 1.16 2008-09-11 22:35:52 jhoblitt Exp $

use strict;
use warnings FATAL => qw( all );

use Test::More tests => 44;

use lib qw( ./t ./lib );

use Nebulous::Server;
use Test::URI;
use Test::Nebulous;

my $neb = Nebulous::Server->new(
    dsn         => $NEB_DB,
    dbuser      => $NEB_USER,
    dbpasswd    => $NEB_PASS,
);

# XXX previously, the logic was: if we request a hard volume location
# using ~node, then we would give an error if no data is available except on that node.

# the code has been modified so that the requested storage object is provided if possible, 
# even if the apparently "hard" target did not exist.

Test::Nebulous->setup;
Test::Nebulous->cleanup; # make sure the database is reset before running any tests

# for (my $i = 0; $i < $Nloc; $i++) {
#	my $loc = $locations->[$i];
#	print "location: $loc\n";
#}

Test::Nebulous->setup;
{
    # key
    my $uri = $neb->create_object("foo");

    my $locations = $neb->find_instances("foo");

    uri_scheme_ok($locations->[0], 'file');
    is($uri, $locations->[0], "URIs match");
}

Test::Nebulous->setup;
{
    # key
    my $uri1 = $neb->create_object("foo");
    my $uri2 = $neb->replicate_object("foo");

    my $locations = $neb->find_instances("foo");

    uri_scheme_ok($locations->[0], 'file');
    uri_scheme_ok($locations->[1], 'file');
    ok(eq_set([$uri1, $uri2], $locations), "URIs match");
}

Test::Nebulous->setup;
{
    # key, volume
    my $uri = $neb->create_object('foo', 'node01');

    my $locations = $neb->find_instances('foo', 'node01');

    uri_scheme_ok($locations->[0], 'file');
    is($uri, $locations->[0], "URIs match");
}

Test::Nebulous->setup;
{
    # key, volume
    my $uri = $neb->create_object('foo', 'node01');

    my $locations = $neb->find_instances('foo', undef);

    uri_scheme_ok($locations->[0], 'file');
    is($uri, $locations->[0], "URIs match");
}

Test::Nebulous->setup;
{
    # key, volume
    my $uri = $neb->create_object('foo', 'node01');

    my $locations = $neb->find_instances('neb://node02/foo', "~any");

    uri_scheme_ok($locations->[0], 'file');
    is($uri, $locations->[0], "URIs match");
}

Test::Nebulous->setup;
{
    # key
    my $uri1 = $neb->create_object("foo");
    my $uri2 = $neb->replicate_object("foo");

    my $locations = $neb->find_instances("foo", "~any");

    uri_scheme_ok($locations->[0], 'file');
    uri_scheme_ok($locations->[1], 'file');
    ok(eq_set([$uri1, $uri2], $locations), "URIs match");
}

# request from a hard_volume returns an instance which is not on the hard volume if available
Test::Nebulous->setup;
eval {
    # key, volume
    my $uri = $neb->create_object('foo', '~node01');
    my $locations = $neb->find_instances('foo', '~node02');

    uri_scheme_ok($locations->[0], 'file');
    is($uri, $locations->[0], "URIs match");
};
is($@, "", "instance on different node (as hard_volume) returned");

# request from a hard_volume returns an instance which is not on the hard volume if available
Test::Nebulous->setup;
eval {
    # key, volume
    my $uri = $neb->create_object('foo', '~node01');
    my $locations = $neb->find_instances('foo', '~node02');

    uri_scheme_ok($locations->[0], 'file');
    is($uri, $locations->[0], "URIs match");
};
is($@, "", "instance on different node (as hard_volume) returned");

### TESTS FOR INSTANCE ON INVALID NODE
diag ("TESTS FOR INSTANCE ON INVALID NODE");

# request from a soft_volume for an instance which is on an invalid node (find_invalid = 1) [SOFT, 1]
Test::Nebulous->setup;
eval {
    # key, volume
    my $uri = $neb->create_object('foo', '~node02');
    Test::Nebulous->switch_node_state;

    my $locations = $neb->find_instances('foo', 'node02', 1);
    uri_scheme_ok($locations->[0], 'file');
    is($uri, $locations->[0], "URIs match");
};
is($@, "", "instance on invalid node (as soft_volume) returned with find_invalid = 1");

# request from a soft_volume for an instance which is on an invalid node (find_invalid = 0) [SOFT, 0]
Test::Nebulous->setup;
eval {
    # key, volume
    my $uri = $neb->create_object('foo', '~node02');
    Test::Nebulous->switch_node_state;

    my $locations = $neb->find_instances('foo', 'node02', 0);
};
like($@, qr/database error/, "instance on invalid node (as soft_volume) not returned (find_invalid = 0)");

# request from a hard_volume for an instance which is on an invalid node (find_instance not supplied) [SOFT, X]
Test::Nebulous->setup;
eval {
    # key, volume
    my $uri = $neb->create_object('foo', '~node02');
    Test::Nebulous->switch_node_state;

    my $locations = $neb->find_instances('foo', 'node02');
};
like($@, qr/no instances available for key/, "instance on invalid node (as soft_volume) not returned (find_invalid not set)");

# request from a hard_volume for an instance which is on an invalid node (find_invalid = 1) [HARD, 1]
Test::Nebulous->setup;
eval {
    # key, volume
    my $uri = $neb->create_object('foo', '~node02');
    Test::Nebulous->switch_node_state;

    my $locations = $neb->find_instances('foo', '~node02', 1);
    uri_scheme_ok($locations->[0], 'file');
    is($uri, $locations->[0], "URIs match");
};
is($@, "", "instance on invalid node (as hard_volume) returned with find_invalid = 1");

# request from a hard_volume for an instance which is on an invalid node (find_invalid = 0) [HARD, 0]
Test::Nebulous->setup;
eval {
    # key, volume
    my $uri = $neb->create_object('foo', '~node02');
    Test::Nebulous->switch_node_state;

    my $locations = $neb->find_instances('foo', '~node02', 0);
};
like($@, qr/no instances on storage volume/, "instance on invalid node (as hard_volume) not returned (find_invalid = 0)");

# request from a hard_volume for an instance which is on an invalid node [HARD, X]
Test::Nebulous->setup;
eval {
    # key, volume
    my $uri = $neb->create_object('foo', '~node02');
    Test::Nebulous->switch_node_state;

    my $locations = $neb->find_instances('foo', '~node02');
};
like($@, qr/no instances on storage volume/, "instance on invalid node (as hard_volume) not returned (find_invalid not set)");

#### TESTING the instance proximity analysis
Test::Nebulous->setup;
{
    my @nodedirs = Test::Nebulous->get_node_dirs();

    # place 4 instances on specific nodes (hard targets works):
    my $uri1 = $neb->create_object("foo", "~node01");
    my $uri2 = $neb->replicate_object("foo", "~node02");
    my $uri3 = $neb->replicate_object("foo", "~node03");
    my $uri4 = $neb->replicate_object("foo", "~node08");

    my $locations = $neb->find_instances("foo", "node01");
    my $Nloc = @$locations;

    is($Nloc, 4, "found 4 instances");
    
    my $loc1 = $locations->[0];
    my $loc2 = $locations->[1];
    my $loc3 = $locations->[2];
    my $loc4 = $locations->[3];

    # choosing instance relative to node01 yields:
    like($loc1, qr/$nodedirs[0]/, "node01 is closest to node01");    
    like($loc2, qr/$nodedirs[1]/, "node02 is 2nd closest to node01");
    like($loc3, qr/$nodedirs[2]/, "node03 is 3rd closest to node01");
    like($loc4, qr/$nodedirs[7]/, "node08 is 4th closest to node01");
}

#### TESTING the instance proximity analysis
Test::Nebulous->setup;
{
    my @nodedirs = Test::Nebulous->get_node_dirs();

    # place 4 instances on specific nodes (hard targets works):
    my $uri1 = $neb->create_object("foo", "~node01");
    my $uri2 = $neb->replicate_object("foo", "~node02");
    my $uri3 = $neb->replicate_object("foo", "~node03");
    my $uri4 = $neb->replicate_object("foo", "~node08");

    my $locations = $neb->find_instances("foo", "node03");
    my $Nloc = @$locations;

    is($Nloc, 4, "found 4 instances");
    
    my $loc1 = $locations->[0]; # 
    my $loc2 = $locations->[1]; # 
    my $loc3 = $locations->[2]; # 
    my $loc4 = $locations->[3]; # 

    # choosing instance relative to node01 yields:
    like($loc1, qr/$nodedirs[2]/, "node03 is closest to node03");
    like($loc2, qr/$nodedirs[7]/, "node08 is 2nd closest to node03");
    like($loc3, qr/$nodedirs[1]/, "node02 is 3rd closest to node03");
    like($loc4, qr/$nodedirs[0]/, "node08 is 4th closest to node03");
}

########

Test::Nebulous->setup;

eval {
    $neb->find_instances('foo');
};
like($@, qr/is valid object key/, "storage object does not exist");

Test::Nebulous->setup;

eval {
    $neb->create_object('foo');
    $neb->find_instances('foo', '~bar');
};
like($@, qr/is not a valid volume name/, "storage volume does not exist");

Test::Nebulous->setup;
eval {
    my $locations = $neb->find_instances();
};
like($@, qr/1 - 3 were expected/, "no params");
# note the new API expects 1-3 parameters

Test::Nebulous->setup;

eval {
    $neb->create_object('foo');
    $neb->find_instances('foo', 'node01', 3, 5);
};
like($@, qr/1 - 3 were expected/, "too many params");
# note the new API expects 1-3 parameters

Test::Nebulous->cleanup;
