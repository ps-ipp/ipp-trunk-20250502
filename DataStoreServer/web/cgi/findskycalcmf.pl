#!/usr/bin/env perl

# findsskycalcmf.pl Locate a skycal cmf file for a skycell.

use strict;
use warnings;

use DBI;
use File::Basename;
use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

my $verbose;

my $skycal_id;
my $skycell_id;
my $tess_id;
my $filter;
my $release;

# NOTE: We do not use the ipp configuration to simplify running from a CGI script
my $dbname = "gpc1";
my $dbserver = "scidbm";
my $dbuser = "ippuser";
my $dbpassword = "ippuser";


GetOptions(
    'skycell_id=s'  =>      \$skycell_id,
    'tess_id=s'     =>      \$tess_id,
    'filter=s'      =>      \$filter,
    'skycal_id=s'   =>      \$skycal_id,
    'release=s'     =>      \$release,
    'dbname=s'      =>      \$dbname,
    'verbose|v'     =>      \$verbose,
) or pod2usage (2);

pod2usage( -msg => "Required options: (--tess_id and --skycell_id and --filter) or  --skycal_id",
           -exitval => 3) 
        unless $skycal_id or ($filter and $tess_id and $skycell_id);

my ($filename, $cmf);
if (defined $skycal_id) {
    ($filename, $cmf) = find_cmf_by_skycal_id($skycal_id)
} elsif (defined $tess_id and defined $skycell_id and defined $filter) {
    ($filename, $cmf) = find_cmf_by_skycell($tess_id, $skycell_id, $filter, $release);
} else {
    # should have been trapped above
    die "not enough parameters\n";
}


# FIX THESE PATHS!
# set up the environment
$ENV{NEB_SERVER}="http://nebserver.ipp.ifa.hawaii.edu:80/nebulous";
$ENV{PERL5LIB} .= ":/home/panstarrs/bills/psconfig/debug.lin64/lib:/home/panstarrs/bills/psconfig/debug.lin64/lib/perl5";

my $neb_locate = "/home/panstarrs/bills/psconfig/debug.lin64/bin/neb-locate -p";
if ($cmf) {
    my $resolved = `$neb_locate $cmf`;
    if (!$resolved) {
        print STDERR "failed to resolve $cmf\n";
        exit 2 if (!$resolved) 
    }
    # HERE IS THE OUTPUT FOR A SUCCESSFUL LOOKUP
    print "$filename $resolved";
} else {
    # the find function prints an appropriate error if it fails
    exit 2;
}

exit 0;

sub open_db {
    my $dsn = "DBI:mysql:host=$dbserver;database=$dbname";

    my $dbh = DBI->connect($dsn, $dbuser, $dbpassword) 
        or die "Cannot connect to database.\n";

    return $dbh;
}

sub find_cmf_by_skycal_id {
    my $skycal_id = shift;

    my $query = "SELECT * from skycalRun JOIN skycalResult USING(skycal_id) WHERE skycal_id = $skycal_id";

    my $dbh = open_db();
    my $stmt = $dbh->prepare($query);
    if (!$stmt->execute()) {
        print "DBI error\n";
        return undef;
    }
    my $results = $stmt->fetchrow_hashref();
    if (!$results) {
        print "results for skycal_id: $skycal_id not found\n";
        return undef;
    }

    if ($results->{fault}) {
        my $fault = $results->{fault};
        $fault .= " (GONE)" if $fault == 26;
        print "camRun $skycal_id has fault $fault\n";
        return undef;
    }

    my $path_base = $results->{path_base};

    my $cmf = $path_base . ".cmf";

    return parse_filename($cmf);
}

sub find_cmf_by_skycell {
    my ($tess_id, $skycell_id, $filter, $release) = @_;

    my $where_clause = " (tess_id = '$tess_id' AND skycell_id = '$skycell_id' and filter LIKE '$filter%')";

    # prioritize by release priority values
    my $order = "priority DESC";
    if ($release) {
        $where_clause .= " AND ippRelease.release_name = '$release'";
    }

    my $query = "SELECT skycal_id, skycalResult.quality, skycalResult.fault, skycalResult.path_base, priority\n"
    . " FROM ippRelease JOIN relStack using(rel_id) JOIN skycalResult using(skycal_id) "
    . " JOIN stackRun USING(stack_id, tess_id, skycell_id, filter)"
    . " WHERE $where_clause \n ORDER by $order limit 1;";

    print STDERR "$query\n" if $verbose;

    my $dbh = open_db();
    my $stmt = $dbh->prepare($query);
    if (!$stmt->execute()) {
        print "DBI error\n";
        return undef;
    }
    my $results = $stmt->fetchrow_hashref();
    if (!$results) {
        print "results not found\n";
        return undef;
    }
    my $skycal_id = $results->{skycal_id};
    if ($results->{fault}) {
        my $fault = $results->{fault};
        if ($fault == 26) {
            $fault .= " (GONE)";
        }
        print "skycalResult $skycal_id has fault $fault\n";
        return undef;
    } elsif ($results->{quality} != 0) {
        print "skycalRun $skycal_id has poor quality $results->{quality}\n";
        return undef;
    }


    my $path_base = $results->{path_base};

    my $cmf = $path_base . ".cmf";
    return parse_filename($cmf);
}

sub parse_filename {
    my $cmf = shift;
    my $filename = basename($cmf);
    return ($filename, $cmf);
}
