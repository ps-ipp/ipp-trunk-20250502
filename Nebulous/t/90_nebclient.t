#!/usr/bin/perl

# Copryight (C) 2004-2005  Joshua Hoblitt
#
# $Id: 90_nebclient.t,v 1.2 2007-04-26 22:27:13 jhoblitt Exp $

use strict;
use warnings FATAL => qw( all );

use Apache::Test qw( -withtestmore );

use lib qw( ./t ./lib );

use Test::Nebulous;

my $hostport = Apache::Test->config->{ 'hostport' };

Test::Nebulous->setup;

$ENV{'NEB_SERVER'} = "http://" . Apache::Test->config->{ 'hostport' } . "/nebulous";

chdir "./nebclient";
system "make check";

Test::Nebulous->cleanup;
