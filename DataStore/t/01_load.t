# -*- perl -*-

# t/001_load.t - check module loading and create testing directory

use lib qw( ./lib ./t );

use Test::More tests => 11;

BEGIN { use_ok('DataStore'); }
BEGIN { use_ok('DataStore::File'); }
BEGIN { use_ok('DataStore::File::Parser'); }
BEGIN { use_ok('DataStore::FileSet'); }
BEGIN { use_ok('DataStore::FileSet::Parser'); }
BEGIN { use_ok('DataStore::Product'); }
BEGIN { use_ok('DataStore::Product::Parser'); }
BEGIN { use_ok('DataStore::Root'); }
BEGIN { use_ok('DataStore::Record'); }
BEGIN { use_ok('DataStore::Response'); }
BEGIN { use_ok('DataStore::Utils'); }
