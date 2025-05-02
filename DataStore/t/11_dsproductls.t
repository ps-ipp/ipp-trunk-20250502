#!/usr/bin/env perl

# Copyright (C) 2006  Joshua Hoblitt
#
# $Id: 11_dsproductls.t,v 1.1 2006-03-18 04:02:35 jhoblitt Exp $

use strict;
use warnings;

use lib qw( ./lib ./t );

use Test::Cmd;
use Test::More tests => 5;

=head1 NAME

t/11_dsproductls.t - tests dsproductls

=head1 SYNOPSIS
    
    prove t/11_dsproductls.t

=cut

# last ditch effort to make sure dsproductls is executable
chmod 0755, 'scripts/dsproductls';

my $test = Test::Cmd->new(prog => 'scripts/dsproductls', workdir => '');
isa_ok($test, 'Test::Cmd');

{
    $test->run(args => '');
    missing_args(3, "Required options: --uri");
}

{
    $test->run(args => '--last_fileset foobar');
    missing_args(3, "Required options: --uri");
}

sub missing_args
{
    my ($exit, $errstr) = @_;

    is($? >> 8, $exit, "error code is: $exit");
    like($test->stderr, qr/$errstr/, "error string is: $errstr");
}

