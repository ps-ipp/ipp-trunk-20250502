#!/usr/bin/perl

# Copryight (C) 2004-2005  Joshua Hoblitt
#
# $Id: 54_client_lock.t,v 1.1 2005-12-03 02:52:31 jhoblitt Exp $

use strict;
use warnings FATAL => qw( all );

use Apache::Test qw( -withtestmore );

plan tests => 19;

use lib qw( ./t ./lib );

use Nebulous::Client;
use Nebulous::Util qw( :standard );
use Test::Nebulous;

my $hostport = Apache::Test->config->{ 'hostport' };

Test::Nebulous->setup;

{
    # key, type
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->create( "foo" );

    ok( $neb->lock( "foo", "read" ), "read lock" );
}

Test::Nebulous->setup;

{
    # key, type, timeout
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->create( "foo" );

    ok( $neb->lock( "foo", "read", 10 ), "read lock" );
}

Test::Nebulous->setup;

{
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->create( "foo" );

    ok( $neb->lock( "foo", "read" ), "read lock" );
    ok( $neb->lock( "foo", "read" ), "read lock" );
}

Test::Nebulous->setup;

{
    # key, type
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->create( "foo" );

    ok( $neb->lock( "foo", "write" ), "write lock" );
}

Test::Nebulous->setup;

{
    # key, type, timeout
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->create( "foo" );

    ok( $neb->lock( "foo", "write", 10 ), "write lock" );
}

Test::Nebulous->setup;

{
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );

    is( $neb->lock( "foo", "read" ), undef, "storage object does not exist" );
}

Test::Nebulous->setup;

{
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );

    is( $neb->lock( "foo", "write" ), undef, "storage object does not exist" );
}

Test::Nebulous->setup;

diag "testing a spinlock, this will be a litle slow...";

eval {
    local $SIG{ALRM} = sub { alarm 0; die "timeout"; };
    alarm 8;

    # key, type, timeout
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->create( "foo" );

    $neb->lock( "foo", "read" );
    $neb->lock( "foo", "write" );

    alarm 0;
};
like( $@, qr/timeout/, "default spinlock" );

Test::Nebulous->setup;

diag "testing a spinlock, this will be a litle slow...";

eval {
    local $SIG{ALRM} = sub { alarm 0; die "timeout"; };
    alarm 12;

    # key, type, timeout
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->create( "foo" );

    $neb->lock( "foo", "read" );
    unless ( $neb->lock( "foo", "write" ) ) {
        alarm 0;
        die "gaveup";
    }

    alarm 0;
};
like( $@, qr/gaveup/, "default spinlock" );

Test::Nebulous->setup;

diag "testing a spinlock, this will be a litle slow...";

eval {
    local $SIG{ALRM} = sub { alarm 0; die "timeout"; };
    alarm 3;

    # key, type, timeout
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->create( "foo" );

    $neb->lock( "foo", "read" );
    $neb->lock( "foo", "write", 5 );

    alarm 0;
};
like( $@, qr/timeout/, "specified spinlock" );

Test::Nebulous->setup;

diag "testing a spinlock, this will be a litle slow...";

eval {
    local $SIG{ALRM} = sub { alarm 0; die "timeout"; };
    alarm 7;

    # key, type, timeout
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->create( "foo" );

    $neb->lock( "foo", "read" );
    unless ( $neb->lock( "foo", "write", 5 ) ) {
        alarm 0;
        die "gaveup";
    }

    alarm 0;
};

like( $@, qr/gaveup/, "specified spinlock" );

Test::Nebulous->setup;

{
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->create( "foo" );

    $neb->lock( "foo", "write" );
    is( $neb->lock( "foo", "write", 1 ), undef, "can not write lock twice" );
}

Test::Nebulous->setup;

{
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->create( "foo" );

    $neb->lock( "foo", "read" );
    is( $neb->lock( "foo", "write", 1 ), undef, "can not write lock after read lock" );
}

Test::Nebulous->setup;

{
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    $neb->create( "foo" );

    $neb->lock( "foo", "write" );
    is( $neb->lock( "foo", "read", 1 ), undef, "can not read lock after write lock" );
}

Test::Nebulous->setup;

eval {
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );

    $neb->lock();
};
like( $@, qr/2 - 3 were expected/, "no params" );

Test::Nebulous->setup;

eval {
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );

    $neb->lock( "foo" );
};
like( $@, qr/2 - 3 were expected/, "not enough params" );

Test::Nebulous->setup;

eval {
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );

    $neb->lock( "foo", "both" );
};
like( $@, qr/is read or write/, "not read or write" );

Test::Nebulous->setup;

eval {
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );

    $neb->lock( "foo", 'read', 3, "bar" );
};
like( $@, qr/2 - 3 were expected/, "too many params" );

Test::Nebulous->cleanup;
