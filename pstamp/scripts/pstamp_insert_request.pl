#!/bin/env perl
###
#
# create a postage stamp request
#
###

use warnings;
use strict;

use Carp;
use Getopt::Long qw( GetOptions );
use Sys::Hostname;
use File::Copy;
use POSIX qw( strftime );

my $host = hostname();
my $verbose = 0;
my $dbname;
my $dbserver;
my $tmp_req_file;
my $workdir;

GetOptions(
    'tmp_req_file=s'=>  \$tmp_req_file,
    'workdir=s'     =>  \$workdir,
    'dbname=s'      =>  \$dbname,
    'dbserver=s'    =>  \$dbserver,
    'verbose'       =>  \$verbose,
);

die "required arguments --tmp_req_file --workdir --dbname --dbserver"
    if( !defined($tmp_req_file) or !defined($workdir) or !defined ($dbname)
        or !defined($dbserver));
use IPC::Cmd 0.36 qw( can_run run );

use PS::IPP::Config qw( :standard );

use Cwd;

my $cwd = cwd();

my $missing_tools;

my $pstamptool = can_run('pstamptool')  or (warn "Can't find pstamptool"  and $missing_tools = 1);
my $fields = can_run('fields')  or (warn "Can't find fields"  and $missing_tools = 1);
my $fhead = can_run('fhead')  or (warn "Can't find fhead"  and $missing_tools = 1);

if ($missing_tools) {
    warn("Can't find required tools.");
    exit ($PS_EXIT_CONFIG_ERROR);
}

my ($extname, $extver, $req_name);
{
    # get the header keywords of interest.
    # Note that if it's a pstamp request then REQ_NAME should be defined.
    # if it's a detectability query it will not have a REQ_NAME but will have a QUERY_ID
    # my $command = "echo $tmp_req_file | $fields -x 0 EXTNAME EXTVER REQ_NAME QUERY_ID";
    my $command = "$fhead -x 0 $tmp_req_file";
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        print STDERR @$stderr_buf;
        exit $error_code >> 8;
    }
    my $output = join "", @$stdout_buf;


    my $makehash = 0;
    my %hash;
    foreach my $line (split "\n", $output) {
        chomp $line;
        # split lines inte left and right using equals sign. left is the keyword
        my ($key, $right) = split "=", $line;
            # skip if there was no '='
        next if !$right;

        $key =~ s/ //g;

        # separate value from comment
        my ($value, $comment) = split "/", $right;
        # remove ' and space characters from key and value
        $value =~ s/'//g;
        $value =~ s/ //g;

        #    print "$key $value\n";

        # extract the values that we are looking for
        $extname  = $value if ($key eq "EXTNAME");
        $extver   = $value if ($key eq "EXTVER");
        $req_name = $value if ($key eq "REQ_NAME");
        $req_name = $value if ($key eq "QUERY_ID");

        # optionally build hash. Duplicate keys get the last value seen
        if ($makehash) {
            $hash{$key} = $value;
        }
    }

    # (undef, $extname, $extver, $req_name) = split " ", $output;

    if (!$extname or ! (($extname eq "PS1_PS_REQUEST") or ($extname eq "MOPS_DETECTABILITY_QUERY"))) {
        print STDERR "invalid request file\n";
        print "invalid request file\n";
        exit $PS_EXIT_DATA_ERROR;
    }

    if (!defined $req_name) {
        print STDERR "invalid request file no REQ_NAME or QUERY_ID\n";
        print "invalid request file no REQ_NAME or QUERY_ID\n";
        exit $PS_EXIT_DATA_ERROR;
    }
    if (!defined $extver) {
        print STDERR "invalid request file no EXTVER found\n";
        print "invalid request file no EXTVER found\n";
        exit $PS_EXIT_DATA_ERROR;
    }
}

# put file in directory for the current date
my $datestr = strftime "%Y/%m/%d", gmtime;
my $datedir = "$workdir/webreq/$datestr";
if (! -e $datedir ) {
    my $rc = system "mkdir -p $datedir";
    if ($rc) {
        my $status = $rc >> 8;
        print STDERR  "failed to create working directory $datedir: $rc $status";
        exit $PS_EXIT_CONFIG_ERROR;
    }
}


my $webreq_num = get_webreq_num();
my $req_file = "$datedir/web_$webreq_num.fits";

if (!copy( $tmp_req_file, $req_file)) {
    die("Unable to copy request file $tmp_req_file to $req_file");
}

# Queue the request
my $req_id = 0;
{
    my $command = "$pstamptool -addreq -uri $req_file -ds_id 0 -name $req_name";
    $command .= " -dbname $dbname" if $dbname;
    $command .= " -dbserver $dbserver" if $dbserver;
    $command .= " -label WEB.UP";
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        my $errbuf = join "", @$stderr_buf;
        print STDERR $errbuf;
        if ($errbuf =~ /Duplicate entry/) {
            print "Request Name $req_name has already been used";
            exit $PS_EXIT_DATA_ERROR;
        } else {
            exit $PS_EXIT_UNKNOWN_ERROR;
        }
    }
    $req_id = ${$stdout_buf}[0];
}

print "$req_id $req_name\n";

exit 0;

# Ask the database for the next web request number
sub get_webreq_num
{
    my $command = "$pstamptool -getwebrequestnum";
    $command .= " -dbname $dbname" if $dbname;
    $command .= " -dbserver $dbserver" if $dbserver;
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
	my $status = $error_code >> 8;
        print STDERR @$stderr_buf;
        die("Unable to perform $command: $error_code : $status");
    }
    my $webreq_num = ${$stdout_buf}[0];
    chomp $webreq_num;

    if (!$webreq_num) {
        die("pstamptool -getwebreqnum returned no value");
    }

    # print STDERR "webreq_num $webreq_num\n";

    return $webreq_num;
}
