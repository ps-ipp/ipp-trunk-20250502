#!/bin/env perl
###
### pstamp_listjobs.pl
### list the job_id, state, and data store uri for the postage stamp jobs queued for a given 
### postage stamp request id
###

use warnings;
use strict;
use Getopt::Long qw( GetOptions );

my $verbose;
my $dbname;
my $dbserver;

GetOptions(
    'verbose'   =>  \$verbose,
    'dbname=s'  =>  \$dbname,
    'dbserver=s'=>  \$dbserver,
);

if (@ARGV != 1) {
    die "usage: $0 request_id\n";
}

my $request_id = $ARGV[0];


use Sys::Hostname;
my $host = hostname();

## This isn't a script to be launched by pantasks we probably want go get rid of this
if ($verbose) {
    print STDERR "\n\n";
    print STDERR "Starting script $0 on $host\n\n";
}

use IPC::Cmd 0.36 qw( can_run run );

use PS::IPP::Metadata::Config;
use PS::IPP::Metadata::Stats;
use PS::IPP::Metadata::List qw( parse_md_list );

use PS::IPP::Config qw($PS_EXIT_SUCCESS
		       $PS_EXIT_UNKNOWN_ERROR
		       $PS_EXIT_SYS_ERROR
		       $PS_EXIT_CONFIG_ERROR
		       $PS_EXIT_PROG_ERROR
		       $PS_EXIT_DATA_ERROR
		       $PS_EXIT_TIMEOUT_ERROR
		       metadataLookupStr
		       metadataLookupBool
		       caturi
		       );

my $missing_tools;

my $pstamptool  = can_run('pstamptool')  or (warn "Can't find pstamptool"  and $missing_tools = 1);

if ($missing_tools) {
    warn("Can't find required tools.");
    exit ($PS_EXIT_CONFIG_ERROR);
}

my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files

my @psjobs;
#Look up the jobs for the given request_id
{
    my $command = "$pstamptool -listjob -req_id $request_id";
    $command .= " -dbname $dbname" if $dbname;
    $command .= " -dbserver $dbserver" if $dbserver;
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        die("Unable to perform pstamptool -pendingreq: $error_code");
    }

    if (@$stdout_buf == 0) {
        print STDERR "no pstamp jobs for request $request_id\n" if $verbose;
        exit 0;
    }
    my $metadata = $mdcParser->parse(join "", @$stdout_buf) or
        die("Unable to parse metdata config doc");

    my $jobs = parse_md_list($metadata);

    @psjobs = @$jobs;
}

if (! @psjobs) {
    print STDERR "no postage stamp jobs for request $request_id\n";
    exit 0;
}

foreach my $job (@psjobs) {
    #
    # convert from filename to data store relative uri
    # XXX arguably we shouldn't do this here.
    # This should be a function of the web interface. However, since this script
    # is only used by the web interface ....
    my $i = index($job->{outputBase}, "dsroot/");

    my $add_fits = 0;
    if ($job->{jobType} ne "get_image") {
#        $add_fits = 1;
    }

    my $uri;
    if ($i > 0) {
        $uri = "/ds" . substr($job->{outputBase}, $i + 6);
    } else {
        $uri = "$job->{outputBase}";
    }

    if ($add_fits) {
        $uri .= ".fits";
    }

    print "$job->{job_id} $job->{state} $job->{fault} $job->{name} $job->{outProduct} $uri\n";
}

exit 0;
