#!/usr/bin/env perl

# listsmf.pl Print a listing of information about smf files for processing meeting
# certain selection criteria.

use strict;
use warnings;

use DBI;
use File::Basename;
use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

my $verbose;

my $cam_id;
my $exp_id;
my $exp_name;
my $filter;
my $label;
my $data_group;
my $dateobs_min;
my $dateobs_max;
my $release;
my $getMagicked;

# NOTE: We do not use the ipp configuration to get this information in order to simplify running from a CGI script
# XXX: probably should be using some local file though.
# XXX: take a look at my module dsdbh which has the function getDBHandle();
#
my $dbname = "gpc1";
my $dbserver = "scidbm";
my $dbuser = "ippuser";
my $dbpassword = "ippuser";


my $defaultToToday = 1;

GetOptions(
    'label=s'       =>      \$label,
    'data_group=s'  =>      \$data_group,
    'dateobs_min=s' =>      \$dateobs_min,
    'dateobs_max=s' =>      \$dateobs_max,
    'release=s'     =>      \$release,
    'filter=s'      =>      \$filter,
    'dbname=s'      =>      \$dbname,

    'exp_name=s'    =>      \$exp_name,
    'exp_id=s'      =>      \$exp_id,
    'cam_id=s'      =>      \$cam_id,
    'verbose|v'     =>      \$verbose,
) or pod2usage (2);


# if data_group is supplied we don't need to default the date
if ($data_group) {
    $defaultToToday = 0;
}

# XXX: since this is going to be used in a cgi script we should carefully vet the supplied
# date format.
if (!$dateobs_min) {
    # default to today
    if ($dateobs_max) {
        print "Must supply begin date if max date is supplied.\n";
        exit 2;
    }
    if ($defaultToToday) {
        my ($day, $month, $year) = (gmtime)[3,4,5];

        $dateobs_min = sprintf "%4d-%02d-%02dT00:00:00Z", 1900+$year, 1+$month, $day; 
        print "$dateobs_min\n" if $verbose;
    }
} else {
    # add times if not supplied
    if (!($dateobs_min =~ 'T')) {
        $dateobs_min .= 'T00:00:00Z';
        print "$dateobs_min\n" if $verbose;
    }
    if (!($dateobs_max =~ 'T')) {
        $dateobs_max .= 'T23:59:59Z';
        print "$dateobs_max\n" if $verbose;
    }
}


my $dbh = open_db();

my $query = 'SELECT cam_id, camRun.label, camRun.state, quality, release_name, camProcessedExp.path_base,'
    . ' exp_name, exp_id, filter, dateobs, comment'
    . ' FROM camRun JOIN camProcessedExp USING(cam_id) JOIN chipRun using(chip_id) JOIN rawExp USING(exp_id)'
    . ' LEFT JOIN relExp USING(cam_id, exp_id) LEFT JOIN ippRelease USING(rel_id)'
    . " WHERE ";

my $and = '';;
if ($data_group) {
    $query .= " camRun.data_group = '$data_group'";
    $and = ' AND';
}

if ($dateobs_min) {
    $query .= "$and dateobs > '$dateobs_min'";
}

if ($dateobs_max) {
    $query .= " AND dateobs <= '$dateobs_max'";
}

# Should I use "LIKE" qualifiers here?
if ($label) {
    $query .= " AND camRun.label = '$label'";
}

if ($release) {
    if ((uc($release) eq 'NONE') or (uc($release) eq 'NULL')) {
        $query .= " AND release_name IS NULL";
    } else {
        $query .= " AND release_name = '$release'";
    }
}

if ($filter) {
    # if single character filter is supplied append wild card 
    # character to the filter string
    if (length($filter) == 1) {
        $filter .= '%';
    }
    $query .= " AND filter LIKE '$filter'";
}

print "$query\n" if $verbose;

my $statement = $dbh->prepare($query);

$statement->execute();

my $numRows = 0;

# Print Header line as a comment.
# XXX: rather than fine tuning the spacing on this I should have used the
# same format line that I use for the data below.

print
'#cam_id smf_name                           quality state   exp_name    label                release        filter      obs_date   obs_time  comment' . "\n";
#1780291 o7616g0016o.1131500.cm.1780291.smf       0 full    o7616g0016o QUB.nightlyscience   none           z.00000     2016-08-16 05:57:32 'Transient PS16cgx z band cell 45 65'

while ( my $row = $statement->fetchrow_hashref() ) {
    $numRows++;

    my $smf_name = basename($row->{path_base}) . '.smf';
    my $release_name = $row->{release_name};
    $release_name = 'none' unless $release_name;

    printf "%ld %s   %5d %-7s %s %-20s %-14s %-11s %s  '%s'\n",
            $row->{cam_id}, $smf_name, $row->{quality}, $row->{state}, 
            $row->{exp_name}, $row->{label}, $release_name, $row->{filter}, 
            $row->{dateobs}, $row->{comment};
}

print "$numRows found\n" if $verbose;

exit 0;

sub open_db {
    my $dsn = "DBI:mysql:host=$dbserver;database=$dbname";

    my $dbh = DBI->connect($dsn, $dbuser, $dbpassword) 
        or die "Cannot connect to database $dbname.\n";

    return $dbh;
}

