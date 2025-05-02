#!/usr/bin/perl

# Copryight (C) 2004-2005  Joshua Hoblitt
#
# $Id: 01_load.t,v 1.9 2008-03-21 01:24:00 jhoblitt Exp $

use strict;
use warnings FATAL => qw( all );

use lib qw( ./t ./lib );

use Test::More tests => 5;

BEGIN { use_ok( 'Nebulous::Key' ); }
BEGIN { use_ok( 'Nebulous::Server' ); }
BEGIN { use_ok( 'Nebulous::Server::Log' ); }
BEGIN { use_ok( 'Nebulous::Server::SOAP' ); }
BEGIN { use_ok( 'Nebulous::Server::SQL' ); }
