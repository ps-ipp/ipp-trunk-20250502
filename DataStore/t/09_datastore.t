#!/usr/bin/env perl

# Copyright (C) 2006  Joshua Hoblitt
#
# $Id: 09_datastore.t,v 1.2 2006-03-16 01:23:10 jhoblitt Exp $

use strict;
use warnings;

use lib qw( ./lib ./t );

use Test::More tests => 7;

=head1 NAME

t/09_datastore.t - tests DataStore

=head1 SYNOPSIS
    
    prove t/09_datastore.t

=cut

use DataStore;

ok(%DataStore::, "DataStore is loaded");
ok(%DataStore::File::Parser::, "DataStore::File::Parser is loaded");
ok(%DataStore::File::, "DataStore::File is loaded");
ok(%DataStore::FileSet::Parser::, "DataStore::FileSet::Parser is loaded");
ok(%DataStore::FileSet::, "DataStore::Set is loaded");
ok(%DataStore::Product::, "DataStore::Product is loaded");
ok(%DataStore::Response::, "DataStore::Response is loaded");

