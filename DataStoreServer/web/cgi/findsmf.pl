#!/usr/bin/env perl

# findsmf.pl Locate an smf file for an exposure either by cam_id, exp_id, or exp_name
#
# If cam_id is used, the smf for that run is selected.
#
# If selecting by exposure the camRun is selected by the following rules
# If a release name is supplied the smf from that release is selected
# If a data_group is supplied the value is used in the lookup (LIKE comparison)
# Otherwise all CamRuns for the exposure are considered.
# If multiple camRuns are found the one selected is the first row in the list formed with
# the following SQL ordering
#       ORDER BY quality, priority DESC, camRun.cam_id DESC
#   runs with zero quality 
#   runs with an entry in the released exposure table ordered by priority
#   the latest camRun
# For best results use the release tables.

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
my $data_group;
my $release;
my $getMagicked;

# NOTE: We do not use the ipp configuration to simplify running from a CGI script
my $dbname = "gpc1";
my $dbserver = "scidbm";
my $dbuser = "ippuser";
my $dbpassword = "ippuser";


GetOptions(
    'exp_name=s'    =>      \$exp_name,
    'exp_id=s'      =>      \$exp_id,
    'cam_id=s'      =>      \$cam_id,
    'release=s'     =>      \$release,
    'data_group=s'  =>      \$data_group,
    'dbname=s'      =>      \$dbname,
    'verbose|v'     =>      \$verbose,
) or pod2usage (2);

pod2usage( -msg => "Required options: --exp_name --exp_id or --cam_id",
           -exitval => 3) 
        unless $exp_name or $exp_id or $cam_id;

my ($filename, $smf, $fallback);
if (defined $cam_id) {
    ($filename, $smf, $fallback) = find_smf_by_cam_id($cam_id, $getMagicked);
} else {
    ($filename, $smf, $fallback) = find_smf_by_exposure($exp_name, $exp_id, $release, $data_group, $getMagicked);
}

# set up the environment
$ENV{NEB_SERVER}="http://nebserver.ipp.ifa.hawaii.edu:80/nebulous";
$ENV{PERL5LIB} .= ":/home/panstarrs/bills/psconfig/debug.lin64/lib:/home/panstarrs/bills/psconfig/debug.lin64/lib/perl5";

my $neb_locate = "/home/panstarrs/bills/psconfig/debug.lin64/bin/neb-locate -p";
if ($smf) {
    my $resolved = `$neb_locate $smf`;
    if (!$resolved) {
        print STDERR "failed to resolve $smf\n";
        if ($fallback) {
            print STDERR " falling back to $fallback\n";
            $resolved = `$neb_locate $fallback`;
        }
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

sub find_smf_by_cam_id {
    my $cam_id = shift;
    my $getMagicked = shift;

    my $query = "SELECT * from camRun JOIN camProcessedExp USING(cam_id) WHERE cam_id = $cam_id";

    my $dbh = open_db();
    my $stmt = $dbh->prepare($query);
    if (!$stmt->execute()) {
        print "DBI error\n";
        return undef;
    }
    my $results = $stmt->fetchrow_hashref();
    if (!$results) {
        print "results for cam_id: $cam_id not found\n";
        return undef;
    }
    my $magicked = $results->{magicked};

    if ($results->{fault}) {
        my $fault = $results->{fault};
        $fault .= " (GONE)" if $fault == 26;
        print "camRun $cam_id has fault $fault\n";
        return undef;
    }

    my $path_base = $results->{path_base};

    my $smf = $path_base . ".smf";

    return parse_filename($smf, ($magicked and !$getMagicked));
}

sub find_smf_by_exposure {
    my $exp_name = shift;
    my $exp_id = shift;
    my $release = shift;
    my $data_group = shift;
    my $getMagicked = shift;

    my $where_clause = "";
    if ($exp_id) {
        $where_clause .= "exp_id = $exp_id";
    } elsif ($exp_name) {
        $where_clause .= "exp_name = '$exp_name'";
    }

    my $joinrelease = "";
    # prioritize good quality runs, with higher release priority values, and finally newer cam_id
    my $order = "quality, priority DESC, camRun.cam_id DESC";
    my $selectmore = "";
    if ($release) {
        $where_clause .= " AND ippRelease.release_name = '$release'";
    } elsif ($data_group) {
        # ignore data_group if release is supplied
        $where_clause .= " AND camRun.data_group LIKE '$data_group'";
    } else {
        # since we have no qualifiers on the selection omit faulted runs so that we have a chance
        # to find a good one
        $where_clause .= " AND camProcessedExp.fault = 0";
    }
        
    my $query = "SELECT cam_id, quality, camRun.state, camProcessedExp.fault, camRun.magicked, camProcessedExp.path_base, IFNULL(priority, 0)\n"
    . " FROM camRun JOIN camProcessedExp USING(cam_id) join chipRun using(chip_id) join rawExp using(exp_id)\n"
    . " LEFT JOIN relExp using(exp_id, cam_id) LEFT JOIN ippRelease USING(rel_id)\n"
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
        if ($exp_name) {
            print "results for exp_name: $exp_name not found\n";
        } else {
            print "results for exp_id: $exp_id not found\n";
        }
        return undef;
    }
    my $cam_id = $results->{cam_id};
    my $magicked = $results->{magicked};
    if ($results->{fault}) {
        my $fault = $results->{fault};
        if ($fault == 26) {
            $fault .= " (GONE)";
        }
        print "camRun $cam_id has fault $fault\n";
        return undef;
    } elsif ($results->{quality} != 0) {
        print "camRun $cam_id has poor quality $results->{quality}\n";
        return undef;
    }


    my $path_base = $results->{path_base};

    my $smf = $path_base . ".smf";
    return parse_filename($smf, ($magicked and !$getMagicked));
}

sub parse_filename {
    my $smf = shift;
    my $getUncensored = shift;
    my $filename = basename($smf);
    my $fallback;
    if ($getUncensored) {
        my $dirname = dirname($smf);
        $fallback = $smf;
        $smf = "$dirname/SR_$filename";
    }
    return ($filename, $smf, $fallback);
}
