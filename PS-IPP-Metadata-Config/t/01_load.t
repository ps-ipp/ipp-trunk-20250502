#!/usr/bin/perl

# Copyright (C) 2005  Joshua Hoblitt
#
# $Id: 01_load.t,v 1.1.1.1 2005-03-01 03:38:45 jhoblitt Exp $

use strict;
use warnings FATAL => qw( all );

use lib qw( ./lib );

use Test::More tests => 1;

BEGIN { use_ok( 'PS::IPP::Metadata::Config' ); }
