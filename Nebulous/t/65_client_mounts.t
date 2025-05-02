#!/usr/bin/perl

# Copryight (C) 2004-2005  Joshua Hoblitt
#
# $Id: 65_client_mounts.t,v 1.3 2008-09-11 22:59:15 jhoblitt Exp $

use strict;
use warnings FATAL => qw( all );

use Apache::Test qw( -withtestmore );

plan tests => 18;

use lib qw( ./t ./lib );

use Nebulous::Client;
use Nebulous::Util qw( :standard );
use Test::Nebulous;

my $hostport = Apache::Test->config->{ 'hostport' };

Test::Nebulous->setup;

{
    # key
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );
    my $mounts = $neb->mounts();

    is(scalar @$mounts, 8, "number of rows");

    my %row;
    # first row
    @row{qw(mountpoint total used vol_id name host path allocate available xattr)}
        = @{$mounts->[0]};

    is($row{total},     100000000000);
    is($row{used},      100000000);
    is($row{vol_id},    1);
    is($row{name},      "node01");
    is($row{host},      "node01");
    is($row{allocate},  1);
    is($row{available}, 1);
    is($row{xattr},     0);

    # 2nd row
    @row{qw(mountpoint total used vol_id name host path allocate available xattr)}
        = @{$mounts->[1]};

    is($row{total},     100000000000);
    is($row{used},      1000000000);
    is($row{vol_id},    2);
    is($row{name},      "node02");
    is($row{host},      "node02");
    is($row{allocate},  1);
    is($row{available}, 1);
    is($row{xattr},     0);
}

Test::Nebulous->setup;

eval {
    my $neb = Nebulous::Client->new(
        proxy => "http://$hostport/nebulous",
    );

    $neb->mounts("foo");
};
like( $@, qr/0 were expected/, "too many params" );

Test::Nebulous->cleanup;
