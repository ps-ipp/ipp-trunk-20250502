#!/usr/bin/env perl

# Copyright (C) 2006  Joshua Hoblitt
#
# $Id: 10_dsget.t,v 1.2 2006-03-17 21:58:23 jhoblitt Exp $

use strict;
use warnings;

use lib qw( ./lib ./t );

use Test::Cmd;
use Test::More tests => 7;

=head1 NAME

t/10_dsget.t - tests dsget

=head1 SYNOPSIS
    
    prove t/10_dsget.t

=cut

# last ditch effort to make sure dsget is executable
chmod 0755, 'scripts/dsget';

my $test = Test::Cmd->new(prog => 'scripts/dsget', workdir => '');
isa_ok($test, 'Test::Cmd');

{
    $test->run(args => '');
    missing_args(3, "Required options: --uri --filename");
}

{
    $test->run(args => '--uri http://example.org/foo');
    missing_args(3, "Required options: --uri --filename");
}

{
    $test->run(args => '--filename /foo/bar/baz');
    missing_args(3, "Required options: --uri --filename");
}

sub missing_args
{
    my ($exit, $errstr) = @_;

    is($? >> 8, $exit, "error code is: $exit");
    like($test->stderr, qr/$errstr/, "error string is: $errstr");
}

