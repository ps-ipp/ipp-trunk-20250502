#!/usr/bin/perl

# Copryight (C) 2004-2005  Joshua Hoblitt
#
# $Id: 01_load.t,v 1.8 2008-03-20 23:21:58 jhoblitt Exp $

use strict;
use warnings FATAL => qw( all );

use lib qw( ./t ./lib );

use Test::More tests => 4;

BEGIN { use_ok( 'Nebulous::Client' ); }
BEGIN { use_ok( 'Nebulous::Client::Log' ); }
BEGIN { use_ok( 'Nebulous::Client::HTTP' ); }
BEGIN { use_ok( 'Nebulous::Util' ); }
