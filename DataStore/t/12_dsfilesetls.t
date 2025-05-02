#!/usr/bin/env perl

# Copyright (C) 2006  Joshua Hoblitt
#
# $Id: 12_dsfilesetls.t,v 1.1 2006-03-20 22:03:53 jhoblitt Exp $

use strict;
use warnings;

use lib qw( ./lib ./t );

use Test::Cmd;
use Test::More tests => 3;

=head1 NAME

t/12_dsfilesetls.t - tests dsfilesetls

=head1 SYNOPSIS
    
    prove t/12_dsfilesetls.t

=cut

my $prog = 'scripts/dsfilesetls';

# last ditch effort to make sure dsproductls is executable
chmod 0755, $prog;

my $test = Test::Cmd->new( prog => $prog, workdir => '' );
isa_ok($test, 'Test::Cmd');

{
    $test->run( args => '' );
    missing_args(3, "Required options: --uri");
}

sub missing_args
{
    my ($exit, $errstr) = @_;

    is($? >> 8, $exit, "error code is: $exit");
    like($test->stderr, qr/$errstr/, "error string is: $errstr");
}

