#!/usr/bin/perl

# Copryight (C) 2004-2005  Joshua Hoblitt
#
# $Id: 59_client_delete.t,v 1.2 2008-08-01 23:55:26 jhoblitt Exp $

use strict;
use warnings FATAL => qw( all );

use Apache::Test qw( -withtestmore );

plan tests => 18;

use lib qw( ./t ./lib );

use Nebulous::Client;
use Nebulous::Util qw( :standard );
use Test::Nebulous;

my $hostport = Apache::Test->config->{ 'hostport' };

# ensure fresh start:
Test::Nebulous->setup;
Test::Nebulous->cleanup;

Test::Nebulous->setup;
{
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    my $uri = $neb->create( "foo" );

    ok( $neb->delete( "foo" ), "delete object" );

    ok( ! -e _get_file_path($uri), "deleted file" );

    my $locations = $neb->find_instances( "foo" );

    is( $locations, undef, "no instances" );
}

Test::Nebulous->setup;

{
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->create( "foo" );
    $neb->replicate( "foo" );

    ok( $neb->delete( "foo" ), "delete object" );

    my $locations = $neb->find_instances( "foo" );

    is( $locations, undef, "no instances" );
}

Test::Nebulous->setup;

{
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->create( "neb://node01/foo" );

    ok( $neb->delete( "neb://node02/foo" ), "delete object" );

    my $locations = $neb->find_instances( "foo" );

    is( $locations, undef, "no instances" );
}

Test::Nebulous->setup;

{
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );

    is( $neb->delete( "foo" ), undef, "delete non-existant object" );
}

# test passing in undefined force flag
Test::Nebulous->setup;
{
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->create( "foo" );

    is( $neb->delete("foo", undef), 1, "force flag false" );
}

# test passing in force flag = 1
Test::Nebulous->setup;
{
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->create( "foo" );

    is( $neb->delete("foo", 1), 1, "force flag false" );
}

# test passing in force flag = 0
Test::Nebulous->setup;
{
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->create( "foo" );

    is( $neb->delete("foo", 0), 1, "force flag false" );
}

# test passing in force flag = 0 where disk file is removed
Test::Nebulous->setup;
eval {
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->create( "foo" );
    my $locations = $neb->find_instances( "foo" );
    my $diskfile = _get_file_path(@$locations[0]);
    unlink $diskfile;
    $neb->delete("foo", 0);
};
like( $@, qr/can't unlink file/, "no force flag, cannot delete inconsistent file" );

# test passing in force flag = 1 where disk file is removed
Test::Nebulous->setup;
{
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->create( "foo" );
    my $locations = $neb->find_instances( "foo" );
    my $diskfile = _get_file_path(@$locations[0]);
    unlink $diskfile;
    is( $neb->delete("foo", 1), 1, "force flag true deletes file" );
}

# test passing in undef force flag where disk file is removed
Test::Nebulous->setup;
eval {
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->create( "foo" );
    my $locations = $neb->find_instances( "foo" );
    my $diskfile = _get_file_path(@$locations[0]);
    unlink $diskfile;
    $neb->delete("foo", undef);
};
like( $@, qr/can't unlink file/, "no force flag, cannot delete inconsistent file" );

# test passing in undefined force flag where disk file is removed
Test::Nebulous->setup;
eval {
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->create( "foo" );
    my $locations = $neb->find_instances( "foo" );
    my $diskfile = _get_file_path(@$locations[0]);
    unlink $diskfile;
    $neb->delete("foo");
};
like( $@, qr/can't unlink file/, "no force flag, cannot delete inconsistent file" );

Test::Nebulous->setup;

eval {
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->delete();
};
like( $@, qr/1 - 3 were expected/, "no params" );

Test::Nebulous->setup;

eval {
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->delete( "foo", 3);
};
like( $@, qr/is boolean/, "force flag not boolean" );

Test::Nebulous->setup;

eval {
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->delete( "foo", 0, 1, 2 );
};
like( $@, qr/1 - 3 were expected/, "too many params" );

Test::Nebulous->cleanup;
