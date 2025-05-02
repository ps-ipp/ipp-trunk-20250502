#!/bin/env perl
###
###     Tests for the PStamp module
###

use warnings;
use strict;


use Getopt::Long qw( GetOptions );
use PS::IPP::PStamp::Job qw( :standard );

use PS::IPP::Config qw( :standard );
my $ipprc = PS::IPP::Config->new(); # IPP Configuration

my $verbose;
my $image_db;
my $req_type;
my $img_type;
my $id;
my $x;
my $y;
my $mjd_min;
my $mjd_max;
my $filter;
my $skycell_id;
my $tess_id;
my $component;
my $data_group;
my $inverse;
my $need_magic = 1;



GetOptions(
    'dbname=s'  =>  \$image_db,
    'req_type=s'=>  \$req_type,
    'img_type=s'=>  \$img_type,
    'id=s'      =>  \$id,
    'component=s'=>  \$component,
    'x=s'       =>  \$x,
    'y=s'       =>  \$y,
    'mjd_min=s' =>  \$mjd_min,
    'mjd_max=s' =>  \$mjd_max,
    'filter=s'  =>  \$filter,
    'verbose'   =>  \$verbose,
);

my $err = "";
$err .= " --req_type is required" if !$req_type;
$err .= " --img_type is required" if !$img_type;
$err .= " --id is required" if ($req_type ne "bycoord") && !$id;
$err .= " --dbname is required" if !$image_db;

if ($err) {
    print STDERR "$err\n";
    exit $PS_EXIT_DATA_ERROR;
}

$component = "null" if !defined($component);

my $results = locate_images($ipprc, $image_db, $req_type, $img_type, $id, $tess_id, $component,
            $inverse, $need_magic, 
            $x, $y, $mjd_min, $mjd_max, $filter, $data_group, $verbose);

foreach my $i (@$results) {
    print "${img_type}_id: $id" if $id;
    if ($i->{exp_id}) {
        print " exp_id $i->{exp_id}\n"
    } else {
        print "\n";
    }
    print "\timage:  $i->{image}\n";
    print "\tmask:   $i->{mask}\n"   if $i->{mask};
    print "\tweight: $i->{weight}\n" if $i->{weight};
    print "\tastrom: $i->{astrom}\n" if $i->{astrom};
    if ($img_type ne "stack") {
        print "\tcamera: $i->{camera}\n";
        print "\texp_id: $i->{exp_id}\n";
        print "\texp_name: $i->{exp_name}\n";
    }
}

exit 0;

