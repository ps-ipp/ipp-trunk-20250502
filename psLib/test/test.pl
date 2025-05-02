#!/usr/bin/env perl

# Copyright (C) 2006  Joshua Hoblitt
#
# $Id: test.pl,v 1.2 2006-09-12 20:27:07 jhoblitt Exp $

use strict;
use warnings FATAL => qw( all);

use vars qw($VERSION);
$VERSION = '0.01';

use File::Find::Rule;
use Cwd;

my $rule = File::Find::Rule->new;
# ignore .lib directories
$rule->or($rule->new
        ->directory
        ->name('.libs')
        ->prune
        ->discard,
        $rule->new
    );
$rule->name(qr/^tap_[^.]*$/)
        ->maxdepth(2)
        ->relative;

my @test_files = $rule->in(getcwd());

system("prove @test_files");
