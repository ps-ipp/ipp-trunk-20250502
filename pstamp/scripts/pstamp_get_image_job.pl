#!/usr/bin/env perl
#
# Execute a get_image job for the postage stamp server
#

use strict;
use warnings;

use Getopt::Long qw( GetOptions );
use Pod::Usage qw( pod2usage );
use DBI;
use PS::IPP::Metadata::Config;
use PS::IPP::Metadata::Stats;
use PS::IPP::Metadata::List qw( parse_md_list );
use File::Copy;
use File::Basename;
use Digest::MD5::File qw( file_md5_hex );
use IPC::Cmd 0.36 qw( can_run run );


use PS::IPP::Config qw( :standard );

my $product;
my $fileset;

my $output_base;
my $bundleroot;

my $verbose;
my $imagedbname;
my $dbname;
my $dbserver;
my $job_id;
my $rownum;

#
# parse args
#

GetOptions(
        'job_id=s'        =>      \$job_id,
        'rownum=s'        =>      \$rownum,
        'output_base=s'   =>      \$output_base,
        'bundleroot=s'    =>      \$bundleroot,
        'imagedbname=s'   =>      \$imagedbname,
        'dbname=s'        =>      \$dbname,
        'dbserver=s'      =>      \$dbserver,
        'verbose'         =>      \$verbose,
) or pod2usage(2);

my $err = "";
$err .= "--job_id is required\n" if (!$job_id);
$err .= "--rownum is required\n" if (!$rownum);
$err .= "--output_base is required to specify the output fileset\n" if (!$output_base);

my_die( $err, $PS_EXIT_PROG_ERROR) if $err;

my $ipprc = PS::IPP::Config->new();

my $params_file = $output_base . ".mdc";
if (! open(INPUT, "<$params_file") ) {
    my_die("failed to open params file: $params_file", $PS_EXIT_UNKNOWN_ERROR);
}

my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files

my $data = $mdcParser->parse(join "", (<INPUT>)) 
    or my_die("failed to parse metadata config doc", $PS_EXIT_UNKNOWN_ERROR);

my $components = parse_md_list($data);
my $n = scalar @$components;
if ($n != 1) {
    my_die("params file $params_file contains unexpected number of components: $n", $PS_EXIT_PROG_ERROR);
}
my $comp = $components->[0];
my $stage = $comp->{stage};
my $stage_id = $comp->{stage_id};
my $component = $comp->{component};
my $path_base = $comp->{path_base};
my $camera = $comp->{camera};
my $magicked = $comp->{magicked};

if ($verbose) {
    print STDERR "\nstage is $stage\n";
    print STDERR "stage_id is $stage_id\n";
    print STDERR "path_base is $path_base\n";
    print STDERR "CAMERA is $camera\n";
    print STDERR "magicked is " . (defined $magicked ? $magicked : "undefined") . "\n";
}

if (!$camera or !$path_base or !$component or !$stage_id or !$stage) {
       my_die("One or more parameters are missing in: $params_file", $PS_EXIT_UNKNOWN_ERROR);
}


# Look for programs we need
my $missing_tools;
my $dist_bundle   = can_run('dist_bundle.pl') or (warn "Can't find dist_bundle.pl" and $missing_tools = 1);
my $pstamptool   = can_run('pstamptool') or (warn "Can't find pstamptool" and $missing_tools = 1);
if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

$pstamptool .= " -dbname $dbname" if $dbname;
$pstamptool .= " -dbserver $dbserver" if $dbserver;

my $outdir = dirname($output_base);
my $basename = basename($path_base);
my $results_file = $output_base . ".bundle_results";
my $outroot;
if ($bundleroot) {
    my (undef, undef, undef, $mday, $month, $year) = gmtime(time());
    $month += 1;
    $year += 1900;

    # This will generate an outroot like:
    # neb://any/pstamp/bundles/2011/11/22/14026916_1_1_o5739g0358o.353654.wrp.215092.skycell.2083.025

    $outroot = $bundleroot . sprintf("/%4d/%02d/%02d/${job_id}_", $year, $month, $mday) . basename($output_base) . "_". $basename;
} else {
    $outroot = $output_base ."_" . $basename;
}

{
    my $command = "$dist_bundle --camera $camera --stage $stage --stage_id $stage_id";
    $command .= " --results_file $results_file";
    $command .= " --component $component --path_base $path_base --outroot $outroot";
    # DING DONG ....
    $command .= " --no_magic";
    $command .= " --dbname $imagedbname" if $imagedbname;
    $command .= " --verbose" if $verbose;
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        my_die("Unable to perform $command: $error_code", $error_code >> 8);
    }
}

if (! open(RESULTS, "<$results_file")) {
    my_die("failed to open bundle results file: $results_file", $PS_EXIT_UNKNOWN_ERROR);
}

my $file_name;
my $bytes;
my $md5sum;
foreach my $line (<RESULTS>) {
    chomp $line;
    next if !$line;
    next if $line =~ /bundleResults/;
    last if $line =~ /END/;

    my ($tag, $type, $val) = split " ", $line;
    if ($tag eq "name") {
        $file_name = $val;
    } elsif ($tag eq "bytes") {
        $bytes = $val;
    } elsif ($tag eq "md5sum") {
        $md5sum = $val;
    } else {
        my_die("unexpected tag: $tag  found in results file: $results_file", $PS_EXIT_PROG_ERROR);
    }
}

my $reglist = "$outdir/reglist$job_id";
if (! open(REGLIST, ">$reglist") ) {
    my_die("failed to open registration list: $reglist", $PS_EXIT_UNKNOWN_ERROR);
}

if ($bundleroot) {
    {
        # delete any existing pstampFile in case this job has faulted and
        # been reverted
        my $command = "$pstamptool -deletefile -job_id $job_id";
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            my_die("Unable to perform $command: $error_code", $error_code >> 8);
        }
    }
    my $linkname = "$outdir/$file_name";
    if (-l $linkname or -e $linkname) {
        unlink $linkname or my_die("Failed to unlink existing symlink: $linkname", $PS_EXIT_UNKNOWN_ERROR);
    }
    my $bundle_name = dirname($outroot) . "/$file_name";
    my $resolved = $ipprc->file_resolve($bundle_name);
    if (!$resolved) {
        my_die("failed to resolve $bundle_name", $PS_EXIT_UNKNOWN_ERROR);
    }
    symlink $resolved, $linkname or my_die("failed to create symlink to $resolved in $outdir", 
        $PS_EXIT_UNKNOWN_ERROR);

    my $command = "$pstamptool -addfile -job_id $job_id -path $bundle_name";
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        my_die("Unable to perform $command: $error_code", $error_code >> 8);
    }
}


print REGLIST "$file_name|$bytes|$md5sum|tgz|\n";

close(REGLIST);

exit 0;

sub my_die {
    my $msg = shift;
    my $rc = shift;

    print STDERR $msg;
    exit $rc ? $rc : $PS_EXIT_UNKNOWN_ERROR;
}
