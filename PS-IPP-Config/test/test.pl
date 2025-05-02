#!/usr/bin/env perl

use warnings;
use strict;

use PS::IPP::Config qw( caturi );

print ( caturi("path://PATH", "file") . "\n");
print ( caturi("path://PATH/", "file") . "\n");
print ( caturi("path://PATH/something", "file") . "\n");
print ( caturi("path://PATH/something/", "file") . "\n");
print ( caturi("neb://", "file") . "\n");
print ( caturi("neb://PATH", "file") . "\n");
print ( caturi("neb://PATH/", "file") . "\n");
print ( caturi("neb://PATH/something", "file") . "\n");
print ( caturi( "/PATH", "file") . "\n");
print ( caturi("/PATH/", "file") . "\n");
print ( caturi( "/PATH/something", "file") . "\n");
print ( caturi("/PATH/something/", "file") . "\n");

print "\n";

my $ipprc = PS::IPP::Config->new();

print ( $ipprc->convert_filename_absolute("path://SIMTEST") . "\n");
print ( $ipprc->convert_filename_absolute("path://SIMTEST/") . "\n");
print ( $ipprc->convert_filename_absolute("path://SIMTEST/something") . "\n");
print ( $ipprc->convert_filename_absolute("path://SIMTEST/something/") . "\n");

print "\n";

print ( $ipprc->convert_filename_relative("/data/mithrandir.2/price/temp") . "\n");
print ( $ipprc->convert_filename_relative("/data/mithrandir.2/price/temp/") . "\n");
print ( $ipprc->convert_filename_relative("/data/mithrandir.2/price/temp/something") . "\n");
print ( $ipprc->convert_filename_relative("/data/mithrandir.2/price/temp/something/") . "\n");

